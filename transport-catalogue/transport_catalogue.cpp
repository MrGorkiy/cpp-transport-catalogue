#include <algorithm>
#include <cassert>
#include <set>


#include "transport_catalogue.h"

using namespace std;

namespace TransportCatalogue {
//----------------------------------------------------------------------------
    TransportCatalogue::TransportCatalogue() {

    }

//----------------------------------------------------------------------------
    void TransportCatalogue::AddStop(const Stop &stop) {
        stops_.push_back(move(stop));
        stops_.back().id = counter_stop_++;
        index_stops_[stops_.back().name] = &stops_.back();
    }

//----------------------------------------------------------------------------
    void TransportCatalogue::AddRangeStops(const StopsLenght &stops_lenght) {
        if (index_stops_.find(stops_lenght.from_stop) != index_stops_.end()
            && index_stops_.find(stops_lenght.to_stop) != index_stops_.end()) {
            index_rage_[pair(index_stops_.at(stops_lenght.from_stop),
                             index_stops_.at(stops_lenght.to_stop))] = stops_lenght.lenght;
        } else {
            cerr << "index_stops_.find(stops_lenght.from/to_stop) == index_stops_.end()";
        }
    }

//----------------------------------------------------------------------------
    void TransportCatalogue::AddBusesFromStop(const Bus &bus) {
        for (const auto &stop: bus.stops) {
            buses_from_stop_[stop->name].insert(&bus);
        }
    }

//----------------------------------------------------------------------------
    std::vector<const Bus *> TransportCatalogue::GetBusesLex() const {
        // использую сет как фильтр уникальных и сортировщик, кладу в вектор так как потом надо будет только итерироваться
        std::set<const domain::Bus *, domain::CmpBuses> set;
        for (const auto &bus: buses_) {
            set.insert(&bus);
        }
        return {set.begin(), set.end()};
    }

//----------------------------------------------------------------------------
    size_t TransportCatalogue::GetRangeStops(const Stop *from_stop, const Stop *to_stop) const {
        size_t result = 0;
        if (index_rage_.count(pair(from_stop, to_stop)) != 0) {
            result = index_rage_.at(pair(from_stop, to_stop));
        } else if (index_rage_.count(pair(to_stop, from_stop)) != 0) {
            result = index_rage_.at(pair(to_stop, from_stop));
        }
        return result;
    }

//----------------------------------------------------------------------------
    BusStat TransportCatalogue::GetBusStat(const Bus *bus) const {
        size_t count_stops = bus->stops.size();
        if (count_stops < 2) {
            throw "coutn_stops < 2";
        }
        size_t length = 0;
        double range = 0;
        std::set<const domain::Stop *> stops(bus->stops.begin(), bus->stops.end());
        for (size_t i = 0, j = 1; j < count_stops; ++i, ++j) {
            const domain::Stop *from_stop = bus->stops[i];
            const domain::Stop *to_stop = bus->stops[j];
            length += GetRangeStops(from_stop, to_stop);
            range += domain::ComputeDistance(from_stop, to_stop);
        }
        return {bus->name, count_stops, stops.size(), length, length / range};
    }

//----------------------------------------------------------------------------
    void TransportCatalogue::AddBus(const Bus &bus) {
        buses_.push_back(move(bus));
        index_buses_[buses_.back().name] = &buses_.back();
        AddBusesFromStop(buses_.back());
    }

//----------------------------------------------------------------------------
    std::optional<const Bus *> TransportCatalogue::FindBus(std::string_view bus_name) const {
        if (index_buses_.count(bus_name) == 0) {
            return nullopt;
        }
        return index_buses_.at(bus_name);
    }

//----------------------------------------------------------------------------
    std::optional<const Stop *> TransportCatalogue::FindStop(std::string_view stop_name) const {
        if (index_stops_.count(stop_name) == 0) {
            return nullopt;
        }
        return index_stops_.at(stop_name);
    }

//----------------------------------------------------------------------------
    const std::map<std::string_view, std::unordered_set<const Bus *>> &TransportCatalogue::GetBusesFromStop() const {
        return buses_from_stop_;
    }

//----------------------------------------------------------------------------
    const std::deque<Bus> &TransportCatalogue::GetBuses() const {
        return buses_;
    }

//----------------------------------------------------------------------------
    const TransportCatalogue::UmapRangStop &TransportCatalogue::GetIndexRageStop() const {
        return index_rage_;
    }
//----------------------------------------------------------------------------
} // namespace TransportCatalogue