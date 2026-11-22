#include "pch.h"
#include "wood_joint_move.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

JointMove JointMove::slide(const Vector& shift) {
    JointMove move;
    move.shift = shift;
    return move;
}

JointMove JointMove::turn(const Line& axis, double angle) {
    JointMove move;
    move.axis = axis;
    move.angle = angle;
    return move;
}

// ═══════════════════════════════════════════════════════════════════════════
// Transformation
// ═══════════════════════════════════════════════════════════════════════════

Xform JointMove::at(double share) const {
    if (is_turn())
        return Xform::rotation_around_line(axis, angle * share, true);
    return Xform::translation(shift[0] * share, shift[1] * share, shift[2] * share);
}

JointMove JointMove::reversed() const {
    return is_turn() ? turn(axis, -angle) : slide(shift * -1.0);
}

JointMove JointMove::transformed(const Xform& xform) const {
    return is_turn() ? turn(axis.transformed(xform), angle) : slide(shift.transformed(xform));
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

bool JointMove::is_turn() const {
    return angle != 0.0;
}

double JointMove::reach(const AABB& box) const {
    if (!is_turn())
        return shift.magnitude();

    // the farthest corner's distance from the line, times the angle in radians
    const Vector along = axis.to_vector().normalized();
    double farthest = 0.0;
    for (const Point& corner : box.get_corners()) {
        const Vector arm = corner - axis.start();
        farthest = std::max(farthest, (arm - along * arm.dot(along)).magnitude());
    }
    return farthest * std::abs(angle) * Tolerance::PI / 180.0;
}

}
