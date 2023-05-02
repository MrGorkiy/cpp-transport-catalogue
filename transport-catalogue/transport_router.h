#pragma once

#include "transport_catalogue.h"
#include "router.h"
#include <memory>

// Структура настроек маршрутизации
struct RoutingSettings {
    int bus_wait_time;
    double bus_velocity;
};

// Структура TwoStopsLink, хранящая информацию о маршруте между двумя остановками
struct TwoStopsLink {
    std::string_view bus_name = {}; // Название маршрута
    graph::VertexId stop_from = {}; // Идентификатор исходной остановки
    graph::VertexId stop_to = {}; // Идентификатор конечной остановки
    size_t number_of_stops = {}; // Количество пройденных остановок

    // Конструктор
    explicit TwoStopsLink(std::string_view bus, graph::VertexId from, graph::VertexId to, size_t num) :
            bus_name(bus), stop_from(from), stop_to(to), number_of_stops(num) {
    }

    TwoStopsLink() = default;

    // Хэш-функция
    size_t operator()(const TwoStopsLink &sor) const {
        return hasher_num_(number_of_stops) + 43 * hasher_num_(sor.stop_from) +
               43 * 43 * hasher_num_(sor.stop_to) + 43 * 43 * 43 * hasher_(bus_name);
    }

    // Оператор сравнения
    bool operator()(const TwoStopsLink &lhs, const TwoStopsLink &rhs) const {
        return lhs.bus_name == rhs.bus_name && lhs.stop_from == rhs.stop_from
               && lhs.stop_to == rhs.stop_to && lhs.number_of_stops == rhs.number_of_stops;
    }

private:
    std::hash<size_t> hasher_num_;
    std::hash<std::string_view> hasher_;
};

// Класс TransportCatalogueRouterGraph, наследуемый от графа с взвешенными ребрами
class TransportCatalogueRouterGraph : public graph::DirectedWeightedGraph<double> {
public:
    // Структура StopOnRoute, хранящая информацию об остановке на маршруте
    struct StopOnRoute {
        size_t stop_number; // Порядковый номер остановки на маршруте
        std::string_view stop_name; // Название остановки
        std::string_view bus_name; // Название маршрута, на котором находится остановка

        // Конструктор
        explicit StopOnRoute(size_t num, std::string_view stop, std::string_view bus) : stop_number(num),
                                                                                        stop_name(stop), bus_name(bus) {
        }

        StopOnRoute() = default;

        // Хэш-функция
        size_t operator()(const StopOnRoute &sor) const {
            return hasher_num_(stop_number) + 43 * hasher_(sor.stop_name) + 43 * 43 * hasher_(sor.bus_name);
        }

        // Оператор сравнения
        bool operator()(const StopOnRoute &lhs, const StopOnRoute &rhs) const {
            return lhs.stop_name == rhs.stop_name && lhs.bus_name == rhs.bus_name && lhs.stop_number == rhs.stop_number;
        }

    private:
        std::hash<size_t> hasher_num_;
        std::hash<std::string_view> hasher_;
    };

public:
    TransportCatalogueRouterGraph(const transport_catalogue::TransportCatalogue &tc, RoutingSettings rs);

    ~TransportCatalogueRouterGraph() = default;

    std::optional<graph::Router<double>::RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

    const StopOnRoute &GetStopById(graph::VertexId id) const;

    const TwoStopsLink &GetLinkById(graph::EdgeId id) const;

    double GetBusWaitingTime() const;

private:
    const transport_catalogue::TransportCatalogue &tc_;
    RoutingSettings rs_;
    graph::EdgeId edge_count_ = 0;
    std::unique_ptr<graph::Router<double>> router_ptr_;

    std::unordered_map<StopOnRoute, graph::VertexId, StopOnRoute, StopOnRoute> stop_to_vertex_;
    std::unordered_map<size_t, StopOnRoute> vertex_to_stop_;
    graph::VertexId vertex_id_count_ = 0;

    std::unordered_map<TwoStopsLink, graph::EdgeId, TwoStopsLink, TwoStopsLink> stoplink_to_edge_;
    std::unordered_map<graph::EdgeId, TwoStopsLink> edge_to_stoplink_;

    graph::VertexId RegisterStop(const StopOnRoute &stop);

    graph::EdgeId StoreLink(const TwoStopsLink &link, graph::EdgeId edge);

    std::optional<graph::EdgeId> CheckLink(const TwoStopsLink &link) const;

    graph::VertexId GetStopVertexId(std::string_view stop_name) const;

    void FillWithReturnRouteStops(const transport_catalogue::BusRoute *bus_route);

    void FillWithCircleRouteStops(const transport_catalogue::BusRoute *bus_route);

    double CalculateTimeForDistance(int distance) const;
};