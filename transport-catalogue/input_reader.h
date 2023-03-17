#pragma once

#include "transport_catalogue.h"

#include <charconv>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace transport_catalogue::query {

    enum class QueryType {
        StopX,
        BusX
    };

    struct Command {
        QueryType type;
        std::string name;
        //data
        std::pair<std::string_view, std::string_view> coordinates;
        std::vector<std::pair<std::string_view, std::string_view>> distances;
        std::vector<std::string_view> route;
        RouteType route_type = RouteType::Direct;
        std::string origin_command;
        std::string desc_command;


        std::pair<std::string_view, std::string_view>
        ParseCoordinates(std::string_view latitude, std::string_view longitude);

        std::vector<std::string_view> ParseBuses(std::vector<std::string_view> vec_input);

        std::vector<std::pair<std::string_view, std::string_view>>
        ParseDistances(std::vector<std::string_view> vec_input);

        void ParseCommandString(std::string input);
    };

    inline std::vector<std::string_view> Split(std::string_view string, char delim);

    class InputReader {
    public:
        void Load(TransportCatalogue &command);

        void LoadCommand(TransportCatalogue &tc, Command command, bool dist);

        void ParseInput(std::istream &in);

    private:
        std::vector<Command> commands_;
    };

}//namespace transport_catalogue