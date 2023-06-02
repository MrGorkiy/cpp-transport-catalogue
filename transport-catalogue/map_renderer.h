#pragma once

#include <algorithm>
#include <map>
#include <variant>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <optional>

#include "geo.h"
#include "svg.h"
#include "domain.h"

namespace renderer {

    inline const double EPSILON = 1e-6;

    bool IsZero(double value);

    class SphereProjector {
    public:
        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template<typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding)
                : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end) {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(geo::Coordinates coords) const {
            return {
                    (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    using namespace std::literals;
    namespace RenderSettingsKey {
        const std::string width = "width"s;
        const std::string height = "height"s;
        const std::string padding = "padding"s;
        const std::string line_width = "line_width"s;
        const std::string stop_radius = "stop_radius"s;
        const std::string bus_label_font_size = "bus_label_font_size"s;
        const std::string bus_label_offset = "bus_label_offset"s;
        const std::string stop_label_font_size = "stop_label_font_size"s;
        const std::string stop_label_offset = "stop_label_offset"s;
        const std::string underlayer_color = "underlayer_color"s;
        const std::string underlayer_width = "underlayer_width"s;
        const std::string color_palette = "color_palette"s;
    }

    struct RenderSettings {
        double width = 600.0;
        double height = 400.0;

        double padding = 50.0;

        double line_width = 14.0;
        double stop_radius = 5.0;

        int bus_label_font_size = 20;
        svg::Point bus_label_offset = {7.0, 15.0};

        int stop_label_font_size = 20;
        svg::Point stop_label_offset = {7.0, -3.0};

        svg::Color underlayer_color = svg::Rgba{255, 255, 255, 0.85};

        double underlayer_width = 3.0;

        std::vector<svg::Color> color_palette = {
                {"green"},
                svg::Rgb{255, 160, 0},
                {"red"}
        };
    };


    class MapRenderer {
        using busesstopes = const std::map<std::string_view, const std::vector<const domain::Stop *> *>;

    public:
        MapRenderer() = default;

        void SetRenderSettings(RenderSettings &&render_settings);

        void SetBuses(std::vector<const domain::Bus *> &&buses);

        // Устанавливае перечень уникальных остановок в лекс поряд используемых в маршрутах
        void SetUnicStops(const std::vector<const domain::Stop *> &&stopes);

        svg::Document GetDocMapBus();

        const RenderSettings &GetRenderSettings() const;

    private:

        SphereProjector CreateProj();

        void DrawLineBuses(svg::Document &doc, const SphereProjector &proj);

        void DrawNameBuses(svg::Document &doc, const SphereProjector &proj);

        void DrawCircStopes(svg::Document &doc, const SphereProjector &proj);

        void DrawNameStopes(svg::Document &doc, const SphereProjector &proj);

        void DrawNameBus(svg::Document &doc, const SphereProjector &proj,
                         const geo::Coordinates &coord, const svg::Color &fill_color, std::string_view name_bus);

        RenderSettings render_settings_;

        //перечень уникальных остановок в лекс поряд используемых в маршрутах
        std::vector<const domain::Stop *> stopes_;
        //перечень маршрутов в лекс поряд
        std::vector<const domain::Bus *> buses_;
    };

}// namespace MapRenderer