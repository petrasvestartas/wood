#pragma once

#include "pch.h"

namespace wood_session {

/// A block: a closed solid lofted between a bottom loop and a top loop, for contact detection; no plate convention.
class Block : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Solid"; // The element_type this block is written under.
    static constexpr std::string_view LEGACY_ELEMENT_TYPE = "BlockElement"; // The element_type wood wrote before, still accepted on read.
    std::vector<session_cpp::Polyline> loops; // Bottom loop, top loop, then their holes paired in order; empty when the solid came as a mesh.

private:
    bool _geometry_synced = false; // True while the Element slot holds the loft of the current loops.

public:
    /// An empty block: no solid.
    Block();

    /// A block lofted between closed loops: [0] bottom, [1] top, [2..] holes of the bottom paired with holes of the top; `name` is the type flag face_contacts() filters on.
    explicit Block(const std::vector<session_cpp::Polyline>& loops, const std::string& name = "block");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The block an Element describes, same guid: any element whose geometry is a mesh; a "Solid" payload gives the loops back.
    static std::shared_ptr<Block> from_element(const session_cpp::Element& element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Marks the Element slot stale; call after assigning the loops by hand.
    void invalidate_geometry();

    /// True once compute_geometry() wrote the loft of the current loops onto the Element; false after any invalidation.
    bool geometry_synced() const { return _geometry_synced; }

    /// Writes the capped loft of the loops onto the Element (a solid given as a mesh stays) with the centroid feature, keeping the joint features the session put there; WoodSession::pb_dump calls it for every stale block.
    void compute_geometry();

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid, inflated on each side; empty when the block has no mesh.
    session_cpp::AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The loops as JSON: the protobuf message printed.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The loops as wood_proto.Block bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override { return std::string(ELEMENT_TYPE); }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Block>(*this); }

    /// Registers the "Solid" factory (and the legacy "BlockElement" tag) with the kernel, so Session::pb_load rebuilds blocks.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Block(name, loops, faces)".
    std::string str() const override;
};

} // namespace wood_session
