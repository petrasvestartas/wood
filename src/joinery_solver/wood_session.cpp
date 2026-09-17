#include "wood_pch.h"
#include "wood_session.h"
#include "wood_face_to_face.h"

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

/// Registers the three element factories with the kernel; always returns true so a static can hold the result.
bool register_element_factories() {

    Plate::register_type();
    Column::register_type();
    Block::register_type();

    return true;
}

/// Registers the three element factories with the kernel, once.
void register_element_types() {
    static const bool done = register_element_factories();
    (void)done;
}

/// One detection view per element, in objects.elements order - the index space element_guids() and every ContactPair share.
std::vector<ContactElement> contact_view(const WoodSession& scene) {

    std::vector<ContactElement> view;
    view.reserve(scene.objects.elements->size());
    for (const std::shared_ptr<Element>& element : *scene.objects.elements)
        if (element)
            view.emplace_back(*element);

    return view;
}

/// First block of a guid - enough to tell two elements apart in an object name.
std::string short_guid(const std::string& guid) {
    return guid.substr(0, guid.find('-'));
}

/// Drops the contacts of one class from every edge, so a recompute of that class replaces rather than accumulates.
void erase_contacts_of_type(WoodSession& scene, ContactType type) {
    for (const auto& [a, b, interaction] : scene.get_interactions()) {
        WoodInteraction kept = interaction;
        kept.contacts.clear();
        for (const FaceContact& contact : interaction.contacts)
            if (contact.type != type)
                kept.contacts.push_back(contact);
        scene.set_interaction(a, b, kept);
    }
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodInteraction
// ═══════════════════════════════════════════════════════════════════════════

WoodInteraction WoodInteraction::flipped() const {

    WoodInteraction out = *this;
    for (FaceContact& contact : out.contacts)
        std::swap(contact.face_a, contact.face_b);

    return out;
}

nlohmann::ordered_json WoodInteraction::jsondump() const {

    nlohmann::ordered_json faces = nlohmann::ordered_json::array();
    for (const FaceContact& contact : contacts)
        faces.push_back(contact.jsondump());

    nlohmann::ordered_json cuts = nlohmann::ordered_json::array();
    for (const WoodJoint& joint : joints)
        cuts.push_back(joint.jsondump());

    return nlohmann::ordered_json{
        {"type", TYPE},
        {"contacts", faces},
        {"joints", cuts},
    };
}

WoodInteraction WoodInteraction::jsonload(const nlohmann::json& data) {

    WoodInteraction interaction;

    if (data.contains("contacts"))
        for (const nlohmann::json& contact : data["contacts"])
            interaction.contacts.push_back(FaceContact::jsonload(contact));

    if (data.contains("joints"))
        for (const nlohmann::json& joint : data["joints"])
            interaction.joints.push_back(WoodJoint::jsonload(joint));

    return interaction;
}

std::string WoodInteraction::to_attribute() const { return jsondump().dump(); }

/// The "type" key is the whole grammar: any other attribute on an edge comes back empty.
WoodInteraction WoodInteraction::from_attribute(const std::string& attribute) {

    if (attribute.empty() || attribute.front() != '{')
        return WoodInteraction{};

    nlohmann::json data;
    try {
        data = nlohmann::json::parse(attribute);
    } catch (const std::exception&) {
        return WoodInteraction{};
    }

    if (!data.is_object() || data.value("type", std::string()) != TYPE)
        return WoodInteraction{};

    return jsonload(data);
}

std::string WoodInteraction::str() const {
    std::ostringstream os;
    os << "WoodInteraction(contacts=" << contacts.size() << ", joints=" << joints.size() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodInteraction& interaction) { return os << interaction.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - elements
// ═══════════════════════════════════════════════════════════════════════════

WoodSession::WoodSession() { register_element_types(); }

WoodSession::WoodSession(const std::string& name) : Session(name) { register_element_types(); }

Group WoodSession::add(std::shared_ptr<Element> element, Group parent) {
    return add_element(std::move(element), std::move(parent));
}

std::vector<std::string> WoodSession::element_guids() const {

    std::vector<std::string> guids;
    guids.reserve(objects.elements->size());
    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (element)
            guids.push_back(element->guid());

    return guids;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - interactions
// ═══════════════════════════════════════════════════════════════════════════

/// Stored oriented to the low guid, so reading from the high one is the other end.
WoodInteraction WoodSession::get_interaction(const std::string& a, const std::string& b) const {

    const auto neighbours = graph.edges.find(a);
    if (neighbours == graph.edges.end())
        return WoodInteraction{};

    const auto edge = neighbours->second.find(b);
    if (edge == neighbours->second.end())
        return WoodInteraction{};

    const WoodInteraction interaction = WoodInteraction::from_attribute(edge->second.attribute);
    return a < b ? interaction : interaction.flipped();
}

/// A pair that already has an edge is written in place: Graph::add_edge would count an overwrite as one more edge.
void WoodSession::set_interaction(const std::string& a, const std::string& b, const WoodInteraction& interaction) {

    const std::string attribute = (a < b ? interaction : interaction.flipped()).to_attribute();
    const auto neighbours = graph.edges.find(a);
    if (neighbours != graph.edges.end() && neighbours->second.count(b)) {
        graph.edge_attribute(a, b, attribute);
        return;
    }

    add_edge(a, b, attribute);
}

/// Every edge is stored twice, once per direction; a < b takes each once, in the orientation it was written.
std::vector<std::tuple<std::string, std::string, WoodInteraction>> WoodSession::get_interactions() const {

    std::vector<std::tuple<std::string, std::string, WoodInteraction>> out;
    for (const auto& [a, neighbours] : graph.edges)
        for (const auto& [b, edge] : neighbours) {

            if (!(a < b))
                continue;

            WoodInteraction interaction = WoodInteraction::from_attribute(edge.attribute);
            if (!interaction.empty())
                out.emplace_back(a, b, std::move(interaction));
        }

    return out;
}

std::vector<ContactPair> WoodSession::contacts() const {

    const std::vector<std::string> guids = element_guids();
    std::unordered_map<std::string, int> index;
    for (size_t i = 0; i < guids.size(); ++i)
        index[guids[i]] = static_cast<int>(i);

    std::vector<ContactPair> pairs;
    for (const auto& [a, b, interaction] : get_interactions()) {

        if (interaction.contacts.empty())
            continue;

        const auto ia = index.find(a);
        const auto ib = index.find(b);
        if (ia == index.end() || ib == index.end())
            continue;

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

/// The side comes from the joint's own two elements: an edge is stored in both directions with the same object.
std::vector<ElementFeature> WoodSession::get_element_features(const std::string& guid) const {

    std::vector<ElementFeature> features;
    const auto neighbours = graph.edges.find(guid);
    if (neighbours == graph.edges.end())
        return features;

    for (const auto& [other, edge] : neighbours->second) {
        const WoodInteraction interaction = WoodInteraction::from_attribute(edge.attribute);
        for (const WoodJoint& joint : interaction.joints) {

            const int side = joint.element_a == guid ? 0 : (joint.element_b == guid ? 1 : -1);
            if (side < 0)
                continue;

            std::array<ElementFeature, 2> sides = joint.to_features();
            features.push_back(std::move(sides[side]));
        }
    }

    return features;
}

std::vector<std::pair<std::string, std::string>> WoodSession::get_collisions() {

    const std::vector<std::tuple<std::string, std::string, WoodInteraction>> kept = get_interactions();
    std::vector<std::pair<std::string, std::string>> pairs = Session::get_collisions();
    for (const auto& [a, b, interaction] : kept)
        set_interaction(a, b, interaction);

    return pairs;
}

void WoodSession::clear_contacts() {
    for (const auto& [a, b, interaction] : get_interactions()) {
        WoodInteraction kept = interaction;
        kept.contacts.clear();
        set_interaction(a, b, kept);
    }
}

void WoodSession::clear_joints() {
    for (const auto& [a, b, interaction] : get_interactions()) {
        WoodInteraction kept = interaction;
        kept.joints.clear();
        set_interaction(a, b, kept);
    }
}

/// Read before write: the pair may already carry joints, or a cross/line contact this call must not disturb.
void WoodSession::compute_face_contacts() {

    erase_contacts_of_type(*this, ContactType::side_side);
    erase_contacts_of_type(*this, ContactType::side_top);
    erase_contacts_of_type(*this, ContactType::top_top);
    erase_contacts_of_type(*this, ContactType::unknown);

    const std::vector<std::string> guids = element_guids();
    for (const ContactPair& pair : face_contacts(contact_view(*this))) {

        if (pair.element_a < 0 || pair.element_b < 0)
            continue;
        if (pair.element_a >= (int)guids.size() || pair.element_b >= (int)guids.size())
            continue;

        const std::string& a = guids[pair.element_a];
        const std::string& b = guids[pair.element_b];
        WoodInteraction interaction = get_interaction(a, b);
        interaction.contacts.insert(interaction.contacts.end(), pair.faces.begin(), pair.faces.end());
        set_interaction(a, b, interaction);
    }
}

void WoodSession::compute_contacts() { compute_face_contacts(); }

void WoodSession::compute_cross_contacts(double angle_tol) {

    erase_contacts_of_type(*this, ContactType::cross);

    const std::vector<std::shared_ptr<Plate>> plates = this->plates();
    for (size_t i = 0; i < plates.size(); ++i) {

        if (plates[i]->polylines.size() < 2 || plates[i]->planes.size() < 2)
            continue;

        for (size_t j = i + 1; j < plates.size(); ++j) {

            if (plates[j]->polylines.size() < 2 || plates[j]->planes.size() < 2)
                continue;

            CrossJoint cj;
            const bool crossing = plane_to_face(
                plates[i]->polylines[0], plates[i]->polylines[1],
                plates[j]->polylines[0], plates[j]->polylines[1],
                plates[i]->planes[0], plates[i]->planes[1],
                plates[j]->planes[0], plates[j]->planes[1],
                cj, angle_tol
            );
            if (!crossing)
                continue;

            FaceContact contact;
            contact.face_a = cj.face_ids_a.first;
            contact.face_b = cj.face_ids_b.first;
            contact.type = ContactType::cross;
            contact.area = cj.joint_area;

            const std::string& a = plates[i]->guid();
            const std::string& b = plates[j]->guid();
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

    std::vector<std::vector<std::vector<Line>>> lines(view.size());
    for (size_t a = 0; a < view.size(); ++a)
        for (const Polyline& loop : view[a].polylines)
            lines[a].push_back(loop.get_lines());

    for (size_t a = 0; a < view.size(); ++a) {
        for (size_t b = a + 1; b < view.size(); ++b) {
            for (size_t la = 0; la < lines[a].size(); ++la) {
                for (const Line& seg_a : lines[a][la]) {
                    for (size_t lb = 0; lb < lines[b].size(); ++lb) {
                        for (const Line& seg_b : lines[b][lb]) {

                            double t0 = 0.0;
                            double t1 = 0.0;
                            if (!Intersection::line_line_parameters(seg_a, seg_b, t0, t1, 0.0, true, true))
                                continue;

                            const Point q0 = seg_a.point_at(t0);
                            const Point q1 = seg_b.point_at(t1);
                            if ((q0 - q1).magnitude_squared() > tol_squared)
                                continue;

                            FaceContact contact;
                            contact.face_a = static_cast<int>(la);
                            contact.face_b = static_cast<int>(lb);
                            contact.type = ContactType::line;
                            contact.area = Polyline({q0, q1});

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

    std::vector<std::shared_ptr<Plate>> plates = this->plates();
    if (plates.empty())
        return;

    clear_joints();

    const std::vector<WoodJoint> joints = get_connection_zones(plates, search_type);
    for (const WoodJoint& joint : joints) {

        if (!get_element<Element>(joint.element_a) || !get_element<Element>(joint.element_b))
            continue;

        WoodInteraction interaction = get_interaction(joint.element_a, joint.element_b);
        interaction.joints.push_back(joint);
        set_interaction(joint.element_a, joint.element_b, interaction);
    }

    sync_joint_features();
}

void WoodSession::sync_joint_features() {
    for (const std::shared_ptr<Element>& element : *objects.elements) {

        std::vector<ElementFeature> features;
        for (const ElementFeature& feature : element->features()) {

            if (feature.feature_type == "joint")
                continue;

            features.push_back(feature);
            if (feature.has_guid())
                features.back().guid() = feature.guid();
        }

        for (ElementFeature& feature : get_element_features(element->guid()))
            features.push_back(std::move(feature));

        element->set_features(std::move(features));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - files
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::sync_geometry() const {
    for (const std::shared_ptr<Plate>& plate : plates())
        if (!plate->geometry_synced())
            plate->compute_geometry();
}

void WoodSession::pb_dump(const std::string& filename) const {
    sync_geometry();
    Session::pb_dump(filename);
}

std::string WoodSession::pb_dumps() const {
    sync_geometry();
    return Session::pb_dumps();
}

WoodSession WoodSession::pb_load(const std::filesystem::path& path) {

    register_element_types();

    const std::filesystem::path file = internal::dataset_path(path.string(), ".pb");
    WoodSession scene;
    if (!std::filesystem::exists(file)) {
        std::cerr << fmt::format("not found: {}\n", file.string());
        return scene;
    }

    static_cast<Session&>(scene) = Session::pb_load(file.string());
    return scene;
}

WoodSession WoodSession::yaml_load(const std::filesystem::path& path) {

    globals::globals_yaml(path.string());

    WoodSession scene(globals::DATA_SET_INPUT_NAME);
    for (const std::shared_ptr<Plate>& plate : internal::load_plates(globals::DATA_SET_OBJ))
        scene.add(plate);

    return scene;
}

std::string WoodSession::str() const {
    std::ostringstream os;
    os << "WoodSession(name=" << name << ", elements=" << objects.elements->size()
       << ", plates=" << plates().size() << ", columns=" << columns().size()
       << ", blocks=" << blocks().size()
       << ", edges=" << graph.number_of_edges() << ")";
    return os.str();
}
std::ostream& operator<<(std::ostream& os, const WoodSession& scene) { return os << scene.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// Writing a scene
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// A plate's merged outlines in the legacy interleaved layout: [hole0_top, hole0_bot, ..., outer_top, outer_bot].
std::vector<Polyline> merged_outlines(const Features& f) {

    std::vector<Polyline> merged;
    if (f.top.empty())
        return merged;

    merged.reserve(f.top.size() * 2);
    for (size_t i = 1; i < f.top.size(); i++) {
        merged.push_back(f.top[i]);
        merged.push_back(f.bottom[i]);
    }
    merged.push_back(f.top[0]);
    merged.push_back(f.bottom[0]);

    return merged;
}

}  // namespace

std::filesystem::path pb_path(const std::string& name) {
    const std::filesystem::path dir = internal::output_dir() / "pb";
    std::filesystem::create_directories(dir);
    return dir / (name + ".pb");
}

void write_parity_dumps(const WoodSession& scene, const std::filesystem::path& pb) {

    const std::vector<std::shared_ptr<Plate>> plates = scene.plates();
    std::ofstream meta(pb.string() + "_meta.txt");
    std::ofstream coords(pb.string() + "_coords.txt");

    for (size_t ei = 0; ei < plates.size(); ei++) {

        const std::vector<Polyline> merged = merged_outlines(plates[ei]->features);
        meta << merged.size();
        for (size_t mi = 0; mi < merged.size(); mi++)
            meta << ' ' << merged[mi].point_count();
        meta << '\n';

        coords << "element " << ei << "\n";
        for (size_t mi = 0; mi < merged.size(); mi++) {
            coords << "  poly " << mi << ":";
            for (size_t pi = 0; pi < merged[mi].point_count(); pi++) {
                const Point p = merged[mi].get_point(pi);
                coords << " " << p[0] << " " << p[1] << " " << p[2];
            }
            coords << "\n";
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

const char* contact_type_name(ContactType type) {

    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top: return "side_top";
        case ContactType::top_top: return "top_top";
        case ContactType::cross: return "cross";
        case ContactType::line: return "line";
        case ContactType::unknown: break;
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

/// A contact takes the colour of the joint class it refines to; an unclassified one is BRG's zero-grey.
Color contact_color(ContactType type) {

    switch (type) {
        case ContactType::side_side: return joint_color(12);
        case ContactType::side_top: return joint_color(20);
        case ContactType::top_top: return joint_color(40);
        case ContactType::cross: return joint_color(30);
        case ContactType::line: return Color(0.086f, 0.635f, 0.667f, 1.0f, "line_teal");
        case ContactType::unknown: break;
    }

    return joint_color(-1);
}

/// The BRG equilibrium palette (brg-teaching.github.io): navy, pink and green carry the meaning, grey is anything inert.
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

// ═══════════════════════════════════════════════════════════════════════════
// The viewer tree
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// The ring of a contact or a joint, as a wide line loop in its own colour.
std::shared_ptr<Polyline> ring(const Polyline& area, const Color& color, const std::string& name) {

    std::shared_ptr<Polyline> outline = std::make_shared<Polyline>(area);
    outline->linecolor = color;
    outline->width = 3.0;
    outline->name = name;

    return outline;
}

/// The tree node named by this guid, wherever it hangs; null when the element has none yet.
std::shared_ptr<TreeNode> node_of(const session_cpp::Tree& tree, const std::string& guid) {

    if (!tree.root())
        return nullptr;

    for (TreeNode* node : tree.root()->descendants())
        if (node->name == guid)
            return node->shared_from_this();

    return nullptr;
}

/// What an element draws as outlines: a solved plate its merged bottom and top with their holes, an unsolved plate its two outlines, anything else the faces of its mesh.
std::vector<Polyline> element_outlines(session_cpp::Element& element) {

    const Plate* plate = dynamic_cast<const Plate*>(&element);
    if (!plate)
        return element.polylines();

    if (!plate->features.top.empty()) {
        std::vector<Polyline> outlines = plate->features.bottom;
        outlines.insert(outlines.end(), plate->features.top.begin(), plate->features.top.end());
        return outlines;
    }

    return std::vector<Polyline>(plate->polylines.begin(), plate->polylines.begin() + std::min<size_t>(2, plate->polylines.size()));
}

/// The child group of `parent` called `name`, made on first use.
Group child_group(WoodSession& scene, std::map<std::string, Group>& made, const Group& parent, const std::string& name) {

    const std::string key = parent->name + "/" + name;
    const auto it = made.find(key);
    if (it != made.end())
        return it->second;

    const Group group = std::make_shared<TreeNode>(name);
    scene.add(group, parent);
    made[key] = group;

    return group;
}

}  // namespace

void WoodSession::add_to_tree(bool geometry, bool outlines, bool contacts, bool joints) {

    std::map<std::string, Group> groups;
    std::map<std::string, Group> children;

    size_t index = 0;
    for (const std::shared_ptr<Element>& element : *objects.elements) {

        if (!element)
            continue;

        const Group group = add_group(fmt::format("{}_{}", element->name, index++));
        groups[element->guid()] = group;

        if (geometry) {
            const std::shared_ptr<TreeNode> node = node_of(tree, element->guid());
            if (!node)
                add(std::make_shared<TreeNode>(element->guid()), group);
            else if (const std::shared_ptr<TreeNode> parent = node->parent()) {
                parent->remove(node);
                group->add(node);
            }
        }

        if (!outlines)
            continue;

        const Group child = child_group(*this, children, group, "outlines");
        for (const Polyline& outline : element_outlines(*element)) {
            std::shared_ptr<Polyline> copy = std::make_shared<Polyline>(outline);
            copy->name = fmt::format("{}_outline", element->name);
            add_polyline(copy, child);
        }
    }

    if (contacts)
        add_contacts_to(groups, children);

    if (joints)
        add_joints_to(groups, children);
}

void WoodSession::add_contacts_to(const std::map<std::string, Group>& groups, std::map<std::string, Group>& children) {

    const std::vector<std::string> guids = element_guids();
    for (const ContactPair& pair : this->contacts()) {

        const auto owner = groups.find(guids[pair.element_a]);
        if (owner == groups.end())
            continue;

        const Group group = child_group(*this, children, owner->second, "contacts");
        for (const FaceContact& contact : pair.faces) {

            const std::string name = fmt::format("contact_{}_{}_f{}_{}_{}", pair.element_a, pair.element_b, contact.face_a, contact.face_b, contact_type_name(contact.type));
            if (contact.type == ContactType::line) {
                add_polyline(ring(contact.area, contact_color(contact.type), name), group);
                continue;
            }

            std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{contact.area}));
            mesh->name = name;
            mesh->set_objectcolor(contact_color(contact.type));
            add_mesh(mesh, group);
        }
    }
}

void WoodSession::add_joints_to(const std::map<std::string, Group>& groups, std::map<std::string, Group>& children) {
    for (const WoodJoint& joint : this->joints()) {

        const auto male = groups.find(joint.element_a);
        const auto female = groups.find(joint.element_b);
        if (male == groups.end() || female == groups.end())
            continue;

        const Color color = joint_color(joint.joint_type);
        const std::string name = fmt::format("joint_{}_{}_{}", short_guid(joint.element_a), short_guid(joint.element_b), joint_type_name(joint.joint_type));
        const Group group = child_group(*this, children, male->second, "joints");

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{joint.contact.area}));
        mesh->name = name;
        mesh->set_objectcolor(color);
        add_mesh(mesh, group);

        for (const std::optional<Polyline>& volume : joint.joint_volumes_pair_a_pair_b)
            if (volume.has_value())
                add_polyline(ring(*volume, color, name + "_volume"), group);

        for (int k = 0; k < 2; ++k) {
            std::shared_ptr<Line> line = std::make_shared<Line>(joint.joint_lines[k]);
            line->linecolor = color;
            line->width = 3.0;
            line->name = fmt::format("{}_line{}", name, k);
            add_line(line, group);
        }

        for (const Polyline& outline : joint.male_outlines[0])
            add_polyline(ring(outline, color, name + "_male_bottom_cut"), group);
        for (const Polyline& outline : joint.male_outlines[1])
            add_polyline(ring(outline, color, name + "_male_top_cut"), group);

        const Group other = child_group(*this, children, female->second, "joints");
        for (const Polyline& outline : joint.female_outlines[0])
            add_polyline(ring(outline, color, name + "_female_bottom_cut"), other);
        for (const Polyline& outline : joint.female_outlines[1])
            add_polyline(ring(outline, color, name + "_female_top_cut"), other);
    }
}

}  // namespace wood_session
