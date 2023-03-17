#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <set>


namespace transport_catalogue::output {

// Вывод информации о маршруте
    void OutputRouteAbout(std::ostream &out, TransportCatalogue &tc, std::string_view route_name) {
        const Bus *route = tc.GetRoute(route_name);
        if (!route) {
            out << "Bus " << route_name << ": not found\n";
        } else {
            const BusStat &stat = tc.GetStatistics(route);

            out << "Bus " << route_name << ": " << stat.number_of_stops
                << " stops on route, " << stat.unique_stops
                << " unique stops, " << std::setprecision(6)
                << stat.real_distance << " route length, "
                << std::setprecision(6) << stat.curvature
                << " curvature\n";
        }
    }

// Вывод информации об остановке
    void OutputStopAbout(std::ostream &out, TransportCatalogue &tc, std::string_view name) {
        const Stop *stop = tc.GetStop(name);
        if (!stop) {
            out << "Stop " << name << ": not found\n";
            return;
        }
        std::set<std::string_view> buses = tc.GetBuses(name);
        if (buses.empty()) {
            out << "Stop " << name << ": no buses\n";
            return;
        }
        out << "Stop " << name << ": buses ";
        for (auto it = buses.begin(); it != buses.end(); ++it) {
            if (it != buses.begin()) {
                out << " ";
            }
            out << (*it);
        }
        out << std::endl;
    }

    void OutputAbout(std::ostream &out, TransportCatalogue &tc, const query::Command &command) {
        if (command.type == query::QueryType::StopX) {
            OutputStopAbout(out, tc, command.name);
        }

        if (command.type == query::QueryType::BusX) {
            OutputRouteAbout(out, tc, command.name);
        }
    }

    void ProcessRequests(std::ostream &out, TransportCatalogue &catalogue) {
        size_t requests_count;
        std::cin >> requests_count;
        for (size_t i = 0; i < requests_count; ++i) {
            std::string keyword, line;
            std::cin >> keyword;
            std::getline(std::cin, line);
            query::Command cur_command;
            cur_command.name = line.substr(1, std::string::npos);
            if (keyword == "Bus") {
                cur_command.type = query::QueryType::BusX;
                OutputAbout(out, catalogue, cur_command);
            }
            if (keyword == "Stop") {
                cur_command.type = query::QueryType::StopX;
                OutputAbout(out, catalogue, cur_command);
            }
        }
    }

}//namespace transport_catalogue