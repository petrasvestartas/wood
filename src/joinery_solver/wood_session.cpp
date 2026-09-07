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

#include <algorithm>
#include <cctype>
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

    for (std::shared_ptr<Element>& e : *session->objects.elements) {
        if (!e) { continue; }
        const std::string& tag = e->element_type_name();
        WoodGeometry object;
        if (tag == WoodElement::ELEMENT_TYPE || tag == WoodElement::LEGACY_ELEMENT_TYPE) {
            object = std::make_shared<WoodElement>(WoodElement::from_element(*e));
        } else if (tag == WoodColumn::ELEMENT_TYPE) {
            object = std::make_shared<WoodColumn>(WoodColumn::from_element(*e));
        } else if (tag == WoodContact::ELEMENT_TYPE) {
            object = std::make_shared<WoodContact>(WoodContact::from_element(*e));
        } else if (tag == WoodJoint::ELEMENT_TYPE) {
            object = std::make_shared<WoodJoint>(WoodJoint::from_element(*e));
        } else {
            // Untagged, or a type this build has never heard of. The kernel carried the tag
            // and payload through untouched, so nothing is destroyed by treating it as a
            // solid - and a solid is enough to take part in contact detection.
            object = std::make_shared<BlockElement>(BlockElement::from_element(*e));
        }
        // from_element wrapped the loaded element into a TaggedElement. The session must hold
        // that same object, or the wrapper and the session diverge from here on.
        const std::shared_ptr<Element> shared =
            std::visit([](const auto& o) -> std::shared_ptr<Element> { return o->element; }, object);
        e = shared;
        session->lookup[shared->guid()] = shared;
        out.lookup[shared->guid()] = object;
        out.objects.push_back(std::move(object));
    }
    session->bvh_cache_dirty = true;
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

std::shared_ptr<TreeNode> WoodSession::add(const WoodGeometry& object,
                                           const std::shared_ptr<TreeNode>& parent) {
    if (!session) { return nullptr; }
    const std::shared_ptr<Element> element =
        std::visit([](const auto& o) -> std::shared_ptr<Element> { return o->to_element(); }, object);
    const std::shared_ptr<TreeNode> node = session->add_element(element, parent);
    lookup[element->guid()] = object;
    objects.push_back(object);
    return node;
}

bool WoodSession::remove_object(const std::string& guid) {
    const auto it = lookup.find(guid);
    if (it == lookup.end()) { return false; }
    lookup.erase(it);
    objects.erase(std::remove_if(objects.begin(), objects.end(),
                                 [&guid](const WoodGeometry& o) { return object_guid(o) == guid; }),
                  objects.end());
    return session ? session->remove_object(guid) : true;
}

std::vector<std::string> WoodSession::order() const {
    std::vector<std::string> guids;
    guids.reserve(objects.size());
    for (const WoodGeometry& object : objects) { guids.push_back(object_guid(object)); }
    return guids;
}

std::vector<std::string> WoodSession::element_guids() const {
    std::vector<std::string> guids;
    for (const WoodGeometry& object : objects)
        std::visit([&guids](const auto& o) {
            using T = std::decay_t<decltype(*o)>;
            if constexpr (!std::is_same_v<T, WoodContact> && !std::is_same_v<T, WoodJoint>) guids.push_back(o->element->guid());
        }, object);
    return guids;
}

// ═══════════════════════════════════════════════════════════════════════════
// EdgeLink
// ═══════════════════════════════════════════════════════════════════════════

std::string EdgeLink::to_attribute() const {
    return joint.empty() ? "c" + contact : "c" + contact + "j" + joint;
}

EdgeLink EdgeLink::from_attribute(const std::string& attribute) {
    // A guid is hex digits and dashes, so 'j' is unambiguous as the separator. Anything
    // off-grammar - "bvh_collision", "default", "" - is someone else's edge, not an error.
    const auto is_guid_char = [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c)) || c == '-';
    };
    EdgeLink link;
    if (attribute.empty() || attribute[0] != 'c') { return link; }
    size_t i = 1;
    while (i < attribute.size() && is_guid_char(attribute[i])) { link.contact += attribute[i++]; }
    if (i < attribute.size()) {
        if (attribute[i++] != 'j') { return EdgeLink{}; }
        while (i < attribute.size() && is_guid_char(attribute[i])) { link.joint += attribute[i++]; }
        if (link.joint.empty()) { return EdgeLink{}; }
    }
    return (i == attribute.size() && !(link.contact.empty() && link.joint.empty())) ? link : EdgeLink{};
}

// ═══════════════════════════════════════════════════════════════════════════
// Connectivity
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::add_contacts(const std::vector<ContactPair>& detected) {
    if (!session) { return; }
    const std::vector<std::string> guids = element_guids();
    std::shared_ptr<TreeNode> group;
    for (const ContactPair& pair : detected) {
        if (pair.element_a < 0 || pair.element_b < 0 ||
            pair.element_a >= (int)guids.size() || pair.element_b >= (int)guids.size()) { continue; }
        if (!group) {
            try { group = session->find_group("Contacts"); }
            catch (const std::runtime_error&) { group = session->add_group("Contacts"); }
        }

        auto contact = std::make_shared<WoodContact>(pair.faces);
        // add_element mints the graph node for the contact and puts it in the tree; the
        // element nodes already exist, so the edge below joins two known nodes.
        session->add_element(contact->to_element(), group);
        session->add_edge(guids[pair.element_a], guids[pair.element_b],
                          EdgeLink{contact->element->guid(), ""}.to_attribute());
        lookup[contact->element->guid()] = contact;
        objects.push_back(std::move(contact));
    }
}

void WoodSession::add_joints(const std::vector<WoodJoint>& detected) {
    if (!session) { return; }
    std::shared_ptr<TreeNode> group;
    for (const WoodJoint& j : detected) {
        if (j.element_a.empty() || j.element_b.empty()) { continue; }
        if (!lookup.count(j.element_a) || !lookup.count(j.element_b)) { continue; }
        if (!group) {
            try { group = session->find_group("Joints"); }
            catch (const std::runtime_error&) { group = session->add_group("Joints"); }
        }
        auto joint = std::make_shared<WoodJoint>(j);
        joint->element.reset();                        // never share the solver copy's
        session->add_element(joint->to_element(), group);

        // The pair's edge, if a contact made one, keeps its contact; add_edge overwrites,
        // so read first. has_edge / edge_attribute are non-const, hence the map walk.
        EdgeLink link;
        const auto u = session->graph.edges.find(j.element_a);
        if (u != session->graph.edges.end()) {
            const auto v = u->second.find(j.element_b);
            if (v != u->second.end()) { link = EdgeLink::from_attribute(v->second.attribute); }
        }
        link.joint = joint->element->guid();
        session->add_edge(j.element_a, j.element_b, link.to_attribute());
        lookup[joint->element->guid()] = joint;
        objects.push_back(std::move(joint));
    }
}

void WoodSession::compute_contacts() {
    add_contacts(face_contacts(contact_view(*this)));
}

void WoodSession::compute_joints(SearchType search_type) {
    const std::vector<std::shared_ptr<WoodElement>> plates = objects_of<WoodElement>();
    if (plates.empty()) { return; }
    // get_connection_zones runs on a vector of values and writes its results into them, so
    // it gets copies - which share the kernel element - and the results are assigned back.
    std::vector<WoodElement> solved;
    solved.reserve(plates.size());
    for (const std::shared_ptr<WoodElement>& p : plates) { solved.push_back(*p); }
    const std::vector<WoodJoint> joints = get_connection_zones(solved, search_type);
    for (size_t i = 0; i < plates.size(); ++i) {
        *plates[i] = solved[i];
        plates[i]->sync_element();
    }
    add_joints(joints);
}

std::vector<std::pair<std::string, std::string>> WoodSession::joint_pairs() const {
    std::unordered_map<std::string, std::pair<std::string, std::string>> by_joint;
    if (session)
        for (const auto& [u, neighbours] : session->graph.edges)
            for (const auto& [v, edge] : neighbours)
                if (u < v) {
                    const EdgeLink link = EdgeLink::from_attribute(edge.attribute);
                    if (!link.joint.empty()) { by_joint[link.joint] = {u, v}; }
                }
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const std::shared_ptr<WoodJoint>& j : joints()) {
        const auto it = by_joint.find(j->element->guid());
        pairs.push_back(it == by_joint.end() ? std::pair<std::string, std::string>{} : it->second);
    }
    return pairs;
}

std::vector<std::pair<std::string, std::string>> WoodSession::contact_pairs() const {
    std::unordered_map<std::string, std::pair<std::string, std::string>> by_contact;
    if (session) {
        // graph.edges holds every edge twice, once per direction; u < v takes each once,
        // the way Graph::pb_dumps does.
        for (const auto& [u, neighbours] : session->graph.edges)
            for (const auto& [v, edge] : neighbours)
                if (u < v) {
                    const EdgeLink link = EdgeLink::from_attribute(edge.attribute);
                    if (!link.contact.empty()) { by_contact[link.contact] = {u, v}; }
                }
    }
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const std::shared_ptr<WoodContact>& c : contacts()) {
        const auto it = by_contact.find(c->element->guid());
        pairs.push_back(it == by_contact.end() ? std::pair<std::string, std::string>{} : it->second);
    }
    return pairs;
}

std::string WoodSession::str() const {
    std::ostringstream os;
    os << "WoodSession(name=" << name() << ", objects=" << objects.size()
       << ", plates=" << plates().size() << ", columns=" << columns().size()
       << ", solids=" << solids().size()
       << ", contacts=" << contacts().size() << ", joints=" << joints().size()
       << ", edges=" << (session ? session->graph.number_of_edges() : 0) << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodSession& s) { return os << s.str(); }

std::vector<ContactElement> contact_view(const WoodSession& scene) {
    std::vector<ContactElement> view;
    view.reserve(scene.size());
    for (const WoodGeometry& object : scene.objects) {
        std::visit([&view](const auto& o) {
            using T = std::decay_t<decltype(*o)>;
            if constexpr (!std::is_same_v<T, WoodContact> && !std::is_same_v<T, WoodJoint>)
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
                  const std::vector<ContactPair>& contacts) {
    for (const ContactPair& pair : contacts) {
        for (const FaceContact& c : pair.faces) {
            auto mesh = std::make_shared<Mesh>(
                Mesh::from_polygon_with_holes({c.area.get_points()}, false));
            mesh->name = fmt::format("contact_{}_{}", pair.element_a, pair.element_b);
            mesh->set_objectcolor(contact_color(c.type));
            session.add_mesh(mesh, parent);
        }
    }
}

void add_contacts_by_type(Session& session, const std::vector<ContactPair>& contacts,
                          const std::string& prefix) {
    // A group per class that actually occurs, so the tree never shows an empty
    // "top_top" for an assembly that has none.
    std::map<std::string, Group> groups;
    for (const ContactPair& pair : contacts) {
        for (const FaceContact& c : pair.faces) {
            const std::string label = fmt::format("{}_{}", prefix, contact_type_name(c.type));
            auto it = groups.find(label);
            if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
            auto mesh = std::make_shared<Mesh>(
                Mesh::from_polygon_with_holes({c.area.get_points()}, false));
            mesh->name = fmt::format("contact_{}_{}_f{}_{}", pair.element_a, pair.element_b,
                                     c.face_a, c.face_b);
            mesh->set_objectcolor(contact_color(c.type));
            session.add_mesh(mesh, it->second);
        }
    }
}

/// First block of a guid - enough to tell two elements apart in an object name, where a
/// full 36-character guid twice over is unreadable.
static std::string short_guid(const std::string& guid) {
    return guid.substr(0, guid.find('-'));
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
        mesh->name = fmt::format("joint_{}_{}_{}", short_guid(j.element_a), short_guid(j.element_b), type_name);
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
