#pragma once
#include "src/templates/vault/vault.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Dome
// ═══════════════════════════════════════════════════════════════════════════

/// A spherical dome of staggered hoops of voussoirs, every two touching voussoirs in contact.
class Dome : public WoodSession {
public:
    const DomeGuide guide;

    /// The dome of the guide as the session named name.
    explicit Dome(const DomeGuide& guide = DomeGuide(), const std::string& name = "vault_dome");
};

}
