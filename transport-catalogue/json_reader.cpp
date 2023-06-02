#include <algorithm>
#include <ostream>
#include <sstream>
#include "json_reader.h"

namespace JsonReader {
//----------------------------------------------------------------------------
    JsonReader::JsonReader(TransportCatalogue::TransportCatalogue &t_c,
                           TransportRouter::TransportRouter &tr,
                           RequestHandler &req_hand,
                           renderer::MapRenderer &renderer)
            : t_c_(t_c), t_r_(tr), req_hand_(req_hand), m_r_(renderer) {

    }

//----------------------------------------------------------------------------
    void JsonReader::ParseJsonMakeBase(std::istream &in) {
        using namespace domain;

        auto main_map = std::move(json::Load(in).GetRoot().AsDict());
        if (auto it = main_map.find(MainReq::base); it != main_map.end()) {
            auto vec_map = std::move(it->second.AsArray());
            ParseRequestsBase(std::move(vec_map));
        }
        if (auto it = main_map.find(MainReq::render_settings); it != main_map.end()) {
            auto map = std::move(it->second.AsDict());
            ParseRequestsRendSett(std::move(map));
        }
        if (auto it = main_map.find(MainReq::routing_settings); it != main_map.end()) {
            auto map = std::move(it->second.AsDict());
            ParseRequestsRoutSett(std::move(map));
        }
        std::string path;
        if (auto it = main_map.find(MainReq::srlzt_settings); it != main_map.end()) {
            auto map = std::move(it->second.AsDict());
            ParseRequestsSrlz(std::move(map), path);
        }
        req_hand_.CallSrlz(path);
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseJsonProcessRequests(std::istream &in) {
        using namespace domain;

        auto main_map = std::move(json::Load(in).GetRoot().AsDict());
        std::string path;
        if (auto it = main_map.find(MainReq::srlzt_settings); it != main_map.end()) {
            auto map = std::move(it->second.AsDict());
            ParseRequestsSrlz(std::move(map), path);
        }
        req_hand_.CallDsrlz(path);
        if (auto it = main_map.find(MainReq::stat); it != main_map.end()) {
            auto vec_map = std::move(it->second.AsArray());
            ParseRequestsStat(vec_map);
        }
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsSrlz(const json::Dict &&req, std::string &path) {
        using namespace domain;
        if (req.find(MainReq::file) != req.end()) {
            path = std::move(req.at(MainReq::file).AsString());
        }
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsBase(json::Array &&vec_map) {
        using namespace domain;

        auto compare = [](const auto &lh, const auto &rh) {
            return lh.AsDict().at(MainReq::type).AsString() > rh.AsDict().at(MainReq::type).AsString();
        };
        std::sort(vec_map.begin(), vec_map.end(), compare);

        for (const auto &map_type_data: vec_map) {
            if (const auto &map_type_stop = map_type_data.AsDict(); map_type_stop.at(MainReq::type).AsString() ==
                                                                    MainReq::stop) {
                t_c_.AddStop(ParseRequestsStops(map_type_stop));
            } else {
                auto bus = ParseRequestsBuses(map_type_data.AsDict());
                t_c_.AddBus(bus);
            }
        }

        for (const auto &map_type_data: vec_map) {
            if (const auto &map_type_stop = map_type_data.AsDict(); map_type_stop.at(MainReq::type).AsString() ==
                                                                    MainReq::stop) {
                ParseRequestsStopsLenght(move(map_type_stop));
            }
        }
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsStat(const json::Array &vec_map) {
        using namespace domain;
        using namespace MainReq;

        std::vector<domain::RequestOut> requests;

        requests.reserve(vec_map.size());
        for (const auto &req: vec_map) {
            const auto cur_req = req.AsDict();
            RequestOut request;
            request.id = std::move(cur_req.at(id).AsInt());
            request.type = std::move(cur_req.at(type).AsString());
            if (request.type == bus || request.type == stop) {
                request.name = std::move(cur_req.at(name).AsString());
            } else if (request.type == route) {
                request.name = std::move(cur_req.at(from).AsString());
                request.name_to = std::move(cur_req.at(to).AsString());
            }
            requests.emplace_back(std::move(request));
        }
        ExecRequestsStat(std::move(requests));
    }

//----------------------------------------------------------------------------
    void JsonReader::ExecRequestsStat(std::vector<domain::RequestOut> &&requests) {
        json::Array vec;
        for (const auto &req: requests) {
            if (req.type == domain::MainReq::stop) {
                vec.emplace_back(PrintResReqStop(req_hand_.GetBusesByStop(req.name), req.id));
            } else if (req.type == domain::MainReq::bus) {
                vec.emplace_back(PrintResReqBus(req_hand_.GetBusStat(req.name), req.id));
            } else if (req.type == domain::MainReq::map) {
                vec.emplace_back(PrintResReqMap(req_hand_.RenderMap(), req.id));
            } else if (req.type == domain::MainReq::route) {
                vec.emplace_back(PrintResReqRoute(req_hand_.GetRouteStat(req.name, req.name_to.value()), req.id));
            }
        }
        json::Print(json::Document{json::Node{vec}}, std::cout);
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsRendSett(const json::Dict &&map) {
        using namespace renderer::RenderSettingsKey;
        renderer::RenderSettings rnd_sett{};
        if (map.find(width) != map.end()) {
            rnd_sett.width = map.at(width).AsDouble();
        }
        if (map.find(height) != map.end()) {
            rnd_sett.height = map.at(height).AsDouble();
        }
        if (map.find(padding) != map.end()) {
            rnd_sett.padding = map.at(padding).AsDouble();
        }
        if (map.find(line_width) != map.end()) {
            rnd_sett.line_width = map.at(line_width).AsDouble();
        }
        if (map.find(stop_radius) != map.end()) {
            rnd_sett.stop_radius = map.at(stop_radius).AsDouble();
        }
        if (map.find(bus_label_font_size) != map.end()) {
            rnd_sett.bus_label_font_size = map.at(bus_label_font_size).AsInt();
        }
        if (map.find(bus_label_offset) != map.end()) {
            const auto &vec = map.at(bus_label_offset).AsArray();
            rnd_sett.bus_label_offset = {vec[0].AsDouble(), vec[1].AsDouble()};
        }
        if (map.find(stop_label_font_size) != map.end()) {
            rnd_sett.stop_label_font_size = map.at(stop_label_font_size).AsInt();
        }
        if (map.find(stop_label_offset) != map.end()) {
            const auto &vec = map.at(stop_label_offset).AsArray();
            rnd_sett.stop_label_offset = {vec[0].AsDouble(), vec[1].AsDouble()};
        }
        if (map.find(underlayer_color) != map.end()) {
            if (map.at(underlayer_color).IsString()) {
                rnd_sett.underlayer_color = map.at(underlayer_color).AsString();
            }
            if (map.at(underlayer_color).IsArray()) {
                const auto &vec = map.at(underlayer_color).AsArray();
                if (vec.size() == 3) {
                    rnd_sett.underlayer_color = svg::Rgb{static_cast<uint8_t>(vec[0].AsInt()),
                                                         static_cast<uint8_t>(vec[1].AsInt()),
                                                         static_cast<uint8_t>(vec[2].AsInt())};
                } else if (vec.size() == 4) {
                    rnd_sett.underlayer_color = svg::Rgba{static_cast<uint8_t>(vec[0].AsInt()),
                                                          static_cast<uint8_t>(vec[1].AsInt()),
                                                          static_cast<uint8_t>(vec[2].AsInt()), vec[3].AsDouble()};
                }

            } else {
                //std:: cout << "underlayer_color unknow type" << std::endl;
            }
        }
        if (map.find(underlayer_width) != map.end()) {
            rnd_sett.underlayer_width = map.at(underlayer_width).AsDouble();
        }
        if (map.find(color_palette) != map.end()) {
            const auto &vec = map.at(color_palette).AsArray();
            rnd_sett.color_palette.clear(); // отчистка заначений по умолчанию
            for (const auto &node: vec) {
                svg::Color color;
                if (node.IsString()) {
                    color = node.AsString();
                } else if (node.IsArray()) {
                    const auto &vec = node.AsArray();
                    if (vec.size() == 3) {
                        color = svg::Rgb{static_cast<uint8_t>(vec[0].AsInt()), static_cast<uint8_t>(vec[1].AsInt()),
                                         static_cast<uint8_t>(vec[2].AsInt())};
                    } else if (vec.size() == 4) {
                        color = svg::Rgba{static_cast<uint8_t>(vec[0].AsInt()), static_cast<uint8_t>(vec[1].AsInt()),
                                          static_cast<uint8_t>(vec[2].AsInt()), vec[3].AsDouble()};
                    }
                } else {
                    //std:: cout << "color_palette unknow type" << std::endl;
                }
                rnd_sett.color_palette.emplace_back(color);
            }
        }
        m_r_.SetRenderSettings(std::move(rnd_sett));
        m_r_.SetBuses(req_hand_.GetBusesLex());
        m_r_.SetUnicStops(req_hand_.GetUnicLexStopsIncludeBuses());
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsRoutSett(const json::Dict &&req) {
        using namespace domain;
        using namespace MainReq;
        try {
            RoutingSettings rout_set;
            if (req.find(bus_wait_time) != req.end()) {
                rout_set.bus_wait_time_minut = req.at(bus_wait_time).AsInt();
            }
            if (req.find(bus_velocity) != req.end()) {
                rout_set.bus_velocity = req.at(bus_velocity).AsDouble();
            }
            t_r_.vInit(std::move(rout_set), t_c_);
        } catch (...) {
            std::cout << "ParseRequestsRoutSett FAIL" << std::endl;
            throw;
        }
    }

//----------------------------------------------------------------------------
    domain::Stop JsonReader::ParseRequestsStops(const json::Dict &req) {
        using namespace domain;
        try {
            return {req.at(MainReq::name).AsString(),
                    {req.at(MainReq::lat).AsDouble(), req.at(MainReq::lon).AsDouble()}, 0};
        } catch (...) {
            std::cout << "Fail Stop" << std::endl;
            throw;
        }
    }

//----------------------------------------------------------------------------
    domain::Bus JsonReader::ParseRequestsBuses(const json::Dict &req) {
        using namespace domain;
        if (req.at(MainReq::type).AsString() == MainReq::bus) {
            try {
                std::vector<const domain::Stop *> stops;
                for (const auto &stop_name: req.at(MainReq::stops).AsArray()) {
                    const auto &str_stop_name = stop_name.AsString();
                    try {
                        stops.push_back(t_c_.FindStop(str_stop_name).value());
                    } catch (...) {
                        std::cout << "tc_.FindStop(name_stop).value()";
                    }
                }
                // если не кольцевой маршрут дублируем остановки в обратном порядке
                if (!req.at(MainReq::is_roundtrip).AsBool() && stops.size() >= 2) {
                    size_t size = stops.size();
                    stops.reserve(size * 2);
                    std::vector<const domain::Stop *>::iterator it = stops.end() - 2;
                    for (size_t i = 0; i < size - 1; ++i, --it) {
                        stops.push_back(*it);
                    }
                }
                return {req.at(MainReq::name).AsString(), move(stops), req.at(MainReq::is_roundtrip).AsBool()};
            } catch (...) {
                std::cout << "Fail Bus" << std::endl;
                throw;
            }
        } else {
            std::cout << "it's not bus" << std::endl;
            throw;
        }
    }

//----------------------------------------------------------------------------
    void JsonReader::ParseRequestsStopsLenght(const json::Dict &req) {
        using namespace domain;
        const std::string &name_from_stop = req.at(MainReq::name).AsString();
        std::string range;
        try {
            for (const auto &rd: req.at(MainReq::road_distances).AsDict()) {
                t_c_.AddRangeStops({std::move(name_from_stop), rd.first, static_cast<size_t>(rd.second.AsInt())});
            }
        } catch (...) {
            std::cout << "ParseRequestsStopsLenght FAIL" << std::endl;
            throw;
        }
    }

//----------------------------------------------------------------------------
    json::Dict JsonReader::PrintResReqBus(std::optional<domain::BusStat> &&bus_stat_opt, int id) {
        if (bus_stat_opt) {
            const auto &[name, count_stops, count_unic_stops, lengh, curvature] = *bus_stat_opt;
            return json::Builder{}.StartDict().Key("curvature"s).Value(curvature)
                    .Key("request_id"s).Value(id)
                    .Key("route_length"s).Value(static_cast<double>(lengh))
                    .Key("stop_count"s).Value(static_cast<double>(count_stops))
                    .Key("unique_stop_count"s).Value(static_cast<double>(count_unic_stops)).EndDict().Build().AsDict();
        } else {
            return json::Builder{}.StartDict().Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s).EndDict().Build().AsDict();
        }
    }

//----------------------------------------------------------------------------
    json::Dict
    JsonReader::PrintResReqStop(std::optional<const std::unordered_set<const domain::Bus *> *> buses_opt, int id) {
        if (buses_opt) {
            const auto &buses = *buses_opt;
            // для сортировки по возрастанию
            std::set<std::string_view> buss;
            for (const auto &bus: *buses) {
                buss.insert(bus->name);
            }
            json::Array vec;
            vec.reserve(buses->size());
            for (const auto &bus: buss) {
                vec.push_back(std::string(bus));
            }
            return json::Builder{}.StartDict().Key("buses"s).Value(vec)
                    .Key("request_id"s).Value(id).EndDict().Build().AsDict();
        } else {
            return json::Builder{}.StartDict().Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s).EndDict().Build().AsDict();
        }
    }

//----------------------------------------------------------------------------
    json::Dict JsonReader::PrintResReqMap(std::optional<svg::Document> &&doc_opt, int id) {
        if (!doc_opt) {
            return {};
        }
        const auto &doc = doc_opt.value();
        std::ostringstream str;
        doc.Render(str);
        return json::Builder{}.StartDict().Key("map"s).Value(str.str())
                .Key("request_id"s).Value(id).EndDict().Build().AsDict();
    }

//----------------------------------------------------------------------------
    json::Dict JsonReader::PrintResReqRoute(std::optional<domain::RoutStat> &&rout_stat_opt, int id) {
        using namespace domain;
        using TransportCatalogue::RoutStat;
        if (!rout_stat_opt) {
            return json::Builder{}.StartDict().Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s).EndDict().Build().AsDict();
        }

        const auto &rout_stat = *rout_stat_opt;
        json::Array vec;
        for (const RoutStat::VariantItem &item: rout_stat.items) {
            json::Dict dict;
            if (std::holds_alternative<RoutStat::ItemsWait>(item)) {
                auto it = std::get<RoutStat::ItemsWait>(item);
                dict.insert({"stop_name"s, it.stop_name});
                dict.insert({"time"s, it.time});
                dict.insert({"type"s, it.type});
            } else if (std::holds_alternative<RoutStat::ItemsBus>(item)) {
                auto it = std::get<RoutStat::ItemsBus>(item);
                dict.insert({"bus"s, it.bus});
                dict.insert({"span_count"s, static_cast<int>(it.span_count)});
                dict.insert({"time"s, it.time});
                dict.insert({"type"s, it.type});
            }
            vec.push_back(dict);
        }
        return json::Builder{}.StartDict().Key("items"s).Value(vec)
                .Key("total_time"s).Value(rout_stat.total_time)
                .Key("request_id"s).Value(id).EndDict().Build().AsDict();
    }
//----------------------------------------------------------------------------
}// namespace JsonReader
