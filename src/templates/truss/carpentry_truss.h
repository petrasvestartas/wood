#pragma once
#include "src/templates/truss/truss_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// CarpentryTruss
// ═══════════════════════════════════════════════════════════════════════════

/// A truss of carpentry joints: a beam per chord segment and per web, each pair that meets joined by the joint its classification asks for.
class CarpentryTruss : public WoodSession {
public:
    const TrussGuide guide; // The lines the truss is drawn with, in the xz plane.
    const double thickness; // mm every member measures across the truss plane, so every notch runs through.

    /// The truss: members, then a joint per pair that meets.
    explicit CarpentryTruss(const TrussGuide& guide, double thickness = 140.0, const std::string& name = "carpentry_truss");

private:
    /// A beam per chord segment, chord_<c>_<s>, and per web, web_<w>: thickness across the plane, twice the guide's radius in it.
    void add_members();

    /// One member: thickness across the plane, twice radius in it, in group.
    void add_member(const Line& axis, double radius, const std::string& name, const std::string& group);

    /// Per pair that meets: an end on a side at an angle a notched oblique thrust, square a blind mortise and tenon; two ends
    /// where a third member runs past bear on that one; any other category throws, its joint not made yet.
    void add_joints();
};

}
