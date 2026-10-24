#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a block.
class Block : public WoodElement {
public:
    std::optional<Mesh> source_mesh;
    static constexpr std::string_view ELEMENT_TYPE =
        "Solid"; // The element_type this block is written under.
    static constexpr std::string_view LEGACY_ELEMENT_TYPE =
        "BlockElement"; // The element_type wood wrote before, still accepted on read.
    std::vector<Polyline>
        loops; // Bottom loop, top loop, then their holes paired in order; empty when the solid came as a mesh.
    std::vector<Plane>
        cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

protected:
    /// The solid trimmed by its cut planes as a Mesh, before the solid cuts.
    Mesh trimmed_mesh() const override;

    /// The solid trimmed by its cut planes as a BRep, before the solid cuts.
    BRep trimmed_brep() const override;

public:
    /// The bottom loop's plane.
    std::optional<Plane> base_plane() const override;

    /// An empty block: no solid.
    Block();
    explicit Block(const Mesh& mesh, const std::string& name = "block");

    /// A block lofted between closed loops.
    explicit Block(const std::vector<Polyline>& loops, const std::string& name = "block");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The block an Element describes, same guid.
    static std::shared_ptr<Block> from_element(Element element);

    /// An upright box: its bottom `x_size` by `y_size` from `corner` along x and y, `height` up z.
    static std::shared_ptr<Block>
    box(const Point& corner,
        double x_size,
        double y_size,
        double height,
        const std::string& name = "block");

    /// Two square boxes stacked: "lower" `lower_size` wide from the origin, "upper" `upper_size` wide centred on its top, each `height` high.
    static std::array<std::shared_ptr<Block>, 2>
    pair_stacked(double lower_size = 400.0, double upper_size = 300.0, double height = 200.0);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const BRep& element_geometry_brep() const override;

    /// One plane per face of the model solid with a Newell normal, so a concave cap (a W.
    std::vector<Plane> compute_planes() const override;

    /// A copy moved by xform from the parameters alone, guid and name kept.
    std::shared_ptr<Block> transformed(const Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the loops and the cuts, and drops the cached solids.
    void place(const Xform& xform) override;

protected:
    /// Writes the model geometry.
    void compute_geometry_mesh_impl() override;

    /// Write the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refresh the dimensions and geometry features while preserving session features.
    void compute_geometry_features();

public:
    /// The kernel's cached box of the solid.
    using Element::aabb;

    /// The box of the solid, inflated on each side; empty when the block has no mesh.
    AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The loops and cuts as JSON: the protobuf message printed.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The loops and cuts as wood_proto.Block bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<Block>(*this);
    }

    /// Registers the "Solid" factory (and the legacy "BlockElement" tag) with the kernel, so Session::pb_load rebuilds blocks.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Block(name, loops, faces)".
    std::string str() const override;
};

}
