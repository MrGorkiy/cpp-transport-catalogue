#include <cmath>

#include "map_renderer.h"

namespace renderer {

    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

//----------------------------------------------------------------------------
    void MapRenderer::SetRenderSettings(RenderSettings &&render_settings) {
        render_settings_ = std::move(render_settings);
    }

//----------------------------------------------------------------------------
    void MapRenderer::SetBuses(std::vector<const domain::Bus *> &&buses) {
        buses_ = std::move(buses);
    }

//----------------------------------------------------------------------------
    SphereProjector MapRenderer::CreateProj() {
        // создаем вектор со всеми остановками входящими в маршруты
        std::vector<geo::Coordinates> vec_common_coord;
        vec_common_coord.reserve(stopes_.size());
        for (const auto &stop: stopes_) {
            vec_common_coord.push_back(stop->coord);
        }
        // Создаём проектор сферических координат на карту
        return SphereProjector{vec_common_coord.begin(), vec_common_coord.end(),
                               render_settings_.width, render_settings_.height, render_settings_.padding};
    }

//----------------------------------------------------------------------------
    void MapRenderer::DrawLineBuses(svg::Document &doc, const SphereProjector &proj) {
        size_t counter_color = 0;
        for (const auto &bus: buses_) {
            if (bus->stops.empty()) {
                continue;
            }
            svg::Polyline polyl;
            polyl.SetStrokeColor(render_settings_.color_palette[counter_color]);
            if (++counter_color >= render_settings_.color_palette.size()) {
                counter_color = 0;
            }
            polyl.SetFillColor(svg::NoneColor);
            polyl.SetStrokeWidth(render_settings_.line_width);
            polyl.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            polyl.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            for (const auto &stop: bus->stops) {
                polyl.AddPoint(proj(stop->coord));
            }
            doc.Add(polyl);
        }
    }

//----------------------------------------------------------------------------
    void MapRenderer::DrawNameBuses(svg::Document &doc, const SphereProjector &proj) {
        size_t counter_color = 0;
        for (const auto &bus: buses_) {
            if (bus->stops.empty()) { // если остановки есть
                continue;
            }
            const domain::Stop *stop = *bus->stops.begin();
            // задаем цвет как у маршрута
            svg::Color fill_color = render_settings_.color_palette[counter_color];
            if (++counter_color >= render_settings_.color_palette.size()) {
                counter_color = 0;
            }
            DrawNameBus(doc, proj, stop->coord, fill_color, bus->name);
            if (!bus->is_round) {
                size_t id = std::ceil(bus->stops.size() / 2); // ATTENTION
                // если остановки начала и конца не равны подписываем остановку конца
                if (stop != bus->stops[id]) {
                    stop = bus->stops[id];
                    DrawNameBus(doc, proj, stop->coord, fill_color, bus->name);
                }
            }
        }
    }

//----------------------------------------------------------------------------
    void MapRenderer::DrawCircStopes(svg::Document &doc, const SphereProjector &proj) {
        for (const auto &stop: stopes_) {
            svg::Circle circle;
            circle.SetCenter(proj(stop->coord));
            circle.SetRadius(render_settings_.stop_radius);
            circle.SetFillColor("white"s);
            doc.Add(circle);
        }
    }

//----------------------------------------------------------------------------
    void MapRenderer::DrawNameStopes(svg::Document &doc, const SphereProjector &proj) {
        for (const auto &stop: stopes_) {
            svg::Text note_stop;
            note_stop.SetPosition(proj(stop->coord));
            note_stop.SetOffset(render_settings_.stop_label_offset);
            note_stop.SetFontSize(render_settings_.stop_label_font_size);
            note_stop.SetFontFamily("Verdana"s);
            note_stop.SetData(std::string(stop->name));
            note_stop.SetFillColor("black"s);

            svg::Text note_stop_backgtound(note_stop);
            note_stop_backgtound.SetFillColor(render_settings_.underlayer_color);
            note_stop_backgtound.SetStrokeColor(render_settings_.underlayer_color);
            note_stop_backgtound.SetStrokeWidth(render_settings_.underlayer_width);
            note_stop_backgtound.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            note_stop_backgtound.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            doc.Add(note_stop_backgtound);
            doc.Add(note_stop);
        }
    }

//----------------------------------------------------------------------------
    void MapRenderer::DrawNameBus(svg::Document &doc, const SphereProjector &proj,
                                  const geo::Coordinates &coord, const svg::Color &fill_color,
                                  std::string_view name_bus) {
        svg::Text text;
        text.SetPosition(proj(coord));
        text.SetOffset(render_settings_.bus_label_offset);
        text.SetFontSize(render_settings_.bus_label_font_size);
        text.SetFontFamily("Verdana"s);
        text.SetFontWeight("bold"s);
        text.SetData(std::string(name_bus));
        text.SetFillColor(fill_color);

        svg::Text text_backgtound(text);
        text_backgtound.SetFillColor(render_settings_.underlayer_color);
        text_backgtound.SetStrokeColor(render_settings_.underlayer_color);
        text_backgtound.SetStrokeWidth(render_settings_.underlayer_width);
        text_backgtound.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text_backgtound.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        doc.Add(text_backgtound);
        doc.Add(text);
    }

//----------------------------------------------------------------------------
    const RenderSettings &MapRenderer::GetRenderSettings() const {
        return render_settings_;
    }

//----------------------------------------------------------------------------
    void MapRenderer::SetUnicStops(const std::vector<const domain::Stop *> &&stopes) {
        stopes_ = std::move(stopes);
    }

//----------------------------------------------------------------------------
    svg::Document MapRenderer::GetDocMapBus() {
        svg::Document doc;
        SphereProjector proj = CreateProj();
        DrawLineBuses(doc, proj);
        DrawNameBuses(doc, proj);
        DrawCircStopes(doc, proj);
        DrawNameStopes(doc, proj);
        return doc;
    }
//----------------------------------------------------------------------------
}// namespace MapRenderer
