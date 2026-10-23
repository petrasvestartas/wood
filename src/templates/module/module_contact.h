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

}
