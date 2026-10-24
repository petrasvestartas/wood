#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Contact
// ═══════════════════════════════════════════════════════════════════════════

/// A contact between two members of a module, stored as their interaction. - `a`.
struct Contact {
    std::shared_ptr<Element> a; // The source member.
    std::shared_ptr<Element> b; // The target member, which hosts the contact.
    std::shared_ptr<InteractionContactFace>
        face; // The face they share, stored as their interaction.
};

// ═══════════════════════════════════════════════════════════════════════════
// Contacts of a framed module
// ═══════════════════════════════════════════════════════════════════════════

/// Stores face contacts between touching beams and plates and draws their polygons.
std::array<size_t, 2> add_module_contacts(WoodSession& session);

}
