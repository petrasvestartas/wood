#pragma once
#include "src/templates/module/module_building_guide.h"
#include "src/templates/module/spatial_module.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// ModuleBuilding
// ═══════════════════════════════════════════════════════════════════════════

/// A building of spatial modules.
class ModuleBuilding : public WoodSession {
public:
    const ModuleBuildingGuide guide;

    /// The building of the guide as the session named name.
    explicit ModuleBuilding(const ModuleBuildingGuide& guide = ModuleBuildingGuide(), const std::string& name = "module_building");

    /// Not copied, as a session is not.
    ModuleBuilding(const ModuleBuilding&) = delete;

    /// Not assigned, as it is not copied.
    ModuleBuilding& operator=(const ModuleBuilding&) = delete;

    /// Modules side by side: those of every room.
    size_t columns() const;

    /// Rows of modules: two with a corridor between them.
    size_t rows() const;

    /// Bays of the skeleton across, 0 without one.
    size_t bays() const;

    /// Levels of the skeleton, 0 without one.
    size_t levels() const;

    /// From a module's underside to what carries it: the batten and its strip.
    double bearing_height() const;

    /// From one level of the skeleton to the next.
    double level_height() const;

    /// From one row of posts to the next.
    double bay_width() const;

    /// The underside corner of the module of a storey and a column.
    Point corner(size_t storey, size_t column) const;

private:
    std::vector<AssemblyModule> _modules; // How every module goes together, in the order they were made.

    /// Adds the skeleton: posts over the full height, beams per level, and decks or ledgers on them.
    void add_skeleton();

    /// Adds the battens and strips under the module of a storey and a column.
    void add_bearings(size_t row, size_t storey, size_t column);

    /// Adds the module of a row, a storey and a column, and the one set into it.
    void add_module(size_t row, size_t storey, size_t column);

    /// Adds the corridor's floors and its roof between the two rows, on ledgers on the modules' ends.
    void add_corridor();

    /// Adds per joint between modules the plate over it at the start ends, with its pins into every floor and ceiling it meets.
    void add_couplers();

    /// Adds the pins that hold the skeleton's beams to its posts.
    void add_beam_pins();

    /// How the building goes together: the skeleton, then storey by storey the bearings, the modules, the corridor and the couplers.
    AssemblyModule compute_assembly() const;

    /// The guide of the module of a column, its long sides open towards the modules of its room.
    SpatialModuleGuide module_guide(size_t column) const;

    /// The room of the module in a column.
    size_t room_of(size_t column) const;

    /// The base of a module: its corner, turned half round in the second row.
    Plane module_base(size_t row, size_t storey, size_t column) const;

    /// The floor's or the ceiling's member at a module's start end: its solid plate or its end rib.
    std::shared_ptr<Element> end_member(const std::string& panel, bool solid) const;
};

}
