#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// What the solver cut at one contact: a plate joint, a beam joint or a plate-to-beam joint, one derived class each.
class InteractionFeature : public Interaction {
public:

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "plate", "beam" or "plate_beam".
    virtual std::string_view kind() const = 0;
};

} // namespace wood_session
