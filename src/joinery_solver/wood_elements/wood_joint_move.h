#pragma once

#include "pch.h"

using namespace session_cpp;

namespace wood_session {

/// One move a piece makes after its approach as it goes in: a slide, or a turn about a line fixed where the joint stands (a
/// Chidori stick turning a quarter about its own axis); at(share) is the placement part way, so a move is swept as matrices.
struct JointMove {
    Vector shift; // The slide; zero for a turn.
    Line axis; // The line a turn goes about, right-handed; unused for a slide.
    double angle = 0.0; // Degrees of the turn; 0 for a slide.

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A slide by shift.
    static JointMove slide(const Vector& shift);

    /// A turn by angle degrees about axis.
    static JointMove turn(const Line& axis, double angle);

    // ═══════════════════════════════════════════════════════════════════════════
    // Transformation
    // ═══════════════════════════════════════════════════════════════════════════

    /// The placement share of the way along the move, 0 where it starts, 1 where it ends.
    Xform at(double share) const;

    /// The move undone: the slide back, or the turn the other way about the same line.
    JointMove reversed() const;

    /// The move where xform puts the joint.
    JointMove transformed(const Xform& xform) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Whether it turns.
    bool is_turn() const;

    /// How far a point of box travels at most: the slide's length, or the arc its farthest corner sweeps.
    double reach(const AABB& box) const;
};

}
