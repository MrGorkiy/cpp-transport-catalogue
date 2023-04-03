#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"

using namespace transport_catalogue;

int main() {
    TransportCatalogue tc;
    JsonReader reader(tc);
    reader.ReadJsonAndFillTransportCatalogue(std::cin);
    reader.QueryTcWriteJsonToStream(std::cout);
}