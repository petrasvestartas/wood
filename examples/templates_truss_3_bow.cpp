#include "wood_session.h"
#include "src/templates/truss/truss.h"
#include "src/templates/truss/truss_graph.h"

using namespace session_cpp;
using namespace wood_session;

/// Every roof truss and bridge frame of Bow's tables (1873), stored as graphs and built by the one Truss template,
/// each its own scene truss_bow_<number>.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "templates" / "truss_bow_1873.json";
    const std::vector<wood_truss::TrussGraph> graphs = wood_truss::TrussGraph::library(path);
    for (const wood_truss::TrussGraph& graph : graphs) {
        wood_truss::Truss truss(graph.guide(), "truss_" + graph.name);
        truss.pb_dump(pb_path("truss_" + graph.name));
    }
    std::cout << graphs.size() << " Bow trusses" << std::endl;
    return 0;
}
