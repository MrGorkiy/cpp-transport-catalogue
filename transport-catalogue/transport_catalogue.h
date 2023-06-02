#pragma once

#include <iostream>
#include <unordered_set>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <unordered_map>
#include <math.h>

#include "domain.h"

namespace TransportCatalogue {

    namespace detail {

        class HasherStopes {
        public:
            size_t operator()(std::pair<const domain::Stop *, const domain::Stop *> par) const {
                size_t from_stop = std::hash<const void *>{}(par.first);
                size_t to_stop = std::hash<const void *>{}(par.second);
                return from_stop * 37 + to_stop * pow(37, 2);
            }
        };
    }// namespace detail

    using namespace domain;

    class TransportCatalogue {
        using UmapRangStop = std::unordered_map<std::pair<const Stop *, const Stop *>, size_t, detail::HasherStopes>;
    public:
        TransportCatalogue();

        void AddBus(const Bus &bus);

        void AddStop(const Stop &stop);

        void AddRangeStops(const StopsLenght &stops_lenght);

        void AddBusesFromStop(const Bus &bus);

        const std::deque<Stop> &GetStops() const {
            return stops_;
        };

        std::vector<const domain::Bus *> GetBusesLex() const;

        std::size_t GetRangeStops(const Stop *from_stop, const Stop *to_stop) const;

        domain::BusStat GetBusStat(const Bus *bus) const;

        std::optional<const Bus *> FindBus(std::string_view bus_name) const;

        std::optional<const Stop *> FindStop(std::string_view stop_name) const;

        const std::map<std::string_view, std::unordered_set<const Bus *>> &GetBusesFromStop() const;

        const std::deque<Bus> &GetBuses() const;

        const UmapRangStop &GetIndexRageStop() const;

    private:
        std::deque<Stop> stops_;

        std::unordered_map<std::string_view, const Stop *> index_stops_;

        std::deque<Bus> buses_;

        // список автобусов через остановку
        std::map<std::string_view, std::unordered_set<const Bus *>> buses_from_stop_;

        std::unordered_map<std::string_view, const Bus *> index_buses_;

        UmapRangStop index_rage_;

        size_t counter_stop_ = 0;
    };

}// namespace TransportCatalogue
