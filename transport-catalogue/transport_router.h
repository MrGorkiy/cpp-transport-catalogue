#pragma once

#include "graph.h"
#include "router.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "unordered_map"
#include "unordered_set"
#include <memory>

namespace TransportRouter {
    using namespace domain;

    class TransportRouter {
    public:
        using OptRouteInfo = std::optional<graph::Router<double>::RouteInfo>;

        // дополнительная информация о ребре
        struct EdgeAditionInfo {
            std::string bus_name; // имя автобуса едущего по ребру
            size_t count_spans = 0; // кол-во пролетов между остановками в этом ребре
        };

        TransportRouter() = default;

        // создает граф
        void CreateGraph(const TransportCatalogue::TransportCatalogue &db);

        // возвращает маршрут и статистику по нему
        std::optional<RoutStat> GetRouteStat(size_t id_stop_from, size_t id_stop_to) const;

        // создает и возвращает маршрутизатор если его еще нет
        const std::unique_ptr<graph::Router<double>> &GetRouter() const;

        // Граф не создан
        bool GetGraphIsNoInit() const;

        void vInit(RoutingSettings routing_settings_, const TransportCatalogue::TransportCatalogue &t_c);

        const std::vector<EdgeAditionInfo> &GetEdgesBuses() const;

        const std::vector<std::string> &GetIdStopes() const;

        const graph::DirectedWeightedGraph<double> &GetGraph() const;

        void SetEdgesBuses(std::vector<EdgeAditionInfo> &&edges_buses);

        void SetIdStopes(std::vector<std::string> &&id_stopes);

        void SetGraph(graph::DirectedWeightedGraph<double> &&graph);

        const RoutingSettings &GetRoutingSettings() const;

        void SetRoutingSettings(RoutingSettings &&routing_settings);

    private:
        // параметры маршрута скорость, ожидание
        RoutingSettings routing_settings_;

        // хранит дополнительная информация о ребре по индексу ребра
        std::vector<EdgeAditionInfo> edges_buses_;

        // хранит имена остановок по индексу
        std::vector<std::string> id_stopes_;

        // граф
        std::optional<graph::DirectedWeightedGraph<double>> opt_graph_;

        // маршрутизатор
        mutable std::unique_ptr<graph::Router<double>> up_router_;
    };
}