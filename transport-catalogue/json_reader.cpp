#include "json_reader.h"

size_t JsonReader::ReadJson(std::istream &input) {
    size_t result = 0;

    try {
        json::Document doc = json::Load(input);
        if (doc.GetRoot().IsMap()) {
            result = doc.GetRoot().AsMap().size();
            root_.push_back(std::move(doc));
        }
    } catch (const json::ParsingError &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return result;
}

size_t JsonReader::ReadJsonAndFillTransportCatalogue(std::istream &input) {
    size_t num_objects_parsed = ReadJson(input);
    if (num_objects_parsed == 0) {
        return 0;
    }

    size_t num_raw_objects = ParseJsonToRawData();
    if (num_raw_objects == 0) {
        return 0;
    }

    FillTransportCatalogue();

    return num_raw_objects;
}

size_t JsonReader::ParseJsonToRawData() {
    const json::Node &root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError("Error reading JSON data for database filling: root is not a map.");
    }

    const json::Dict &dict = root_node.AsMap();
    auto iter = dict.find(BASE_DATA);
    if (iter == dict.end() || !iter->second.IsArray()) {
        throw json::ParsingError(
                "Error reading JSON data for database filling: 'base_requests' array is missing or not an array.");
    }

    const json::Array &nodes = iter->second.AsArray();
    size_t result = nodes.size();
    raw_stops_.reserve(result);
    raw_buses_.reserve(result);

    for (const auto &node: nodes) {
        const BaseRequest data = ParseAddDataNode(node);
        if (const domain::StopWithDistances *stop = std::get_if<domain::StopWithDistances>(&data)) {
            raw_stops_.push_back(*stop);
        } else if (const BusRouteJson *bus = std::get_if<BusRouteJson>(&data)) {
            raw_buses_.push_back(*bus);
        } else {
            throw json::ParsingError("Error reading JSON data for database filling: unexpected data type.");
        }
    }

    return result;
}


BaseRequest JsonReader::ParseAddDataNode(const json::Node &node) const {
    using namespace std::literals;
    using namespace transport_catalogue;
    if (!node.IsMap()) return {};
    const json::Dict &dict = node.AsMap();
    const auto type_i = dict.find("type");
    if (type_i == dict.end()) return {};
    if (type_i->second == "Stop") {
        StopWithDistances stop;

        const auto name_i = dict.find("name");
        if (name_i == dict.end() || !name_i->second.IsString()) return {};
        stop.stop_name = name_i->second.AsString();

        const auto lat_i = dict.find("latitude");
        if (lat_i == dict.end() || !lat_i->second.IsDouble()) return {};
        stop.coordinates.lat = lat_i->second.AsDouble();

        const auto lng_i = dict.find("longitude");
        if (lng_i == dict.end() || !lng_i->second.IsDouble()) return {};
        stop.coordinates.lng = lng_i->second.AsDouble();

        const auto dist_i = dict.find("road_distances");
        if (dist_i != dict.end() && !(dist_i->second.IsMap())) return {};
        for (const auto &[other_name, other_dist]: dist_i->second.AsMap()) {
            if (!other_dist.IsInt()) return {};
            stop.distances.emplace_back(StopDistanceData{other_name, static_cast<size_t>(other_dist.AsInt())});
        }
        return {stop};

    } else if (type_i->second == "Bus") {
        BusRouteJson route;

        const auto name_i = dict.find("name");
        if (name_i == dict.end() || !name_i->second.IsString()) return {};
        route.bus_name = name_i->second.AsString();

        const auto route_i = dict.find("is_roundtrip");
        if (route_i == dict.end() || !route_i->second.IsBool()) return {};
        route.type = route_i->second.AsBool() ? RouteType::CIRCLE_ROUTE : RouteType::RETURN_ROUTE;

        const auto stops_i = dict.find("stops");
        if (stops_i != dict.end() && !(stops_i->second.IsArray())) return {};
        for (const auto &stop_name: stops_i->second.AsArray()) {
            if (!stop_name.IsString()) return {};
            route.route_stops.emplace_back(stop_name.AsString());
        }
        return {route};
    }
    return {};
}


bool JsonReader::FillTransportCatalogue() {
    // Заполнение каталога остановок данными из объектов raw_stops_
    for (const auto &stop: raw_stops_) {
        transport_catalogue_.AddStop(stop);
    }

    // Заполнение каталога расстояний между остановками на основе данных из объектов raw_stops_
    for (const auto &stop: raw_stops_) {
        for (const auto &[other, distance]: stop.distances) {
            bool pairOK = transport_catalogue_.SetDistanceBetweenStops(stop.stop_name, other, distance);
            if (!pairOK) {
                std::cerr << "ERROR while adding distance to stop pair of " << stop.stop_name << " and " << other
                          << "." << std::endl;
            }
        }
    }

    // Заполнение каталога маршрутов автобусов на основе данных из объектов raw_buses_
    for (auto &route: raw_buses_) {
        if (route.route_stops.size() < 2) {
            std::cerr << "Error while adding bus routes for bus: " << route.bus_name
                      << ". Number of stops must be at least 2." << std::endl;
            continue;
        }
        transport_catalogue::BusRoute br;
        br.bus_name = std::move(route.bus_name);
        br.type = route.type;
        for (auto &route_stop: route.route_stops) {
            br.route_stops.emplace_back(&(transport_catalogue_.FindStop(route_stop).second));
        }

        transport_catalogue_.AddBus(br);
    }
    return true;
}


size_t JsonReader::QueryTC_WriteJsonToStream(std::ostream &out) {
    const auto &root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    const json::Dict &dict = root_node.AsMap();
    auto iter = dict.find(USER_REQUESTS);
    if (iter == dict.end() || !(iter->second.IsArray())) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    json::Array result;
    for (const json::Node &node: iter->second.AsArray()) {
        if (!node.IsMap()) {
            throw json::ParsingError(
                    "Error reading JSON data with user requests to database. One of nodes is not a dictionary.");
        }
        result.emplace_back(std::move(ProcessOneUserRequestNode(node)));
    }
    Print(json::Document{result}, out);
    return result.size();
}

size_t JsonReader::ReadJson_QueryTC_WriteJsonToStream(std::istream &input, std::ostream &out) {
    ReadJson(input);
    return QueryTC_WriteJsonToStream(out);
}

json::Node JsonReader::ProcessOneUserRequestNode(const json::Node &user_request) {
    using namespace std::literals;
    using namespace transport_catalogue;

    if (!user_request.IsMap()) {
        throw json::ParsingError("One of nodes is not a dictionary.");
    }

    const json::Dict &request_fields = user_request.AsMap();
    const auto id_i = request_fields.find("id"s);
    if (id_i == request_fields.end() || !id_i->second.IsInt()) {
        throw json::ParsingError("Node's fields are incomplete or corrupted.");
    }

    const int id = id_i->second.AsInt();
    const auto type_i = request_fields.find("type"s);
    if (type_i == request_fields.end() || !type_i->second.IsString()) {
        throw json::ParsingError("Node's fields are incomplete or corrupted.");
    }

    const std::string type = type_i->second.AsString();
    if (type == "Map"s) {
        RendererSettings rs = GetRendererSetting();
        MapRenderer mr(rs);

        std::ostringstream stream;
        mr.RenderSvgMap(transport_catalogue_, stream);

        json::Dict result;
        result.emplace("request_id"s, id);
        result.emplace("map"s, std::move(stream.str()));

        return {result};
    }

    const auto name_i = request_fields.find("name"s);
    if (name_i == request_fields.end() || !name_i->second.IsString()) {
        throw json::ParsingError("Node's fields are incomplete or corrupted.");
    }

    const std::string name = name_i->second.AsString();
    if (type == "Bus"s) {
        const BusInfo bi = transport_catalogue_.GetBusInfo(name);
        if (bi.type == RouteType::NOT_SET) {
            return GetErrorNode(id);
        }
        json::Dict result;
        result.emplace("request_id"s, id);
        result.emplace("curvature"s, bi.curvature);
        result.emplace("route_length"s, static_cast<int>(bi.route_length));
        result.emplace("stop_count"s, static_cast<int>(bi.stops_number));
        result.emplace("unique_stop_count"s, static_cast<int>(bi.unique_stops));

        return {result};
    }

    if (type == "Stop"s) {
        if (!transport_catalogue_.FindStop(name).first) {
            return GetErrorNode(id);
        }
        json::Dict result;
        json::Array buses;
        const std::set<std::string_view> &bus_routes = transport_catalogue_.GetBusesForStop(name);
        for (auto bus_route: bus_routes) {
            buses.emplace_back(std::move(std::string{bus_route}));
        }
        result.emplace("request_id"s, id);
        result.emplace("buses"s, buses);

        return {result};
    }
    throw json::ParsingError("Node's type field contains invalid data.");
}

RendererSettings JsonReader::GetRendererSetting() const {

    // Обращение к последнему элементу root_
    const auto &root_node = root_.back().GetRoot();

    // Проверка, что корневой узел - это объект
    if (!root_node.IsMap()) {
        throw json::ParsingError("Error reading JSON data with render settings.");
    }

    // Получение словаря с настройками отрисовки
    const json::Dict &dict = root_node.AsMap();
    auto iter = dict.find(RENDER_SETTINGS);
    if (iter == dict.end() || !(iter->second.IsMap())) {
        throw json::ParsingError("Error reading JSON data with render settings..");
    }

    // Создание объекта настроек отрисовки, который мы будем заполнять данными
    RendererSettings settings;
    const json::Dict render_settings = iter->second.AsMap();

    // Проверка наличия и правильности значения ключа "width"
    if (const auto width_i = render_settings.find("width"); width_i != render_settings.end() &&
                                                             width_i->second.IsDouble()) {
        settings.width = width_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, width data.");
    }

    // Проверка наличия и правильности значения ключа "height"
    if (const auto height_i = render_settings.find("height"); height_i != render_settings.end() && height_i->second.IsDouble()) {
        settings.height = height_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, height data.");
    }

    // Проверка наличия и правильности значения ключа "padding"
    if (const auto padd_i = render_settings.find("padding"); padd_i != render_settings.end() && padd_i->second.IsDouble()) {
        settings.padding = padd_i->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, padding data.");
    }

    // Проверка наличия и правильности значения ключа "line_width"
    if (const auto field_iter = render_settings.find("line_width"); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.line_width = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, line width data.");
    }

    // Проверка наличия и правильности значения ключа "stop_radius"
    if (const auto field_iter = render_settings.find("stop_radius"); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.stop_radius = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop radius data.");
    }

    // Проверка наличия и правильности значения ключа "bus_label_font_size"
    if (const auto field_iter = render_settings.find("bus_label_font_size"); field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.bus_label_font_size = field_iter->second.AsInt();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, bus label font size data.");
    }

    // Проверка наличия и правильности значения ключа "bus_label_offset"
    if (const auto field_iter = render_settings.find("bus_label_offset"); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2) throw json::ParsingError("Error while parsing renderer settings, bus label font offset data.");
        settings.bus_label_offset.x = arr[0].AsDouble();
        settings.bus_label_offset.y = arr[1].AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, bus label font offset data.");
    }

    // Проверка наличия и правильности значения ключа "stop_label_font_size"
    if (const auto field_iter = render_settings.find("stop_label_font_size"); field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.stop_label_font_size = field_iter->second.AsInt();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop label font size data.");
    }

    // Проверка наличия и правильности значения ключа "stop_label_offset"
    if (const auto field_iter = render_settings.find("stop_label_offset"); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2) throw json::ParsingError("Error while parsing renderer settings, stop label font offset data.");
        settings.stop_label_offset.x = arr[0].AsDouble();
        settings.stop_label_offset.y = arr[1].AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, stop label font offset data.");
    }

    // Проверка наличия и правильности значения ключа "underlayer_color"
    if (const auto field_iter = render_settings.find("underlayer_color"); field_iter != render_settings.end() ) {
        svg::Color color = ParseColor(field_iter->second);
        if (std::holds_alternative<std::monostate>(color)) {
            throw json::ParsingError("Error while parsing renderer settings, underlayer color data.");
        }

        settings.underlayer_color = color;
    } else {
        throw json::ParsingError("Error while parsing renderer settings, underlayer color data.");
    }

    // Проверка наличия и правильности значения ключа "underlayer_width"
    if (const auto field_iter = render_settings.find("underlayer_width"); field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.underlayer_width = field_iter->second.AsDouble();
    } else {
        throw json::ParsingError("Error while parsing renderer settings, underlayer width data.");
    }

    // Проверка наличия и правильности значения ключа "color_palette"
    if (const auto field_iter = render_settings.find("color_palette"); field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        for (const auto& color_node : arr) {
            svg::Color color = ParseColor(color_node);
            if (std::holds_alternative<std::monostate>(color)) {
                throw json::ParsingError("Error while parsing renderer settings, color palette data.");
            }
            settings.color_palette.emplace_back(color);
        }
    } else {
        throw json::ParsingError("Error while parsing renderer settings, color palette data.");
    }

    return settings;
}

svg::Color ParseColor(const json::Node &node) {
    if (node.IsString()) {
        return {node.AsString()};
    }
    if (node.IsArray()) {
        json::Array arr = node.AsArray();
        if (arr.size() == 3 || arr.size() == 4) {
            uint8_t red = arr[0].AsInt();
            uint8_t green = arr[1].AsInt();
            uint8_t blue = arr[2].AsInt();

            if (arr.size() == 3) {
                return {svg::Rgb(red, green, blue)};
            } else {
                double opacity = arr[3].AsDouble();
                return {svg::Rgba(red, green, blue, opacity)};
            }
        }
    }
    return {};
}

inline json::Node GetErrorNode(int id) {
    json::Dict result;
    result.emplace("request_id", id);
    result.emplace("error_message", "not found");

    return {result};
}