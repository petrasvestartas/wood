#pragma once

#include "pch.h"
#include "wood_element_edge_straight.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a raw timber member: modelled as a round or polygonal beam along an axis, with an optional unprocessed mesh and point cloud attached from a stock database by a transformation matrix.
class EdgeRawElement : public EdgeStraightElement {
public:
    static constexpr std::string_view ELEMENT_TYPE =
        "EdgeRawElement"; // The element_type this raw timber member is written under.

    Mesh raw_mesh; // Unprocessed closed surface mesh in stock coordinates.
    PointCloud raw_pointcloud; // Unprocessed scanned point cloud in stock coordinates.
    Xform raw_xform =
        Xform::identity(); // Matrix mapping raw_mesh, raw_pointcloud and branch_axes from stock coordinates to world space.
    std::vector<Polyline>
        branch_axes; // Secondary skeleton axes in stock coordinates (such as the side branch of a tree fork).
    bool use_raw_mesh =
        true; // True when the member's solid is placed_raw_mesh() rather than its low-poly proxy section.
    bool show_pointcloud =
        false; // Whether the synced raw_pointcloud attribute under the element node starts visible.

    /// An empty raw timber member.
    EdgeRawElement();

    /// A raw timber member of round proxy section along `axis`, optionally carrying a stock `raw_mesh` and `raw_pointcloud` placed by `raw_xform`.
    EdgeRawElement(
        const Line& axis,
        double radius,
        const Mesh& raw_mesh = Mesh(),
        const PointCloud& raw_pointcloud = PointCloud(),
        const Xform& raw_xform = Xform::identity(),
        const std::string& name = "edge_raw"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A round-section proxy raw member along `axis`, before any stock mesh or point cloud is attached.
    static std::shared_ptr<EdgeRawElement> round(
        const Line& axis,
        double radius,
        double chord_tolerance = 0.2,
        const std::string& name = "edge_raw"
    );

    /// A raw member along `axis` with `data/raw/<stock_name>` (`.obj` mesh, `_cloud.pb` point cloud and `_axes.txt` skeleton) aligned onto it.
    static std::shared_ptr<EdgeRawElement> from_stock(
        const Line& axis,
        double radius,
        const std::string& stock_name,
        double roll_degrees = 0.0,
        bool center_along_axis = true,
        const std::string& name = "edge_raw"
    );

    /// The raw member an Element tagged "EdgeRawElement" describes, same guid.
    static std::shared_ptr<EdgeRawElement> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Raw stock attachment
    // ═══════════════════════════════════════════════════════════════════════════

    /// True when an unprocessed stock mesh is attached.
    bool has_raw_mesh() const;

    /// True when an unprocessed scanned point cloud is attached.
    bool has_raw_pointcloud() const;

    /// True when the member's solid uses the attached raw mesh rather than the low-poly proxy section.
    bool using_raw_mesh() const;

    /// The unprocessed mesh placed into world space by `raw_xform`.
    Mesh placed_raw_mesh() const;

    /// The unprocessed point cloud placed into world space by `raw_xform`.
    PointCloud placed_raw_pointcloud() const;

    /// The secondary branch skeleton axes placed into world space by `raw_xform`.
    std::vector<Polyline> placed_branch_axes() const;

    /// The matrix mapping `stock_plane` (origin at stock start, +z along stock axis) onto this member's `base_plane()`, rolled by `roll_degrees` and optionally centred for `stock_length` > 0.
    Xform align_stock(
        const Plane& stock_plane = Plane::xy_plane(),
        double stock_length = 0.0,
        double roll_degrees = 0.0
    ) const;

    /// Attaches an unprocessed mesh and point cloud with placement matrix `xform`, invalidating cached solids.
    void attach_raw(
        const Mesh& mesh,
        const PointCloud& pointcloud = PointCloud(),
        const Xform& xform = Xform::identity()
    );

    /// Loads `data/raw/<stock_name>` and aligns its mesh, point cloud and branch axes onto this member.
    void attach_stock(
        const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true
    );

    /// How far `placed_raw_mesh()` reaches radially past the nominal `radius(0)` around `chord()`, plus `margin`: what joinery cutters add so no bark sliver remains.
    double raw_overshoot(double margin = 20.0) const;

    /// How far `placed_raw_mesh()` reaches axially past either end of `chord()`, plus `margin`: what end cutters add to trim the full raw log end.
    double raw_end_extension(double margin = 20.0) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The uncut solid as a Mesh: `placed_raw_mesh()` when active, else the round/polygonal proxy beam.
    const Mesh& element_geometry_mesh() const override;

    /// The uncut solid as a BRep: `placed_raw_mesh()` as planar faces when active, else the proxy BRep.
    const BRep& element_geometry_brep() const override;

    /// The cut solid as a BRep, with plane cuts, joinery boolean cuts and exact cylindrical drill bores applied.
    const BRep& model_geometry_brep() const override;

    /// A copy moved by `xform`, keeping its stock mesh and point cloud and composing `xform * raw_xform`.
    std::shared_ptr<EdgeRawElement> transformed(const Xform& xform) const;

    /// Moves the axis, cuts, features and `raw_xform` by `xform`, dropping cached solids.
    void place(const Xform& xform) override;

protected:
    /// Writes the model mesh and geometry features into the session slot.
    void compute_geometry_mesh_impl() override;

    /// Writes the model BRep and geometry features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refreshes the axis, branch axes and section features while preserving session features.
    void compute_geometry_features();

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // JSON & Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The raw member's parameters, stock geometry and placement matrix as JSON.
    nlohmann::ordered_json element_data_jsondump() const;

    /// The raw member's state as `wood_proto.EdgeRawElement` bytes.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A polymorphic copy with a fresh guid.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<EdgeRawElement>(*this);
    }

    /// Registers the "EdgeRawElement" factory with the kernel.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "EdgeRawElement(name, segments, raw_faces, raw_points)".
    std::string str() const override;
};

}
