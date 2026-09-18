#pragma once

#include "pch.h"

namespace wood_session {

/// A column: a solid that knows its own axis and the section it is cut from.
class Column : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Column"; // The element_type this column is written under.
    session_cpp::Line axis; // Centreline, base to head, in world space.
    session_cpp::Polyline section; // Closed cross-section about the axis base; empty when unknown.

    /// An empty column: no solid, a zero-length axis, no section.
    Column();

    /// A column from its solid, its axis and its section; `name` is the type flag face_contacts() filters on.
    Column(
        const session_cpp::Mesh& solid,
        const session_cpp::Line& axis,
        const session_cpp::Polyline& section,
        const std::string& name = "column"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The column an Element tagged "Column" describes, same guid; a missing payload leaves axis and section default.
    static std::shared_ptr<Column> from_element(const session_cpp::Element& element);

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis and section as JSON: axis, section, type.
    std::string element_data_dumps() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override { return std::string(ELEMENT_TYPE); }

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid when there is one, else of the axis and the section, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Column>(*this); }

    /// Registers the "Column" factory with the kernel, so Session::pb_load rebuilds columns.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Column(name, axis_length, section_pts)".
    std::string str() const override;
};

} // namespace wood_session
