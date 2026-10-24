#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// FramedPanel
// ═══════════════════════════════════════════════════════════════════════════

/// A timber-frame panel in a plane, added to a session.
class FramedPanel {
public:
    const WallGuide
        guide; // Length along x, height along y, the member section, the stud spacing, the sheet and the openings.
    const Plane base; // Origin at the panel's start on its foot, x along it, y up it, z across it.
    const std::string name; // Prefix of every member and group, empty for none.

    /// The panel of the guide in base, its members added to session under the group name in parent.
    FramedPanel(
        WoodSession& session,
        const WallGuide& guide,
        const Plane& base,
        const std::string& name = "",
        const std::shared_ptr<TreeNode>& parent = nullptr
    );

private:
    WoodSession& _session;
    std::shared_ptr<TreeNode> _group;

    /// Adds the bottom rail, split at every door, and the top rail, in rails.
    void add_rails();

    /// Adds per opening a full-height jamb stud either side, the header between them and under a window its sill, in opening_<o>.
    void add_openings();

    /// Adds edge, grid and cripple studs around the panel openings.
    void add_studs();

    /// Adds one sheet on each face of the whole panel.
    void add_sheathing();

    /// The point at along the panel, across it from the plane and up it.
    Point local(double along, double across, double up) const;

    /// The part named under this panel's name.
    std::string named(const std::string& part) const;

    /// The group of a part under this panel's group, made the first time.
    std::shared_ptr<TreeNode> group(const std::string& part) const;

    /// A frame member from start to end, stud_depth across the panel and size along up.
    std::shared_ptr<Beam> member(
        const Point& start, const Point& end, double size, const Vector& up, const std::string& part
    ) const;
};

}
