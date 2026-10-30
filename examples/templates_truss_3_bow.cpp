#include "wood_session.h"
#include "src/templates/truss/truss.h"

using namespace session_cpp;
using namespace wood_session;

/// Every roof truss and bridge frame of Bow's tables (1873), stored as graphs and built by the one Truss template,
/// each its own scene truss_bow_<number>.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "templates" / "truss_bow_1873.json";
    const std::vector<std::pair<std::string, wood_truss::TrussGuide>> guides = wood_truss::TrussGuide::library(path);
    for (const auto& [name, guide] : guides) {
        wood_truss::Truss truss(guide, "truss_" + name);
        truss.pb_dump(pb_path("truss_" + name));
    }
    std::cout << guides.size() << " Bow trusses" << std::endl;
    return 0;
}
