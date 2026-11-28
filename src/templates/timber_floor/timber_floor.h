#pragma once
#include "src/templates/timber_floor/timber_floor_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// TimberFloor
// ═══════════════════════════════════════════════════════════════════════════

/// One bay of a timber floor added to a session: two beams, ribs and boards or panels on them.
class TimberFloor {
public:
    const TimberFloorGuide guide;
    const Plane base; // Origin at the middle of the first beam on the floor's first side, x along the span, the floor's underside in it.
    const std::string name; // Prefix of every member and group.

    /// The floor of the guide in base, its members added to session under the group name.
    TimberFloor(WoodSession& session, const TimberFloorGuide& guide, const Plane& base, const std::string& name);

    /// How the floor goes together: the beams, then the ribs and boards or the panels, the concrete, the pins.
    AssemblyModule assembly() const;

private:
    WoodSession& _session;
    std::shared_ptr<TreeNode> _group;

    /// Adds the two beams, plain, rebated or with a batten glued to each face.
    void add_beams();

    /// Adds the ribs along the span with a board under and on them.
    void add_ribs();

    /// Adds the panels side by side along the span, solid, hollow or of boards on edge, their edges shaped for what joins them.
    void add_panels();

    /// Adds a panel of boards on edge and the dowels through them.
    void add_lamellas(size_t panel, double start);

    /// Adds all the layers on the structure as one, set back from the floor's front.
    void add_layers();

    /// Adds the strips, the cover boards or the keys along the joints between panels.
    void add_joints();

    /// Adds the concrete on the timber and what holds it: a notched panel, crossed pins or flat steel locks.
    void add_composite();

    /// A member along the span with a section drawn across it, from the point at 0 along the span.
    std::shared_ptr<EdgeStraightElement> along(const std::vector<std::array<double, 2>>& section, const std::string& part) const;

    /// The point at x along the span, y across it and z up from the underside.
    Point local(double x, double y, double z) const;

    /// Where the middles of the ribs are across the floor.
    std::vector<double> rib_middles() const;

    /// Where the floor's members start and end along the span.
    std::array<double, 2> reach() const;

    /// The panels and the width of each.
    std::pair<size_t, double> panels() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// TimberFloors
// ═══════════════════════════════════════════════════════════════════════════

/// Timber floors side by side, their sections to the front.
class TimberFloors : public WoodSession {
public:
    static constexpr double GAP = 900.0; // Between two floors.

    const std::vector<std::pair<std::string, TimberFloorGuide>> floors; // Every floor's name and guide.

    /// One bay per floor as the session named name, in rows of count.
    explicit TimberFloors(
        const std::vector<std::pair<std::string, TimberFloorGuide>>& floors, const std::string& name = "timber_floors", size_t count = 8
    );
};

}
