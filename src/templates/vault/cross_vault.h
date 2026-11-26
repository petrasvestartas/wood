#pragma once
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// CrossVault
// ═══════════════════════════════════════════════════════════════════════════

/// A cross (groin) vault over a square bay, every two touching voussoirs in contact.
class CrossVault : public WoodSession {
public:
    const CrossVaultGuide guide;

    /// The cross vault of the guide as the session named name.
    explicit CrossVault(const CrossVaultGuide& guide = CrossVaultGuide(), const std::string& name = "vault_cross_vault");
};

}
