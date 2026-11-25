#pragma once
#include "src/templates/module/framed_panel.h"
#include "src/templates/module/module_contact.h"
#include "src/templates/module/spatial_module_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// SpatialModule
// ═══════════════════════════════════════════════════════════════════════════

/// One spatial module added to a session: a floor, walls or posts on it, a ceiling on them.
class SpatialModule {
public:
    const SpatialModuleGuide guide;
    const Plane base; // Origin at the module's corner at its underside, x along it, y across it.
    const std::string name; // Prefix of every member and group.

    /// The module of the guide in base, its members added to session under the group name in parent.
    SpatialModule(
        WoodSession& session,
        const SpatialModuleGuide& guide,
        const Plane& base,
        const std::string& name,
        const std::shared_ptr<TreeNode>& parent = nullptr
    );

    /// How the module goes together: its floor, its walls, its posts and beams, what is set inside, its ceiling, pins and braces.
    AssemblyModule assembly(const std::vector<AssemblyModule>& inside = {}) const;

private:
    WoodSession& _session;
    std::shared_ptr<TreeNode> _group;

    /// Adds the floor, a solid plate or a box of ribs between two boards.
    void add_floor();

    /// Adds the long walls over the whole length and the end walls between them, on the floor.
    void add_walls();

    /// Adds per open long side its corner and side posts on the floor and the downstand beam on them.
    void add_frames();

    /// Adds the ceiling on the walls and the downstand beams.
    void add_ceiling();

    /// Adds two crossed boards on the inner face of the posts of every open long side.
    void add_braces();

    /// Adds the pins along every edge where two solid plates meet.
    void add_pins();

    /// Adds a panel: one solid plate, or a framed panel.
    void add_panel(const std::string& part, const WallGuide& panel, const Plane& plane, bool solid, double thickness);

    /// The point at along the module, across it and up it.
    Point local(double along, double across, double up) const;

    /// The size of the ribs, the boards and the openings of a floor or a ceiling.
    WallGuide box_guide(double rib, double sheet) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// ModuleTypes
// ═══════════════════════════════════════════════════════════════════════════

/// Spatial modules side by side, each alone: the construction types.
class ModuleTypes : public WoodSession {
public:
    static constexpr double GAP = 2000.0; // Between two modules.

    const std::vector<std::pair<std::string, SpatialModuleGuide>> types; // Every module's name and guide.

    /// One module per type as the session named name.
    explicit ModuleTypes(
        const std::vector<std::pair<std::string, SpatialModuleGuide>>& types, const std::string& name = "module_types"
    );
};

}
