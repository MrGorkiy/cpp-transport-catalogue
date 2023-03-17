#include "transport_catalogue.h"

#include <algorithm>

using namespace std;

namespace transport_catalogue {

    void TransportCatalogue::AddStop(std::string_view name, double latitude, double longitude) {
        stops_.push_back({{name.begin(), name.end()}, latitude, longitude});
        stop_by_name_[stops_.back().name] = &stops_.back();
    }

    void TransportCatalogue::AddRoute(std::string_view number, RouteType type, const std::vector<std::string_view> &stops) {
        Bus result;
        result.number = string(number);

        for (auto &stop: stops) {
            auto found_stop = GetStop(stop);

            if (found_stop != nullptr) {
                result.stops.push_back(found_stop);
            }
        }
        if (type == RouteType::Direct) {
            vector<StopPtr> temp;
            temp = result.stops;

            for (int i = temp.size() - 2; i >= 0; --i) {
                result.stops.push_back(result.stops[i]);
            }
        }

        buses_.push_back(std::move(result));
        bus_by_name_[buses_.back().number] = &buses_.back();

        for (auto &stop: stops) {
            auto found_stop = GetStop(stop);

            if (found_stop != nullptr) {
                bus_by_stop_[found_stop].insert(buses_.back().number);
            }
        }
    }

    BusPtr TransportCatalogue::GetRoute(std::string_view name) {
        return bus_by_name_.count(name) ? bus_by_name_.at(name) : nullptr;
    }

    StopPtr TransportCatalogue::GetStop(std::string_view name) {
        return stop_by_name_.count(name) ? stop_by_name_.at(name) : nullptr;
    }

    set<std::string_view> TransportCatalogue::GetBuses(std::string_view stop) {
        if (auto found_stop = GetStop(stop); found_stop != nullptr) {
            if (bus_by_stop_.count(found_stop)) {
                return bus_by_stop_.at(found_stop);
            }
        }
        return {};
    }

    void TransportCatalogue::SetStopDistance(std::string_view stop1, uint64_t dist, std::string_view stop2) {
        auto p_stop1 = GetStop(stop1);
        auto p_stop2 = GetStop(stop2);

        if (p_stop1 != nullptr && p_stop2 != nullptr) {
            auto [it, success] = di_to_stop.try_emplace(std::make_pair(p_stop1, p_stop2), dist);
            if (!success) {
                it->second = dist;
            }
        }
    }

    uint64_t TransportCatalogue::GetStopDistance(StopPtr p_stop1, StopPtr p_stop2) {
        if (p_stop1 != nullptr && p_stop2 != nullptr) {
            if (di_to_stop.count({p_stop1, p_stop2})) {
                return di_to_stop.at({p_stop1, p_stop2});
            } else {
                if (di_to_stop.count({p_stop2, p_stop1})) {
                    return di_to_stop.at({p_stop2, p_stop1});
                }
            }
        }
        return 0;
    }

    BusStat TransportCatalogue::GetStatistics(BusPtr bus) {
        BusStat statistics;

        statistics.number_of_stops = bus->stops.size();

        auto temp = bus->stops;
        sort(temp.begin(), temp.end());
        auto it = unique(temp.begin(), temp.end());
        temp.resize(distance(temp.begin(), it));
        statistics.unique_stops = temp.size();

        // Считаем расстояния между остановками
        double distance = 0.0;
        uint64_t real_distance = 0;
        for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
            auto stop1 = bus->stops[i];
            auto stop2 = bus->stops[i + 1];

            distance += ComputeDistance(stop1->coordinates, stop2->coordinates);
            real_distance += GetStopDistance(stop1, stop2);
        }
        statistics.real_distance = real_distance;
        statistics.curvature = real_distance / distance;

        return statistics;
    }

}//namespace transport_catalogue