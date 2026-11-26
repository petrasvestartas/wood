#include "src/templates/vault/cloister_vault.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CloisterVault::CloisterVault(const CloisterVaultGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // voussoirs: the four webs, one towards each side
    for (const std::shared_ptr<Element>& voussoir : cloister_vault(guide))
        add(voussoir);

    // contacts: between every two touching voussoirs
    compute_face_contacts(0);

    // assembly: the voussoirs raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
