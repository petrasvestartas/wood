#pragma once

#include "../src/element.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"

#include <memory>
#include <string>
#include <vector>

namespace wood_session {

/// Merged cut outlines of a plate, per face: [0] the outer boundary, [1..] holes.
struct Features {
    std::vector<session_cpp::Polyline> top;
    std::vector<session_cpp::Polyline> bottom;
};

/// A timber plate: a bottom and a top outline, one side face per edge, and the joints cut into it.
class Plate : public session_cpp::Element {
public:
    static constexpr const char* ELEMENT_TYPE = "Plate";
    static constexpr const char* LEGACY_ELEMENT_TYPE = "WoodElement";

    Plate();
    /// `name` is the plate's type flag: face_contacts() filters on it.
    Plate(const session_cpp::Polyline& bottom, const session_cpp::Polyline& top, const std::string& name = "plate");

    std::vector<session_cpp::Polyline> polylines;    ///< [0] bottom, [1] top, [2..] sides
    std::vector<session_cpp::Plane>    planes;       ///< one per outline
    std::vector<int>                   joint_types;  ///< per face; empty = auto
    bool   reversed = false;
    double thickness = 0.0;
    Features features;

    using session_cpp::Element::insertion_vectors;
    std::vector<session_cpp::Vector>& insertion_vectors() { return _insertion_vectors; }

    /// The plate's solid onto the Element: the loft of its cut outlines when solved, of its two outlines otherwise, plus dimensions and face features.
    void compute_geometry();
    /// Outline extent in the plate's own frame, thickness in z.
    session_cpp::Vector nominal_dimensions() const;
    /// One ElementFeature per face with a joint type ("joint_type_<code>") or cut outlines ("cut"); [0] bottom, [1] top, [2..] sides.
    std::vector<session_cpp::ElementFeature> face_features() const;

    std::string element_type_name() const override { return ELEMENT_TYPE; }
    std::string element_data_dumps() const override;
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Plate>(*this); }
    /// The plate an Element written by pb_dumps() describes, same guid; an element without the outline payload comes back empty.
    static std::shared_ptr<Plate> from_element(const session_cpp::Element& element);
    /// Registers the "Plate" factory (and the legacy "WoodElement" tag) with the kernel, so Session::pb_load rebuilds plates.
    static void register_type();

    std::string str() const override;
    std::string repr() const override;

protected:
    std::vector<session_cpp::Polyline> compute_polylines() const override { return polylines; }
    std::vector<session_cpp::Plane>    compute_planes()    const override { return planes; }
};

} // namespace wood_session
