#pragma once
#include "src/templates/vault/vault.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Wall
// ═══════════════════════════════════════════════════════════════════════════

/// A wall of bricks in stretcher bond, every two touching bricks in contact.
class Wall : public WoodSession {
public:
    const WallGuide guide;

    /// The wall of the guide as the session named name.
    explicit Wall(const WallGuide& guide = WallGuide(), const std::string& name = "vault_wall");
};

}
