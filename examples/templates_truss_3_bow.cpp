#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every roof truss and bridge frame of Bow's tables (1873), stored as graphs and built by the one Truss template,
/// each its own scene truss_bow_<number>.
int main() {

    const std::vector<wood_truss::Bow> designs = wood_truss::Bow::all();
    for (const wood_truss::Bow& design : designs) {
        wood_truss::Truss truss(wood_truss::TrussGuide(design), "truss_" + design.name());
        truss.pb_dump(pb_path("truss_" + design.name()));
    }
    std::cout << designs.size() << " Bow trusses" << std::endl;
    return 0;
}
