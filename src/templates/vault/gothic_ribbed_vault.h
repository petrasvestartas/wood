#pragma once
#include "src/templates/vault/vault.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// GothicRibbedVault
// ═══════════════════════════════════════════════════════════════════════════

/// A Gothic ribbed vault bay of mitered EdgeCurvedElement ribs, corner support blocks, and cylindrical web panels.
class GothicRibbedVault : public WoodSession {
public:
    const GothicRibbedVaultGuide guide;

    /// The bay of the guide as the session named name, its supports, ribs and panels each in their group.
    explicit GothicRibbedVault(
        const GothicRibbedVaultGuide& guide = GothicRibbedVaultGuide(), const std::string& name = "gothic_ribbed_vault"
    );
};

}
