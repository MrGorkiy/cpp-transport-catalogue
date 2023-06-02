#pragma once

#include "geo.h"
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace domain {

    namespace MainReq {
        using namespace std::literals;
        // типы запросов
        // make_base
        const std::string base = "base_requests"s;
        const std::string render_settings = "render_settings"s;
        const std::string routing_settings = "routing_settings"s;
        // process_requests
        const std::string stat = "stat_requests"s;
        // common
        const std::string srlzt_settings = "serialization_settings"s;

        // параметры serialization_settings
        const std::string file = "file"s;

        // параметры routing_settings
        const std::string bus_velocity = "bus_velocity"s;
        const std::string bus_wait_time = "bus_wait_time"s;

        // параметры общие для base_requests
        const std::string name = "name"s;
        const std::string type = "type"s;

        // параметры base_requests type Stop
        const std::string lat = "latitude"s;
        const std::string lon = "longitude"s;
        const std::string road_distances = "road_distances"s;

        // параметры base_requests type Bus
        const std::string is_roundtrip = "is_roundtrip"s;
        const std::string stops = "stops"s;

        // параметры stat_requests
        const std::string id = "id"s;
        const std::string route = "Route"s;
        const std::string from = "from"s;
        const std::string to = "to"s;
        const std::string map = "Map"s;

        // параметры общие для stat_requests и base_requests
        const std::string bus = "Bus"s;
        const std::string stop = "Stop"s;
    }

    struct Request {
        std::string type;
        std::string data;
    };

    struct RequestOut {
        int id;
        std::string type;
        std::string name;
        std::optional<std::string> name_to;
    };

    struct BusStat {
        std::string name;
        std::size_t count_stops = 0;
        std::size_t count_unic_stops = 0;
        std::size_t length = 0;
        double curvature = 0;
    };

    struct StopInfo {
        const std::string &name;
        bool b_stop_is_not_exist = false;
        std::optional<std::set<std::string>> buses;
    };

    struct Stop {
        std::string name;
        geo::Coordinates coord;
        size_t id;
    };

    struct CmpStops {
        bool operator()(const domain::Stop *lth, const domain::Stop *rth) const {
            return lth->name < rth->name;
        }
    };

    struct Bus {
        std::string name;
        std::vector<const Stop *> stops;
        bool is_round;
    };

    struct CmpBuses {
        bool operator()(const domain::Bus *lth, const domain::Bus *rth) const {
            return lth->name < rth->name;
        }
    };

    struct StopsLenght {
        std::string from_stop;
        std::string to_stop;
        size_t lenght;
    };

    struct RoutingSettings {
        int bus_wait_time_minut = 0; // минуты
        double bus_velocity = 0; // км/ч
    };

    struct RoutStat {

        struct ItemsWait {
            std::string type;
            double time = 0;
            std::string stop_name;
        };

        struct ItemsBus {
            std::string type;
            double time = 0;
            size_t span_count = 0;
            std::string bus;
        };

        using VariantItem = std::variant<ItemsBus, ItemsWait>;

        double total_time = 0; // минуты
        std::vector<VariantItem> items;
    };

    double ComputeDistance(const Stop *from_stop, const Stop *to_stop);

    double GetMetrMinFromKmH(double km_h);

}//namespace domain