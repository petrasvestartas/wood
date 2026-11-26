#include "src/templates/vault/dome.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
Dome::Dome(const DomeGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // voussoirs: hoop by hoop from the oculus down
    for (const std::shared_ptr<Element>& voussoir : dome(guide))
        add(voussoir);

    // contacts: between every two touching voussoirs
    compute_face_contacts(0);

    // assembly: the voussoirs raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
