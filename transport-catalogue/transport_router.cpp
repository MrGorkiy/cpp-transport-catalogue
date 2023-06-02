#include "transport_router.h"

namespace TransportRouter {
//----------------------------------------------------------------------------
    void TransportRouter::CreateGraph(const TransportCatalogue::TransportCatalogue &db) {
        graph::DirectedWeightedGraph<double> graph(db.GetStops().size());
        id_stopes_.resize(db.GetStops().size());
        for (const auto &bus: db.GetBuses()) {

            for (auto it_from = bus.stops.begin(); it_from != bus.stops.end(); ++it_from) {
                const Stop *stop_from = *it_from;
                double lengh = 0;
                const Stop *prev_stop = stop_from;
                id_stopes_[stop_from->id] = stop_from->name;
                for (auto it_to = std::next(it_from); it_to != bus.stops.end(); ++it_to) {
                    const Stop *stop_to = *it_to;
                    lengh += db.GetRangeStops(prev_stop, stop_to);
                    prev_stop = stop_to;
                    double time_on_bus = lengh / GetMetrMinFromKmH(routing_settings_.bus_velocity); // minute
                    // вес ребра учитывает и ожидание и время в пути, чтобы учитывать затраты на пересадки
                    graph.AddEdge({stop_from->id, stop_to->id, (time_on_bus + routing_settings_.bus_wait_time_minut)});
                    // запоминает имя автобуса и количество прогонов между остановками
                    edges_buses_.push_back({bus.name, static_cast<size_t>(std::distance(it_from, it_to))});
                }
            }
        }
        opt_graph_ = std::move(graph);
    }

//----------------------------------------------------------------------------
    std::optional<RoutStat> TransportRouter::GetRouteStat(size_t id_stop_from, size_t id_stop_to) const {
        // попытка построить маршрут
        const OptRouteInfo opt_route_info = GetRouter()->BuildRoute(id_stop_from, id_stop_to);
        // проверка маршрута
        if (!opt_route_info.has_value()) {
            return std::nullopt;
        }

        const graph::Router<double>::RouteInfo &route_info = opt_route_info.value();
        double total_time = route_info.weight;
        // хранит данные для вывода в поток
        std::vector<RoutStat::VariantItem> items;
        for (const auto &edge_id: route_info.edges) {
            // ребро по id
            const auto &edge = opt_graph_.value().GetEdge(edge_id);
            // номер автобуса едущий по этому ребру и количество прогонов в ребре
            const auto [bus_num, span_count] = edges_buses_[edge_id];
            items.push_back(RoutStat::ItemsWait{"Wait", static_cast<double>(routing_settings_.bus_wait_time_minut),
                                                std::string(id_stopes_[edge.from])});
            // вычитаем из веса время ожидания
            items.push_back(
                    RoutStat::ItemsBus{"Bus", edge.weight - static_cast<double>(routing_settings_.bus_wait_time_minut),
                                       span_count, std::string(bus_num)});
        }
        return RoutStat{total_time, items};
    }

//----------------------------------------------------------------------------
    bool TransportRouter::GetGraphIsNoInit() const {
        return !opt_graph_.has_value();
    }

//----------------------------------------------------------------------------
    void TransportRouter::vInit(RoutingSettings routing_settings_, const TransportCatalogue::TransportCatalogue &t_c) {
        SetRoutingSettings(std::move(routing_settings_));
        // создаем граф и маршрутизатор
        if (GetGraphIsNoInit()) {
            CreateGraph(t_c);
        }
    }

//----------------------------------------------------------------------------
    const std::vector<TransportRouter::EdgeAditionInfo> &TransportRouter::GetEdgesBuses() const {
        return edges_buses_;
    }

//----------------------------------------------------------------------------
    const std::vector<std::string> &TransportRouter::GetIdStopes() const {
        return id_stopes_;
    }

//----------------------------------------------------------------------------
    const graph::DirectedWeightedGraph<double> &TransportRouter::GetGraph() const {
        return opt_graph_.value();
    }

//----------------------------------------------------------------------------
    void TransportRouter::SetEdgesBuses(std::vector<EdgeAditionInfo> &&edges_buses) {
        edges_buses_ = std::move(edges_buses);
    }

//----------------------------------------------------------------------------
    void TransportRouter::SetIdStopes(std::vector<std::string> &&id_stopes) {
        id_stopes_ = id_stopes;
    }

//----------------------------------------------------------------------------
    void TransportRouter::SetGraph(graph::DirectedWeightedGraph<double> &&graph) {
        opt_graph_ = graph;
    }

//----------------------------------------------------------------------------
    const RoutingSettings &TransportRouter::GetRoutingSettings() const {
        return routing_settings_;
    }

//----------------------------------------------------------------------------
    void TransportRouter::SetRoutingSettings(RoutingSettings &&routing_settings) {
        routing_settings_ = std::move(routing_settings);
    }

//----------------------------------------------------------------------------
    const std::unique_ptr<graph::Router<double>> &TransportRouter::GetRouter() const {
        // граф создан
        if (GetGraphIsNoInit()) {
            std::cerr << " ! opt_graph_.has_value()" << std::endl;
            throw ("! opt_graph_.has_value()");
        }
        // создает маршрутизатор если его еще нет
        if (!up_router_) {
            up_router_ = std::make_unique<graph::Router<double>>(opt_graph_.value());
        }
        return up_router_;
    }
//----------------------------------------------------------------------------
}