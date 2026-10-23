#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Contact
// ═══════════════════════════════════════════════════════════════════════════

/// A contact between two members of a module, stored as their interaction.
///
/// - `a`: the source member, the one a pin passes through first.
/// - `b`: the target member, which hosts the contact.
/// - `face`: the face they share.
struct Contact {
    std::shared_ptr<Element> a; // The source member.
    std::shared_ptr<Element> b; // The target member, which hosts the contact.
    std::shared_ptr<InteractionContactFace> face; // The face they share, stored as their interaction.
};

// ═══════════════════════════════════════════════════════════════════════════
// Contacts of a framed module
// ═══════════════════════════════════════════════════════════════════════════

/// Searches the face contact between every two beams and plates of session whose boxes touch, stores it as their interaction and
/// draws its polygon as a red mesh. Between two beams, in contacts_frame, the member that ends on the contact is its target, named
/// `<target>_on_<source>`, and two side by side `<target>_beside_<source>`; with a plate, in contacts_sheathing, the plate is the
/// source, named `<plate>_on_<other>`. Returns how many it found in each group.
std::array<size_t, 2> add_module_contacts(WoodSession& session);

}
