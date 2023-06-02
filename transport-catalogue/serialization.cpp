#include <fstream>
#include "serialization.h"

Serialization::Serialization() {

}

//----------------------------------------------------------------------------
void Serialization::Serialize(const TransportCatalogue::TransportCatalogue &t_c, const renderer::MapRenderer &m_r,
                              const TransportRouter::TransportRouter &t_r, const std::filesystem::path &path) const {
    t_c_srlz::TransportCatalogue s_t_c;
    SerializeTC(s_t_c, t_c);
    SerializeRS(s_t_c, m_r);
    SerializeTR(s_t_c, t_r);
    std::ofstream out(path, std::ios::binary);
    s_t_c.SerializeToOstream(&out);
}

//----------------------------------------------------------------------------
void Serialization::Deserialize(const std::filesystem::path &path, TransportCatalogue::TransportCatalogue &t_c,
                                renderer::MapRenderer &m_r, TransportRouter::TransportRouter &t_r) const {
    std::ifstream in(path, std::ios::binary);
    t_c_srlz::TransportCatalogue s_t_c;
    s_t_c.ParseFromIstream(&in);
    DeserializeTC(s_t_c, t_c);
    DeserializeRS(s_t_c, m_r);
    DeserializeTR(s_t_c, t_r);
}

//----------------------------------------------------------------------------
void Serialization::SerializeTC(t_c_srlz::TransportCatalogue &s_t_c,
                                const TransportCatalogue::TransportCatalogue &t_c) const {
    for (const auto &stop: t_c.GetStops()) {
        t_c_srlz::Stop s_stop;
        s_stop.set_name(stop.name);

        t_c_srlz::Coord s_coord;
        s_coord.set_latitude(stop.coord.lat);
        s_coord.set_longitude(stop.coord.lng);
        *s_stop.mutable_coord() = std::move(s_coord);

        s_stop.set_id(stop.id);
        *s_t_c.add_list_stop() = std::move(s_stop);
    }
    for (const auto &bus: t_c.GetBuses()) {
        t_c_srlz::Bus s_bus;
        s_bus.set_name(bus.name);

        for (const auto &stop: bus.stops) {
            *s_bus.add_list_name_stop() = stop->name;
        }

        s_bus.set_is_roundtrip(bus.is_round);
        *s_t_c.add_list_bus() = std::move(s_bus);
    }
    for (const auto &struc: t_c.GetIndexRageStop()) {
        t_c_srlz::StopsLenght s_stop_lenght;
        s_stop_lenght.set_from_stop(struc.first.first->name);
        s_stop_lenght.set_to_stop(struc.first.second->name);
        s_stop_lenght.set_lenght(struc.second);
        *s_t_c.add_list_stop_lenght() = std::move(s_stop_lenght);
    }
}

//----------------------------------------------------------------------------
void Serialization::SerializeRS(t_c_srlz::TransportCatalogue &s_t_c, const renderer::MapRenderer &m_r) const {
    const renderer::RenderSettings &r_s = m_r.GetRenderSettings();
    r_s_srlz::RenderSettings &s_r_s = *s_t_c.mutable_render_settings();
    s_r_s.set_width(r_s.width);
    s_r_s.set_height(r_s.height);
    s_r_s.set_padding(r_s.padding);
    s_r_s.set_line_width(r_s.line_width);
    s_r_s.set_stop_radius(r_s.stop_radius);
    s_r_s.set_bus_label_font_size(r_s.bus_label_font_size);
    r_s_srlz::Point p_bus;
    p_bus.set_x(r_s.bus_label_offset.x);
    p_bus.set_y(r_s.bus_label_offset.y);
    *s_r_s.mutable_bus_label_offset() = std::move(p_bus);
    s_r_s.set_stop_label_font_size(r_s.stop_label_font_size);
    r_s_srlz::Point p_stop;
    p_stop.set_x(r_s.stop_label_offset.x);
    p_stop.set_y(r_s.stop_label_offset.y);
    *s_r_s.mutable_stop_label_offset() = std::move(p_stop);
    *s_r_s.mutable_underlayer_color() = GetCurrentVariantSrlzColor(r_s.underlayer_color);
    s_r_s.set_underlayer_width(r_s.underlayer_width);
    bool is_clear = false;
    for (const auto &color: r_s.color_palette) {
        if (!is_clear) {
            s_r_s.clear_color_palette(); // очистка значений по умолчанию
            is_clear = true;
        }
        *s_r_s.add_color_palette() = GetCurrentVariantSrlzColor(color);
    }
}

//----------------------------------------------------------------------------
void
Serialization::SerializeTR(t_c_srlz::TransportCatalogue &s_t_c, const TransportRouter::TransportRouter &t_r) const {
    s_t_c.mutable_t_r_()->mutable_routing_settings()->set_bus_velocity(t_r.GetRoutingSettings().bus_velocity);
    s_t_c.mutable_t_r_()->mutable_routing_settings()->set_bus_wait_time_minut(
            t_r.GetRoutingSettings().bus_wait_time_minut);
    for (const auto &edge_bus: t_r.GetEdgesBuses()) {
        t_r_srlz::EdgeAditionInfo edge_adition_info;
        edge_adition_info.set_bus_name(std::string(edge_bus.bus_name));
        edge_adition_info.set_count_spans(edge_bus.count_spans);
        *s_t_c.mutable_t_r_()->add_edges_buses() = std::move(edge_adition_info);
    }
    for (const auto &id_stop: t_r.GetIdStopes()) {
        *s_t_c.mutable_t_r_()->add_id_stopes() = id_stop;
    }
    // граф
    s_t_c.mutable_t_r_()->mutable_graph()->set_vertex_count(t_r.GetGraph().GetVertexCount());
    for (const auto &edge: t_r.GetGraph().GetEdges()) {
        t_r_srlz::Edge s_edge;
        s_edge.set_from(edge.from);
        s_edge.set_to(edge.to);
        s_edge.set_weight(edge.weight);
        *s_t_c.mutable_t_r_()->mutable_graph()->add_edges() = std::move(s_edge);
    }
}

//----------------------------------------------------------------------------
void Serialization::DeserializeTC(const t_c_srlz::TransportCatalogue &s_t_c,
                                  TransportCatalogue::TransportCatalogue &t_c) const {
    size_t count_stop = s_t_c.list_stop_size();
    for (size_t i = 0; i < count_stop; ++i) {
        domain::Stop stop;
        stop.name = s_t_c.list_stop(i).name();
        stop.coord.lat = s_t_c.list_stop(i).coord().latitude();
        stop.coord.lng = s_t_c.list_stop(i).coord().longitude();
        stop.id = s_t_c.list_stop(i).id();
        t_c.AddStop(std::move(stop));
    }

    size_t count_bus = s_t_c.list_bus_size();
    for (size_t i = 0; i < count_bus; ++i) {
        domain::Bus bus;
        bus.name = s_t_c.list_bus(i).name();
        size_t count_name_stop = s_t_c.list_bus(i).list_name_stop_size();
        for (size_t j = 0; j < count_name_stop; ++j) {
            bus.stops.push_back(t_c.FindStop(s_t_c.list_bus(i).list_name_stop(j)).value());
        }
        bus.is_round = s_t_c.list_bus(i).is_roundtrip();
        t_c.AddBus(std::move(bus));
    }

    size_t count_ranges = s_t_c.list_stop_lenght_size();
    for (size_t i = 0; i < count_ranges; ++i) {
        domain::StopsLenght stop_lenght;
        stop_lenght.from_stop = s_t_c.list_stop_lenght(i).from_stop();
        stop_lenght.to_stop = s_t_c.list_stop_lenght(i).to_stop();
        stop_lenght.lenght = s_t_c.list_stop_lenght(i).lenght();
        t_c.AddRangeStops(std::move(stop_lenght));
    }
}

//----------------------------------------------------------------------------
void Serialization::DeserializeRS(const t_c_srlz::TransportCatalogue &s_t_c, renderer::MapRenderer &m_r) const {
    renderer::RenderSettings render_settings;
    render_settings.width = s_t_c.render_settings().width();
    render_settings.height = s_t_c.render_settings().height();
    render_settings.padding = s_t_c.render_settings().padding();
    render_settings.line_width = s_t_c.render_settings().line_width();
    render_settings.stop_radius = s_t_c.render_settings().stop_radius();
    render_settings.bus_label_font_size = s_t_c.render_settings().bus_label_font_size();
    render_settings.bus_label_offset = {s_t_c.render_settings().bus_label_offset().x(),
                                        s_t_c.render_settings().bus_label_offset().y()};
    render_settings.stop_label_font_size = s_t_c.render_settings().stop_label_font_size();
    render_settings.stop_label_offset = {s_t_c.render_settings().stop_label_offset().x(),
                                         s_t_c.render_settings().stop_label_offset().y()};
    render_settings.underlayer_color = GetCurrentVariantColor(s_t_c.render_settings().underlayer_color());
    render_settings.underlayer_width = s_t_c.render_settings().underlayer_width();
    bool is_clear = false;
    for (int i = 0; i < s_t_c.render_settings().color_palette_size(); ++i) {
        if (!is_clear) {
            render_settings.color_palette.clear(); // очистка значений по умолчанию
            is_clear = true;
        }
        render_settings.color_palette.emplace_back(GetCurrentVariantColor(s_t_c.render_settings().color_palette(i)));
    }
    m_r.SetRenderSettings(std::move(render_settings));

}

//----------------------------------------------------------------------------
void
Serialization::DeserializeTR(const t_c_srlz::TransportCatalogue &s_t_c, TransportRouter::TransportRouter &t_r) const {
    t_r.SetRoutingSettings({.bus_wait_time_minut = s_t_c.t_r_().routing_settings().bus_wait_time_minut(),
                                   .bus_velocity = s_t_c.t_r_().routing_settings().bus_velocity()});

    std::vector<TransportRouter::TransportRouter::EdgeAditionInfo> edges_buses(s_t_c.t_r_().edges_buses_size());
    for (int i = 0; i < s_t_c.t_r_().edges_buses_size(); ++i) {
        edges_buses[i].bus_name = s_t_c.t_r_().edges_buses(i).bus_name();
        edges_buses[i].count_spans = s_t_c.t_r_().edges_buses(i).count_spans();
    }
    t_r.SetEdgesBuses(std::move(edges_buses));

    std::vector<std::string> id_stopes(s_t_c.t_r_().id_stopes_size());
    for (int i = 0; i < s_t_c.t_r_().id_stopes_size(); ++i) {
        id_stopes[i] = s_t_c.t_r_().id_stopes(i);
    }
    t_r.SetIdStopes(std::move(id_stopes));

    graph::DirectedWeightedGraph<double> graph(s_t_c.t_r_().graph().vertex_count());
    for (int i = 0; i < s_t_c.t_r_().graph().edges_size(); ++i) {
        graph::Edge<double> edge;
        edge.from = s_t_c.t_r_().graph().edges(i).from();
        edge.to = s_t_c.t_r_().graph().edges(i).to();
        edge.weight = s_t_c.t_r_().graph().edges(i).weight();
        graph.AddEdge(std::move(edge));
    }
    t_r.SetGraph(std::move(graph));
}

//----------------------------------------------------------------------------
r_s_srlz::Color Serialization::GetCurrentVariantSrlzColor(const svg::Color &color) const {
    r_s_srlz::Color s_color;
    if (std::holds_alternative<std::string>(color)) {
        *s_color.mutable_color() = std::get<std::string>(color);
    } else if (std::holds_alternative<svg::Rgb>(color)) {
        const svg::Rgb &rgb = std::get<svg::Rgb>(color);
        s_color.mutable_rgb()->set_red(rgb.red);
        s_color.mutable_rgb()->set_green(rgb.green);
        s_color.mutable_rgb()->set_blue(rgb.blue);
    } else if (std::holds_alternative<svg::Rgba>(color)) {
        const svg::Rgba &rgba = std::get<svg::Rgba>(color);
        s_color.mutable_rgba()->set_red(rgba.red);
        s_color.mutable_rgba()->set_green(rgba.green);
        s_color.mutable_rgba()->set_blue(rgba.blue);
        s_color.mutable_rgba()->set_opacity(rgba.opacity);
    }
    return s_color;
}

//----------------------------------------------------------------------------
svg::Color Serialization::GetCurrentVariantColor(const r_s_srlz::Color &s_color) const {
    svg::Color color;
    switch (s_color.ColorVariant_case()) {
        case r_s_srlz::Color::ColorVariantCase::kColor:
            color = s_color.color();
            break;
        case r_s_srlz::Color::ColorVariantCase::kRgb: {
            svg::Rgb rgb;
            rgb.red = s_color.rgb().red();
            rgb.green = s_color.rgb().green();
            rgb.blue = s_color.rgb().blue();
            color = std::move(rgb);
            break;
        }
        case r_s_srlz::Color::ColorVariantCase::kRgba: {
            svg::Rgba rgba;
            rgba.red = s_color.rgba().red();
            rgba.green = s_color.rgba().green();
            rgba.blue = s_color.rgba().blue();
            rgba.opacity = s_color.rgba().opacity();
            color = std::move(rgba);
            break;
        }
        default:
            break;
    }
    return color;
}
//----------------------------------------------------------------------------