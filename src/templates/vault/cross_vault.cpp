#include "src/templates/vault/cross_vault.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CrossVault::CrossVault(const CrossVaultGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // voussoirs: course by course, the groin stones and the four webs
    for (const std::shared_ptr<Element>& voussoir : cross_vault(guide))
        add(voussoir);

    // contacts: between every two touching voussoirs
    compute_face_contacts(0);

    // assembly: the voussoirs raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
