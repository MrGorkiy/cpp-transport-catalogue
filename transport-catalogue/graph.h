#pragma once

#include "ranges.h"

#include <cstdlib>
#include <vector>

namespace graph {
    // Вершина графа
    using VertexId = size_t;  // Идентификатор вершины
    using EdgeId = size_t;  // Идентификатор ребра

    template<typename Weight>
    struct Edge {  // Структура, представляющая ребро графа
        VertexId from;  // Идентификатор начальной вершины ребра
        VertexId to;  // Идентификатор конечной вершины ребра
        Weight weight;  // Вес ребра
    };

    template<typename Weight>
    class DirectedWeightedGraph {
    private:
        using IncidenceList = std::vector<EdgeId>;  // Список инцидентных ребер для каждой вершины
        using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;  // Диапазон инцидентных ребер для вершины

    public:
        DirectedWeightedGraph() = default;  // Конструктор по умолчанию

        explicit DirectedWeightedGraph(size_t vertex_count);  // Конструктор с указанием количества вершин

        EdgeId AddEdge(const Edge<Weight> &edge);  // Добавление ребра в граф

        size_t GetVertexCount() const;  // Получение количества вершин в графе

        size_t GetEdgeCount() const;  // Получение количества ребер в графе

        const Edge<Weight> &GetEdge(EdgeId edge_id) const;  // Получение ребра по его идентификатору

        IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;  // Получение диапазона инцидентных ребер для вершины

        const std::vector <Edge<Weight>> &GetEdges() const;  // Получение всех ребер графа

        const std::vector <IncidenceList> &GetIncidenceLists() const;  // Получение списков инцидентных ребер для каждой вершины

    private:
        std::vector <Edge<Weight>> edges_;  // Вектор ребер графа
        std::vector <IncidenceList> incidence_lists_;  // Вектор списков инцидентных ребер для каждой вершины
    };

    template<typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
            : incidence_lists_(vertex_count) {
    }

    template<typename Weight>
    EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight> &edge) {
        edges_.push_back(edge);  // Добавление ребра в вектор ребер
        const EdgeId id = edges_.size() - 1;  // Получение идентификатора добавленного ребра
        incidence_lists_.at(edge.from).push_back(id);  // Добавление идентификатора ребра в список инцидентных ребер для начальной вершины
        return id;  // Возвращение идентификатора ребра
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists_.size();  // Возвращение количества вершин в графе
    }

    template<typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges_.size();  // Возвращение количества ребер в графе
    }

    template<typename Weight>
    const Edge<Weight> &DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
        return edges_.at(edge_id);  // Возвращение ребра по его идентификатору
    }

    template<typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
    DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
        return ranges::AsRange(incidence_lists_.at(vertex));  // Возвращение диапазона инцидентных ребер для указанной вершины
    }

    template<typename Weight>
    const std::vector <Edge<Weight>> &DirectedWeightedGraph<Weight>::GetEdges() const {
        return edges_;  // Возвращение всех ребер графа
    }
}  // namespace graph