#include "input_reader.h"
#include "stat_reader.h"

#include <algorithm>
#include <unordered_map>

using namespace std;

namespace transport_catalogue::query {

    inline std::vector<std::string_view> Split(std::string_view str, char delim) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        const size_t pos_end = str.npos;

        while (true) {
            const size_t delimiter = str.find(delim, pos);
            result.push_back(delimiter == pos_end ? str.substr(pos) : str.substr(pos, delimiter - pos));

            if (delimiter == pos_end) {
                break;
            } else {
                pos = delimiter + 1;
            }
        }

        return result;
    }

    std::pair<std::string_view, std::string_view>
    Command::ParseCoordinates(std::string_view latitude, std::string_view longitude) {
        latitude.remove_prefix(std::min(latitude.find_first_not_of(' '), latitude.size()));
        longitude.remove_prefix(std::min(longitude.find_first_not_of(' '), longitude.size()));
        latitude.remove_suffix(std::min(latitude.size() - latitude.find_last_not_of(' ') - 1, latitude.size()));
        longitude.remove_suffix(std::min(longitude.size() - longitude.find_last_not_of(' ') - 1, longitude.size()));
        return {latitude, longitude};
    }

    std::vector<std::pair<std::string_view, std::string_view>>
    Command::ParseDistances(std::vector<std::string_view> vec_input) {
        std::vector<std::pair<std::string_view, std::string_view>> result;
        size_t i = 2;

        while (i < vec_input.size()) {
            vec_input[i].remove_prefix(std::min(vec_input[i].find_first_not_of(' '), vec_input[i].size()));

            const size_t pos_m = vec_input[i].find('m');
            std::string_view dist = vec_input[i].substr(0, pos_m);
            size_t pos_t = vec_input[i].find('t') + 2;
            std::string_view stop = vec_input[i].substr(pos_t, vec_input[i].length() - pos_t);
            stop.remove_prefix(std::min(stop.find_first_not_of(' '), stop.size()));
            stop.remove_suffix(std::min(stop.size() - stop.find_last_not_of(' ') - 1, stop.size()));
            result.emplace_back(dist, stop);
            ++i;
        }
        return result;
    }

    std::vector<std::string_view> Command::ParseBuses(std::vector<std::string_view> vec_input) {
        std::vector<std::string_view> result;
        std::vector<std::string_view> parsed_buses;

        if (vec_input.size() > 3) {
            if (desc_command.find('-') != std::string_view::npos) {
                route_type = RouteType::Direct;
                parsed_buses = Split(desc_command, '-');
            }

            if (desc_command.find('>') != std::string_view::npos) {
                route_type = RouteType::Round;
                parsed_buses = Split(desc_command, '>');
            }
        }
        for (auto &bus: parsed_buses) {
            bus.remove_prefix(std::min(bus.find_first_not_of(' '), bus.size()));
            bus.remove_suffix(std::min(bus.size() - bus.find_last_not_of(' ') - 1, bus.size()));
            result.push_back(bus);
        }

        return result;
    }

    //  Удаляет все начальные и конечные пробельные символы из строки
    void trim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));

        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    void Command::ParseCommandString(string input) {
        static const unordered_map<string, QueryType> table = {
                {"Stop", QueryType::StopX},
                {"Bus",  QueryType::BusX}
        };
        origin_command = input;
        const auto vec_input = Split(origin_command, ' ');
        const auto pos_start = origin_command.find_first_not_of(' ');
        const auto pos_end_of_command = origin_command.find(' ', pos_start);
        const string temp_type = {origin_command.begin() + pos_start, origin_command.begin() + pos_end_of_command};

        switch (table.at(temp_type)) {
            case QueryType::StopX: {
                type = QueryType::StopX;
                const auto pos = origin_command.find(':');
                if (pos != string::npos) {
                    desc_command = origin_command.substr(pos + 2);
                    auto temp = Split(desc_command, ',');
                    name = origin_command.substr(pos_end_of_command + 1, pos - pos_end_of_command - 1);
                    trim(name);
                    coordinates = ParseCoordinates(temp[0], temp[1]);
                    if (temp.size() > 2) {
                        distances = ParseDistances(temp);
                    }
                } else {
                    name = origin_command.substr(pos_end_of_command + 1);
                    trim(name);
                }
                break;
            }
            case QueryType::BusX: {
                type = QueryType::BusX;
                const auto pos = origin_command.find(':');
                if (pos != string::npos) {
                    desc_command = origin_command.substr(pos + 2);
                    name = origin_command.substr(pos_end_of_command + 1, pos - pos_end_of_command - 1);
                    trim(name);
                    route = ParseBuses(vec_input);
                } else {
                    name = origin_command.substr(pos_end_of_command + 1);
                    trim(name);
                }
                break;
            }
        }
    }

    void InputReader::ParseInput() {
        int query_count;
        cin >> query_count;
        cin.ignore();
        for (int i = 0; i < query_count; ++i) {
            string command;
            getline(cin, command);
            Command cur_command;
            cur_command.ParseCommandString(std::move(command));
            commands_.emplace_back(std::move(cur_command));
        }
    }

    void InputReader::Load(TransportCatalogue &tc) {
        auto it_desc = std::partition(commands_.begin(), commands_.end(),
                                      [](const Command &command) { return !command.desc_command.empty(); });
        auto it_stops = std::partition(commands_.begin(), it_desc,
                                       [](const Command &command) { return command.type == QueryType::StopX; });

        for (auto cur_it = commands_.begin(); cur_it != it_stops; ++cur_it) {
            InputReader::LoadCommand(tc, *cur_it, false);
        }
        for (auto cur_it = commands_.begin(); cur_it != it_stops; ++cur_it) {
            InputReader::LoadCommand(tc, *cur_it, true);
        }
        for (auto cur_it = it_stops; cur_it != it_desc; ++cur_it) {
            InputReader::LoadCommand(tc, *cur_it, false);
        }
        for (auto cur_it = it_desc; cur_it != commands_.end(); ++cur_it) {
            InputReader::LoadCommand(tc, *cur_it, false);
        }
    }

    void InputReader::LoadCommand(TransportCatalogue &tc, Command command, bool dist) {
        switch (command.type) {
            case QueryType::StopX:
                if (command.coordinates != pair<string_view, string_view>()) {

                    string lat = {command.coordinates.first.begin(), command.coordinates.first.end()};
                    string lon = {command.coordinates.second.begin(), command.coordinates.second.end()};

                    if (!dist) {
                        tc.AddStop(command.name, stod(lat), stod(lon));
                    } else {
                        for (const auto &[dist, stop]: command.distances) {
                            tc.SetStopDistance(command.name, stoull(string(dist)), stop);
                        }
                    }
                } else {
                    output::OutputStopAbout(tc, command.name);
                }

                break;
            case QueryType::BusX:
                if (!command.route.empty()) {
                    tc.AddRoute(command.name, command.route_type, command.route);
                } else {
                    output::OutputRouteAbout(tc, command.name);
                }
                break;
        }
    }

}//namespace transport_catalogue