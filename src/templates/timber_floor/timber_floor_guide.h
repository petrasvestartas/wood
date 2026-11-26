#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// FloorSize
// ═══════════════════════════════════════════════════════════════════════════

/// One row of the preliminary sizing tables: the floors that carry a use over a span, in mm, deflection up to span / 500.
struct FloorSize {
    double span; // Single span.
    bool heavy; // Floor build-up of 1.8 kN/m2, else 0.8.
    std::array<std::array<double, 2>, 3> joists; // Width and depth of C24 / GL24 joists every 500, 600 and 700.
    double box_rib; // Rib depth of a box of 80 ribs every 500 between two 27 boards, glued.
    double dowelled; // Thickness of a dowel-laminated floor.
    std::array<double, 2> composite; // Dowel-laminated boards and the concrete on them.
    double hollow; // Depth of a hollow box element.
};

/// The sizes per use, every span first light, then heavy: homes 2 kN/m2, offices 3 kN/m2, assembly rooms 4 kN/m2.
inline const std::array<std::array<FloorSize, 6>, 3> FLOOR_SIZES = {{
    {{
        {4000.0, false, {{{100.0, 220.0}, {120.0, 220.0}, {120.0, 220.0}}}, 100.0, 130.0, {100.0, 80.0}, 140.0},
        {4000.0, true, {{{140.0, 220.0}, {120.0, 240.0}, {140.0, 240.0}}}, 120.0, 150.0, {100.0, 80.0}, 160.0},
        {5000.0, false, {{{140.0, 240.0}, {140.0, 260.0}, {120.0, 280.0}}}, 160.0, 170.0, {100.0, 80.0}, 180.0},
        {5000.0, true, {{{120.0, 280.0}, {160.0, 280.0}, {180.0, 280.0}}}, 180.0, 180.0, {100.0, 80.0}, 200.0},
        {6000.0, false, {{{100.0, 320.0}, {120.0, 320.0}, {140.0, 320.0}}}, 200.0, 200.0, {120.0, 100.0}, 220.0},
        {6000.0, true, {{{100.0, 360.0}, {120.0, 360.0}, {140.0, 360.0}}}, 240.0, 220.0, {120.0, 100.0}, 240.0},
    }},
    {{
        {5000.0, false, {{{140.0, 260.0}, {140.0, 280.0}, {160.0, 280.0}}}, 180.0, 180.0, {100.0, 80.0}, 220.0},
        {5000.0, true, {{{160.0, 280.0}, {180.0, 280.0}, {140.0, 320.0}}}, 200.0, 200.0, {100.0, 80.0}, 220.0},
        {6000.0, false, {{{200.0, 280.0}, {160.0, 320.0}, {200.0, 320.0}}}, 240.0, 220.0, {120.0, 100.0}, 280.0},
        {6000.0, true, {{{180.0, 320.0}, {160.0, 360.0}, {180.0, 360.0}}}, 260.0, 240.0, {120.0, 100.0}, 280.0},
        {7500.0, false, {{{180.0, 360.0}, {160.0, 400.0}, {180.0, 400.0}}}, 320.0, 280.0, {140.0, 140.0}, 320.0},
        {7500.0, true, {{{160.0, 400.0}, {160.0, 440.0}, {180.0, 440.0}}}, 360.0, 320.0, {160.0, 140.0}, 360.0},
    }},
    {{
        {5000.0, false, {{{160.0, 280.0}, {200.0, 280.0}, {160.0, 320.0}}}, 200.0, 200.0, {100.0, 80.0}, 220.0},
        {5000.0, true, {{{200.0, 280.0}, {160.0, 320.0}, {180.0, 320.0}}}, 220.0, 200.0, {100.0, 80.0}, 240.0},
        {6000.0, false, {{{200.0, 320.0}, {160.0, 360.0}, {180.0, 360.0}}}, 260.0, 240.0, {120.0, 100.0}, 280.0},
        {6000.0, true, {{{160.0, 360.0}, {200.0, 360.0}, {160.0, 400.0}}}, 280.0, 250.0, {120.0, 100.0}, 280.0},
        {7500.0, false, {{{180.0, 400.0}, {160.0, 440.0}, {180.0, 440.0}}}, 360.0, 320.0, {160.0, 140.0}, 360.0},
        {7500.0, true, {{{160.0, 440.0}, {200.0, 440.0}, {180.0, 480.0}}}, 400.0, 320.0, {160.0, 140.0}, 360.0},
    }},
}};

/// All the layers on a structure together: 0.8 kN/m2 (dry boards, a fill) and 1.8 kN/m2 (screed, insulation, a fill).
inline constexpr double LAYERS_LIGHT = 60.0;
inline constexpr double LAYERS_HEAVY = 110.0;

// ═══════════════════════════════════════════════════════════════════════════
// TimberFloorGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of one bay of a timber floor in mm: ribs and boards, or panels side by side, on two beams.
struct TimberFloorGuide {
    double span = 5000.0; // Between the middles of the two beams, along x.
    double width = 2400.0; // Across the span, along y.

    double rib_width = 0.0; // Joist or rib width, 0 for none.
    double rib_depth = 0.0; // Joist or rib depth.
    double rib_spacing = 600.0; // Largest spacing of the ribs, dividing the width evenly.
    double flange_width = 0.0; // Flanges of an I-joist, the rib its web; 0 for a plain rib.
    double flange_depth = 0.0;
    double top = 0.0; // Board on the ribs, 0 for none.
    double bottom = 0.0; // Board under the ribs, 0 for none.
    double bottom_width = 0.0; // A strip this wide under every rib instead of the board, 0 for the board.

    double panel = 0.0; // Thickness of the panels, 0 for none.
    double panel_width = 0.0; // Width of a panel, dividing the width evenly; 0 for one panel.
    size_t edge = 0; // Panel edges: 0 butt, 1 tongue in groove, 2 strip in two grooves, 3 board in two rebates, 4 keys across.
    double lamella = 0.0; // Thickness of the boards on edge a panel is made of, 0 for a solid panel.
    double dowel = 20.0; // Diameter of the dowels through the boards of a panel.
    double dowel_spacing = 600.0; // Dowel spacing along the span.
    double skin = 0.0; // Shell thickness of a hollow panel, its webs too, 0 for a full one.
    double web_spacing = 200.0; // Webs of a hollow panel.
    size_t plies = 0; // Cross-laminated: the panel's layers, 0 for one piece.
    double concrete = 0.0; // Thickness of the concrete on the timber, 0 for none.
    size_t connector = 1; // Of the concrete: 0 glue, 1 notches in a panel, 2 crossed pins, 3 flat steel locks.
    double notch = 20.0; // Depth of the notches the concrete fills.
    size_t notches = 3; // Notches, pins or locks in each half of the span.

    double layers = 0.0; // All the layers on the structure together, as one, 0 for none.

    size_t seat = 0; // Bearings: 0 on the beams, 1 in their rebates, 2 on battens glued to their faces.
    double beam_width = 160.0; // Width of the beams.
    double beam_depth = 280.0; // Depth of the beams below the floor.
    double seat_length = 60.0; // Length of the bearing in a rebate or on a batten.

    /// Depth of the structure.
    double depth() const {
        return bottom + rib_depth + top + panel + concrete;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Producers, as deep as a span and the loads in kN/m2 ask (floor_sizing.cpp)
    // ═══════════════════════════════════════════════════════════════════════════

    /// Pollmeier BauBuche: GL75 joists 120 every 850 under BauBuche Q 40, 8 Hz under the permanent load.
    static TimberFloorGuide baubuche(double span, double g = 3.0, double q = 3.0);

    /// Metsa Kerto-Ripa: veneer ribs every 612 under 25, a strip 300 x 43 under each when boxed, 8 Hz.
    static TimberFloorGuide kerto_ripa(double span, bool boxed, double g = 1.9, double q = 2.0);

    /// VMG Lignum joists: flanges 45, 60 or 90 x 39, a web 10, under a deck 22, span / 250 with creep and 8 Hz.
    static TimberFloorGuide vmg_joists(double span, double spacing = 600.0, double g = 1.0, double q = 2.0);

    /// Lignatur surface elements LFE, 1000 wide, span / 475 under the whole load.
    static TimberFloorGuide lignatur(double span, double g = 1.0, double q = 2.8);

    /// Schneider CLT BOX: glulam ribs between cross-laminated flanges 60, span / 300 of the lasting deflection.
    static TimberFloorGuide clt_box(double span, double g = 2.5, double q = 3.0);

    /// Novatop Element: ribs 27 every 340 between panels 27, span / 300.
    static TimberFloorGuide novatop(double span, double g = 2.0, double q = 3.0);

    /// Hasslacher: glulam ribs 160 every 625 under cross-laminated 120, 8 Hz under the permanent load.
    static TimberFloorGuide clt_ribs(double span, double g = 1.5, double q = 3.0);

    /// The smallest row of a use (0 homes, 1 offices, 2 assembly rooms) over at least a span.
    static FloorSize size_of(size_t use, double span, bool heavy) {

        for (const FloorSize& size : FLOOR_SIZES.at(use))
            if (size.span >= span && size.heavy == heavy)
                return size;

        throw std::invalid_argument("no floor that long in the table");
    }

    /// Joists every 500, 600 or 700 under a 27 board.
    static TimberFloorGuide joists(size_t use, double span, bool heavy, size_t spacing = 1) {

        const FloorSize size = size_of(use, span, heavy);
        TimberFloorGuide guide;
        guide.span = span;
        guide.layers = heavy ? LAYERS_HEAVY : LAYERS_LIGHT;
        guide.rib_width = size.joists.at(spacing)[0];
        guide.rib_depth = size.joists.at(spacing)[1];
        guide.rib_spacing = 500.0 + 100.0 * static_cast<double>(spacing);
        guide.top = 27.0;

        return guide;
    }

    /// A box of 80 ribs every 500 between two 27 boards, glued.
    static TimberFloorGuide box(size_t use, double span, bool heavy) {

        TimberFloorGuide guide = joists(use, span, heavy, 0);
        guide.rib_width = 80.0;
        guide.rib_depth = size_of(use, span, heavy).box_rib;
        guide.bottom = 27.0;

        return guide;
    }

    /// Dowel-laminated panels of 40 boards on edge.
    static TimberFloorGuide dowelled(size_t use, double span, bool heavy) {

        TimberFloorGuide guide;
        guide.span = span;
        guide.layers = heavy ? LAYERS_HEAVY : LAYERS_LIGHT;
        guide.panel = size_of(use, span, heavy).dowelled;
        guide.panel_width = 600.0;
        guide.lamella = 40.0;

        return guide;
    }

    /// Solid panels under concrete, notched into each other.
    static TimberFloorGuide composite(size_t use, double span, bool heavy) {

        const FloorSize size = size_of(use, span, heavy);
        TimberFloorGuide guide;
        guide.span = span;
        guide.layers = heavy ? LAYERS_HEAVY : LAYERS_LIGHT;
        guide.panel = size.composite[0];
        guide.concrete = size.composite[1];

        return guide;
    }

    /// Hollow box panels of 31 shells and webs.
    static TimberFloorGuide hollow(size_t use, double span, bool heavy) {

        TimberFloorGuide guide;
        guide.span = span;
        guide.layers = heavy ? LAYERS_HEAVY : LAYERS_LIGHT;
        guide.panel = size_of(use, span, heavy).hollow;
        guide.panel_width = 1200.0;
        guide.skin = 31.0;

        return guide;
    }
};

}
