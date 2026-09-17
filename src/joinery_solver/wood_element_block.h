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

/// An element for CONTACT DETECTION only, and a SOLID is all it is: the mesh in
/// `element`, one n-gon face per closed loop. `polylines` / `planes` are that solid's
/// face outlines (Mesh::face_outlines(), via Element::polylines()) and one plane each,
/// cached because the O(faces²) scan reads them per candidate pair.
///
/// WoodElement carries the plate convention the joint classifier depends on -
/// polylines[0] is the top face, [1] the bottom, [2..] the sides, in that
/// order, plus thickness, insertion vectors and merged features. That
/// convention is exactly what loose geometry does NOT have: a list of closed
/// loops off a brep says nothing about which loop is which, and neither does
/// mesh face order.
///
/// BlockElement drops all of it. It is enough for adjacency_search,
/// faces_coplanar and face_overlap_area - which only ever read `polylines` and
/// `planes` - and deliberately not enough for face_to_face_wood, which needs
/// the ordering to tell a side joint from a top joint.
struct BlockElement {
    BlockElement();

    /// Builds the solid from `loops` - one face per loop, vertices unwelded - and reads
    /// the face views back off it. Loops with fewer than 3 points are dropped.
    /// `name` is the block's type flag - "column", "inner_ribs" - and face_contacts()
    /// filters on it.
    explicit BlockElement(const std::vector<session_cpp::Polyline>& loops,
                          const std::string& name = "block");

    static constexpr const char* ELEMENT_TYPE = "Solid";
    static constexpr const char* LEGACY_ELEMENT_TYPE = "BlockElement";

    /// The kernel half, and the block itself: its geometry is the solid, so a block needs
    /// no payload beyond the mesh to come back whole. Identity (guid, name) lives here.
    /// The kernel half - SHARED, not owned by value. This is the same object a Session
    /// holds, so adding it to one copies nothing and its guid is the guid on the wire.
    std::shared_ptr<wood_session::TaggedElement> element;

    /// The solid's face outlines and their planes; refreshed by sync_faces().
    std::vector<session_cpp::Polyline> polylines;
    std::vector<session_cpp::Plane>    planes;

    /// The solid. Empty when the element carries no mesh.
    session_cpp::Mesh mesh() const;

    /// Refresh `polylines` / `planes` from the solid, after replacing the geometry.
    void sync_faces();
    /// Nothing to do - the solid in `element` IS the block.
    void sync_element() const;
    std::shared_ptr<session_cpp::Element> to_element() const;
    /// Any Element whose geometry is a Mesh: its face outlines become the block's faces.
    /// An element with no mesh degrades to an empty block.
    static BlockElement from_element(const session_cpp::Element& e);

    nlohmann::ordered_json jsondump() const;
    static BlockElement jsonload(const nlohmann::json& data);
    std::string file_json_dumps() const;
    static BlockElement file_json_loads(const std::string& json_string);
    void file_json_dump(const std::string& filename) const;
    static BlockElement file_json_load(const std::string& filename);

    std::string pb_dumps() const;
    static BlockElement pb_loads(const std::string& data);
    void pb_dump(const std::string& filename) const;
    static BlockElement pb_load(const std::string& filename);

    std::string str() const;
    friend std::ostream& operator<<(std::ostream& os, const BlockElement& e);
};

} // namespace wood_session
