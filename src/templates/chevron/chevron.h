#pragma once
#include "src/templates/chevron/chevron_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_chevron {

/// The 23 Annen NURBS surfaces of annen_surfaces.json, an empty list when the file is missing.
std::vector<NurbsSurface> annen_surfaces(const std::string& json_path);

/// The faces of a chevron mesh.
struct ChevronFaces {
    std::vector<std::vector<size_t>> vertices; // Per face its vertex keys.
    std::map<std::pair<size_t, size_t>, std::vector<int>>
        edge_faces; // Per sorted vertex pair the faces on it.
};

/// The faces joined in strips along v.
struct ChevronStrips {
    std::vector<std::array<int, 2>> chevron_edges; // Per face its two chevron edges.
    std::vector<bool> flips; // Per face whether its strip is even.
    std::vector<int> order; // The faces strip by strip.
};

}

// ═══════════════════════════════════════════════════════════════════════════
// Chevron
// ═══════════════════════════════════════════════════════════════════════════

/// A chevron shell.
class Chevron : public WoodSession {
public:
    const ChevronGuide guide;

    /// The chevron shell of the guide as the session named name.
    explicit Chevron(const ChevronGuide& guide, const std::string& name = "chevron");

    /// Not copied, as a session is not.
    Chevron(const Chevron&) = delete;

    /// Not assigned, as it is not copied.
    Chevron& operator=(const Chevron&) = delete;

    /// The chevron quad mesh.
    const Mesh& mesh() const;

    /// Per plate six insertion vectors as 18 numbers: the top and bottom plates' four sides along the box diagonal, Christopher's insertion vector, the walls' mitre faces along the box's up.
    const std::vector<std::array<double, 18>>& insertion_vectors() const;

    /// Per plate six joint types, one per face: 22 the Annen tenons on the plates' sides, 17 the Annen fingers on the axis wall's mitre face, 10 on the other wall's, 0 none.
    const std::vector<std::array<int, 6>>& joints_per_face() const;

    /// Per plate six division counts, one per face: Christopher's round(0.8 mesh edge / tab_length) on the plates' sides, finger_divisions on the walls' mitre faces, 0 none.
    const std::vector<std::array<int, 6>>& divisions_per_face() const;

    /// Rows {s0, s1, e20, e31}: plate pairs s0 and e20 both join s1, so its joint is trimmed for both.
    const std::vector<std::array<int, 4>>& three_valence() const;

    /// The plate pairs that share a joint.
    const std::vector<std::pair<int, int>>& adjacency() const;

private:
    Mesh _mesh;
    wood_chevron::ChevronFaces _faces;
    wood_chevron::ChevronStrips _strips;
    std::vector<Plane> _face_planes;
    std::vector<std::array<Plane, 4>> _edge_planes;
    std::vector<std::array<std::optional<Plane>, 4>> _bisectors;
    std::vector<std::array<double, 18>> _insertion_vectors;
    std::vector<std::array<int, 6>> _joints_per_face;
    std::vector<std::array<int, 6>> _divisions_per_face;
    std::vector<std::array<int, 4>> _three_valence;
    std::vector<std::pair<int, int>> _adjacency;

    /// Strips across u, rows from both v ends growing by scale until less than stop_threshold remains to the middle, mirrored there, each row a chevron.
    Mesh compute_mesh() const;

    /// Every face's reversed vertices and the faces on every edge.
    wood_chevron::ChevronFaces compute_faces() const;

    /// The faces grown into strips along v, alternate strips with the other two chevron edges.
    wood_chevron::ChevronStrips compute_strips() const;

    /// A plane through every face centre along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// A plane on every face edge, through its middle, along the edge and the faces' mean normal.
    std::vector<std::array<Plane, 4>> compute_edge_planes() const;

    /// The edge planes with every inner chevron edge offset or rotated, copied flipped to its neighbour, the boundary ones snapped.
    std::vector<std::array<Plane, 4>>
    compute_turned_edge_planes(std::vector<std::array<Plane, 4>> edge_planes) const;

    /// The dihedral plane at every face corner, between the edge planes meeting there.
    std::vector<std::array<std::optional<Plane>, 4>> compute_bisectors() const;

    /// Four plates per face in strip order: top and bottom inset in the box, a side on each chevron edge.
    std::vector<std::shared_ptr<FacePlanarElement>> compute_plates() const;

    /// The insertion vectors, joint types, three-valence groups and adjacency of the plate pairs, and an insertion line per face.
    std::vector<Line> compute_joinery();

    /// The three points of a chevron row at u: its sides at v_side, its middle at v_middle.
    std::array<Point, 3>
    chevron_row(const NurbsSurface& transposed, double u, double v_side, double v_middle) const;

    /// The faces on the edge between vertices a and b.
    const std::vector<int>& edge_faces(size_t a, size_t b) const;

    /// The face's normal from its reversed vertices: the cross product of the diagonals of a quad.
    Vector face_normal(int face) const;

    /// The face's two chevron edges in the order its side plates are made.
    std::array<int, 2> sorted_chevron_edges(int face) const;

    /// The plane through the line where p0 and p1 meet, halfway between them; none when they are parallel or share an origin.
    static std::optional<Plane> dihedral_plane(const Plane& p0, const Plane& p1);

    /// The plane with its normal snapped to the world axis given by axis, 1 the dominant one, and moved to the boundary plane on that axis when one is given.
    static Plane
    snapped_to_axis(const Plane& plane, int axis, const std::array<double, 3>& boundary);

    /// A face's edge planes as the box's sides, the chevron ones moved out by a plate, where the walls stand.
    std::vector<Plane> box_sides(int face) const;
};
