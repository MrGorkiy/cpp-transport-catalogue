#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <set>


namespace transport_catalogue::output {

// Вывод информации о маршруте
    void OutputRouteAbout(TransportCatalogue &tc, std::string_view route_name) {
        const Bus *route = tc.GetRoute(route_name);
        if (!route) {
            std::cout << "Bus " << route_name << ": not found\n";
        } else {
            const BusStat &stat = tc.GetStatistics(route);

            std::cout << "Bus " << route_name << ": " << stat.number_of_stops
                      << " stops on route, " << stat.unique_stops
                      << " unique stops, " << std::setprecision(6)
                      << stat.real_distance << " route length, "
                      << std::setprecision(6) << stat.curvature
                      << " curvature\n";
        }
    }

// Вывод информации об остановке
    void OutputStopAbout(TransportCatalogue &tc, std::string_view name) {
        const Stop *stop = tc.GetStop(name);
        if (!stop) {
            std::cout << "Stop " << name << ": not found" << std::endl;
            return;
        }
        std::set<std::string_view> buses = tc.GetBuses(name);
        if (buses.empty()) {
            std::cout << "Stop " << name << ": no buses" << std::endl;
            return;
        }
        std::cout << "Stop " << name << ": buses ";
        for (auto it = buses.begin(); it != buses.end(); ++it) {
            if (it != buses.begin()) {
                std::cout << " ";
            }
            std::cout << (*it);
        }
        std::cout << std::endl;
    }

    void OutputAbout(TransportCatalogue &tc, const query::Command &command) {
        if (command.type == query::QueryType::StopX) {
            OutputStopAbout(tc, command.name);
        }

        if (command.type == query::QueryType::BusX) {
            OutputRouteAbout(tc, command.name);
        }
    }

}//namespace transport_catalogue