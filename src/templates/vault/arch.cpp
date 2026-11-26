#include "src/templates/vault/arch.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
Arch::Arch(const ArchGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // voussoirs: from springing to springing
    for (const std::shared_ptr<Element>& voussoir : arch(guide))
        add(voussoir);

    // contacts: between every two touching voussoirs
    compute_face_contacts(0);

    // assembly: the voussoirs raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
