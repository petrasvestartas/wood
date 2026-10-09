#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a column base, parametric after the Sherpa Power Base L 140 C: a drilled base plate on the slab, an adjustment nut, a threaded rod, a coupling nut and the head plate the column end is let onto; the dimensions default to the manufacturer's table.
class Support : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Support"; // The element_type this support is written under.
    Plane plane = Plane::xy_plane(); // Base plate underside centre, z up the column, x along a base plate side.
    double height = 150.0; // Base plate underside to head plate top, 150 to 200 by the adjustment.
    double head_plate_diameter = 106.0; // Head plate disc.
    double head_plate_thickness = 12.0; // Head plate disc.
    double head_plate_recess = 12.0; // Depth the head plate is let into the column end, the datasheet's own thickness.
    double base_plate_size = 140.0; // Square base plate side.
    double base_plate_thickness = 12.0; // Square base plate.
    double base_plate_hole_diameter = 15.0; // The four anchor drillings.
    double base_plate_hole_spacing = 104.0; // Centre distance of the drillings along each side.
    double adjustment_nut_across_flats = 36.0; // Hexagon on the base plate.
    double adjustment_nut_top = 64.0; // Top of that hexagon above the plate underside.
    double rod_diameter = 30.0; // Threaded rod from the adjustment nut to the coupling nut.
    double coupling_nut_across_flats = 55.0; // Hexagon under the head plate.
    double coupling_nut_height = 30.0; // Hexagon under the head plate.
    int screw_count = 3; // Screws from the head plate up into the column end.
    double screw_diameter = 8.0; // Column screws.
    double screw_length = 180.0; // Column screws.
    double screw_angle = 25.0; // Degrees between a pair of opposed screws, half of it each off the axis.
    double screw_circle_diameter = 50.0; // Circle the screws start on, on the head plate top.
    double anchor_diameter = 12.0; // Anchors through the drillings into the slab.
    double anchor_embedment = 100.0; // Anchor depth below the plate underside.
    double chord_tolerance = 0.05; // Largest deviation of a round part's facets.

    /// A support at the world origin standing up z, with the manufacturer's dimensions.
    Support();

    /// A support on plane: its origin the base plate underside centre, z up the column; `name` is the type flag face_contacts() filters on.
    explicit Support(const Plane& plane, const std::string& name = "support");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The support an Element tagged "Support" describes, same guid; a missing payload leaves the defaults.
    static std::shared_ptr<Support> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The point on the axis at a height above the base plate underside.
    Point at(double level) const;

    /// Where the column end stands: the head plate top less the recess, on the axis.
    Point column_foot() const;

    /// The axis of the column standing on it: from column_foot() straight up to the level top_z.
    Line column_axis(double top_z) const;

    /// The column screws from the head plate top, spread outwards by half the screw angle.
    std::vector<Line> screws() const;

    /// The anchors from the base plate top down into the slab, one per drilling.
    std::vector<Line> anchors() const;

    /// The base plate with its drillings, the two nuts, the rod and the head plate as one mesh of five closed solids.
    const Mesh& element_geometry_mesh() const override;

    /// The same five solids as a BRep, the rod and the head plate exact cylinders.
    const BRep& element_geometry_brep() const override;

    /// The support is not cut: its element mesh.
    const Mesh& model_geometry_mesh() const override;

    /// The support is not cut: its element BRep.
    const BRep& model_geometry_brep() const override;

    /// One plane per face of the solid with a Newell normal.
    std::vector<Plane> compute_planes() const override;


    /// A copy moved by xform, guid and name kept; nullptr for a mirror.
    std::shared_ptr<Support> transformed(const Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the plane, and drops the cached solids.
    void place(const Xform& xform) override;

protected:
    /// Writes the mesh and the element features into the session slot.
    void compute_geometry_mesh_impl() override;

    /// Writes the BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

public:
    /// Its plane: the base plate underside centre, z up the column.
    std::optional<Plane> base_plane() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The plane and dimensions as JSON: the protobuf message printed.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The plane and dimensions as wood_proto.Support bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<Support>(*this);
    }

    /// Registers the "Support" factory with the kernel, so Session::pb_load rebuilds supports.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Support(name, height, head_plate_diameter)".
    std::string str() const override;
};

} // namespace wood_session
