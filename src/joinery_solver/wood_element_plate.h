#pragma once

#include "wood_pch.h"

namespace wood_session {

/// Merged cut outlines of a plate, per face: [0] the outer boundary, [1..] holes.
struct Features {
    /// Top face: the outer outline first, then one outline per hole.
    std::vector<session_cpp::Polyline> top;

    /// Bottom face: the outer outline first, then one outline per hole.
    std::vector<session_cpp::Polyline> bottom;
};

/// A timber plate: a bottom and a top outline, one side face per edge, and the joints cut into it.
class Plate : public session_cpp::Element {
public:
    /// The element_type this plate is written under.
    static constexpr const char* ELEMENT_TYPE = "Plate";

    /// The element_type wood wrote before, still accepted on read.
    static constexpr const char* LEGACY_ELEMENT_TYPE = "WoodElement";

    /// An empty plate: no outlines, no planes, nothing to loft.
    Plate();

    /// A plate from its bottom and top outline; `name` is the type flag face_contacts() filters on.
    Plate(const session_cpp::Polyline& bottom, const session_cpp::Polyline& top, const std::string& name = "plate");

    /// Face outlines: [0] bottom, [1] top, [2..] one closed quad per side.
    std::vector<session_cpp::Polyline> polylines;

    /// One plane per outline, normals pointing out of the plate.
    std::vector<session_cpp::Plane> planes;

    /// Joint type per face, indexed like polylines; empty lets the solver decide.
    std::vector<int> joint_types;

    /// True when the constructor reversed both outlines to make the bottom normal point away from the top.
    bool reversed = false;

    /// Distance between the bottom and the top plane.
    double thickness = 0.0;

    /// Merged cut outlines after compute_joints; empty before.
    Features features;

    /// The insertion vectors the Element holds, read-only.
    using session_cpp::Element::insertion_vectors;

    /// The insertion vectors the Element holds, one per face, writable by the solver.
    std::vector<session_cpp::Vector>& insertion_vectors() { return _insertion_vectors; }

    /// Lofts the plate onto the Element (its cut outlines when solved, its two outlines otherwise) and sets dimensions and face features.
    void compute_geometry();

    /// Outline extent in the plate's own frame, thickness in z.
    session_cpp::Vector nominal_dimensions() const;

    /// One ElementFeature per face with a joint type ("joint_type_<code>") or cut outlines ("cut"); [0] bottom, [1] top, [2..] sides.
    std::vector<session_cpp::ElementFeature> face_features() const;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override { return ELEMENT_TYPE; }

    /// The outline payload as JSON: bottom, reversed, top, type.
    std::string element_data_dumps() const override;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Plate>(*this); }

    /// The plate an Element written by pb_dumps() describes, same guid; an element without the outline payload comes back empty.
    static std::shared_ptr<Plate> from_element(const session_cpp::Element& element);

    /// Registers the "Plate" factory (and the legacy "WoodElement" tag) with the kernel, so Session::pb_load rebuilds plates.
    static void register_type();

    /// "Plate(name, polylines, thickness)".
    std::string str() const override;

    /// "Plate(name, polylines, planes, reversed, thickness, features)".
    std::string repr() const override;

protected:
    /// The plate's own outlines, so Element::polylines() agrees with the solver's view.
    std::vector<session_cpp::Polyline> compute_polylines() const override { return polylines; }

    /// The plate's own planes, so Element::planes() agrees with the solver's view.
    std::vector<session_cpp::Plane> compute_planes() const override { return planes; }
};

} // namespace wood_session
