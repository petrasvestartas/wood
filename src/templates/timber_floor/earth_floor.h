#pragma once
#include "src/templates/timber_floor/earth_floor_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// EarthFloor
// ═══════════════════════════════════════════════════════════════════════════

/// One floor element of timber and rammed earth added to a session, on two beams.
class EarthFloor {
public:
    const EarthFloorGuide guide;
    const Plane base; // Origin at the element's corner at its underside, x along its beams.
    const std::string name; // Prefix of every member and group.

    /// The element of the guide in base, its members added to session under the group name.
    EarthFloor(WoodSession& session, const EarthFloorGuide& guide, const Plane& base, const std::string& name);

    /// How the element goes together: the beams, the earth rammed between them from above, the panel and its pins, the rods.
    AssemblyModule assembly() const;

private:
    WoodSession& _session;
    std::shared_ptr<TreeNode> _group;

    /// Adds the two beams the element lies on.
    void add_bearings();

    /// Adds the halves of every beam, wider at the foot, and the board under them.
    void add_beams();

    /// Adds the earth between two beams, its underside arched or flat.
    void add_earth();

    /// Adds the panel on the beams.
    void add_panel();

    /// Adds the pins of the panel and the rods across the beams.
    void add_pins();

    /// A member along the beams with a section drawn across them.
    std::shared_ptr<EdgeStraightElement> along(const std::vector<std::array<double, 2>>& section, const std::string& part) const;

    /// The point at x along the beams, y across them and z up from the underside.
    Point local(double x, double y, double z) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// EarthFloors
// ═══════════════════════════════════════════════════════════════════════════

/// Floor elements of timber and rammed earth side by side, their sections to the front.
class EarthFloors : public WoodSession {
public:
    static constexpr double GAP = 900.0; // Between two elements.

    const std::vector<std::pair<std::string, EarthFloorGuide>> floors; // Every element's name and guide.

    /// One element per guide as the session named name.
    explicit EarthFloors(
        const std::vector<std::pair<std::string, EarthFloorGuide>>& floors, const std::string& name = "earth_floors"
    );
};

}
