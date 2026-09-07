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
#include <set>
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
        // to_element(), not a copy of `element.element`: an Element copy mints a fresh guid
        // (element.cpp, copy ctor omits _guid), so a copied plate enters the session as a
        // different object and every guid written beside it - a graph edge, a contact -
        // points at nothing. to_element() restores the guid and carries the type tag.
        std::shared_ptr<session_cpp::Element> copy = element.to_element();
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

// ═══════════════════════════════════════════════════════════════════════════
// EdgeLink
// ═══════════════════════════════════════════════════════════════════════════

std::string EdgeLink::to_attribute() const {
    return joint < 0 ? fmt::format("c{}", contact) : fmt::format("c{}j{}", contact, joint);
}

EdgeLink EdgeLink::from_attribute(const std::string& attribute) {
    // Hand-rolled rather than std::stoi, which throws on "bvh_collision" and accepts
    // leading whitespace. Anything off-grammar is not an error, it is someone else's edge.
    const EdgeLink none;
    size_t i = 0;
    const auto number = [&](int& out) {
        const size_t start = i;
        if (i < attribute.size() && attribute[i] == '-') { i++; }
        while (i < attribute.size() && std::isdigit(static_cast<unsigned char>(attribute[i]))) { i++; }
        if (i == start || (i == start + 1 && attribute[start] == '-')) { return false; }
        out = std::stoi(attribute.substr(start, i - start));
        return true;
    };
    EdgeLink link;
    if (i >= attribute.size() || attribute[i++] != 'c') { return none; }
    if (!number(link.contact)) { return none; }
    if (i < attribute.size()) {
        if (attribute[i++] != 'j') { return none; }
        if (!number(link.joint)) { return none; }
    }
    return i == attribute.size() ? link : none;
}

// ═══════════════════════════════════════════════════════════════════════════
// Elements, contacts, connectivity
// ═══════════════════════════════════════════════════════════════════════════

std::vector<std::string> WoodSession::element_guids() const {
    std::vector<std::string> guids;
    guids.reserve(size());
    for (const WoodElement& e : plates)  { guids.push_back(e.element.guid()); }
    for (const WoodColumn& e : columns)  { guids.push_back(e.element.guid()); }
    for (const BlockElement& e : solids) { guids.push_back(e.element.guid()); }
    return guids;
}

int WoodSession::element_index(const std::string& guid) const {
    const std::vector<std::string> guids = element_guids();
    for (size_t i = 0; i < guids.size(); ++i)
        if (guids[i] == guid) { return static_cast<int>(i); }
    return -1;
}

void WoodSession::add_contacts(const std::vector<ContactPair>& detected) {
    const std::vector<std::string> guids = element_guids();
    for (const ContactPair& pair : detected) {
        if (pair.element_a < 0 || pair.element_b < 0 ||
            pair.element_a >= (int)guids.size() || pair.element_b >= (int)guids.size()) { continue; }
        const int index = static_cast<int>(contacts.size());
        contacts.emplace_back(pair.faces);
        graph.add_node(guids[pair.element_a]);
        graph.add_node(guids[pair.element_b]);
        graph.add_edge(guids[pair.element_a], guids[pair.element_b],
                       EdgeLink{index, -1}.to_attribute());
    }
}

std::vector<std::pair<int, int>> WoodSession::contact_pairs() const {
    std::vector<std::pair<int, int>> pairs(contacts.size(), {-1, -1});
    // graph.edges holds every edge twice, once per direction; u < v takes each once, the
    // way Graph::pb_dumps does.
    for (const auto& [u, neighbours] : graph.edges) {
        for (const auto& [v, edge] : neighbours) {
            if (!(u < v)) { continue; }
            const EdgeLink link = EdgeLink::from_attribute(edge.attribute);
            if (link.contact < 0 || link.contact >= (int)pairs.size()) { continue; }
            pairs[link.contact] = { element_index(u), element_index(v) };
        }
    }
    return pairs;
}

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

Session WoodSession::to_session(const std::string& title) const {
    Session session(title);
    const Group inputs = session.add_group("Inputs");
    add_solids(session, inputs, plates);
    add_solids(session, inputs, columns);
    add_solids(session, inputs, solids);

    if (!contacts.empty()) {
        const Group group = session.add_group("Contacts");
        for (const WoodContact& c : contacts) { session.add_element(c.to_element(), group); }
    }

    // Edges last: add_element mints the node for each element guid, and Graph::add_edge on a
    // missing node would create one with an empty attribute instead.
    for (const auto& [u, neighbours] : graph.edges)
        for (const auto& [v, edge] : neighbours)
            if (u < v) { session.add_edge(u, v, edge.attribute); }
    return session;
}

// ═══════════════════════════════════════════════════════════════════════════
// Printing
// ═══════════════════════════════════════════════════════════════════════════

namespace {
template <class T>
void append_group(std::ostringstream& os, const std::string& name, const std::vector<T>& elements, bool last) {
    os << (last ? "└── " : "├── ") << name << " (" << elements.size() << ")\n";
    const std::string prefix = last ? "    " : "│   ";
    for (size_t i = 0; i < elements.size(); ++i)
        os << prefix << (i + 1 == elements.size() ? "└── " : "├── ") << elements[i].str() << "\n";
}
}

std::string WoodSession::str() const {
    std::ostringstream os;
    os << "WoodSession(plates=" << plates.size() << ", columns=" << columns.size()
       << ", solids=" << solids.size() << ")\n";
    append_group(os, "plates", plates, false);
    append_group(os, "columns", columns, false);
    append_group(os, "solids", solids, true);
    std::string out = os.str();
    out.pop_back();
    return out;
}
std::ostream& operator<<(std::ostream& os, const WoodSession& s) { return os << s.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// Reading
// ═══════════════════════════════════════════════════════════════════════════

std::filesystem::path WoodSession::pb_dump(const std::string& name) const {
    return wood_session::pb_dump(to_session(name), name);
}

WoodSession WoodSession::from_session(const Session& session) {
    WoodSession out;
    if (!session.objects.elements) { return out; }

    for (const std::shared_ptr<Element>& e : *session.objects.elements) {
        if (!e) { continue; }
        const std::string& tag = e->element_type_name();
        if (tag == WoodElement::ELEMENT_TYPE || tag == WoodElement::LEGACY_ELEMENT_TYPE) {
            out.plates.push_back(WoodElement::from_element(*e));
        } else if (tag == WoodColumn::ELEMENT_TYPE) {
            out.columns.push_back(WoodColumn::from_element(*e));
        } else if (tag == WoodContact::ELEMENT_TYPE) {
            out.contacts.push_back(WoodContact::from_element(*e));
        } else {
            // Untagged, or a type this build has never heard of. The kernel carried the tag
            // and payload through untouched, so nothing is destroyed by treating it as a
            // solid - and a solid is enough to take part in contact detection.
            out.solids.push_back(BlockElement::from_element(*e));
        }
    }

    // Connectivity: keep only the edges that join two of THIS scene's elements. The session
    // graph also holds a node per mesh, per polyline and per contact element, and an edge
    // written by something else - "bvh_collision", "default" - is not ours to read.
    const std::vector<std::string> guids = out.element_guids();
    const std::set<std::string> owned(guids.begin(), guids.end());
    for (const auto& [u, neighbours] : session.graph.edges)
        for (const auto& [v, edge] : neighbours)
            if (u < v && owned.count(u) && owned.count(v) &&
                EdgeLink::from_attribute(edge.attribute).contact >= 0) {
                out.graph.add_node(u);
                out.graph.add_node(v);
                out.graph.add_edge(u, v, edge.attribute);
            }
    return out;
}

std::filesystem::path dataset_pb(const std::string& name) {
    return internal::session_data_dir() / (name + ".pb");
}

std::vector<ContactElement> contact_view(const WoodSession& elements) {
    std::vector<ContactElement> view;
    view.reserve(elements.size());
    for (const WoodElement& e : elements.plates) {
        view.push_back({&e.polylines, &e.planes, &e.element.name, /*plate_convention=*/true});
    }
    for (const WoodColumn& e : elements.columns) {
        view.push_back({&e.polylines, &e.planes, &e.element.name, false});
    }
    for (const BlockElement& e : elements.solids) {
        view.push_back({&e.polylines, &e.planes, &e.element.name, false});
    }
    return view;
}

std::vector<BlockElement> load_block_elements(const std::filesystem::path& pb) {
    std::vector<BlockElement> elements;
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\n", pb.string());
        return elements;
    }

    Session session = Session::pb_load(pb.string());
    std::shared_ptr<TreeNode> root = session.tree.root();
    if (!root) { return elements; }

    // objects.elements, not the tree: add_element only puts a node in the tree when
    // it is given a parent, and the list keeps insertion order anyway.
    for (const std::shared_ptr<Element>& e : *session.objects.elements) {
        if (e) { elements.push_back(BlockElement::from_element(*e)); }
    }
    if (!elements.empty()) { return elements; }

    // Older files (and brep_to_pb.py) hold one GROUP of loose polylines per solid,
    // with the name on the group node - which select_by_type() drops, hence the walk.
    for (TreeNode* group : root->children()) {
        std::vector<Polyline> loops;
        for (TreeNode* node : group->descendants()) {
            if (std::shared_ptr<const Polyline> loop = session.get_object<Polyline>(node->name)) {
                loops.push_back(*loop);
            }
        }
        if (loops.empty()) { continue; }
        elements.emplace_back(loops, group->name);
    }
    return elements;
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
