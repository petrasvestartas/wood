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

#include <map>
#include <sstream>
#include <string>

namespace wood_session {

using session_cpp::Color;
using session_cpp::Element;
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

/// The element's own geometry, whatever it is. Copying the Element mints a fresh guid,
/// which is what a display copy wants - the scene never claims to BE the input.
/// Grey unless the element says otherwise: a solid is read by its shape, and a colour per
/// element would compete with the contact and joint colours that DO carry meaning.
const session_cpp::Color SOLID_GREY(0.84f, 0.84f, 0.86f, 1.0f, "solid_grey");

template <class WoodType>
void add_solids_impl(Session& session, const Group& parent,
                     const std::vector<WoodType>& elements) {
    for (const WoodType& element : elements) {
        // The element itself, shared - never a copy, which would mint a new guid and enter
        // the session as a different object. to_element() refreshes its payload first.
        const std::shared_ptr<session_cpp::Element> copy = element.to_element();
        // geometry() is const, so the colour goes on a copy that is set back.
        if (const session_cpp::Mesh* mesh = std::get_if<session_cpp::Mesh>(&copy->geometry())) {
            session_cpp::Mesh grey = *mesh;
            // set_objectcolor only assigns the colour; it does NOT switch color_mode, so a
            // mesh that arrived with facecolors would keep drawing them and ignore this.
            grey.clear_facecolors();
            grey.clear_pointcolors();
            grey.set_objectcolor(SOLID_GREY);
            copy->set_geometry(grey);
        } else if (const session_cpp::BRep* brep = std::get_if<session_cpp::BRep>(&copy->geometry())) {
            session_cpp::BRep grey = *brep;
            grey.surfacecolor = SOLID_GREY;
            copy->set_geometry(grey);
        }
        session.add_element(copy, parent);
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
// Reading
// ═══════════════════════════════════════════════════════════════════════════

/// Every wood object answers `element`, so its guid is one visit away.
static const std::string& object_guid(const WoodGeometry& object) {
    return std::visit([](const auto& o) -> const std::string& { return o->element->guid(); }, object);
}

WoodSession WoodSession::from_session(const std::shared_ptr<Session>& session) {
    WoodSession out;
    out.session = session;
    if (!session || !session->objects.elements) { return out; }

    for (const std::shared_ptr<Element>& e : *session->objects.elements) {
        if (!e) { continue; }
        const std::string& tag = e->element_type_name();
        WoodGeometry object;
        if (tag == WoodElement::ELEMENT_TYPE || tag == WoodElement::LEGACY_ELEMENT_TYPE) {
            object = std::make_shared<WoodElement>(WoodElement::from_element(*e));
        } else if (tag == WoodColumn::ELEMENT_TYPE) {
            object = std::make_shared<WoodColumn>(WoodColumn::from_element(*e));
        } else {
            // Untagged, or a type this build has never heard of. The kernel carried the tag
            // and payload through untouched, so nothing is destroyed by treating it as a
            // solid - and a solid is enough to take part in contact detection.
            object = std::make_shared<BlockElement>(BlockElement::from_element(*e));
        }
        out.lookup[object_guid(object)] = object;
        out.objects.push_back(std::move(object));
    }
    return out;
}

WoodSession WoodSession::load(const std::filesystem::path& pb) {
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\n", pb.string());
        return WoodSession{};
    }
    return from_session(std::make_shared<Session>(Session::pb_load(pb.string())));
}

const std::shared_ptr<Session>& WoodSession::to_session() const {
    for (const WoodGeometry& object : objects) {
        std::visit([](const auto& o) { o->to_element(); }, object);
    }
    return session;
}

std::filesystem::path WoodSession::pb_dump(const std::string& name) const {
    to_session();
    return session ? wood_session::pb_dump(*session, name) : std::filesystem::path();
}

const std::string& WoodSession::name() const {
    static const std::string none;
    return session ? session->name : none;
}

const std::string& WoodSession::guid() const {
    static const std::string none;
    return session ? session->guid() : none;
}

/// The typed views: one pass over the collection, keeping the alternatives that match.
template <class T>
static std::vector<T*> objects_of(const std::vector<WoodGeometry>& objects) {
    std::vector<T*> out;
    for (const WoodGeometry& object : objects)
        if (const std::shared_ptr<T>* p = std::get_if<std::shared_ptr<T>>(&object)) { out.push_back(p->get()); }
    return out;
}

std::vector<WoodElement*>  WoodSession::plates() const  { return objects_of<WoodElement>(objects); }
std::vector<WoodColumn*>   WoodSession::columns() const { return objects_of<WoodColumn>(objects); }
std::vector<BlockElement*> WoodSession::solids() const  { return objects_of<BlockElement>(objects); }

std::string WoodSession::str() const {
    std::ostringstream os;
    const std::vector<WoodElement*>  p = plates();
    const std::vector<WoodColumn*>   c = columns();
    const std::vector<BlockElement*> b = solids();
    os << "WoodSession(name=" << name() << ", objects=" << objects.size()
       << ", plates=" << p.size() << ", columns=" << c.size() << ", solids=" << b.size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodSession& s) { return os << s.str(); }

std::vector<ContactElement> contact_view(const WoodSession& scene) {
    std::vector<ContactElement> view;
    view.reserve(scene.size());
    for (const WoodGeometry& object : scene.objects) {
        std::visit([&view](const auto& o) {
            using T = std::decay_t<decltype(*o)>;
            view.push_back({&o->polylines, &o->planes, &o->element->name,
                            std::is_same_v<T, WoodElement>});
        }, object);
    }
    return view;
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

void add_faces(Session& session, const Group& parent,
               const std::vector<WoodColumn>& elements) {
    add_faces_impl(session, parent, elements);
}

void add_solids(Session& session, const Group& parent,
                const std::vector<WoodElement>& elements) {
    add_solids_impl(session, parent, elements);
}

void add_solids(Session& session, const Group& parent,
                const std::vector<BlockElement>& elements) {
    add_solids_impl(session, parent, elements);
}

void add_solids(Session& session, const Group& parent,
                const std::vector<WoodColumn>& elements) {
    add_solids_impl(session, parent, elements);
}

const char* contact_type_name(ContactType type) {
    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top:  return "side_top";
        case ContactType::top_top:   return "top_top";
        case ContactType::unknown:   break;
    }
    return "unknown";
}

std::string joint_type_name(int joint_type) {
    switch (joint_type) {
        case 11: return "ss_op_11";
        case 12: return "ss_ip_12";
        case 13: return "ss_rot_13";
        case 20: return "ts_20";
        case 30: return "cross_30";
        case 40: return "tt_40";
        default: return fmt::format("type_{}", joint_type);
    }
}

// Palette: the BRG equilibrium drawings (brg-teaching.github.io), whose PAL is built
// for white paper - navy/pink/green carry the meaning, grey is anything inert. Taken as
// is rather than re-tuned, so a wood scene sits beside those drawings without clashing.
//
//   #1a1eb2 navy   #ce4095 pink   #3f9c20 green   #b9b9bd zero-grey
//   #e07a26 orange #e8ac00 yellow #a83179 deep pink
Color contact_color(ContactType type) {
    switch (type) {
        case ContactType::side_side: return Color(0.102f, 0.118f, 0.698f, 1.0f, "ss_navy");
        case ContactType::side_top:  return Color(0.808f, 0.251f, 0.584f, 1.0f, "ts_pink");
        case ContactType::top_top:   return Color(0.247f, 0.612f, 0.125f, 1.0f, "tt_green");
        case ContactType::unknown:   break;
    }
    // BRG's "zero" - a member that is neither in compression nor tension. A contact the
    // detector could not classify is the same statement, so it gets the same colour.
    return Color(0.725f, 0.725f, 0.741f, 1.0f, "unknown_zero");
}

/// Same palette, and deliberately the same hue per contact class: a joint keeps the colour
/// of the contact it came from (12/13 side-side navy, 20 top-side pink, 40 top-top green),
/// so the Joints groups read as a subset of the Contacts groups rather than a second legend.
Color joint_color(int joint_type) {
    switch (joint_type) {
        case 12: return Color(0.102f, 0.118f, 0.698f, 1.0f, "ss_ip_navy");
        case 11: return Color(0.878f, 0.478f, 0.149f, 1.0f, "ss_op_orange");
        case 13: return Color(0.659f, 0.192f, 0.475f, 1.0f, "ss_rot_deep_pink");
        case 20: return Color(0.808f, 0.251f, 0.584f, 1.0f, "ts_pink");
        case 40: return Color(0.247f, 0.612f, 0.125f, 1.0f, "tt_green");
        case 30: return Color(0.910f, 0.675f, 0.000f, 1.0f, "cross_yellow");
        default: return Color(0.725f, 0.725f, 0.741f, 1.0f, "unknown_zero");
    }
}

void add_contacts(Session& session, const Group& parent,
                  const std::vector<FaceContact>& contacts) {
    for (const FaceContact& c : contacts) {
        auto mesh = std::make_shared<Mesh>(
            Mesh::from_polygon_with_holes({c.area.get_points()}, false));
        mesh->name = fmt::format("contact_{}_{}", c.element_a, c.element_b);
        mesh->set_objectcolor(contact_color(c.type));
        session.add_mesh(mesh, parent);
    }
}

void add_contacts_by_type(Session& session, const std::vector<FaceContact>& contacts,
                          const std::string& prefix) {
    // A group per class that actually occurs, so the tree never shows an empty
    // "top_top" for an assembly that has none.
    std::map<std::string, Group> groups;
    for (const FaceContact& c : contacts) {
        const std::string label = fmt::format("{}_{}", prefix, contact_type_name(c.type));
        auto it = groups.find(label);
        if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
        auto mesh = std::make_shared<Mesh>(
            Mesh::from_polygon_with_holes({c.area.get_points()}, false));
        mesh->name = fmt::format("contact_{}_{}_f{}_{}", c.element_a, c.element_b, c.face_a, c.face_b);
        mesh->set_objectcolor(contact_color(c.type));
        session.add_mesh(mesh, it->second);
    }
}

void add_joints_by_type(Session& session, const std::vector<WoodJoint>& joints,
                        const std::string& prefix) {
    std::map<std::string, Group> groups;
    for (const WoodJoint& j : joints) {
        const std::string type_name = joint_type_name(j.joint_type);
        const std::string label = fmt::format("{}_{}", prefix, type_name);
        auto it = groups.find(label);
        if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
        auto mesh = std::make_shared<Mesh>(
            Mesh::from_polygon_with_holes({j.contact.area.get_points()}, false));
        mesh->name = fmt::format("joint_{}_{}_{}", j.contact.element_a, j.contact.element_b, type_name);
        mesh->set_objectcolor(joint_color(j.joint_type));
        session.add_mesh(mesh, it->second);
    }
}

void add_joints(Session& session, const std::vector<WoodJoint>& joints) {
    Group areas = session.add_group("Joint Areas");
    Group lines = session.add_group("Joint Lines");
    Group volumes = session.add_group("Joint Volumes");

    for (const WoodJoint& j : joints) {
        session.add_polyline(std::make_shared<Polyline>(j.contact.area), areas);
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
