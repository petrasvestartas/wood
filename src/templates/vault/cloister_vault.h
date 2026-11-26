#pragma once
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// CloisterVault
// ═══════════════════════════════════════════════════════════════════════════

/// A cloister (pavilion) vault over a square bay, every two touching voussoirs in contact.
class CloisterVault : public WoodSession {
public:
    const CloisterVaultGuide guide;

    /// The cloister vault of the guide as the session named name.
    explicit CloisterVault(const CloisterVaultGuide& guide = CloisterVaultGuide(), const std::string& name = "vault_cloister_vault");
};

}
