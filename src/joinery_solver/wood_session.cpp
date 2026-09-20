#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection_beam.h"
#include "wood_session.pb.h"

namespace wood_session {

using namespace session_cpp;

namespace {

/// Registers the four element factories with the kernel; always returns true so a static can hold the result.
bool register_element_factories() {

    Plate::register_type();
    Column::register_type();
    Block::register_type();
    Beam::register_type();

    return true;
}

/// Registers the four element factories with the kernel, once.
void register_element_types() {
    static const bool done = register_element_factories();
    (void)done;
}

/// First block of a guid - enough to tell two elements apart in an object name.
std::string short_guid(const std::string& guid) {
    return guid.substr(0, guid.find('-'));
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession
// ═══════════════════════════════════════════════════════════════════════════

WoodSession::WoodSession() { register_element_types(); }

WoodSession::WoodSession(const std::string& name) : Session(name) { register_element_types(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Static constructors
// ═══════════════════════════════════════════════════════════════════════════

WoodSession WoodSession::pb_load(const std::filesystem::path& path) {

    register_element_types();

    const std::filesystem::path file = config::dataset_path(path.string(), ".pb");
    if (!std::filesystem::exists(file)) {
        std::cerr << fmt::format("not found: {}\n", file.string());
        return WoodSession();
    }

    std::ifstream in(file, std::ios::binary);
    const std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    return pb_loads(data);
}

/// The kernel reads its fields and skips field 100; the same bytes read as wood_proto.WoodSession give the interactions.
WoodSession WoodSession::pb_loads(const std::string& data) {

    register_element_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::pb_loads(data);
    scene.index_edges();

    wood_proto::WoodSession proto;
    proto.ParseFromString(data);
    for (const wood_proto::Interaction& entry : proto.interactions()) {
        Interaction interaction = Interaction::pb_loads(entry.SerializeAsString());
        if (scene._edges.count(interaction.guid))
            scene.interactions[interaction.guid] = std::move(interaction);
    }
    if (proto.has_settings())
        scene.settings = Settings::pb_loads(proto.settings().SerializeAsString());

    return scene;
}

WoodSession WoodSession::obj_load(const std::filesystem::path& path, double duplicate_pts_tol) {

    const std::vector<Polyline> polylines = config::load_obj(path.string(), duplicate_pts_tol);

    if (polylines.size() % 2 != 0)
        throw std::runtime_error("obj_load: unpaired outline in " + path.string());

    WoodSession scene(path.stem().string());
    for (size_t i = 0; i < polylines.size(); i += 2)
        scene.add(std::make_shared<Plate>(polylines[i], polylines[i + 1]));

    return scene;
}

WoodSession WoodSession::yaml_load(const std::filesystem::path& path) {

    const Settings settings = config::load_yaml(path.string());

    WoodSession scene = obj_load(config::DATA_SET_OBJ, settings.duplicate_points_tolerance);
    scene.settings = settings;
    scene.name = config::DATA_SET_INPUT_NAME;
    scene.load_sidecars();

    return scene;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const WoodSession& scene) { return os << scene.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Geometry
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::clear_contacts() {
    for (auto& [guid, interaction] : interactions) {
        interaction.contacts.clear();
        for (InteractionFeature& feature : interaction.features)
            feature.contact = -1;
    }
}

void WoodSession::clear_features() {
    for (auto& [guid, interaction] : interactions)
        interaction.features.clear();
}

/// The features' contact indices follow the erased entries; a feature whose contact went forgets it.
void WoodSession::erase_contacts(std::string_view kind) {
    for (auto& [guid, interaction] : interactions) {

        std::vector<int> remap(interaction.contacts.size(), -1);
        std::vector<InteractionContact> kept;
        for (size_t i = 0; i < interaction.contacts.size(); ++i) {
            if (interaction.contacts[i].kind() == kind)
                continue;
            remap[i] = static_cast<int>(kept.size());
            kept.push_back(std::move(interaction.contacts[i]));
        }

        for (InteractionFeature& feature : interaction.features)
            feature.contact = feature.contact >= 0 && feature.contact < (int)remap.size() ? remap[feature.contact] : -1;

        interaction.contacts = std::move(kept);
    }
}

void WoodSession::compute_face_contacts() {

    erase_contacts("face");

    const std::vector<std::string> guids = element_guids();
    for (const auto& [ia, ib, contact] : face_contacts(*objects.elements, settings))
        add_contact(guids[ia], guids[ib], InteractionContact(contact));
}

void WoodSession::compute_axis_contacts(double min_distance) {

    erase_contacts("axis");

    const std::vector<std::shared_ptr<Beam>> beams = this->beams();
    for (const auto& [ia, ib, contact] : axis_contacts(beams, min_distance))
        add_contact(beams[ia]->guid(), beams[ib]->guid(), InteractionContact(contact));
}

void WoodSession::compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male) {
    for (auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        const std::shared_ptr<Beam> beam_a = get_element<Beam>(ends.first);
        const std::shared_ptr<Beam> beam_b = get_element<Beam>(ends.second);
        if (!beam_a || !beam_b)
            continue;

        std::vector<InteractionFeature> kept;
        for (InteractionFeature& feature : interaction.features)
            if (!feature.beam())
                kept.push_back(std::move(feature));
        interaction.features = std::move(kept);

        for (size_t k = 0; k < interaction.contacts.size(); ++k) {

            const ContactAxis* axis = interaction.contacts[k].axis();
            FeatureBeam feature;
            if (!axis || !beam_to_beam(*beam_a, *beam_b, *axis, volume_length, cross_or_side_to_end, flip_male, feature))
                continue;

            InteractionFeature entry(feature);
            entry.contact = static_cast<int>(k);
            interaction.add_feature(std::move(entry));
        }
    }
}

void WoodSession::compute_contacts() { compute_face_contacts(); }

void WoodSession::compute_cross_contacts(double angle_tol) {

    erase_contacts("cross");

    const std::vector<std::shared_ptr<Plate>> plates = this->plates();
    for (size_t i = 0; i < plates.size(); ++i) {

        if (plates[i]->polylines.size() < 2 || plates[i]->planes.size() < 2)
            continue;

        for (size_t j = i + 1; j < plates.size(); ++j) {

            if (plates[j]->polylines.size() < 2 || plates[j]->planes.size() < 2)
                continue;

            ContactCross crossing;
            const bool crossed = plane_to_face(
                plates[i]->polylines[0], plates[i]->polylines[1],
                plates[j]->polylines[0], plates[j]->polylines[1],
                plates[i]->planes[0], plates[i]->planes[1],
                plates[j]->planes[0], plates[j]->planes[1],
                settings.distance_squared, crossing, angle_tol
            );
            if (crossed)
                add_contact(plates[i]->guid(), plates[j]->guid(), InteractionContact(crossing));
        }
    }
}

void WoodSession::compute_line_contacts(double tolerance) {

    erase_contacts("axis");

    const double tol = tolerance >= 0.0 ? tolerance : settings.distance;
    const double tol_squared = tol * tol;
    const std::vector<std::string> guids = element_guids();
    const std::vector<std::shared_ptr<Element>>& elements = *objects.elements;

    std::vector<std::vector<std::vector<Line>>> lines(elements.size());
    for (size_t a = 0; a < elements.size(); ++a)
        for (const Polyline& loop : elements[a]->polylines())
            lines[a].push_back(loop.get_lines());

    for (size_t a = 0; a < elements.size(); ++a)
        for (size_t b = a + 1; b < elements.size(); ++b)
            for (size_t la = 0; la < lines[a].size(); ++la)
                for (size_t sa = 0; sa < lines[a][la].size(); ++sa)
                    for (size_t lb = 0; lb < lines[b].size(); ++lb)
                        for (size_t sb = 0; sb < lines[b][lb].size(); ++sb) {

                            double t0 = 0.0;
                            double t1 = 0.0;
                            if (!Intersection::line_line_parameters(lines[a][la][sa], lines[b][lb][sb], t0, t1, 0.0, true, true))
                                continue;

                            const Point q0 = lines[a][la][sa].point_at(t0);
                            const Point q1 = lines[b][lb][sb].point_at(t1);
                            if ((q0 - q1).magnitude_squared() > tol_squared)
                                continue;

                            add_contact(guids[a], guids[b], InteractionContact(ContactAxis(Line::from_points(q0, q1), t0, t1, (int)la, (int)sa, (int)lb, (int)sb)));
                        }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Interactions
// ═══════════════════════════════════════════════════════════════════════════

/// The kernel copies an Edge without its guid into both directions, so the guid minted on one copy is written onto the other.
Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b) {

    if (!graph.has_edge(std::make_tuple(a, b)))
        add_edge(a, b);

    const Edge& edge = graph.edges[a][b];
    const std::string id = edge.guid();
    graph.edges[b][a].guid() = id;
    _edges[id] = {edge.v0, edge.v1};

    Interaction& interaction = interactions[id];
    interaction.guid = id;

    return interaction;
}

Interaction* WoodSession::get_interaction(const std::string& a, const std::string& b) {

    const auto row = graph.edges.find(a);
    if (row == graph.edges.end())
        return nullptr;

    const auto edge = row->second.find(b);
    if (edge == row->second.end() || !edge->second.has_guid())
        return nullptr;

    const auto found = interactions.find(edge->second.guid());
    return found == interactions.end() ? nullptr : &found->second;
}

const Interaction* WoodSession::get_interaction(const std::string& a, const std::string& b) const {
    return const_cast<WoodSession*>(this)->get_interaction(a, b);
}

const Interaction& WoodSession::get_interaction(const std::string& guid) const { return interactions.at(guid); }

std::pair<std::string, std::string> WoodSession::edge_of(const Interaction& interaction) const {

    const auto found = _edges.find(interaction.guid);
    return found == _edges.end() ? std::pair<std::string, std::string>{} : found->second;
}

std::string WoodSession::add_contact(const std::string& a, const std::string& b, InteractionContact contact) {

    Interaction& interaction = add_interaction(a, b);
    if (_edges[interaction.guid].first != a)
        contact = contact.flipped();

    return interaction.contacts[interaction.add_contact(std::move(contact))].guid;
}

namespace {

/// A cross joint's contact is the crossing itself: both side faces per element, the mid-plane polygon, its two lines and its two volumes; any other joint's is the face contact.
InteractionContact contact_of(const FeaturePlate& joint) {

    if (joint.joint_type != 30)
        return InteractionContact(joint.contact);

    ContactCross crossing;
    crossing.faces_a = {joint.contact.face_a, joint.cross_faces[0]};
    crossing.faces_b = {joint.contact.face_b, joint.cross_faces[1]};
    crossing.polygon = joint.contact.polygon;
    for (int k = 0; k < 2; ++k) {
        crossing.lines[k] = Polyline({joint.joint_lines[k].start(), joint.joint_lines[k].end()});
        if (joint.joint_volumes[k].has_value())
            crossing.volumes[k] = *joint.joint_volumes[k];
    }

    return InteractionContact(crossing);
}

}  // namespace

std::string WoodSession::add_joint(const FeaturePlate& joint) {

    Interaction& interaction = add_interaction(joint.element_a, joint.element_b);
    const bool reversed = _edges[interaction.guid].first != joint.element_a;

    const InteractionContact contact = contact_of(joint);
    InteractionFeature feature(joint);
    feature.contact = interaction.add_contact(reversed ? contact.flipped() : contact);
    feature.plate()->sync_features();

    return interaction.features[interaction.add_feature(std::move(feature))].guid;
}

std::vector<InteractionContact> WoodSession::get_contacts() const {

    std::vector<InteractionContact> out;
    for (const auto& [guid, interaction] : interactions)
        out.insert(out.end(), interaction.contacts.begin(), interaction.contacts.end());

    return out;
}

std::vector<InteractionFeature> WoodSession::get_features() const {

    std::vector<InteractionFeature> out;
    for (const auto& [guid, interaction] : interactions)
        out.insert(out.end(), interaction.features.begin(), interaction.features.end());

    return out;
}

std::vector<FeaturePlate> WoodSession::get_joints() const {

    std::vector<FeaturePlate> out;
    for (const auto& [guid, interaction] : interactions)
        for (const InteractionFeature& feature : interaction.features)
            if (const FeaturePlate* plate = feature.plate())
                out.push_back(*plate);

    return out;
}

/// Side [0] of a plate feature belongs to its element_a, side [1] to its element_b.
std::vector<ElementFeature> WoodSession::get_element_features(const std::string& guid) const {

    std::vector<ElementFeature> features;
    for (const auto& [id, interaction] : interactions)
        for (const InteractionFeature& feature : interaction.features) {

            const FeaturePlate* plate = feature.plate();
            if (!plate)
                continue;

            const int side = plate->element_a == guid ? 0 : (plate->element_b == guid ? 1 : -1);
            if (side < 0)
                continue;

            std::array<ElementFeature, 2> sides = plate->to_features();
            features.push_back(std::move(sides[side]));
        }

    return features;
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

/// Every edge once, its guid on both copies; edges without an interaction (bvh_collision) are indexed too, so a later add_interaction finds them.
void WoodSession::index_edges() {

    _edges.clear();
    for (const auto& [u, v] : graph.get_edges()) {
        const std::string id = graph.edges[u][v].guid();
        graph.edges[v][u].guid() = id;
        _edges[id] = {graph.edges[u][v].v0, graph.edges[u][v].v1};
    }
}

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
std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes_by_guid(const session_cpp::Tree& tree) {

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes;
    if (!tree.root())
        return nodes;

    for (TreeNode* node : tree.root()->descendants())
        nodes.emplace(node->name, node->shared_from_this());

    return nodes;
}

/// What an element draws as outlines: a solved plate its merged bottom and top with their holes, an unsolved plate its two outlines, anything else the faces of its mesh.
std::vector<Polyline> element_outlines(session_cpp::Element& element) {

    if (const Beam* beam = dynamic_cast<const Beam*>(&element))
        return {beam->axis};

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
std::shared_ptr<TreeNode> child_group(WoodSession& scene, std::map<std::string, std::shared_ptr<TreeNode>>& made, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::string key = parent->name + "/" + name;
    const auto it = made.find(key);
    if (it != made.end())
        return it->second;

    const std::shared_ptr<TreeNode> group = std::make_shared<TreeNode>(name);
    scene.add(group, parent);
    made[key] = group;

    return group;
}

}  // namespace

void WoodSession::add_to_tree(bool with_geometry, bool with_outlines, bool with_contacts, bool with_joints) {

    std::map<std::string, std::shared_ptr<TreeNode>> groups;
    std::map<std::string, std::shared_ptr<TreeNode>> children;

    const std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes = nodes_by_guid(tree);

    size_t index = 0;
    for (const std::shared_ptr<Element>& element : *objects.elements) {

        if (!element)
            continue;

        const std::shared_ptr<TreeNode> group = add_group(fmt::format("{}_{}", element->name, index++));
        groups[element->guid()] = group;

        if (with_geometry) {
            const std::unordered_map<std::string, std::shared_ptr<TreeNode>>::const_iterator found = nodes.find(element->guid());
            if (found == nodes.end())
                add(std::make_shared<TreeNode>(element->guid()), group);
            else if (const std::shared_ptr<TreeNode> parent = found->second->parent()) {
                parent->remove(found->second);
                group->add(found->second);
            }
        }

        if (!with_outlines)
            continue;

        const std::shared_ptr<TreeNode> child = child_group(*this, children, group, "outlines");
        for (const Polyline& outline : element_outlines(*element)) {
            std::shared_ptr<Polyline> copy = std::make_shared<Polyline>(outline);
            copy->name = fmt::format("{}_outline", element->name);
            add_polyline(copy, child);
        }
    }

    if (with_contacts)
        add_contacts_to(groups, children);

    if (with_joints)
        add_joints_to(groups, children);
}

void WoodSession::sync_geometry() const {
    for (const std::shared_ptr<Plate>& plate : plates())
        if (!plate->geometry_synced())
            plate->compute_geometry();
}

/// A face or cross contact draws its polygon as a region, an axis contact its segment as a line.
void WoodSession::add_contacts_to(const std::map<std::string, std::shared_ptr<TreeNode>>& groups, std::map<std::string, std::shared_ptr<TreeNode>>& children) {

    const std::vector<std::string> guids = element_guids();
    std::unordered_map<std::string, int> index;
    for (size_t i = 0; i < guids.size(); ++i)
        index[guids[i]] = static_cast<int>(i);

    for (const auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        const auto owner = groups.find(ends.first);
        if (owner == groups.end() || !index.count(ends.first) || !index.count(ends.second))
            continue;

        const std::shared_ptr<TreeNode> group = child_group(*this, children, owner->second, "contacts");
        const std::string prefix = fmt::format("contact_{}_{}", index[ends.first], index[ends.second]);
        for (const InteractionContact& contact : interaction.contacts) {

            if (const ContactAxis* axis = contact.axis()) {
                const std::string name = fmt::format("{}_s{}_{}_axis", prefix, axis->segment_a, axis->segment_b);
                add_polyline(ring(Polyline({axis->segment.start(), axis->segment.end()}), Color(0.086f, 0.635f, 0.667f, 1.0f, "axis_teal"), name), group);
                continue;
            }

            const ContactFace* face = contact.face();
            const ContactCross* cross = contact.cross();
            const std::string name = face
                ? fmt::format("{}_f{}_{}_{}", prefix, face->face_a, face->face_b, contact_type_name(face->type))
                : fmt::format("{}_f{}_{}_cross", prefix, cross->faces_a[0], cross->faces_b[0]);
            std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{face ? face->polygon : cross->polygon}));
            mesh->name = name;
            mesh->set_objectcolor(face ? contact_color(face->type) : joint_color(30));
            add_mesh(mesh, group);
        }
    }
}

void WoodSession::add_joints_to(const std::map<std::string, std::shared_ptr<TreeNode>>& groups, std::map<std::string, std::shared_ptr<TreeNode>>& children) {

    for (const auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        const auto owner = groups.find(ends.first);
        if (owner == groups.end())
            continue;

        for (const InteractionFeature& feature : interaction.features) {

            const FeatureBeam* beam = feature.beam();
            if (!beam)
                continue;

            const std::shared_ptr<TreeNode> group = child_group(*this, children, owner->second, "joints");
            const std::string name = fmt::format("beam_{}_{}_{}", short_guid(ends.first), short_guid(ends.second), beam->end_type);
            for (const Polyline& volume : beam->volumes)
                add_polyline(ring(volume, Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta"), name + "_volume"), group);
        }
    }

    for (const FeaturePlate& joint : get_joints()) {

        const auto male = groups.find(joint.element_a);
        const auto female = groups.find(joint.element_b);
        if (male == groups.end() || female == groups.end())
            continue;

        const Color color = joint_color(joint.joint_type);
        const std::string name = fmt::format("joint_{}_{}_{}", short_guid(joint.element_a), short_guid(joint.element_b), joint_type_name(joint.joint_type));
        const std::shared_ptr<TreeNode> group = child_group(*this, children, male->second, "joints");

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{joint.contact.polygon}));
        mesh->name = name;
        mesh->set_objectcolor(color);
        add_mesh(mesh, group);

        for (const std::optional<Polyline>& volume : joint.joint_volumes)
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

        const std::shared_ptr<TreeNode> other = child_group(*this, children, female->second, "joints");
        for (const Polyline& outline : joint.female_outlines[0])
            add_polyline(ring(outline, color, name + "_female_bottom_cut"), other);
        for (const Polyline& outline : joint.female_outlines[1])
            add_polyline(ring(outline, color, name + "_female_top_cut"), other);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::pb_dump(const std::string& filename) {
    const std::string data = pb_dumps();
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), data.size());
}

/// The kernel's bytes parse into the superset message field for field; the interactions follow in guid order.
std::string WoodSession::pb_dumps() {

    sync_geometry();

    wood_proto::WoodSession proto;
    proto.ParseFromString(Session::pb_dumps());
    for (const auto& [guid, interaction] : interactions)
        proto.add_interactions()->ParseFromString(interaction.pb_dumps());
    proto.mutable_settings()->ParseFromString(settings.pb_dumps());

    return proto.SerializeAsString();
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - String
// ═══════════════════════════════════════════════════════════════════════════

std::string WoodSession::str() const {
    std::ostringstream os;
    os << "WoodSession(name=" << name << ", elements=" << objects.elements->size()
       << ", plates=" << plates().size() << ", columns=" << columns().size()
       << ", blocks=" << blocks().size()
       << ", interactions=" << interactions.size() << ")";
    return os.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Assignment
// ═══════════════════════════════════════════════════════════════════════════

int WoodSession::nearest_slot(const Plate& plate, const Point& point, double threshold, bool faces) {

    if (plate.polylines.size() < 2)
        return -1;

    size_t segment_top = 0;
    size_t segment_bottom = 0;
    Point closest;
    const double distance_top = plate.polylines[1].closest_distance_and_point(point, segment_top, closest);
    const double distance_bottom = plate.polylines[0].closest_distance_and_point(point, segment_bottom, closest);
    const bool top = distance_top * distance_top <= distance_bottom * distance_bottom;
    const double nearest = top ? distance_top : distance_bottom;
    if (nearest * nearest >= threshold)
        return -1;

    if (faces)
        return top ? 1 : 0;

    return static_cast<int>(2 + (top ? segment_top : segment_bottom));
}

SpatialRTree<int, double, 3> WoodSession::plate_rtree(const std::vector<std::shared_ptr<Plate>>& plates, double radius) const {

    SpatialRTree<int, double, 3> rtree;
    for (size_t i = 0; i < plates.size(); i++) {

        if (plates[i]->polylines.empty())
            continue;

        const AABB box = plates[i]->aabb(radius);
        const Point lo = box.min_point();
        const Point hi = box.max_point();
        const double low[3] = {lo[0], lo[1], lo[2]};
        const double high[3] = {hi[0], hi[1], hi[2]};
        rtree.insert(low, high, static_cast<int>(i));
    }

    return rtree;
}

/// A slot per face: bottom, top, then one per side of the top outline.
static size_t slot_count(const Plate& plate) {
    const size_t n = plate.polylines.size() > 1 ? plate.polylines[1].point_count() : 0;
    return 2 + (n > 0 ? n - 1 : 0);
}

void WoodSession::assign_joint_types(const std::vector<Point>& points, const std::vector<int>& types) {

    const double threshold = settings.distance_squared * 100.0;
    const double radius = std::max(settings.distance, std::sqrt(threshold));
    const std::vector<std::shared_ptr<Plate>> plates = this->plates();
    for (const std::shared_ptr<Plate>& plate : plates)
        plate->joint_types.assign(slot_count(*plate), -1);

    if (points.empty() || types.size() < points.size())
        return;

    const SpatialRTree<int, double, 3> rtree = plate_rtree(plates, radius);
    for (size_t i = 0; i < points.size(); i++) {

        const Point& point = points[i];
        const int type = types[i];
        const double low[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double high[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(low, high, [&](const int index) {
            const int slot = nearest_slot(*plates[index], point, threshold, type < 0);
            if (slot >= 0 && slot < static_cast<int>(plates[index]->joint_types.size()))
                plates[index]->joint_types[slot] = std::abs(type);
            return true;
        });
    }
}

void WoodSession::assign_insertion_vectors(const std::vector<Line>& lines) {

    const double threshold = settings.distance_squared * 100.0;
    const double radius = std::max(settings.distance, std::sqrt(threshold));
    const std::vector<std::shared_ptr<Plate>> plates = this->plates();
    for (const std::shared_ptr<Plate>& plate : plates)
        plate->insertion_vectors().assign(slot_count(*plate), Vector(0.0, 0.0, 0.0));

    if (lines.empty())
        return;

    const SpatialRTree<int, double, 3> rtree = plate_rtree(plates, radius);
    for (const Line& line : lines) {

        const Point point = line.start();
        const Vector direction = line.to_vector();
        const double low[3] = {point[0] - radius, point[1] - radius, point[2] - radius};
        const double high[3] = {point[0] + radius, point[1] + radius, point[2] + radius};
        rtree.search(low, high, [&](const int index) {
            const int slot = nearest_slot(*plates[index], point, threshold, false);
            if (slot >= 0 && slot < static_cast<int>(plates[index]->insertion_vectors().size()))
                plates[index]->insertion_vectors()[slot] = direction;
            return true;
        });
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Elements
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> WoodSession::add(std::shared_ptr<Element> element, std::shared_ptr<TreeNode> parent) {
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
    const std::filesystem::path dir = config::output_dir() / "pb";
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

std::string_view contact_type_name(ContactType type) {

    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top: return "side_top";
        case ContactType::top_top: return "top_top";
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

}  // namespace wood_session
