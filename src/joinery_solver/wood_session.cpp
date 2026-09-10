// wood/wood_session.cpp — the scene half of wood_session.h.
//
// The declarations in that header are implemented across several translation units (see its
// own comment); this one owns the model: a WoodSession, the WoodInteraction its graph edges
// carry, and the few writers that turn either into drawable geometry. It is the only file in
// wood that knows the layout of a live scene or where a .pb goes.

#include "wood_session.h"
#include "wood_face_to_face.h"

#include "../src/color.h"
#include "../src/intersection.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/point.h"
#include "../src/polyline.h"
#include "../src/session.h"
#include "../src/tree.h"

#include <fmt/core.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>

namespace wood_session {

using session_cpp::Color;
using session_cpp::Element;
using session_cpp::ElementFeature;
using session_cpp::Intersection;
using session_cpp::Line;
using session_cpp::Mesh;
using session_cpp::Point;
using session_cpp::Polyline;
using session_cpp::Session;
using session_cpp::TreeNode;

using Group = std::shared_ptr<TreeNode>;

namespace {

/// One view per wrapped element, in objects.elements order - the index space element_guids()
/// and every ContactPair share. An element the scene does not wrap is skipped by both.
std::vector<ContactElement> contact_view(const WoodSession& scene) {
    std::vector<ContactElement> view;
    view.reserve(scene.elements.size());
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        const auto it = scene.elements.find(element->guid());
        if (it == scene.elements.end()) { continue; }
        std::visit([&view](const auto& object) {
            using Wood = std::decay_t<decltype(*object)>;
            view.push_back({&object->polylines, &object->planes, &object->element->name,
                            std::is_same_v<Wood, WoodElement>});
        }, it->second);
    }
    return view;
}

/// First block of a guid - enough to tell two elements apart in an object name, where a full
/// 36-character guid twice over is unreadable.
std::string short_guid(const std::string& guid) {
    return guid.substr(0, guid.find('-'));
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction
// ═══════════════════════════════════════════════════════════════════════════

WoodInteraction WoodInteraction::flipped() const {
    WoodInteraction out = *this;
    for (FaceContact& contact : out.contacts) { std::swap(contact.face_a, contact.face_b); }
    return out;
}

nlohmann::ordered_json WoodInteraction::jsondump() const {
    nlohmann::ordered_json faces = nlohmann::ordered_json::array();
    for (const FaceContact& contact : contacts) { faces.push_back(contact.jsondump()); }
    nlohmann::ordered_json cuts = nlohmann::ordered_json::array();
    for (const WoodJoint& joint : joints) { cuts.push_back(joint.jsondump()); }
    return nlohmann::ordered_json{
        {"type", TYPE},
        {"contacts", faces},
        {"joints", cuts},
    };
}

WoodInteraction WoodInteraction::jsonload(const nlohmann::json& data) {
    WoodInteraction interaction;
    if (data.contains("contacts"))
        for (const auto& contact : data["contacts"]) { interaction.contacts.push_back(FaceContact::jsonload(contact)); }
    if (data.contains("joints"))
        for (const auto& joint : data["joints"]) { interaction.joints.push_back(WoodJoint::jsonload(joint)); }
    return interaction;
}

std::string WoodInteraction::to_attribute() const { return jsondump().dump(); }

WoodInteraction WoodInteraction::from_attribute(const std::string& attribute) {
    // The "type" key is the whole grammar: it rejects get_collisions()'s "bvh_collision",
    // add_relationship's "default" and an edge somebody else wrote, without a parser.
    if (attribute.empty() || attribute.front() != '{') { return WoodInteraction{}; }
    nlohmann::json data;
    try {
        data = nlohmann::json::parse(attribute);
    } catch (const std::exception&) {
        return WoodInteraction{};
    }
    if (!data.is_object() || data.value("type", std::string()) != TYPE) { return WoodInteraction{}; }
    return jsonload(data);
}

std::string WoodInteraction::str() const {
    std::ostringstream os;
    os << "WoodInteraction(contacts=" << contacts.size() << ", joints=" << joints.size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction) { return os << interaction.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession — the elements
// ═══════════════════════════════════════════════════════════════════════════

Group WoodSession::add(const WoodGeometry& object, const Group& parent) {
    const std::shared_ptr<Element> element =
        std::visit([](const auto& wood) -> std::shared_ptr<Element> { return wood->to_element(); }, object);
    const Group node = add_element(element, parent);
    elements[element->guid()] = object;
    return node;
}

bool WoodSession::remove_object(const std::string& guid) {
    elements.erase(guid);
    return Session::remove_object(guid);
}

std::vector<std::string> WoodSession::element_guids() const {
    std::vector<std::string> guids;
    guids.reserve(elements.size());
    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (elements.count(element->guid())) { guids.push_back(element->guid()); }
    return guids;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession — the interactions
// ═══════════════════════════════════════════════════════════════════════════

WoodInteraction WoodSession::get_interaction(const std::string& a, const std::string& b) const {
    const auto neighbours = graph.edges.find(a);
    if (neighbours == graph.edges.end()) { return WoodInteraction{}; }
    const auto edge = neighbours->second.find(b);
    if (edge == neighbours->second.end()) { return WoodInteraction{}; }
    // Stored oriented to the low guid, so reading from the high one is the other end.
    const WoodInteraction interaction = WoodInteraction::from_attribute(edge->second.attribute);
    return a < b ? interaction : interaction.flipped();
}

void WoodSession::set_interaction(const std::string& a, const std::string& b, const WoodInteraction& interaction) {
    const std::string attribute = (a < b ? interaction : interaction.flipped()).to_attribute();
    // Graph::add_edge counts an overwrite as one more edge and re-mints the Edge, so a pair
    // that already has one is written in place instead.
    const auto neighbours = graph.edges.find(a);
    if (neighbours != graph.edges.end() && neighbours->second.count(b)) {
        graph.edge_attribute(a, b, attribute);
        return;
    }
    add_edge(a, b, attribute);
}

std::vector<std::tuple<std::string, std::string, WoodInteraction>> WoodSession::get_interactions() const {
    std::vector<std::tuple<std::string, std::string, WoodInteraction>> out;
    // Every edge is stored twice, once per direction; a < b takes each once, and is also the
    // orientation the attribute was written in.
    for (const auto& [a, neighbours] : graph.edges)
        for (const auto& [b, edge] : neighbours) {
            if (!(a < b)) { continue; }
            WoodInteraction interaction = WoodInteraction::from_attribute(edge.attribute);
            if (!interaction.empty()) { out.emplace_back(a, b, std::move(interaction)); }
        }
    return out;
}

std::vector<ContactPair> WoodSession::contacts() const {
    const std::vector<std::string> guids = element_guids();
    std::unordered_map<std::string, int> index;
    for (size_t i = 0; i < guids.size(); ++i) { index[guids[i]] = static_cast<int>(i); }

    std::vector<ContactPair> pairs;
    for (const auto& [a, b, interaction] : get_interactions()) {
        if (interaction.contacts.empty()) { continue; }
        const auto ia = index.find(a);
        const auto ib = index.find(b);
        if (ia == index.end() || ib == index.end()) { continue; }
        pairs.push_back({ia->second, ib->second, interaction.contacts});
    }
    return pairs;
}

std::vector<WoodJoint> WoodSession::joints() const {
    std::vector<WoodJoint> out;
    for (const auto& [a, b, interaction] : get_interactions())
        out.insert(out.end(), interaction.joints.begin(), interaction.joints.end());
    return out;
}

std::vector<ElementFeature> WoodSession::get_element_features(const std::string& guid) const {
    std::vector<ElementFeature> features;
    const auto neighbours = graph.edges.find(guid);
    if (neighbours == graph.edges.end()) { return features; }
    for (const auto& [other, edge] : neighbours->second) {
        const WoodInteraction interaction = WoodInteraction::from_attribute(edge.attribute);
        for (const WoodJoint& joint : interaction.joints) {
            // The side comes from the joint's own two elements: a kernel edge is stored in
            // both directions with the same object, so its v0 is not a reliable source.
            const int side = joint.element_a == guid ? 0 : (joint.element_b == guid ? 1 : -1);
            if (side < 0) { continue; }
            std::array<ElementFeature, 2> sides = joint.to_features();
            features.push_back(std::move(sides[side]));
        }
    }
    return features;
}

std::vector<std::pair<std::string, std::string>> WoodSession::get_collisions() {
    const std::vector<std::tuple<std::string, std::string, WoodInteraction>> kept = get_interactions();
    std::vector<std::pair<std::string, std::string>> pairs = Session::get_collisions();
    for (const auto& [a, b, interaction] : kept) { set_interaction(a, b, interaction); }
    return pairs;
}

// Computing twice must replace, not accumulate: an edge keeps whatever the other compute put
// there, and loses only what this one is about to write again.
void WoodSession::clear_contacts() {
    for (const auto& [a, b, interaction] : get_interactions()) {
        WoodInteraction kept = interaction;
        kept.contacts.clear();
        set_interaction(a, b, kept);
    }
}

// Same idea as clear_contacts(), but scoped to one ContactType: compute_cross_contacts()
// recomputing must not erase what compute_face_contacts() left on the same edge, and vice
// versa, since both may legitimately hold a contact on the same element pair.
static void erase_contacts_of_type(WoodSession& scene, ContactType type) {
    for (const auto& [a, b, interaction] : scene.get_interactions()) {
        WoodInteraction kept = interaction;
        auto& contacts = kept.contacts;
        contacts.erase(std::remove_if(contacts.begin(), contacts.end(),
                                       [type](const FaceContact& fc) { return fc.type == type; }),
                       contacts.end());
        scene.set_interaction(a, b, kept);
    }
}


void WoodSession::clear_joints() {
    for (const auto& [a, b, interaction] : get_interactions()) {
        WoodInteraction kept = interaction;
        kept.joints.clear();
        set_interaction(a, b, kept);
    }
}

void WoodSession::compute_face_contacts() {
    erase_contacts_of_type(*this, ContactType::side_side);
    erase_contacts_of_type(*this, ContactType::side_top);
    erase_contacts_of_type(*this, ContactType::top_top);
    erase_contacts_of_type(*this, ContactType::unknown);
    const std::vector<std::string> guids = element_guids();
    for (const ContactPair& pair : face_contacts(contact_view(*this))) {
        if (pair.element_a < 0 || pair.element_b < 0) { continue; }
        if (pair.element_a >= (int)guids.size() || pair.element_b >= (int)guids.size()) { continue; }
        const std::string& a = guids[pair.element_a];
        const std::string& b = guids[pair.element_b];
        // Read before write: add_edge overwrites the whole attribute, and the pair may
        // already carry joints, or a cross/line contact this call must not disturb.
        WoodInteraction interaction = get_interaction(a, b);
        interaction.contacts.insert(interaction.contacts.end(), pair.faces.begin(), pair.faces.end());
        set_interaction(a, b, interaction);
    }
}

void WoodSession::compute_contacts() { compute_face_contacts(); }

void WoodSession::compute_cross_contacts(double angle_tol) {
    erase_contacts_of_type(*this, ContactType::cross);
    const std::vector<std::shared_ptr<WoodElement>> plates = this->plates();
    for (size_t i = 0; i < plates.size(); ++i) {
        if (plates[i]->polylines.size() < 2 || plates[i]->planes.size() < 2) { continue; }
        for (size_t j = i + 1; j < plates.size(); ++j) {
            if (plates[j]->polylines.size() < 2 || plates[j]->planes.size() < 2) { continue; }
            CrossJoint cj;
            if (!plane_to_face(plates[i]->polylines[0], plates[i]->polylines[1],
                                plates[j]->polylines[0], plates[j]->polylines[1],
                                plates[i]->planes[0], plates[i]->planes[1],
                                plates[j]->planes[0], plates[j]->planes[1],
                                cj, angle_tol)) {
                continue;
            }
            FaceContact contact;
            contact.face_a = cj.face_ids_a.first;
            contact.face_b = cj.face_ids_b.first;
            contact.type = ContactType::cross;
            contact.area = cj.joint_area;
            const std::string& a = plates[i]->element->guid();
            const std::string& b = plates[j]->element->guid();
            WoodInteraction interaction = get_interaction(a, b);
            interaction.contacts.push_back(contact);
            set_interaction(a, b, interaction);
        }
    }
}

void WoodSession::compute_line_contacts(double tolerance) {
    erase_contacts_of_type(*this, ContactType::line);
    const double tol = tolerance >= 0.0 ? tolerance : globals::DISTANCE;
    const double tol_squared = tol * tol;
    const std::vector<std::string> guids = element_guids();
    const std::vector<ContactElement> view = contact_view(*this);
    for (size_t a = 0; a < view.size(); ++a) {
        for (size_t b = a + 1; b < view.size(); ++b) {
            const std::vector<Polyline>& loops_a = *view[a].polylines;
            const std::vector<Polyline>& loops_b = *view[b].polylines;
            for (size_t la = 0; la < loops_a.size(); ++la) {
                const std::vector<Point> pa = loops_a[la].get_points();
                for (size_t sa = 0; sa + 1 < pa.size(); ++sa) {
                    const Line seg_a = Line::from_points(pa[sa], pa[sa + 1]);
                    for (size_t lb = 0; lb < loops_b.size(); ++lb) {
                        const std::vector<Point> pb = loops_b[lb].get_points();
                        for (size_t sb = 0; sb + 1 < pb.size(); ++sb) {
                            const Line seg_b = Line::from_points(pb[sb], pb[sb + 1]);
                            double t0 = 0.0, t1 = 0.0;
                            if (!Intersection::line_line_parameters(seg_a, seg_b, t0, t1, 0.0,
                                                                     /*intersect_segments=*/true,
                                                                     /*near_parallel_as_closest=*/true)) {
                                continue;
                            }
                            const Point q0 = seg_a.point_at(t0);
                            const Point q1 = seg_b.point_at(t1);
                            const double dx = q0[0] - q1[0], dy = q0[1] - q1[1], dz = q0[2] - q1[2];
                            if (dx * dx + dy * dy + dz * dz > tol_squared) { continue; }
                            FaceContact contact;
                            contact.face_a = static_cast<int>(la);
                            contact.face_b = static_cast<int>(lb);
                            contact.type = ContactType::line;
                            contact.area = Polyline(std::vector<Point>{q0, q1});
                            const std::string& x = guids[a];
                            const std::string& y = guids[b];
                            WoodInteraction interaction = get_interaction(x, y);
                            interaction.contacts.push_back(contact);
                            set_interaction(x, y, interaction);
                        }
                    }
                }
            }
        }
    }
}

void WoodSession::compute_joints(SearchType search_type) {
    const std::vector<std::shared_ptr<WoodElement>> plates = this->plates();
    if (plates.empty()) { return; }
    clear_joints();
    // get_connection_zones writes into the vector it is given: copies in, results back. A
    // WoodElement copy shares its TaggedElement, so the guid survives the round trip.
    std::vector<WoodElement> solved;
    solved.reserve(plates.size());
    for (const std::shared_ptr<WoodElement>& plate : plates) { solved.push_back(*plate); }
    const std::vector<WoodJoint> joints = get_connection_zones(solved, search_type);
    for (size_t i = 0; i < plates.size(); ++i) {
        *plates[i] = solved[i];
        plates[i]->sync_element();
    }
    for (const WoodJoint& joint : joints) {
        if (!elements.count(joint.element_a) || !elements.count(joint.element_b)) { continue; }
        WoodInteraction interaction = get_interaction(joint.element_a, joint.element_b);
        interaction.joints.push_back(joint);
        set_interaction(joint.element_a, joint.element_b, interaction);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession — conversion
// ═══════════════════════════════════════════════════════════════════════════

/// One wood object for one loaded element, by its `element_type` tag.
static WoodGeometry from_element(const Element& element) {
    const std::string tag = element.element_type_name();
    if (tag == WoodElement::ELEMENT_TYPE || tag == WoodElement::LEGACY_ELEMENT_TYPE)
        return std::make_shared<WoodElement>(WoodElement::from_element(element));
    if (tag == WoodColumn::ELEMENT_TYPE)
        return std::make_shared<WoodColumn>(WoodColumn::from_element(element));
    // Untagged, or a type this build has never heard of. The kernel carried the tag and the
    // payload through untouched, so nothing is destroyed by treating it as a solid - and a
    // solid is enough to take part in contact detection.
    return std::make_shared<BlockElement>(BlockElement::from_element(element));
}

/// `source`'s subtree under `node`, rebuilt under `parent`. A child whose name is a guid the
/// source holds is an object; anything else is a group, and groups nest, which is why they are
/// made here rather than with Session::add_group - that one always attaches to the root.
static void rebuild(WoodSession& scene, const Session& source, const TreeNode& node, const Group& parent) {
    for (const TreeNode* child : node.children()) {
        if (const std::shared_ptr<const Element> element = source.get_object<Element>(child->name)) {
            scene.add(from_element(*element), parent);
            continue;
        }
        if (source.lookup.count(child->name)) { continue; }   // an object, but not an Element
        const Group group = std::make_shared<TreeNode>(child->name);
        group->color = child->color;
        scene.add(group, parent);
        rebuild(scene, source, *child, group);
    }
}

WoodSession WoodSession::from_session(const Session& session) {
    WoodSession scene(session.name);
    scene.guid() = session.guid();

    // The tree first, so every element lands under the node it was under. Session::add_element
    // makes the graph node as it goes, so the graph's vertices come with the elements.
    if (session.tree.root()) { rebuild(scene, session, *session.tree.root(), nullptr); }
    // An element the tree never named is still the session's; it joins at the root.
    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (element && !scene.elements.count(element->guid())) { scene.add(from_element(*element), nullptr); }

    // Guids do not change, so everything keyed on one comes across by name: the placements,
    // and the edges with the contacts and joints they carry.
    for (const auto& [guid, xform] : session.xforms)
        if (scene.elements.count(guid)) { scene.set_xform(guid, xform); }
    for (const auto& [a, neighbours] : session.graph.edges)
        for (const auto& [b, edge] : neighbours)
            if (a < b && scene.elements.count(a) && scene.elements.count(b)) { scene.add_edge(a, b, edge.attribute); }

    // A wood scene is elements. Anything else the session held is not carried, and saying so is
    // the difference between a conversion and a quiet loss.
    if (session.lookup.size() > scene.elements.size()) {
        fprintf(stderr, "  WARNING: WoodSession::from_session: %zu of %zu objects are not elements "
                        "and were not carried over.\n",
                session.lookup.size() - scene.elements.size(), session.lookup.size());
        fflush(stderr);
    }
    return scene;
}

const Session& WoodSession::to_session() const {
    for (const std::shared_ptr<Element>& element : *objects.elements) {
        const auto it = elements.find(element->guid());
        if (it != elements.end())
            std::visit([](const auto& wood) { wood->to_element(); }, it->second);
    }
    // Second pass, and the order matters: WoodElement::to_element() replaces the whole
    // feature list, so a joint feature added before it would be wiped. Rebuilding the list
    // without the previous joint features is what makes writing twice idempotent for a
    // column or a solid, whose to_element() leaves the list alone.
    for (const std::shared_ptr<Element>& element : *objects.elements) {
        std::vector<ElementFeature> features;
        for (const ElementFeature& feature : element->features()) {
            if (feature.feature_type == "joint") { continue; }
            features.push_back(feature);
            // A copy mints a fresh guid; put the old one back.
            if (feature.has_guid()) { features.back().guid() = feature.guid(); }
        }
        for (ElementFeature& feature : get_element_features(element->guid()))
            features.push_back(std::move(feature));
        element->set_features(std::move(features));
    }
    return *this;
}

WoodSession WoodSession::pb_load(const std::filesystem::path& path) {
    const std::filesystem::path file = internal::dataset_path(path.string(), ".pb");
    if (!std::filesystem::exists(file)) {
        fmt::print(stderr, "not found: {}\n", file.string());
        return WoodSession{};
    }
    return from_session(Session::pb_load(file.string()));
}

WoodSession WoodSession::yaml_load(const std::filesystem::path& path) {
    globals::globals_yaml(path.string());
    WoodSession scene(globals::DATA_SET_INPUT_NAME);
    for (const WoodElement& plate : internal::load_plates(globals::DATA_SET_OBJ))
        scene.add(std::make_shared<WoodElement>(plate));
    return scene;
}

std::string WoodSession::str() const {
    std::ostringstream os;
    os << "WoodSession(name=" << name << ", elements=" << elements.size()
       << ", plates=" << plates().size() << ", columns=" << columns().size()
       << ", solids=" << solids().size()
       << ", edges=" << graph.number_of_edges() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodSession& scene) { return os << scene.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
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

std::filesystem::path pb_dump(const WoodSession& scene, const std::string& name) {
    scene.to_session();
    return pb_dump(static_cast<const Session&>(scene), name);
}

// ═══════════════════════════════════════════════════════════════════════════
// The contact and joint coloring scheme
// ═══════════════════════════════════════════════════════════════════════════

const char* contact_type_name(ContactType type) {
    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top:  return "side_top";
        case ContactType::top_top:   return "top_top";
        case ContactType::cross:     return "cross";
        case ContactType::line:      return "line";
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

// Palette: the BRG equilibrium drawings (brg-teaching.github.io), whose PAL is built for
// white paper - navy/pink/green carry the meaning, grey is anything inert. Taken as is
// rather than re-tuned, so a wood scene sits beside those drawings without clashing.
//
//   #1a1eb2 navy   #ce4095 pink   #3f9c20 green   #b9b9bd zero-grey
//   #e07a26 orange #e8ac00 yellow #a83179 deep pink
Color contact_color(ContactType type) {
    switch (type) {
        case ContactType::side_side: return Color(0.102f, 0.118f, 0.698f, 1.0f, "ss_navy");
        case ContactType::side_top:  return Color(0.808f, 0.251f, 0.584f, 1.0f, "ts_pink");
        case ContactType::top_top:   return Color(0.247f, 0.612f, 0.125f, 1.0f, "tt_green");
        // Same yellow as joint_color(30): a cross contact previews the same crossing a
        // solved type-30 joint would refine.
        case ContactType::cross:     return Color(0.910f, 0.675f, 0.000f, 1.0f, "cross_yellow");
        case ContactType::line:      return Color(0.086f, 0.635f, 0.667f, 1.0f, "line_teal");
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

void add_outlines(Session& session, const WoodSession& scene, const std::string& prefix) {
    const Group group = session.add_group(prefix);
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        const auto it = scene.elements.find(element->guid());
        if (it == scene.elements.end()) { continue; }
        std::visit([&session, &group](const auto& wood) {
            using Wood = std::decay_t<decltype(*wood)>;
            // A plate keeps the convention: [0] bottom, [1] top. A column or a solid has no
            // such pair, so every face outline it has is drawn instead.
            const size_t count = std::is_same_v<Wood, WoodElement> ? std::min<size_t>(2, wood->polylines.size())
                                                                   : wood->polylines.size();
            for (size_t i = 0; i < count; ++i) {
                auto outline = std::make_shared<Polyline>(wood->polylines[i]);
                outline->name = fmt::format("{}_{}", wood->element->name, i);
                session.add_polyline(outline, group);
            }
        }, it->second);
    }
}

/// The ring of a contact or a joint, as a line loop in its own colour. Wide on purpose: it is
/// read against the element outlines, and at hairline width the two are hard to tell apart.
static std::shared_ptr<Polyline> ring(const Polyline& area, const Color& color, const std::string& name) {
    auto outline = std::make_shared<Polyline>(area);
    outline->linecolor = color;
    outline->width = 3.0;
    outline->name = name;
    return outline;
}

void add_contacts_by_type(Session& session, const std::vector<ContactPair>& contacts,
                          const std::string& prefix) {
    // A group per class that actually occurs, so the tree never shows an empty "top_top" for
    // an assembly that has none.
    std::map<std::string, Group> groups;
    for (const ContactPair& pair : contacts) {
        for (const FaceContact& contact : pair.faces) {
            // A line contact's area is a 2-point open segment, not a closed polygon -
            // add_line_contacts_by_type draws those.
            if (contact.type == ContactType::line) { continue; }
            const std::string label = fmt::format("{}_{}", prefix, contact_type_name(contact.type));
            auto it = groups.find(label);
            if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
            session.add_polyline(ring(contact.area, contact_color(contact.type),
                                      fmt::format("contact_{}_{}_f{}_{}", pair.element_a, pair.element_b,
                                                  contact.face_a, contact.face_b)),
                                 it->second);
        }
    }
}

void add_line_contacts_by_type(Session& session, const std::vector<ContactPair>& contacts,
                               const std::string& prefix) {
    std::map<std::string, Group> groups;
    for (const ContactPair& pair : contacts) {
        for (const FaceContact& contact : pair.faces) {
            if (contact.type != ContactType::line) { continue; }
            const std::string label = fmt::format("{}_{}", prefix, contact_type_name(contact.type));
            auto it = groups.find(label);
            if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
            auto polyline = std::make_shared<Polyline>(contact.area);
            polyline->name = fmt::format("line_contact_{}_{}_l{}_{}", pair.element_a, pair.element_b,
                                         contact.face_a, contact.face_b);
            polyline->linecolor = contact_color(contact.type);
            session.add_polyline(polyline, it->second);
        }
    }
}

void add_joints_by_type(Session& session, const std::vector<WoodJoint>& joints,
                        const std::string& prefix) {
    std::map<std::string, Group> groups;
    for (const WoodJoint& joint : joints) {
        const std::string type_name = joint_type_name(joint.joint_type);
        const std::string label = fmt::format("{}_{}", prefix, type_name);
        auto it = groups.find(label);
        if (it == groups.end()) { it = groups.emplace(label, session.add_group(label)).first; }
        session.add_polyline(ring(joint.contact.area, joint_color(joint.joint_type),
                                  fmt::format("joint_{}_{}_{}", short_guid(joint.element_a),
                                              short_guid(joint.element_b), type_name)),
                             it->second);
    }
}

} // namespace wood_session
