#pragma once

#include "pch.h"

#include "wood_element_plate.h"

namespace wood_session {

/// One element as detection sees it: face outlines, their planes, the name to filter on, and whether the plate face convention ([0] bottom, [1] top, [2..] sides) applies.
struct ContactElement {
    std::vector<session_cpp::Polyline> polylines; // The face outlines.
    std::vector<session_cpp::Plane> planes; // One plane per outline.
    std::string name; // The element name, what `names` filters on.
    bool plate_convention = false; // True when polylines follow the plate convention: [0] bottom, [1] top, [2..] sides.

    /// An empty view.
    ContactElement() = default;

    /// A plate's own outlines and planes, with the plate convention.
    explicit ContactElement(const Plate& plate);

    /// A plate through its own fields, any other element through the face outlines of its mesh.
    explicit ContactElement(session_cpp::Element& element);
};

} // namespace wood_session
