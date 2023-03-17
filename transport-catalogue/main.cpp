#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;
using namespace query;
using namespace output;

int main() {
    TransportCatalogue tc;
    InputReader ir;
    ir.ParseInput(std::cin);
    ir.Load(tc);
    ProcessRequests(cout, tc);
    return 0;
}