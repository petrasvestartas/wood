#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a single-curved or double-curved glulam beam (after Tom Svilans' GluLamb).
class BeamCurved : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE =
        "BeamCurved"; // The element_type this curved beam is written under.
    NurbsCurve centreline; // Curved spine in world space.
    double width = 0.0; // Total cross-section width along local frame X.
    double height = 0.0; // Total cross-section height along local frame Y.
    std::vector<Vector>
        directions; // Cross-section up (Y) guide vectors: empty (planar normal or RMF), 1 (vector driver), or >=2 (interpolated twist over RMF).
    std::vector<double>
        direction_parameters; // Optional curve parameters for directions when directions.size() >= 2.
    int num_width = 1; // Number of lamella stacks across width.
    int num_height = 4; // Number of lamella layers across height.
    int samples = 32; // Number of cross-section subdivisions along the curve.
    double extension = 0.0; // Straight tangent extension past each end of centreline.
    std::vector<Plane>
        cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

protected:
    /// The solid trimmed by its cut planes as a Mesh, before the solid cuts.
    Mesh trimmed_mesh() const override;

    /// The solid trimmed by its cut planes as a BRep, before the solid cuts.
    BRep trimmed_brep() const override;

public:
    /// The start frame of the curved beam: origin at start, x along width, y along height, z along start tangent.
    std::optional<Plane> base_plane() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// An empty curved beam.
    BeamCurved();

    /// A curved beam of rectangular section (width x height) swept along centreline; planar curves use in-plane bending orientation, 3D curves use RMF.
    BeamCurved(
        const NurbsCurve& centreline,
        double width,
        double height,
        const std::string& name = "beam_curved"
    );

    /// A curved glulam beam with a single reference up vector and lamella stack subdivisions (num_width x num_height).
    BeamCurved(
        const NurbsCurve& centreline,
        double width,
        double height,
        const Vector& up,
        int num_width = 1,
        int num_height = 4,
        int samples = 32,
        const std::string& name = "beam_curved"
    );

    /// A curved glulam beam with one or more guide directions interpolated along the curve via RMF twist.
    BeamCurved(
        const NurbsCurve& centreline,
        double width,
        double height,
        const std::vector<Vector>& directions,
        int num_width = 1,
        int num_height = 4,
        int samples = 32,
        const std::string& name = "beam_curved"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Single-curved glulam beam along a planar curve with num_height lamella layers bent in the plane of curvature.
    static std::shared_ptr<BeamCurved> single_curved(
        const NurbsCurve& centreline,
        double width,
        double height,
        int num_height = 6,
        int samples = 32,
        const std::string& name = "beam_curved"
    );

    /// Double-curved glulam beam along a 3D space curve with optional guide vectors and parameters interpolated over RMF.
    static std::shared_ptr<BeamCurved> double_curved(
        const NurbsCurve& centreline,
        double width,
        double height,
        const std::vector<Vector>& directions = {},
        const std::vector<double>& parameters = {},
        int num_width = 2,
        int num_height = 6,
        int samples = 32,
        const std::string& name = "beam_curved"
    );

    /// Double-curved glulam beam oriented by the surface normals of a reference NurbsSurface along centreline.
    static std::shared_ptr<BeamCurved> from_surface(
        const NurbsCurve& centreline,
        const NurbsSurface& surface,
        double width,
        double height,
        int num_width = 2,
        int num_height = 6,
        int samples = 32,
        const std::string& name = "beam_curved"
    );

    /// The curved beam an Element tagged "BeamCurved" describes, same guid.
    static std::shared_ptr<BeamCurved> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Glulam queries & lamellae
    // ═══════════════════════════════════════════════════════════════════════════

    /// True when the centreline is planar and the cross-section does not twist out of the curve plane.
    bool is_single_curved(double tolerance = 1e-3) const;

    /// True when the centreline is non-planar or the cross-section twists along the curve.
    bool is_double_curved(double tolerance = 1e-3) const;

    /// Width of one lamella column (width / num_width).
    double lamella_width() const;

    /// Thickness of one lamella layer (height / num_height).
    double lamella_height() const;

    /// Cross-section Plane at curve parameter t (origin on centreline, x along width, y along height, z along tangent).
    Plane get_plane(double t) const;

    /// Cross-section Planes at the (samples + 1) stations along the curve, including end extensions.
    std::vector<Plane> frames() const;

    /// Closed rectangular cross-section Polylines at the (samples + 1) stations along the curve.
    std::vector<Polyline> sections() const;

    /// The four interpolated cubic NURBS corner curves running the length of the beam.
    std::array<NurbsCurve, 4> edge_curves() const;

    /// Curve offset by (x, y) in the local cross-section frames along the beam.
    NurbsCurve offset_curve(double x, double y) const;

    /// Centreline NURBS curve of lamella (ix, iy) in [0..num_width-1] x [0..num_height-1].
    NurbsCurve lamella_curve(int ix, int iy) const;

    /// Centreline NURBS curves of all (num_width * num_height) lamellae.
    std::vector<NurbsCurve> lamella_curves() const;

    /// Individual closed Mesh solids for each of the (num_width * num_height) lamellae.
    std::vector<Mesh> lamella_meshes() const;

    /// Individual closed 6-patch NURBS BRep solids for each of the (num_width * num_height) lamellae.
    std::vector<BRep> lamella_breps() const;

    /// Maximum curvature magnitude along the centreline, writing the maximum width-axis and height-axis components to max_kw and max_kh.
    double max_curvature(double& max_kw, double& max_kh) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The uncut swept solid as a closed Mesh.
    const Mesh& element_geometry_mesh() const override;

    /// The uncut solid as a closed 6-patch NURBS BRep bounded by edge_curves().
    const BRep& element_geometry_brep() const override;

    /// One plane per face of the model solid with a Newell normal.
    std::vector<Plane> compute_planes() const override;

    /// A copy moved by xform from the parameters alone, guid and name kept; nullptr for a mirror.
    std::shared_ptr<BeamCurved> transformed(const Xform& xform) const;

    /// Moves the curve, directions, cuts and features by xform, dropping cached solids.
    void place(const Xform& xform) override;

protected:
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
    void compute_geometry_features();

public:
    /// The kernel's cached box of the solid.
    using Element::aabb;

    /// The box of the solid inflated on each side.
    AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON & Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The curved beam parameters and features as JSON.
    nlohmann::ordered_json element_data_jsondump() const;

    /// The curved beam parameters and features serialized for Element::element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<BeamCurved>(*this);
    }

    /// Registers the "BeamCurved" factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "BeamCurved(name, width, height, lamellae, samples)".
    std::string str() const override;
};

}
