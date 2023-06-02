#include "svg.h"

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext &context) const {
        context.RenderIndent();
        // Делегирует вывод тега своим подклассам
        RenderObject(context);
        context.out << std::endl;
    }

    std::ostream &operator<<(std::ostream &out, const Color &color) {
        visit(OstreamColorPrinter{out}, color);
        return out;
    }

    std::ostream &operator<<(std::ostream &out, StrokeLineCap stroke_linecap) {
        switch (stroke_linecap) {
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

    std::ostream &operator<<(std::ostream &out, StrokeLineJoin stroke_linejoin) {
        switch (stroke_linejoin) {
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
// ---------- Circle ------------------

    Circle &Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle &Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext &context) const {
        auto &out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

// ---------- Polyline ------------------
    Polyline &Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext &context) const {
        auto &out = context.out;
        out << "<polyline points=\""sv;
        bool start = true;
        for (const auto &point: points_) {
            if (!start) {
                out << " ";
            }
            out << point.x << "," << point.y;
            start = false;
        }
        out << "\"";
        RenderAttrs(out);
        out << "/>"sv;
    }

// ---------- Text ------------------
    Text &Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text &Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text &Text::SetFontSize(uint32_t size) {
        size_font_ = size;
        return *this;
    }

    Text &Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text &Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text &Text::SetData(std::string data) {
        data_ = move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext &context) const {
        auto &out = context.out;
        out << "<text x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
        out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
        out << " font-size=\""sv << size_font_ << "\""sv;
        if (font_family_ != ""sv) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }
        if (font_weight_ != "") {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }
        RenderAttrs(out);
        out << ">"sv;
        out << PrepareData(data_);
        out << "</text>"sv;
    }

    std::string Text::PrepareData(std::string data) const {
        std::string result;
        for (const auto &c: data) {
            switch (c) {
                case '"':
                    result += "&quot;";
                    break;
                case '<':
                    result += "&lt;";
                    break;
                case '>':
                    result += "&gt;";
                    break;
                case '\'':
                    result += "&apos;";
                    break;
                case '&':
                    result += "&amp;";
                    break;
                default:
                    result += c;
                    break;
            }
        }
        return result;
    }

// ---------- Document ------------------
    void Document::AddPtr(std::unique_ptr<Object> &&obj) {
        objects_.push_back(move(obj));
    }

    void Document::Render(std::ostream &out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx(out, 2, 2);
        for (const auto &obj: objects_) {
            obj->Render(ctx);
        }
        out << "</svg>"sv << std::endl;
    }

}  // namespace svg
