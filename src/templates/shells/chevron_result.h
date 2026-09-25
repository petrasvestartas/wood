#pragma once
#include "polyline.h"

#include <array>
#include <utility>
#include <vector>

namespace wood_chevron {

/// Output of chevron_plates() — plate geometry + full joinery solver data.
/// Matches the Grasshopper get_joinery_solver_output() from code.py.
///
/// Plate ordering: 8 polylines per mesh face (in f_order), grouped as 4
/// plate-pairs (index k = counter*4 + role):
///   role 0 = top face plate  (plines counter*8+0/1)
///   role 1 = bottom face plate (plines counter*8+2/3)
///   role 2 = side plate at chevron edge 0 (plines counter*8+4/5)
///   role 3 = side plate at chevron edge 1 (plines counter*8+6/7)
struct ChevronResult {
    /// 8 polylines per face (all faces in f_order, consecutively).
    std::vector<session_cpp::Polyline> plines;

    /// Insertion vector per plate-pair.  One line = one plate-pair = 6 Vec3
    /// packed as 18 doubles (x0 y0 z0  x1 y1 z1  ...  x5 y5 z5).
    /// Positions 0-1 are zero (top/bottom faces of the plate).
    /// Positions 2-5 are the bisector directions for the T-joint tenons.
    std::vector<std::array<double,18>> insertion_vectors;

    /// Joint type per plate-pair.  One entry = 6 ints (one per plate face).
    ///   0  = no joint
    ///  10  = tenon (male)
    ///  20  = mortise (female)
    std::vector<std::array<int,6>> joints_per_face;

    /// Three-valence alignment groups (Annen method, type 0).
    /// Each row [s0, s1, e20, e31]: plate-pair s0 connects to s1,
    /// and plate-pair e20 also connects to s1 — the joint on s1 must be
    /// trimmed so both tenons fit without colliding.
    /// Written to `<dataset>_three_valence.txt` for get_connection_zones().
    std::vector<std::array<int,4>> three_valence;

    /// Adjacency pairs (plate-pair index pairs that share a joint).
    /// Written to `<dataset>_adjacency.txt` for get_connection_zones().
    std::vector<std::pair<int,int>> adjacency;

    /// One 2-point polyline per mesh face (box bisector visualization line).
    std::vector<session_cpp::Polyline> box_insertion_lines;
};

} // namespace wood_chevron
