#include "transport_router.h"


TransportCatalogueRouterGraph::TransportCatalogueRouterGraph(const transport_catalogue::TransportCatalogue &tc,
                                                             RoutingSettings rs) :
        graph::DirectedWeightedGraph<double>(tc.RawStopsIndex().size()), tc_(tc), rs_(rs) {

    // Создание вершин графа из всех остановок транспорта.
    const auto &routes_index = tc_.GetAllRoutesIndex();
    for (const auto &[stop_name, stop_ptr]: tc_.RawStopsIndex()) {
        StopOnRoute stop{0, stop_name, {}};
        RegisterStop(stop);
    }

    // Заполнение графа связями между остановками на маршруте транспорта.
    for (const auto &[_, bus_route]: routes_index) {
        if (bus_route->type == transport_catalogue::RouteType::RETURN_ROUTE) {
            FillWithReturnRouteStops(bus_route);
        } else {
            FillWithCircleRouteStops(bus_route);
        }
    }

    router_ptr_ = std::make_unique<graph::Router<double>>(*this);
}

// Функция заполнения графа связями между остановками на маршруте транспорта с обратным направлением
void TransportCatalogueRouterGraph::FillWithReturnRouteStops(const transport_catalogue::BusRoute *bus_route) {
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        size_t stop_distance = 1;
        int accumulated_distance_direct = 0;
        int accumulated_distance_reverse = 0;

        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        auto from_id = GetStopVertexId((*start)->stop_name);
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            auto to_id = GetStopVertexId((*second)->stop_name);

            // Добавляем прямое ребро от from_id к to_id
            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(
                    accumulated_distance_direct);
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            // Добавляем обратное ребро от to_id к from_id
            TwoStopsLink reverse_link(bus_route->bus_name, to_id, from_id, stop_distance);
            const int reverse_distance = tc_.GetDistanceBetweenStops((*second)->stop_name, (*first)->stop_name);
            accumulated_distance_reverse += reverse_distance;
            const double reverse_link_time = wait_time_at_stop + CalculateTimeForDistance(
                    accumulated_distance_reverse);
            const auto reverse_edge_id = AddEdge({to_id, from_id, reverse_link_time});
            StoreLink(reverse_link, reverse_edge_id);

            ++stop_distance;

            edge_count_ = reverse_edge_id;
        }
    }
}

// Функция заполнения графа связями между остановками на кольцевом маршруте транспорта.
void TransportCatalogueRouterGraph::FillWithCircleRouteStops(const transport_catalogue::BusRoute *bus_route) {
    // Проходимся по всем остановкам маршрута.
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        // Задаем расстояние до следующей остановки и накопленное расстояние.
        size_t stop_distance = 1;
        int accumulated_distance_direct = 0;

        // Определяем время ожидания на остановке.
        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        // Получаем идентификатор вершины, соответствующей текущей остановке, иначе кидаем исключение.
        auto from_id = GetStopVertexId((*start)->stop_name);

        // Повторяем для каждой смежной вершины
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            // Получаем идентификатор вершины, соответствующей следующей остановке, иначе кидаем исключение.
            auto to_id = GetStopVertexId((*second)->stop_name);

            // Задаем информацию о двусторонней дуге и добавляем ее в граф.
            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_direct);
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            ++stop_distance;

            edge_count_ = direct_edge_id;
        }
    }
}

// Функция регистрирует остановку на маршруте, если она еще не была зарегистрирована.
graph::VertexId TransportCatalogueRouterGraph::RegisterStop(const StopOnRoute &stop) {
    // Проверяем, была ли остановка на маршруте зарегистрирована. Если да, возвращаем идентификатор соответствующей вершины.
    auto iter = stop_to_vertex_.find(stop);
    if (iter != stop_to_vertex_.end()) {
        return iter->second;
    }

    // Регистрируем новую остановку. Запоминаем соответствие между остановкой и вершиной графа.
    auto result = vertex_id_count_;
    stop_to_vertex_[stop] = vertex_id_count_;
    vertex_to_stop_[vertex_id_count_] = stop;
    ++vertex_id_count_;

    return result;
}

// Функция проверяет наличие связи между двумя остановками на маршруте.
std::optional<graph::EdgeId> TransportCatalogueRouterGraph::CheckLink(const TwoStopsLink &link) const {
    // Проверяем, есть ли связь между двумя остановками на маршруте. Если да, возвращаем идентификатор ребра графа.
    auto iter = stoplink_to_edge_.find(link);

    if (iter != stoplink_to_edge_.end()) {
        return iter->second;
    }

    return {};
}

// Функция сохраняет связь между двумя остановками на маршруте.
graph::EdgeId TransportCatalogueRouterGraph::StoreLink(const TwoStopsLink &link, graph::EdgeId edge) {
    // Сохраняем новую связь между двумя остановками на маршруте.
    auto iter = edge_to_stoplink_.find(edge);
    if (iter != edge_to_stoplink_.end()) {
        return iter->first;
    }

    stoplink_to_edge_[link] = edge;
    edge_to_stoplink_[edge] = link;

    return edge;
}

// Функция вычисляет время, необходимое для пройденного расстояния.
double TransportCatalogueRouterGraph::CalculateTimeForDistance(int distance) const {
    return static_cast<double>(distance) / (rs_.bus_velocity * transport_catalogue::MET_MIN_RATIO);
}

// Функция возвращает идентификатор вершины графа, соответствующей остановке маршрута.
graph::VertexId TransportCatalogueRouterGraph::GetStopVertexId(std::string_view stop_name) const {
    // Получаем идентификатор вершины графа, соответствующей остановке маршрута.
    // Если такой остановки не существует, кидаем исключение.
    StopOnRoute stop{0, stop_name, {}};
    auto iter = stop_to_vertex_.find(stop);

    if (iter != stop_to_vertex_.end()) {
        return iter->second;
    }

    throw std::logic_error("Error, no stop name: " + std::string(stop_name));
}

// Функция возвращает остановку маршрута по ее идентификатору.
const TransportCatalogueRouterGraph::StopOnRoute &TransportCatalogueRouterGraph::GetStopById(graph::VertexId id) const {
    return vertex_to_stop_.at(id);
}

// Функция возвращает время ожидания автобуса на остановке.
double TransportCatalogueRouterGraph::GetBusWaitingTime() const {
    return static_cast<double>(rs_.bus_wait_time);
}

// Функция возвращает связь между двумя остановками по идентификатору ребра графа.
const TwoStopsLink &TransportCatalogueRouterGraph::GetLinkById(graph::EdgeId id) const {
    auto iter = edge_to_stoplink_.find(id);

    if (iter != edge_to_stoplink_.end()) {
        return iter->second;
    }

    throw std::logic_error("Error fetching the TwoStopsLink, no Edge id: " + std::to_string(id));
}

// Функция строит маршрут между двумя остановками.
std::optional<graph::Router<double>::RouteInfo>
TransportCatalogueRouterGraph::BuildRoute(std::string_view from, std::string_view to) const {
    // Усли нет построителя маршрутов, возвращаем пустое значение.
    if (!router_ptr_) return {};

    // Находим идентификаторы вершин, соответствующие начальной и конечной остановкам.
    graph::VertexId from_id = GetStopVertexId(from);
    graph::VertexId to_id = GetStopVertexId(to);

    // Вызываем построитель маршрутов.
    return router_ptr_->BuildRoute(from_id, to_id);
}
