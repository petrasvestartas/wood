#pragma once
#include "src/templates/reciprocal/reciprocal_guide.h"
#include "src/templates/reciprocal/reciprocal_boundary.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalRotation
// ═══════════════════════════════════════════════════════════════════════════

/// A reciprocal frame by rotation: every interior mesh edge a beam turned about its midpoint, framed by straight boundary beams.
class ReciprocalRotation : public WoodSession {
public:
    const ReciprocalRotationGuide guide;

    /// The reciprocal frame of the guide as the session named name.
    explicit ReciprocalRotation(const ReciprocalRotationGuide& guide, const std::string& name = "reciprocal_rotation");

    /// Not copied, as a session is not.
    ReciprocalRotation(const ReciprocalRotation&) = delete;

    /// Not assigned, as it is not copied.
    ReciprocalRotation& operator=(const ReciprocalRotation&) = delete;

    /// The interior beams, in the order of the mesh's edges.
    const std::vector<ReciprocalBeam>& beams() const;

    /// The frame beams, in boundary-loop order.
    const std::vector<ReciprocalBeam>& frame_beams() const;

private:
    /// One interior edge turned into a beam axis.
    struct Axis {
        std::array<size_t, 2> vertices; // The edge's start and end vertex keys.
        Line line; // The scaled and rotated axis.
        Vector up; // The average normal of the two faces of the edge.
    };

    MeshFaces _faces;
    std::vector<Axis> _axes;
    BoundaryFrame _frame;
    std::vector<ReciprocalBeam> _frame_beams;
    std::vector<ReciprocalBeam> _beams;

    /// Every interior edge scaled about its midpoint and turned about its up by angle.
    std::vector<Axis> compute_axes() const;

    /// The faces one end of axis i at vertex can stop at.
    std::vector<CutFace> compute_end_cuts(
        size_t i, size_t vertex, const Point& axis_end, const std::vector<size_t>& at_vertex
    ) const;

    /// Every axis boxed and cut at both ends.
    std::vector<ReciprocalBeam> compute_beams() const;
};

}
