#include "src/templates/vault/wall.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
Wall::Wall(const WallGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // bricks: course by course from the ground up
    for (const std::shared_ptr<Element>& brick : wall(guide))
        add(brick);

    // contacts: between every two touching bricks
    compute_face_contacts(0);

    // assembly: the bricks raised one by one
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

}
