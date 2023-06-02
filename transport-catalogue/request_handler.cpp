#include "request_handler.h"
#include "serialization.h"

//----------------------------------------------------------------------------
RequestHandler::RequestHandler(TransportCatalogue::TransportCatalogue &t_c,
                               TransportRouter::TransportRouter &t_r,
                               renderer::MapRenderer &m_r)
        : t_c_(t_c), t_r_(t_r), m_r_(m_r) {

}

//----------------------------------------------------------------------------
std::optional<domain::BusStat> RequestHandler::GetBusStat(const std::string_view &bus_name) const {
    if (auto opt_bus = t_c_.FindBus(bus_name); opt_bus) {
        return CreateBusStat(opt_bus.value());
    } else {
        return std::nullopt;
    }
}

//----------------------------------------------------------------------------
std::optional<domain::RoutStat>
RequestHandler::GetRouteStat(std::string_view stop_from, std::string_view stop_to) const {
    if (t_r_.GetGraphIsNoInit()) {
        std::cerr << "CreateGraph" << std::endl;
        t_r_.CreateGraph(t_c_);
    }
    return t_r_.GetRouteStat(t_c_.FindStop(stop_from).value()->id, t_c_.FindStop(stop_to).value()->id);
}

//----------------------------------------------------------------------------
std::optional<const std::unordered_set<const domain::Bus *> *>
RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
    if (auto opt_stop = t_c_.FindStop(stop_name); opt_stop) {
        if (t_c_.GetBusesFromStop().count(stop_name) != 0) {
            return &t_c_.GetBusesFromStop().at(stop_name);
        } else {
            static std::unordered_set<const domain::Bus *> emp{};
            return &emp;
        }
    } else {
        // если остановок с таким именем нет
        return std::nullopt;
    }

}

//----------------------------------------------------------------------------
std::vector<const domain::Bus *> RequestHandler::GetBusesLex() const {
    return t_c_.GetBusesLex();
}

//----------------------------------------------------------------------------
const std::vector<const domain::Stop *> RequestHandler::GetUnicLexStopsIncludeBuses() const {
    // использую сет как фильтр уникальных и сортировщик, кладу в вектор так как потом надо будет только итерироваться
    std::set<const domain::Stop *, domain::CmpStops> set;
    for (const auto &bus: t_c_.GetBuses()) {
        for (const auto &stop: bus.stops) {
            set.insert(stop);
        }
    }
    return {set.begin(), set.end()};
}

//----------------------------------------------------------------------------
svg::Document RequestHandler::RenderMap() const {
    return m_r_.GetDocMapBus();
}

//----------------------------------------------------------------------------
void RequestHandler::CallDsrlz(const std::filesystem::path &path) {
    Serialization d_srlz;
    d_srlz.Deserialize(path, t_c_, m_r_, t_r_);
    m_r_.SetBuses(GetBusesLex());
    m_r_.SetUnicStops(GetUnicLexStopsIncludeBuses());
}

//----------------------------------------------------------------------------
void RequestHandler::CallSrlz(const std::filesystem::path &path) const {
    Serialization srlz;
    srlz.Serialize(t_c_, m_r_, t_r_, path);
}

//----------------------------------------------------------------------------
domain::BusStat RequestHandler::CreateBusStat(const domain::Bus *bus) const {
    return t_c_.GetBusStat(bus);
}
//----------------------------------------------------------------------------