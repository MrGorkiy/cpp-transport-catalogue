#include "svg.h"
#include <iomanip>

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

// ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center)  {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius)  {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << std::setprecision(6) << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << std::setprecision(6) << radius_ << "\""sv;

        RenderAttrs(out);

        out << "/>"sv;
    }


// ---------- Polyline ------------------
    Polyline& Polyline::AddPoint(svg::Point point) {
        points_.emplace_back(std::move(point));

        return *this;
    }

    void Polyline::RenderObject(const svg::RenderContext &context) const {
        auto& out = context.out;

        out << "<polyline points=\""sv;
        for (auto iter = points_.begin(); iter != points_.end(); ++iter) {
            auto [x, y] = *iter;
            out << std::setprecision(6) << x << ","sv << y;
            if (std::next(iter) != points_.end()) {
                out << " "sv;
            }
        }
        out << "\""sv;

        RenderAttrs(out);

        out << "/>"sv;
    }


// ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;

        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;

        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;

        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;

        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;

        return *this;
    }

    Text& Text::SetData(std::string data) {
        text_data_ = data;

        return *this;
    }

    void Text::RenderObject(const RenderContext &context) const {
        auto& out = context.out;
        out << "<text"sv;

        RenderAttrs(out);

        // text position and offset
        out << std::setprecision(6) << " x=\""sv << position_.x << "\" y=\""sv << position_.y
            << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;

        // font size
        out << " font-size=\""sv << font_size_ << "\""sv;

        // font family
        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        // font weight
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        // end of text tag
        out << ">"sv;

        for (char c : text_data_) {
            out << CheckAndScreenCharacter(c);
        }

        out << "</text>"sv;
    }

    std::string Text::CheckAndScreenCharacter(char c) const {
        std::string out_c;

        switch (c) {
            case '\"':
                out_c = "&quot;";
                break;
            case '\'':
                out_c = "&apos;";
                break;
            case '<':
                out_c = "&lt;";
                break;
            case '>':
                out_c = "&gt;";
                break;
            case '&':
                out_c = "&amp;";
                break;
            default:
                out_c = c;
        }

        return out_c;
    }

// ---------- Document ------------------
    void Document::Render(std::ostream &out) const {
        RenderContext context(out, 2, 2);

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        for (auto iter = objects_ptrs_.begin(); iter != objects_ptrs_.end(); ++iter) {
            (*iter)->Render(context);
        }

        out << "</svg>"sv;
    }


// ---------- StrokeLineCap ------------------
    std::ostream& operator<<(std::ostream& out, const StrokeLineCap& slc) {
        switch (slc) {
            case StrokeLineCap::BUTT:
                out << "butt";
                break;
            case StrokeLineCap::ROUND:
                out << "round";
                break;
            case StrokeLineCap::SQUARE:
                out << "square";
                break;
        }
        return out;
    }

// ---------- StrokeLineJoin ------------------
    std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& slj) {
        switch (slj) {
            case StrokeLineJoin::ARCS:
                out << "arcs";
                break;
            case StrokeLineJoin::BEVEL:
                out << "bevel";
                break;
            case StrokeLineJoin::MITER:
                out << "miter";
                break;
            case StrokeLineJoin::MITER_CLIP:
                out << "miter-clip";
                break;
            case StrokeLineJoin::ROUND:
                out << "round";
                break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream &out, const Rgb &color) {
        out << "rgb(" << std::to_string(color.red) << "," << std::to_string(color.green) << "," << std::to_string(color.blue) << ")";
        return out;
    }

    std::ostream& operator<<(std::ostream &out, const Rgba &color) {
        out << "rgba(" << std::to_string(color.red) << "," << std::to_string(color.green) << "," << std::to_string(color.blue) << "," << color.opacity << ")";
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit([&out](Color value){
            if (std::holds_alternative<std::monostate>(value)){
                out << NoneColor;
            } else if (const auto* val1 = std::get_if<std::string>(&value)){
                out << *val1;
            } else if (const auto* val2 = std::get_if<Rgb>(&value)) {
                out << *val2;
            } else if (const auto* val3 = std::get_if<Rgba>(&value)) {
                out << *val3;
            }
        }, color);
        return out;
    }

}  // namespace svg