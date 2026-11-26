#include "src/templates/vault/barrel.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
Barrel::Barrel(const BarrelGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // voussoirs: course by course from springing to springing
    for (const std::shared_ptr<Element>& voussoir : barrel(guide))
        add(voussoir);

    // contacts: between every two touching voussoirs
    compute_face_contacts(0);

    // assembly: the voussoirs raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
