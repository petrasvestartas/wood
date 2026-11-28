#pragma once
#include "src/templates/vault/vault.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Barrel
// ═══════════════════════════════════════════════════════════════════════════

/// A barrel vault of staggered voussoir courses along y, every two touching voussoirs in contact.
class Barrel : public WoodSession {
public:
    const BarrelGuide guide;

    /// The barrel vault of the guide as the session named name.
    explicit Barrel(const BarrelGuide& guide = BarrelGuide(), const std::string& name = "vault_barrel");
};

}
