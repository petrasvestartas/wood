// wood/wood_session.cpp — the scene-writing half of wood_session.h.
//
// The declarations in that header are implemented across several translation
// units (see its own comment); this one owns everything that turns wood results
// into a session .pb. It is the only file in wood that knows the layout of a
// live scene or where a .pb goes.

#include "wood_session.h"
#include "wood_face_to_face.h"

#include "../src/color.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/polyline.h"
#include "../src/session.h"
#include "../src/tree.h"

#include <fmt/core.h>

namespace wood_session {

using session_cpp::Color;
using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Polyline;
using session_cpp::Session;
using session_cpp::TreeNode;

using Group = std::shared_ptr<TreeNode>;

namespace {

/// WoodElement and BlockElement share only `polylines`, so the loop is written
/// once here and the two public overloads below name the types.
template <class Element>
void add_faces_impl(Session& session, const Group& parent,
                    const std::vector<Element>& elements) {
    for (size_t i = 0; i < elements.size(); ++i) {
        for (size_t f = 0; f < elements[i].polylines.size(); ++f) {
            auto pl = std::make_shared<Polyline>(elements[i].polylines[f]);
            pl->name = fmt::format("element_{}_face_{}", i, f);
            session.add_polyline(pl, parent);
        }
    }
}

template <class Element>
std::filesystem::path write_impl(const std::string& title,
                                 const std::vector<Element>& elements,
                                 const std::vector<FaceContact>& contacts,
                                 const std::string& name) {
    Session session(title);
    add_faces_impl(session, session.add_group("Inputs"), elements);
    add_contacts(session, session.add_group("Contacts"), contacts);
    return pb_dump(session, name);
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Writing
// ═══════════════════════════════════════════════════════════════════════════

std::filesystem::path pb_path(const std::string& name) {
    const std::filesystem::path dir = internal::output_dir() / "pb";
    std::filesystem::create_directories(dir);
    return dir / (name + ".pb");
}

std::filesystem::path pb_dump(const Session& session, const std::string& name) {
    const std::filesystem::path path = pb_path(name);
    session.pb_dump(path.string());
    return path;
}

std::filesystem::path write_element_and_contacts(
        const std::string& title,
        const std::vector<WoodElement>& elements,
        const std::vector<FaceContact>& contacts,
        const std::string& name) {
    return write_impl(title, elements, contacts, name);
}

std::filesystem::path write_element_and_contacts(
        const std::string& title,
        const std::vector<BlockElement>& elements,
        const std::vector<FaceContact>& contacts,
        const std::string& name) {
    return write_impl(title, elements, contacts, name);
}

// ═══════════════════════════════════════════════════════════════════════════
// Pieces
// ═══════════════════════════════════════════════════════════════════════════

void add_faces(Session& session, const Group& parent,
               const std::vector<WoodElement>& elements) {
    add_faces_impl(session, parent, elements);
}

void add_faces(Session& session, const Group& parent,
               const std::vector<BlockElement>& elements) {
    add_faces_impl(session, parent, elements);
}

void add_contacts(Session& session, const Group& parent,
                  const std::vector<FaceContact>& contacts) {
    for (const FaceContact& c : contacts) {
        auto mesh = std::make_shared<Mesh>(
            Mesh::from_polygon_with_holes({c.area.get_points()}, false));
        mesh->name = fmt::format("contact_{}_{}", c.element_a, c.element_b);
        mesh->set_objectcolor(Color(1.0f, 0.0f, 0.0f, 1.0f, "red"));
        session.add_mesh(mesh, parent);
    }
}

void add_joints(Session& session, const std::vector<WoodJoint>& joints) {
    Group areas = session.add_group("Joint Areas");
    Group lines = session.add_group("Joint Lines");
    Group volumes = session.add_group("Joint Volumes");

    for (const WoodJoint& j : joints) {
        session.add_polyline(std::make_shared<Polyline>(j.joint_area), areas);
        for (const Line& l : j.joint_lines)
            session.add_line(std::make_shared<Line>(l), lines);
        // A joint carries up to four bounding quads; the pairs it does not use
        // are nullopt rather than empty polylines.
        for (const std::optional<Polyline>& v : j.joint_volumes_pair_a_pair_b)
            if (v) session.add_polyline(std::make_shared<Polyline>(*v), volumes);
    }
}

void add_lofts(Session& session, const Group& parent,
               const std::vector<WoodElement>& elements) {
    for (const WoodElement& element : elements) {
        if (element.features.bottom.empty() || element.features.top.empty())
            continue;
        session.add_mesh(
            std::make_shared<Mesh>(Mesh::loft(element.features.bottom, element.features.top)),
            parent);
    }
}

} // namespace wood_session
