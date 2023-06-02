#pragma once

#include <filesystem>
#include <transport_catalogue.pb.h>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

class Serialization {
public:
    Serialization();

    void Serialize(const TransportCatalogue::TransportCatalogue &t_c, const renderer::MapRenderer &m_r,
                   const TransportRouter::TransportRouter &t_r, const std::filesystem::path &path) const;

    void Deserialize(const std::filesystem::path &path, TransportCatalogue::TransportCatalogue &t_c,
                     renderer::MapRenderer &m_r, TransportRouter::TransportRouter &t_r) const;

private:
    void SerializeTC(t_c_srlz::TransportCatalogue &s_t_c, const TransportCatalogue::TransportCatalogue &t_c) const;

    void SerializeRS(t_c_srlz::TransportCatalogue &s_t_c, const renderer::MapRenderer &m_r) const;

    void SerializeTR(t_c_srlz::TransportCatalogue &s_t_c, const TransportRouter::TransportRouter &t_r) const;

    void DeserializeTC(const t_c_srlz::TransportCatalogue &s_t_c, TransportCatalogue::TransportCatalogue &t_c) const;

    void DeserializeRS(const t_c_srlz::TransportCatalogue &s_t_c, renderer::MapRenderer &m_r) const;

    void DeserializeTR(const t_c_srlz::TransportCatalogue &s_t_c, TransportRouter::TransportRouter &t_r) const;

    r_s_srlz::Color GetCurrentVariantSrlzColor(const svg::Color &color) const;

    svg::Color GetCurrentVariantColor(const r_s_srlz::Color &s_color) const;
};
