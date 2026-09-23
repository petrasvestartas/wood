#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection_beam.h"
#include "wood_session.pb.h"
#include "wood_element_geometry.h"

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

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession
// ═══════════════════════════════════════════════════════════════════════════

WoodSession::WoodSession() { register_element_types(); }

WoodSession::WoodSession(const std::string& name) : Session(name) { register_element_types(); }

WoodSession::WoodSession(const WoodSession& other) : Session(other), settings(other.settings), interactions(other.interactions), adjacency(other.adjacency), three_valence(other.three_valence), definition_keys(other.definition_keys), _edges(other._edges) {
    claim_records();
}

WoodSession::WoodSession(WoodSession&& other) noexcept : Session(std::move(other)), settings(std::move(other.settings)), interactions(std::move(other.interactions)), adjacency(std::move(other.adjacency)), three_valence(std::move(other.three_valence)), definition_keys(std::move(other.definition_keys)), _edges(std::move(other._edges)) {
    claim_records();
}

WoodSession& WoodSession::operator=(const WoodSession& other) {

    if (this == &other)
        return *this;

    Session::operator=(other);
    settings = other.settings;
    interactions = other.interactions;
    adjacency = other.adjacency;
    three_valence = other.three_valence;
    definition_keys = other.definition_keys;
    _edges = other._edges;
    claim_records();

    return *this;
}

WoodSession& WoodSession::operator=(WoodSession&& other) noexcept {

    if (this == &other)
        return *this;

    Session::operator=(std::move(other));
    settings = std::move(other.settings);
    interactions = std::move(other.interactions);
    adjacency = std::move(other.adjacency);
    three_valence = std::move(other.three_valence);
    definition_keys = std::move(other.definition_keys);
    _edges = std::move(other._edges);
    claim_records();

    return *this;
}

void WoodSession::claim_records() {
    for (auto& [guid, interaction] : interactions)
        interaction.set_session(this);
}

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
        if (scene._edges.count(interaction.guid)) {
            interaction.set_session(&scene);
            scene.interactions[interaction.guid] = std::move(interaction);
        }
    }
    if (proto.has_settings())
        scene.settings = Settings::pb_loads(proto.settings().SerializeAsString());

    return scene;
}

WoodSession WoodSession::obj_load(const std::filesystem::path& path, double duplicate_pts_tol) {

    const std::vector<Polyline> polylines = io::load_obj(path.string(), duplicate_pts_tol);

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
    scene.load_sidecars(scene.world_elements<Plate>());

    return scene;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Operators
// ═══════════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const WoodSession& scene) { return os << scene.str(); }

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Geometry
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// A contact as the feature its edge's first element carries: the face or cross polygon, or the axis segment, in the colour of its class.
ElementFeature contact_feature(const InteractionContact& contact) {

    Polyline outline;
    int face = -1;
    std::string name = "axis";
    if (const ContactFace* touch = contact.face()) {
        outline = touch->polygon;
        outline.linecolor = contact_color(touch->type);
        face = touch->face_a;
        name = std::string(contact_type_name(touch->type));
    } else if (const ContactCross* cross = contact.cross()) {
        outline = cross->polygon;
        outline.linecolor = joint_color(30);
        name = "cross";
    } else if (const ContactAxis* axis = contact.axis()) {
        outline = Polyline({axis->segment.start(), axis->segment.end()});
        outline.linecolor = Color(0.086f, 0.635f, 0.667f, 1.0f, "axis_teal");
    }

    ElementFeature feature("contact", face, {outline}, name);
    feature.guid() = contact.guid;

    return feature;
}

/// Every outline of a feature in one colour.
void paint(ElementFeature& feature, const Color& color) {
    for (Polyline& outline : feature.outlines)
        outline.linecolor = color;
}

/// Every feature of every element but the ones `drop` picks, guids and visibility kept; an element without features is not read, so it does not loft.
void drop_features(const std::vector<std::shared_ptr<Element>>& elements, const std::function<bool(const ElementFeature&)>& drop) {
    for (const std::shared_ptr<Element>& element : elements) {

        if (!element || element->features_count() == 0)
            continue;

        std::vector<ElementFeature> features;
        for (const ElementFeature& feature : element->features()) {

            if (drop(feature))
                continue;

            features.push_back(feature);
            features.back().guid() = feature.guid();
        }

        element->set_features(std::move(features));
    }
}

/// The same over the instances' own features.
void drop_features(const std::vector<std::shared_ptr<InstanceRef>>& instances, const std::function<bool(const ElementFeature&)>& drop) {
    for (const std::shared_ptr<InstanceRef>& instance : instances)
        std::erase_if(instance->features, drop);
}

/// drop_features on the one element or instance guid names.
void drop_host_features(WoodSession& session, const std::string& guid, const std::function<bool(const ElementFeature&)>& drop) {

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = session.instance_lookup.find(guid);

    if (instance != session.instance_lookup.end())
        drop_features({instance->second}, drop);
    else
        drop_features({session.get_element<Element>(guid)}, drop);
}

}  // namespace

void WoodSession::clear_contacts() {
    for (auto& [guid, interaction] : interactions) {
        interaction.contacts.clear();
        for (InteractionFeature& feature : interaction.features)
            feature.contact = -1;
    }

    const std::function<bool(const ElementFeature&)> drop = [](const ElementFeature& feature) { return feature.feature_type == "contact"; };
    drop_features(*objects.elements, drop);
    drop_features(*objects.instances, drop);
}

void WoodSession::clear_features() {
    for (auto& [guid, interaction] : interactions)
        interaction.features.clear();

    const std::function<bool(const ElementFeature&)> drop = [](const ElementFeature& feature) { return feature.feature_type == "joint"; };
    drop_features(*objects.elements, drop);
    drop_features(*objects.instances, drop);
}

/// The features' contact indices follow the erased entries; a feature whose contact went forgets it.
void WoodSession::erase_contacts(std::string_view kind) {

    std::unordered_set<std::string> erased;
    for (auto& [guid, interaction] : interactions) {

        std::vector<int> remap(interaction.contacts.size(), -1);
        std::vector<InteractionContact> kept;
        for (size_t i = 0; i < interaction.contacts.size(); ++i) {
            if (interaction.contacts[i].kind() == kind) {
                erased.insert(interaction.contacts[i].guid);
                continue;
            }
            remap[i] = static_cast<int>(kept.size());
            kept.push_back(std::move(interaction.contacts[i]));
        }

        for (InteractionFeature& feature : interaction.features)
            feature.contact = feature.contact >= 0 && feature.contact < (int)remap.size() ? remap[feature.contact] : -1;

        interaction.contacts = std::move(kept);
    }

    const std::function<bool(const ElementFeature&)> drop = [&erased](const ElementFeature& feature) { return feature.feature_type == "contact" && erased.count(feature.guid()); };
    drop_features(*objects.elements, drop);
    drop_features(*objects.instances, drop);
}

/// Stored elements are lofted first, since a mesh gives their outlines; a view comes with its outlines seeded.
void WoodSession::compute_face_contacts(int level) {

    erase_contacts("face");

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes;
    for (const std::shared_ptr<TreeNode>& node : tree.nodes())
        nodes[node->name] = node;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        element->geometry_mesh();

    std::map<const TreeNode*, std::vector<std::shared_ptr<Element>>> branches;
    for (const std::shared_ptr<Element>& element : world_elements()) {

        const std::unordered_map<std::string, std::shared_ptr<TreeNode>>::const_iterator found = nodes.find(element->guid());
        const TreeNode* branch = nullptr;
        if (found != nodes.end()) {
            const std::vector<TreeNode*> ancestors = found->second->ancestors();
            branch = level < static_cast<int>(ancestors.size()) ? ancestors[ancestors.size() - 1 - level] : found->second.get();
        }

        branches[branch].push_back(element);
    }

    for (const auto& [branch, elements] : branches)
        for (const auto& [ia, ib, contact] : face_contacts(elements, settings))
            add_contact(elements[ia]->guid(), elements[ib]->guid(), InteractionContact(contact));
}

void WoodSession::compute_axis_contacts(double min_distance) {

    erase_contacts("axis");

    const std::vector<std::shared_ptr<Beam>> beams = world_elements<Beam>();
    for (const auto& [ia, ib, contact] : axis_contacts(beams, min_distance))
        add_contact(beams[ia]->guid(), beams[ib]->guid(), InteractionContact(contact));
}

void WoodSession::compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male) {

    std::unordered_map<std::string, std::shared_ptr<Beam>> beams;
    for (const std::shared_ptr<Beam>& beam : world_elements<Beam>())
        beams[beam->guid()] = beam;

    for (auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        if (!beams.count(ends.first) || !beams.count(ends.second))
            continue;

        const std::shared_ptr<Beam> beam_a = beams.at(ends.first);
        const std::shared_ptr<Beam> beam_b = beams.at(ends.second);

        std::unordered_set<std::string> erased;
        std::vector<InteractionFeature> kept;
        for (InteractionFeature& feature : interaction.features)
            if (feature.beam())
                erased.insert(feature.guid);
            else
                kept.push_back(std::move(feature));
        interaction.features = std::move(kept);
        drop_host_features(*this, ends.first, [&erased](const ElementFeature& feature) { return erased.count(feature.guid()) > 0; });

        for (size_t k = 0; k < interaction.contacts.size(); ++k) {

            const ContactAxis* axis = interaction.contacts[k].axis();
            FeatureBeam feature;
            if (!axis || !beam_to_beam(*beam_a, *beam_b, *axis, volume_length, cross_or_side_to_end, flip_male, feature))
                continue;

            InteractionFeature entry(feature);
            entry.contact = static_cast<int>(k);
            const int index = interaction.add_feature(std::move(entry));

            ElementFeature side("joint", -1, std::vector<Polyline>(feature.volumes.begin(), feature.volumes.end()), fmt::format("beam_{}", feature.end_type));
            side.guid() = interaction.features[index].guid;
            paint(side, Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta"));
            host_feature(ends.first, std::move(side));
        }
    }
}

void WoodSession::compute_contacts(int level) { compute_face_contacts(level); }

void WoodSession::compute_cross_contacts(double angle_tol) {

    erase_contacts("cross");

    const std::vector<std::shared_ptr<Plate>> plates = world_elements<Plate>();
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
    const std::vector<std::shared_ptr<Element>> elements = world_elements();

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

                            add_contact(elements[a]->guid(), elements[b]->guid(), InteractionContact(ContactAxis(Line::from_points(q0, q1), t0, t1, (int)la, (int)sa, (int)lb, (int)sb)));
                        }
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Interactions
// ═══════════════════════════════════════════════════════════════════════════

/// The kernel copies an Edge without its guid into both directions, so the guid minted on one copy is written onto the other.
Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b) {

    Session::add_interaction(a, b);

    const Edge& edge = graph.edges[a][b];
    const std::string id = edge.guid();
    graph.edges[b][a].guid() = id;
    _edges[id] = {edge.v0, edge.v1};

    Interaction& interaction = interactions[id];
    interaction.guid = id;
    interaction.set_session(this);

    return interaction;
}

Interaction* WoodSession::get_interaction(const std::string& a, const std::string& b) {
    return const_cast<Interaction*>(std::as_const(*this).get_interaction(a, b));
}

const Interaction* WoodSession::get_interaction(const std::string& a, const std::string& b) const {
    for (const auto& [first, second] : {std::pair{a, b}, std::pair{b, a}}) {
        const auto row = graph.edges.find(first);
        if (row == graph.edges.end())
            continue;
        const auto edge = row->second.find(second);
        if (edge == row->second.end() || !edge->second.has_guid())
            continue;
        const auto found = interactions.find(edge->second.guid());
        if (found != interactions.end())
            return &found->second;
    }
    return nullptr;
}

bool WoodSession::has_interaction(const std::string& a, const std::string& b) const {
    return Session::has_interaction(a, b);
}

void WoodSession::remove_interaction(const std::string& a, const std::string& b) {
    if (const Interaction* interaction = get_interaction(a, b)) {
        const std::string id = interaction->guid;
        std::unordered_set<std::string> erased;
        for (const InteractionContact& contact : interaction->contacts)
            erased.insert(contact.guid);
        for (const InteractionFeature& feature : interaction->features) {
            erased.insert(feature.guid);
            if (const FeaturePlate* plate = feature.plate()) {
                erased.insert(plate->feature_guid(0));
                erased.insert(plate->feature_guid(1));
            }
        }
        const auto drop = [&erased](const ElementFeature& feature) { return erased.count(feature.guid()) > 0; };
        drop_host_features(*this, a, drop);
        drop_host_features(*this, b, drop);
        interactions.erase(id);
    }
    std::erase_if(_edges, [&a, &b](const auto& item) {
        const auto& ends = item.second;
        return (ends.first == a && ends.second == b) || (ends.first == b && ends.second == a);
    });
    Session::remove_interaction(a, b);
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

    return interaction.contacts[place_contact(interaction, std::move(contact))].guid;
}

int WoodSession::place_contact(Interaction& interaction, InteractionContact contact) {

    const size_t count = interaction.contacts.size();
    const int index = interaction.add_contact(std::move(contact));
    if (interaction.contacts.size() > count)
        host_feature(_edges[interaction.guid].first, contact_feature(interaction.contacts[index]));

    return index;
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

std::string WoodSession::add_feature(const FeaturePlate& joint) {

    Interaction& interaction = add_interaction(joint.element_a, joint.element_b);
    const bool reversed = _edges[interaction.guid].first != joint.element_a;

    const InteractionContact contact = contact_of(joint);
    InteractionFeature feature(joint);
    feature.guid = joint.guid;
    feature.contact = place_contact(interaction, reversed ? contact.flipped() : contact);
    feature.plate()->sync_features();

    const int index = interaction.add_feature(std::move(feature));
    FeaturePlate& plate = *interaction.features[index].plate();
    plate.guid = interaction.features[index].guid;

    std::array<ElementFeature, 2> sides = plate.to_features();
    const std::array<std::string, 2> hosts = {plate.element_a, plate.element_b};
    for (int k = 0; k < 2; ++k) {
        paint(sides[k], joint_color(plate.joint_type));
        host_feature(hosts[k], std::move(sides[k]));
    }

    return plate.guid;
}

namespace {

void validate_manual_feature(const std::string& a, const std::string& b, const InteractionFeature& feature,
                             const std::vector<InteractionContact>& contacts) {
    if (feature.contact < -1 || feature.contact >= static_cast<int>(contacts.size()))
        throw std::invalid_argument("WoodSession::add_interaction: feature contact index is out of range");
    if (const FeaturePlate* plate = feature.plate()) {
        const bool unspecified = plate->element_a.empty() && plate->element_b.empty();
        const bool same_pair = (plate->element_a == a && plate->element_b == b) || (plate->element_a == b && plate->element_b == a);
        if (!unspecified && !same_pair)
            throw std::invalid_argument("WoodSession::add_interaction: plate feature belongs to a different pair");
        if (feature.contact >= 0 && !contacts[feature.contact].face() && !contacts[feature.contact].cross())
            throw std::invalid_argument("WoodSession::add_interaction: a plate feature requires a face or cross contact");
        if (feature.contact >= 0 && plate->joint_type == 30 && !contacts[feature.contact].cross())
            throw std::invalid_argument("WoodSession::add_interaction: a cross joint requires a cross contact");
    }
}

/// Copy an explicitly linked contact into the plate's solver fields, read from its male side.
void assign_plate_contact(FeaturePlate& plate, const InteractionContact& contact) {
    if (const ContactFace* face = contact.face()) {
        plate.contact = *face;
    } else if (const ContactCross* cross = contact.cross()) {
        plate.joint_type = 30;
        plate.contact.face_a = cross->faces_a[0];
        plate.contact.face_b = cross->faces_b[0];
        plate.contact.polygon = cross->polygon;
        plate.cross_faces = {cross->faces_a[1], cross->faces_b[1]};
        for (int k = 0; k < 2; ++k) {
            if (cross->lines[k].point_count() >= 2)
                plate.joint_lines[k] = Line::from_points(cross->lines[k].get_point(0), cross->lines[k].get_point(1));
            plate.joint_volumes[k] = cross->volumes[k];
        }
    }
}

} // namespace

Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b, Interaction incoming) {
    // Validate the whole incoming record before creating an edge or storing any contacts.
    for (const InteractionFeature& feature : incoming.features)
        validate_manual_feature(a, b, feature, incoming.contacts);

    Interaction& stored = add_interaction(a, b);
    const bool reversed = edge_of(stored).first != a;
    std::vector<int> remap;
    remap.reserve(incoming.contacts.size());
    for (InteractionContact& contact : incoming.contacts)
        remap.push_back(place_contact(stored, reversed ? contact.flipped() : std::move(contact)));
    for (InteractionFeature& feature : incoming.features) {
        if (feature.contact >= 0)
            feature.contact = remap[feature.contact];
        add_interaction(a, b, std::move(feature));
    }
    if (incoming.structure)
        stored.structure = std::move(incoming.structure);
    return stored;
}

Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b, InteractionContact contact) {
    add_contact(a, b, std::move(contact));
    return *get_interaction(a, b);
}

Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b, InteractionFeature feature) {
    const Interaction* existing = get_interaction(a, b);
    const std::vector<InteractionContact> empty;
    validate_manual_feature(a, b, feature, existing ? existing->contacts : empty);
    Interaction& stored = add_interaction(a, b);
    const auto ends = edge_of(stored);

    if (FeaturePlate* plate = feature.plate()) {
        if (plate->element_a.empty()) {
            plate->element_a = a;
            plate->element_b = b;
        }
        if (!feature.guid.empty())
            plate->guid = feature.guid;
        if (feature.contact >= 0) {
            const InteractionContact& contact = stored.contacts[feature.contact];
            assign_plate_contact(*plate, plate->element_a == ends.first ? contact : contact.flipped());
        }
        add_feature(*plate);
        return stored;
    }

    if (FeatureBeam* beam = std::get_if<FeatureBeam>(&feature.data)) {
        if (a != ends.first) {
            std::swap(beam->volumes[0], beam->volumes[2]);
            std::swap(beam->volumes[1], beam->volumes[3]);
        }
    }
    const int index = stored.add_feature(std::move(feature));
    const InteractionFeature& added = stored.features[index];
    if (const FeatureBeam* beam = added.beam()) {
        ElementFeature side("joint", -1, std::vector<Polyline>(beam->volumes.begin(), beam->volumes.end()), fmt::format("beam_{}", beam->end_type));
        side.guid() = added.guid;
        paint(side, Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta"));
        host_feature(ends.first, std::move(side));
    }
    return stored;
}

Interaction& WoodSession::add_interaction(const std::string& a, const std::string& b, InteractionStructure structure) {
    Interaction& stored = add_interaction(a, b);
    stored.structure = std::move(structure);
    return stored;
}

/// A plate feature keeps a copy of its contact; the copy must be the stored contact read from the feature's own side.
bool WoodSession::consistent() const {
    for (const auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        if (ends.first.empty())
            return false;

        for (const InteractionFeature& feature : interaction.features) {

            if (feature.guid.empty() || feature.contact < -1 || feature.contact >= (int)interaction.contacts.size())
                return false;

            const FeaturePlate* plate = feature.plate();
            if (!plate)
                continue;

            const bool reversed = plate->element_a == ends.second;
            if (plate->guid != feature.guid || (!reversed && plate->element_a != ends.first) || (reversed && plate->element_b != ends.first))
                return false;

            if (feature.contact < 0)
                continue;
            const InteractionContact stored = reversed ? interaction.contacts[feature.contact].flipped() : interaction.contacts[feature.contact];
            if (!contact_of(*plate).coincides(stored))
                return false;
        }
    }

    return true;
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

std::vector<FeaturePlate> WoodSession::get_plate_features() const {

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

void WoodSession::set_features_visible(std::string_view feature_type, bool visible) {
    for (const std::shared_ptr<Element>& element : *objects.elements) {

        std::vector<ElementFeature> features;
        for (const ElementFeature& feature : element->features()) {

            features.push_back(feature);
            features.back().guid() = feature.guid();
            if (feature.feature_type == feature_type)
                features.back().visible = visible;
        }

        element->set_features(std::move(features));
    }

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        for (ElementFeature& feature : instance->features)
            if (feature.feature_type == feature_type)
                feature.visible = visible;
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

namespace {

/// The name of the element or instance guid names, the guid itself when the scene holds neither.
std::string name_of(const WoodSession& scene, const std::string& guid) {

    if (const std::shared_ptr<Element> element = scene.get_element<Element>(guid))
        return element->name;

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = scene.instance_lookup.find(guid);

    return instance == scene.instance_lookup.end() ? guid : instance->second->name;
}

}  // namespace

std::string WoodSession::str() const {

    const std::string bar(80, '=');
    std::ostringstream os;
    os << Session::str() << "Joints\n" << bar << "\n";

    for (const auto& [guid, interaction] : interactions) {

        const auto [a, b] = edge_of(interaction);
        os << name_of(*this, a) << " -- " << name_of(*this, b)
           << " : " << interaction.contacts.size() << " contacts, " << interaction.features.size() << " features";

        std::set<std::string_view> kinds;
        for (const InteractionFeature& feature : interaction.features)
            kinds.insert(feature.kind());

        for (const std::string_view kind : kinds)
            os << (kind == *kinds.begin() ? " (" : ", ") << kind;

        if (!kinds.empty())
            os << ")";

        os << "\n";
    }

    os << bar << "\n";

    return os.str();
}

std::string WoodSession::repr() const {
    std::ostringstream os;
    os << "WoodSession(name=" << name << ", elements=" << objects.elements->size()
       << ", plates=" << plates().size() << ", columns=" << columns().size()
       << ", blocks=" << blocks().size() << ", definitions=" << definition_lookup.size()
       << ", instances=" << objects.instances->size() << ", interactions=" << interactions.size() << ")";
    return os.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Elements
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<TreeNode> WoodSession::add(std::shared_ptr<Element> element, std::shared_ptr<TreeNode> parent) {
    return add_element(std::move(element), std::move(parent));
}

std::vector<std::string> WoodSession::element_guids() const {

    std::vector<std::string> guids;
    for (const std::shared_ptr<Element>& element : world_elements())
        guids.push_back(element->guid());

    return guids;
}

}  // namespace wood_session
