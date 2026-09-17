#pragma once

#include <array>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../src/element.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/plane.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/vector.h"
#include "wood_element.h"

namespace wood_session {

using session_cpp::Element;
using session_cpp::ElementFeature;
using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Plane;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Vector;

/// A column: a solid that knows its own axis.
///
/// For CONTACT detection a column is exactly a BlockElement - `polylines` / `planes` are
/// the solid's faces, and it has no top/bottom convention, so every contact it takes part
/// in is ContactType::unknown. What it adds is `axis` and `section`, which is what a
/// plate-to-column joint needs and a bare mesh cannot supply: the centreline gives the
/// notch direction, the section the stock it is cut from.
///
/// Nothing in wood consumes `axis` / `section` yet. They are carried because the producer
/// has them and a mesh cannot be reverse-engineered back into them.
struct WoodColumn {
    WoodColumn();

    static constexpr const char* ELEMENT_TYPE = "Column";

    /// The kernel half - identity (guid, name) and the solid.
    /// The kernel half - SHARED, not owned by value. This is the same object a Session
    /// holds, so adding it to one copies nothing and its guid is the guid on the wire.
    std::shared_ptr<wood_session::TaggedElement> element;

    /// Centreline, base to head, in world space.
    session_cpp::Line axis;
    /// Closed cross-section outline about the axis base. Empty when the producer had none.
    session_cpp::Polyline section;

    /// The solid's face outlines and their planes; refreshed by sync_faces().
    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    session_cpp::Mesh mesh() const;
    void sync_faces();
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// An Element tagged "Column": the mesh becomes the faces, `element_data`
    /// {"axis","section"} the axis and section. A missing payload leaves those default and
    /// still yields a usable solid.
    static WoodColumn from_element(const session_cpp::Element& e);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const WoodColumn& e);
};

} // namespace wood_session
