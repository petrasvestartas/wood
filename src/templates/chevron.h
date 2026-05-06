#pragma once
#include "session.h"
#include "wood_session.h"
#include "../wood_chevron.h"
#include "nurbssurface.h"

#include <stdexcept>

using namespace session_cpp;
using namespace wood_session;

/// A chevron shell from a NURBS surface with thickness.
///
/// Subdivides a NURBS surface into a V-shaped zigzag (chevron) quad mesh,
/// then offsets each face into 4 plate-pairs per face (top, bottom, side0, side1)
/// stored as WoodElement objects. Joinery data is also stored for use with
/// get_connection_zones().
///
/// Usage:
///   Chevron ch;                              // built-in flat 3000×5000 surface
///   Chevron ch(my_surface, 4, 900.0);        // custom surface, 4 strips, 900mm rows
class Chevron {
public:
    Mesh mesh;
    std::vector<WoodElement> elements;

    /// Joinery solver inputs (see wood_chevron::ChevronResult)
    std::vector<std::array<double,18>> insertion_vectors;
    std::vector<std::array<int,6>>     joints_per_face;
    std::vector<std::array<int,4>>     three_valence;
    std::vector<std::pair<int,int>>    adjacency;

    Chevron(NurbsSurface surface       = default_surface(),
            int    u_divisions         = 4,
            double v_division_dist     = 900.0,
            double shift               = 0.5,
            double scale               = 0.05799,
            double box_height          = 760.0,
            double top_plate_inlet     = 80.0,
            double plate_thickness     = 40.0,
            double edge_rotation       = 1.0,
            double edge_offset         = 0.5)
    {
        if (u_divisions < 1)
            throw std::invalid_argument("Chevron: u_divisions must be >= 1");
        if (plate_thickness == 0.0)
            throw std::invalid_argument("Chevron: plate_thickness must not be zero");

        mesh = wood_chevron::chevron_mesh(
            surface, u_divisions, v_division_dist, shift, scale);

        auto result = wood_chevron::chevron_plates(
            mesh, edge_rotation, edge_offset,
            box_height, top_plate_inlet, plate_thickness, true);

        // 8 polylines per face → 4 plate-pairs → 4 WoodElements per face
        for (size_t i = 0; i + 1 < result.plines.size(); i += 2)
            elements.emplace_back(result.plines[i], result.plines[i + 1]);

        insertion_vectors = std::move(result.insertion_vectors);
        joints_per_face   = std::move(result.joints_per_face);
        three_valence     = std::move(result.three_valence);
        adjacency         = std::move(result.adjacency);
    }

    /// Degree-3 bicubic flat surface: 3000×5000 mm in the XY plane.
    /// chevron_mesh will use V as the long axis (5000 mm arc).
    static NurbsSurface default_surface() {
        // order=4 (degree 3), 4×4 control points.
        // Knots use physical mm values [0,W] and [0,L] so that chevron_mesh
        // can use v_division_dist directly as a parametric step (matching
        // the original Python where baseStepV = v_division_dist with physical domains).
        // Clamped knot vector for 4 CVs degree-3: full=[0,0,0,0,W,W,W,W],
        // OpenNURBS strips first/last → [0,0,0,W,W,W].
        const double W = 3000.0, L = 5000.0;

        NurbsSurface srf;
        srf.create_raw(3, false, 4, 4, 4, 4);

        const double ku[] = {0.0, 0.0, 0.0, W, W, W};
        const double kv[] = {0.0, 0.0, 0.0, L, L, L};
        for (int i = 0; i < 6; i++) {
            srf.set_nurbsknot(0, i, ku[i]);
            srf.set_nurbsknot(1, i, kv[i]);
        }

        const double us[] = {0.0, W/3.0, 2.0*W/3.0, W};
        const double vs[] = {0.0, L/3.0, 2.0*L/3.0, L};

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                srf.set_cv(i, j, Point(us[i], vs[j], 0.0));

        return srf;
    }
};
