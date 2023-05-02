#pragma once

#include "graph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace graph {

    template<typename Weight>
    class Router {
    private:
        using Graph = DirectedWeightedGraph<Weight>;

    public:
        // класс Router, который ищет кратчайший маршрут в графе
        explicit Router(const Graph &graph);

        struct RouteInfo {
            Weight weight;  // вес маршрута
            std::vector<EdgeId> edges;  // список ребер маршрута
        };

        std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;

    private:
        struct RouteInternalData {
            Weight weight;  // вес текущего маршрута
            std::optional<EdgeId> prev_edge;  // предыдущее ребро в маршруте
        };
        // матрица для хранения данных о маршруте
        using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

        // метод инициализации матрицы данными о маршруте
        void InitializeRoutesInternalData(const Graph &graph) {
            const size_t vertex_count = graph.GetVertexCount();
            for (VertexId vertex = 0; vertex < vertex_count; ++vertex) {
                // маршрут от вершины к самой себе
                routes_internal_data_[vertex][vertex] = RouteInternalData{ZERO_WEIGHT, std::nullopt};
                for (const EdgeId edge_id: graph.GetIncidentEdges(vertex)) {
                    const auto &edge = graph.GetEdge(edge_id);
                    // проверяем, что вес ребра неотрицательный
                    if (edge.weight < ZERO_WEIGHT) {
                        throw std::domain_error("Edges' weights should be non-negative");
                    }
                    auto &route_internal_data = routes_internal_data_[vertex][edge.to];
                    // если нет данных о маршруте или вес маршрута больше нового ребра, то заменяем
                    if (!route_internal_data || route_internal_data->weight > edge.weight) {
                        route_internal_data = RouteInternalData{edge.weight, edge_id};
                    }
                }
            }
        }

        // метод обновления данных о маршруте
        void RelaxRoute(VertexId vertex_from, VertexId vertex_to, const RouteInternalData &route_from,
                        const RouteInternalData &route_to) {
            auto &route_relaxing = routes_internal_data_[vertex_from][vertex_to];
            // новый вес маршрута
            const Weight candidate_weight = route_from.weight + route_to.weight;
            // если нет данных о маршруте или вес маршрута меньше предыдущего, то заменяем
            if (!route_relaxing || candidate_weight < route_relaxing->weight) {
                route_relaxing = {candidate_weight,
                                  route_to.prev_edge ? route_to.prev_edge : route_from.prev_edge};
            }
        }

        // метод, проходящий через каждую вершину для обновления матрицы маршрутов
        void RelaxRoutesInternalDataThroughVertex(size_t vertex_count, VertexId vertex_through) {
            for (VertexId vertex_from = 0; vertex_from < vertex_count; ++vertex_from) {
                // если есть данные о маршруте от вершины vertex_from до vertex_through
                if (const auto &route_from = routes_internal_data_[vertex_from][vertex_through]) {
                    // идем до конца графа
                    for (VertexId vertex_to = 0; vertex_to < vertex_count; ++vertex_to) {
                        // если есть данные о маршруте от вершины vertex_through до vertex_to
                        if (const auto &route_to = routes_internal_data_[vertex_through][vertex_to]) {
                            // обновляем информацию о маршруте
                            RelaxRoute(vertex_from, vertex_to, *route_from, *route_to);
                        }
                    }
                }
            }
        }

        static constexpr Weight ZERO_WEIGHT{};
        const Graph &graph_;
        RoutesInternalData routes_internal_data_;
    };

    // конструктор класса Router
    template<typename Weight>
    Router<Weight>::Router(const Graph &graph)
            : graph_(graph), routes_internal_data_(graph.GetVertexCount(),
                                                   std::vector<std::optional<RouteInternalData>>(
                                                           graph.GetVertexCount())) {
        // инициализируем матрицу данными о маршрутах для вершин графа
        InitializeRoutesInternalData(graph);

        const size_t vertex_count = graph.GetVertexCount();
        for (VertexId vertex_through = 0; vertex_through < vertex_count; ++vertex_through) {
            RelaxRoutesInternalDataThroughVertex(vertex_count, vertex_through);
        }
    }

    // метод построения маршрута между двумя вершинами
    template<typename Weight>
    std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexId from,
                                                                                 VertexId to) const {
        // получаем данные о маршруте от from до to
        const auto &route_internal_data = routes_internal_data_.at(from).at(to);
        // если данные не найдены, то возвращаем нулевой массив
        if (!route_internal_data) {
            return std::nullopt;
        }
        // получаем вес маршрута
        const Weight weight = route_internal_data->weight;
        std::vector<EdgeId> edges;
        // начинаем с последнего ребра
        for (std::optional<EdgeId> edge_id = route_internal_data->prev_edge;
             edge_id;
             // проходим по всем предыдущим ребрам
             edge_id = routes_internal_data_[from][graph_.GetEdge(*edge_id).from]->prev_edge) {
            // добавляем ребро в маршрут
            edges.push_back(*edge_id);
        }
        // переворачиваем маршрут, чтобы начинать с вершины from
        std::reverse(edges.begin(), edges.end());

        // возвращаем построенный маршрут
        return RouteInfo{weight, std::move(edges)};
    }

}  // namespace graph