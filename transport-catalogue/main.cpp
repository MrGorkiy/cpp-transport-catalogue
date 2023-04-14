#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"

using namespace transport_catalogue;
using namespace std::literals;

int main() {
    TransportCatalogue tc;
    JsonReader reader(tc);
    reader.ReadJsonToTransportCatalogue(std::cin);
    reader.QueryTcWriteJsonToStream(std::cout);
}