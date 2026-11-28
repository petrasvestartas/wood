#pragma once
#include "src/templates/vault/vault.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Arch
// ═══════════════════════════════════════════════════════════════════════════

/// An arch of voussoirs over a span, every two touching voussoirs in contact.
class Arch : public WoodSession {
public:
    const ArchGuide guide;

    /// The arch of the guide as the session named name.
    explicit Arch(const ArchGuide& guide = ArchGuide(), const std::string& name = "vault_arch");
};

}
