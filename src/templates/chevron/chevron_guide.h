#pragma once
#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// ChevronGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a chevron shell: the surface, its rows of chevrons and the boxes of plates on them.
struct ChevronGuide {
    NurbsSurface surface = default_surface(); // The surface the chevrons are laid on.
    int u_divisions = 4; // Strips across u.
    double v_division_dist = 900.0; // The first row's height in v, in the surface's parameter units.
    double shift = 0.5; // Share of a row's height the middle of a chevron stands ahead of its sides.
    double scale = 0.05799; // Growth of the row height per row, towards the middle.
    double box_height = 760.0; // mm between the box's top and bottom faces.
    double top_plate_inlet = 80.0; // mm the top and bottom plates stand in from the box faces.
    double plate_thickness = 40.0; // mm, every plate.
    double edge_rotation = 1.0; // Degrees the even chevron edges turn about their plane's y axis.
    double edge_offset = 0.5; // Share of the plate thickness the odd chevron edges move along their normal.
    double tab_length = 80.0; // mm of edge per tenon division, Christopher's L_tab: round(0.8 edge / tab_length) raised to 4m + 1 segments gives m tenons.
    double finger_angle = 15.0; // Degrees the corner finger cuts turn from the box faces, each way in turn.
    int finger_divisions = 7; // Parts every corner is cut in.
    double stop_threshold = 2000.0; // The rows stop and mirror when less than this remains to the middle, in the surface's parameter units.
    std::array<int, 4> ortho_edges = {1, 1, 1, 1}; // Per edge index the axis a boundary edge plane snaps to.
    std::array<double, 3> boundary_planes = {NAN, NAN, NAN}; // The y of the front and of the back boundary plane and the z of the ground; NaN keeps a boundary edge plane through its edge.

    /// A flat bicubic surface, 3000 by 5000, centred on the origin.
    static NurbsSurface default_surface() {

        const double half_u = 1500.0;
        const double half_v = 2500.0;
        std::vector<Point> points;

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                points.push_back({half_u * (2.0 * i / 3.0 - 1.0), half_v * (2.0 * j / 3.0 - 1.0), 0.0});

        NurbsSurface flat = NurbsSurface::create(false, false, 3, 3, 4, 4, points);
        flat.set_domain(0, -half_u, half_u);
        flat.set_domain(1, -half_v, half_v);
        return flat;
    }

    /// The Annen chevron shell on surface: 760 mm boxes of 40 mm plates on four strips.
    static ChevronGuide annen(const NurbsSurface& surface) {

        ChevronGuide guide;
        guide.surface = surface;

        return guide;
    }

    /// The sliders of the 2017 MAX2 prototype, arch 22 as it was built on surface: rows growing by 0.04993, edges turned 1.1 degrees and moved 0.45 of a plate, the sides closed at y 132000 and 126000 and the ground at z 3520.
    static ChevronGuide max2_arch(const NurbsSurface& surface) {

        ChevronGuide guide = annen(surface);
        guide.scale = 0.04993;
        guide.edge_rotation = 1.1;
        guide.edge_offset = 0.45;
        guide.boundary_planes = {132000.0, 126000.0, 3520.0};

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ChevronShiftedGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a shifted chevron shell: the surface, its grid of staggered hexagonal cells and the boxes of plates on them.
struct ChevronShiftedGuide {
    NurbsSurface surface = ChevronGuide::default_surface(); // The surface the cells are laid on, its u running along the shell.
    int iso_count = 54; // Iso curves across the surface, the rows of the point grid.
    int point_count = 9; // Points on every iso curve, one more than the columns.
    double shift = 0.4; // Share of the row spacing the interior points move towards the next row in even columns and the previous one in odd columns.
    double edge_rotation = 0.05 * Tolerance::TO_DEGREES; // Degrees the merged long-side planes of the even columns turn about the surface normal, the sign alternating per column pair.
    double plate_offset = 300.0; // mm from the surface to the middle of the top and bottom plates.
    double plate_thickness = 40.0; // mm, every plate.
    double wall_extend = 80.0; // mm the walls run past the outer faces of the top and bottom plates.
    bool average_planes = true; // Merge the two planes of every even cell's long sides into one turned plane.
    int divisions = 1; // Tenon steps per edge: 2·divisions + 1 on a short edge, 4·divisions + 1 on a long one, a tenon at every odd step.
    double long_edge = 700.0; // mm; an edge longer than this takes twice the steps.
    double short_edge = 200.0; // mm; an edge shorter than this takes no tenons.
    int finger_divisions = 7; // Parts every box corner's Annen fingers cut it in.
    bool closed_boxes = true; // A cell's plates enter its own walls along its own vector and its outward walls' corners are plain, so every box goes together on its own; false for the definition's joints as they were.

    /// The 2018 Annen pattern on surface with closed boxes: 54 iso curves of 9 points, 40 mm plates 300 mm from the surface.
    static ChevronShiftedGuide annen(const NurbsSurface& surface) {

        ChevronShiftedGuide guide;
        guide.surface = surface;

        return guide;
    }

    /// The 2018 Annen pattern on surface with the definition's joints as they were, its boxes not closed.
    static ChevronShiftedGuide definition(const NurbsSurface& surface) {

        ChevronShiftedGuide guide = annen(surface);
        guide.closed_boxes = false;

        return guide;
    }
};
