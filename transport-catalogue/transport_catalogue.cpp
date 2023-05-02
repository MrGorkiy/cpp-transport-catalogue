#include "transport_catalogue.h"
#include <numeric>
#include <utility>
#include <iostream>
#include <set>
#include <iomanip>

namespace transport_catalogue {


    // Добавление остановки в каталог
    void TransportCatalogue::AddStop(const std::string &name, const geo::Coordinates coords) {
        // Создаем объект остановки и добавляем его в каталог
        const Stop stop{name, coords};
        AddStop(stop);
    }

    // Добавление остановки в каталог
    void TransportCatalogue::AddStop(const Stop &stop) {
        // Если остановка уже есть в каталоге, ничего не делаем
        if (stops_index_.count(stop.stop_name) > 0) return;

        // Добавляем остановку в каталог и создаем индекс для быстрого поиска остановки по имени
        const Stop *ptr = &stops_.emplace_back(stop);
        std::string_view stop_name(ptr->stop_name);
        stops_index_.emplace(stop_name, ptr);
    }

    // Поиск остановки по имени в каталоге
    std::pair<bool, const Stop &> TransportCatalogue::FindStop(const std::string_view name) const {
        const auto iter = stops_index_.find(name);
        // Если остановка не найдена, возвращается флаг false и пустой объект остановки
        if (iter == stops_index_.end()) return {false, EMPTY_STOP};

        // Возвращается флаг true и найденная остановка
        return {true, *iter->second};
    }

    // Добавление маршрута в каталог
    bool TransportCatalogue::AddBus(const BusRoute &bus_route) {
        const auto iter = routes_index_.find(bus_route.bus_name);
        // Если маршрут уже есть в каталоге, возвращается false
        if (iter != routes_index_.end()) return false;

        // Добавляем маршрут в каталог, создаем индекс для быстрого поиска маршрута по имени
        const BusRoute *ptr = &bus_routes_.emplace_back(bus_route);
        std::string_view bus_name(ptr->bus_name);
        routes_index_.emplace(bus_name, ptr);

        // Создаем индекс, связывающий остановки и маршруты, проходящие через них
        for (const Stop *stop: ptr->route_stops) {
            stop_and_buses_[stop->stop_name].insert(bus_name);
        }

        return true;
    }

    // Поиск информации о маршруте по имени
    const BusRoute &TransportCatalogue::FindBus(std::string_view name) {
        const auto iter = routes_index_.find(name);
        if (iter == routes_index_.end()) return EMPTY_BUS_ROUTE;

        return *iter->second;
    }

    // Получение информации о маршруте по имени
    BusInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
        BusInfo result;
        result.type = RouteType::NOT_SET;

        // Поиск маршрута по имени
        auto iter = routes_index_.find(bus_name);
        if (iter == routes_index_.end()) return result;

        const BusRoute &route = *iter->second;

        // Вычисление количества уникальных остановок на маршруте
        std::set<const Stop *> unique_stops(route.route_stops.begin(), route.route_stops.end());
        result.unique_stops = unique_stops.size();

        // Вычисление длины маршрута в метрах и географическом пространстве
        double length_geo = 0.0;
        size_t length_meters = 0;
        for (auto first = route.route_stops.begin(); first != route.route_stops.end(); ++first) {
            auto second = std::next(first);
            if (second == route.route_stops.end()) break;

            length_geo += ComputeDistance((**first).coordinates, (**second).coordinates);

            length_meters += GetDistanceBetweenStops((**first).stop_name, (**second).stop_name);

            // Если маршрут туда и обратно, учитываем обратный путь
            if (route.type == RouteType::RETURN_ROUTE) {
                length_meters += GetDistanceBetweenStops((**second).stop_name, (**first).stop_name);
            }
        }

        // Вычисление прочих параметров маршрута
        result.stops_number = route.route_stops.size();
        if (route.type == RouteType::RETURN_ROUTE) {
            result.stops_number *= 2;
            result.stops_number -= 1;
            length_geo *= 2;
        }
        result.route_length = length_meters;
        result.curvature = static_cast<double>(length_meters) / length_geo;
        result.bus_name = route.bus_name;
        result.type = route.type;

        return result;
    }

    // Получение списка всех маршрутов, проходящих через остановку
    const std::set<std::string_view> &TransportCatalogue::GetBusesForStop(std::string_view stop) const {
        const auto iter = stop_and_buses_.find(stop);

        // Если остановка не найдена, возвращается пустой список маршрутов
        if (iter == stop_and_buses_.end()) {
            return EMPTY_BUS_ROUTE_SET;
        }

        // Возвращается список маршрутов, проходящих через остановку
        return iter->second;
    }

    // Установка расстояния между двумя остановками
    bool TransportCatalogue::SetDistanceBetweenStops(std::string_view stop, std::string_view other_stop, int dist) {
        auto iter_stop = stops_index_.find(stop);
        auto iter_other = stops_index_.find(other_stop);
        if (iter_stop == stops_index_.end() || iter_other == stops_index_.end()) return false;

        StopsPointers direct{};
        direct.stop = iter_stop->second;
        direct.other = iter_other->second;
        stops_distance_index_[direct] = dist;

        StopsPointers reverse{};
        reverse.stop = direct.other;
        reverse.other = direct.stop;
        auto iter_rev = stops_distance_index_.find(reverse);
        if (iter_rev == stops_distance_index_.end()) {
            stops_distance_index_[reverse] = dist;
        }

        return true;
    }

    // Получение расстояния между двумя остановками
    int TransportCatalogue::GetDistanceBetweenStops(std::string_view stop, std::string_view other_stop) const {
        const auto iter_stop = stops_index_.find(stop);
        const auto iter_other = stops_index_.find(other_stop);
        if (iter_stop == stops_index_.end() || iter_other == stops_index_.end()) return -1;

        StopsPointers direct{};
        direct.stop = iter_stop->second;
        direct.other = iter_other->second;

        auto iter_dist = stops_distance_index_.find(direct);
        if (iter_dist == stops_distance_index_.end()) return -1;

        return iter_dist->second;
    }

    // Получение индекса всех маршрутов
    const std::map<std::string_view, const BusRoute *> TransportCatalogue::GetAllRoutesIndex() const {
        std::map<std::string_view, const BusRoute *> result(routes_index_.begin(), routes_index_.end());
        return result;
    }

    // Получение индекса всех остановок
    const std::map<std::string_view, const Stop *> TransportCatalogue::GetAllStopsIndex() const {
        std::map<std::string_view, const Stop *> result(stops_index_.begin(), stops_index_.end());
        return result;
    }

    // Получение неупорядоченного индекса остановок
    const std::unordered_map<std::string_view, const Stop *> &
    TransportCatalogue::RawStopsIndex() const {
        return stops_index_;
    }

    // Получение неупорядоченного индекса расстояний между остановками
    const std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> &
    TransportCatalogue::RawDistancesIndex() const {
        return stops_distance_index_;
    }

    // Получение индекса остановок и маршрутов, проходящих через них
    const std::unordered_map<std::string_view, std::set<std::string_view>> &
    TransportCatalogue::GetStopAndBuses() const {
        return stop_and_buses_;
    }

    // Получение количества остановок на всех маршрутах
    size_t TransportCatalogue::GetNumberOfStopsOnAllRoutes() const {
        size_t result = 0;

        for (const auto &[bus, route]: routes_index_) {
            result += route->route_stops.size();
            // Если маршрут кольцевой, последняя остановка не учитывается
            if (route->type == RouteType::CIRCLE_ROUTE) {
                --result;
            }
        }

        return result;
    }

} // transport_catalogue namespace
