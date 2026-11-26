#pragma once
#include "src/templates/reciprocal/reciprocal_guide.h"
#include "src/templates/reciprocal/reciprocal_boundary.h"

#include <optional>

using namespace session_cpp;
using namespace wood_session;

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalMove
// ═══════════════════════════════════════════════════════════════════════════

/// A reciprocal frame by translation.
class ReciprocalMove : public WoodSession {
public:
    const ReciprocalMoveGuide guide;

    /// The reciprocal frame of the guide as the session named name.
    explicit ReciprocalMove(const ReciprocalMoveGuide& guide, const std::string& name = "reciprocal_move");

    /// Not copied, as a session is not.
    ReciprocalMove(const ReciprocalMove&) = delete;

    /// Not assigned, as it is not copied.
    ReciprocalMove& operator=(const ReciprocalMove&) = delete;

    /// The interior beams, face by face.
    const std::vector<ReciprocalBeam>& beams() const;

    /// The frame beams, in boundary-loop order.
    const std::vector<ReciprocalBeam>& frame_beams() const;

private:
    /// One half-edge of a face as a beam axis.
    struct Axis {
        Line line; // The moved and trimmed axis.
        Vector dir; // The unit direction of the half-edge.
        Vector up; // The beam's up, from edge_up.
        bool beam; // Whether it becomes a beam: the colour taken, and an interior edge.
    };

    MeshFaces _faces;
    std::vector<std::vector<Axis>> _axes;
    BoundaryFrame _frame;
    std::vector<ReciprocalBeam> _frame_beams;
    std::vector<ReciprocalBeam> _beams;

    /// The faces of the mesh in vertex index order, with their Newell normals turned up.
    MeshFaces compute_faces() const;

    /// The half-edge across the edge from local edge j of face i, none on the boundary.
    std::optional<std::pair<int, int>> opposite(int i, int j) const;

    /// Every half-edge's axis, the colour-0 ones moved sideways by shift and trimmed to their neighbours.
    std::vector<std::vector<Axis>> compute_axes() const;

    /// The side face of the beam on half-edge (ci, cj) that the beam on (i, j) stops at.
    Plane compute_cut_plane(int i, int j, const std::pair<int, int>& crossing, const Point& endpoint) const;

    /// Every beam axis boxed and cut at both ends.
    std::vector<ReciprocalBeam> compute_beams() const;
};

}
