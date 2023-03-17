#pragma once

#include "input_reader.h"
#include "transport_catalogue.h"
#include "string_view"

namespace transport_catalogue::output {

    void OutputRouteAbout(std::ostream& out, TransportCatalogue &tc, std::string_view route);

    void OutputStopAbout(std::ostream& out, TransportCatalogue &tc, std::string_view name);

    void OutputAbout(std::ostream& out, TransportCatalogue &tc, const query::Command &command);

    void ProcessRequests(std::ostream& out, TransportCatalogue& catalogue);

}//transport_catalogue