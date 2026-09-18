#pragma once

#include "pch.h"

namespace wood_session {

/// A solid for contact detection only: one face per closed loop, no plate convention.
class Block : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Solid"; // The element_type this block is written under.
    static constexpr std::string_view LEGACY_ELEMENT_TYPE = "BlockElement"; // The element_type wood wrote before, still accepted on read.

    /// An empty block: no solid.
    Block();

    /// A block from closed loops, one n-gon face per loop (Mesh::from_polylines); `name` is the type flag face_contacts() filters on.
    explicit Block(const std::vector<session_cpp::Polyline>& loops, const std::string& name = "block");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The block an Element describes, same guid: any element whose geometry is a mesh.
    static std::shared_ptr<Block> from_element(const session_cpp::Element& element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override { return std::string(ELEMENT_TYPE); }

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid, inflated on each side; empty when the block has no mesh.
    session_cpp::AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Block>(*this); }

    /// Registers the "Solid" factory (and the legacy "BlockElement" tag) with the kernel, so Session::pb_load rebuilds blocks.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Block(name, faces)".
    std::string str() const override;
};

} // namespace wood_session
