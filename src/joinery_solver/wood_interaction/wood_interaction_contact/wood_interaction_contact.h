#pragma once

#include "pch.h"

namespace wood_session {

/// One place where the edge's two elements touch: a face overlap, a closest segment or a crossing, one derived class each.
class InteractionContact : public session_cpp::Interaction {
public:
    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// "face", "axis" or "cross".
    virtual std::string_view kind() const = 0;

    /// The contact read from the other end of the edge, same guid.
    virtual std::shared_ptr<InteractionContact> flipped() const = 0;

    /// Same kind and place: what detection reuses instead of storing twice.
    virtual bool coincides(const InteractionContact& other) const = 0;
};

} // namespace wood_session
