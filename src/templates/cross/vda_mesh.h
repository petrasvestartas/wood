#pragma once
#include "src/templates/cross/cross_guide.h"
#include "intersection.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cross {

// ═══════════════════════════════════════════════════════════════════════════
// CrossLap
// ═══════════════════════════════════════════════════════════════════════════

/// The cross slot where a connector passes through a face plate, with the two plates it joins in its target order.
struct CrossLap {
    std::shared_ptr<JointPlateElement>
        joint; // cross_lap_<face0>_<face1>_<k>_<face>_<layer>, merged into both outlines.
    std::shared_ptr<FacePlanarElement> connector; // connector_<face0>_<face1>_<k>, the joint's target 0.
    std::shared_ptr<FacePlanarElement> plate; // plate_<face>_<layer>, the joint's target 1.
};

/// A face's turn in the assembly: the unit direction its plate slides in along, in its own plane, onto the connectors of the faces before it.
struct FaceSlide {
    size_t face;
    Vector in; // Zero for the first face.
};

// ═══════════════════════════════════════════════════════════════════════════
// VdaMesh
// ═══════════════════════════════════════════════════════════════════════════

/// The plates and connectors of a mesh.
class VdaMesh : public WoodSession {
public:
    const VdaMeshGuide guide;
    const Mesh mesh; // The guide's mesh, welded at 0.01.

    /// The plates and connectors of the guide's mesh as the session named name.
    explicit VdaMesh(const VdaMeshGuide& guide, const std::string& name = "vda_mesh");

    /// Not copied, as a session is not.
    VdaMesh(const VdaMesh&) = delete;

    /// Not assigned, as it is not copied.
    VdaMesh& operator=(const VdaMesh&) = delete;


    /// The faces in an order that goes together: from a foot on the ground ring by ring over the mesh, breadth first, every ring
    /// peeled by the face its remaining neighbours leave the widest way out of in its own plane, and reversed.
    static std::vector<FaceSlide> assembly_order(const Mesh& mesh);

    /// The insertion lines that let every plate slide onto all its connectors at once: per interior edge the slide of its later face.
    static std::vector<Line> assembly_lines(const Mesh& mesh);

    /// The faces either side of every edge, by index in Mesh::faces order, edges in Mesh::edges order.
    const std::vector<std::vector<size_t>>& edge_faces() const;

    /// The bisector plane of every face corner, corner j between edges j and j + 1.
    const std::vector<std::vector<Plane>>& bisector_planes() const;

    /// The bottom and top outline of every face per layer.
    const std::vector<std::vector<std::array<Polyline, 2>>>& face_outlines() const;

    /// The top plane of every face per layer.
    const std::vector<std::vector<Plane>>& face_outline_planes() const;

    /// The frames of every edge's connectors, none on a naked edge.
    const std::vector<std::vector<Plane>>& connector_frames() const;

    /// The bottom and top rectangle of every edge's connectors.
    const std::vector<std::vector<std::array<Polyline, 2>>>& connector_outlines() const;

private:
    std::vector<std::vector<size_t>> _edge_faces;
    std::vector<Plane> _face_planes;
    std::vector<std::vector<Plane>> _edge_planes;
    std::vector<std::vector<Plane>> _bisector_planes;
    std::vector<std::vector<std::array<Polyline, 2>>> _face_outlines;
    std::vector<std::vector<Plane>> _face_outline_planes;
    std::vector<std::vector<Plane>> _connector_frames;
    std::vector<std::vector<std::array<Polyline, 2>>> _connector_outlines;

    /// The faces either side of every edge.
    std::vector<std::vector<size_t>> compute_edge_faces() const;

    /// The plane of every face, at its centroid along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// Per face edge, the plane through the edge leaning on the average normal of the faces that share it.
    std::vector<std::vector<Plane>> compute_edge_planes() const;

    /// Per face corner, the plane through the corner vertex halving the angle between the edge planes that meet there.
    std::vector<std::vector<Plane>> compute_bisector_planes() const;

    /// Per face and layer, the bottom and top outline where the edge planes cut the offset face plane.
    std::vector<std::vector<std::array<Polyline, 2>>> compute_face_outlines() const;

    /// Per face and layer, the top plane of its plate.
    std::vector<std::vector<Plane>> compute_face_outline_planes() const;

    /// Per interior edge, a frame at every division: z along the edge, y the average normal, x across into the first face.
    std::vector<std::vector<Plane>> compute_connector_frames() const;

    /// Per connector frame, the bottom and top rectangle connector_thickness apart along the edge.
    std::vector<std::vector<std::array<Polyline, 2>>> compute_connector_outlines() const;

    /// Per connector and plate of its two faces.
    std::vector<CrossLap> compute_cross_laps() const;

    /// The closed outline where consecutive edge planes cross the base plane, nearly parallel neighbours merged.
    static Polyline outline(
        const Plane& base,
        const std::vector<Plane>& edge_planes,
        const std::vector<Plane>& bisector_planes
    );
};

}
