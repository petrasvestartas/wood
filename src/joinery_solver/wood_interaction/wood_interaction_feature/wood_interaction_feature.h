#pragma once

#include "pch.h"

namespace wood_session {

/// What the solver cut at one contact: a plate joint, a beam joint or a plate-to-beam joint, one derived class each.
class InteractionFeature : public session_cpp::Interaction {
public:
    std::string contact_guid; // Guid of the stored contact this was solved from; empty when none.

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "plate", "beam" or "plate_beam".
    virtual std::string_view kind() const = 0;
};

} // namespace wood_session
