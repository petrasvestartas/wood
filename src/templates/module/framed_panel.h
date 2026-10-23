#pragma once
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// FramedPanel
// ═══════════════════════════════════════════════════════════════════════════

/// A timber-frame panel in a plane, added to a session: rails along its foot and top, the bottom rail split at every door, an edge
/// stud at both ends, per opening a jamb stud either side with the header and the sill between them, studs on the grid of the stud
/// spacing where no edge or jamb stud stands, cripples over and under the openings, rows of noggings between the studs, and one sheet
/// on each face, every door a notch and every window a hole. A wall stands in its plane; a floor or a ceiling lies in it, its rails the rim joists and its studs the
/// joists.
///
/// Fields: the guide and the plane it is built in, x along the panel, y up it, the frame centred on the plane across it.
/// Every member is named `<name>_<part>` under the group `<name>`, its parts `rails`, `studs`, `opening_<o>`, `noggings`, `sheathing_<side>`;
/// with no name the parts are named alone under the session's root.
class FramedPanel {
public:
    const WallGuide guide; // Length along x, height along y, the member section, the stud spacing, the sheet and the openings.
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

    /// Adds the edge studs and a stud at every multiple of the stud spacing from the panel's start where no edge or jamb stud stands;
    /// one inside an opening is a cripple over its header and under its sill, in opening_<o>. Returns every full-height stud and jamb
    /// stud as its centre along the panel and its half width, left to right.
    std::vector<std::array<double, 2>> add_studs();

    /// Adds per row of noggings a block between every two neighbouring studs, none between the jamb studs of an opening, in noggings.
    void add_noggings(const std::vector<std::array<double, 2>>& studs);

    /// Adds one sheet on each face of the whole panel, every door a notch and every window a hole, in sheathing_0 and sheathing_1.
    void add_sheathing();

    /// The point at along the panel, across it from the plane and up it.
    Point local(double along, double across, double up) const;

    /// The part named under this panel's name.
    std::string named(const std::string& part) const;

    /// The group of a part under this panel's group, made the first time.
    std::shared_ptr<TreeNode> group(const std::string& part) const;

    /// A frame member from start to end, stud_depth across the panel and size along up.
    std::shared_ptr<Beam> member(
        const Point& start,
        const Point& end,
        double size,
        const Vector& up,
        const std::string& part
    ) const;
};

}
