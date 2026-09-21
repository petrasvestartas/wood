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

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession
// ═══════════════════════════════════════════════════════════════════════════

WoodSession::WoodSession() { register_element_types(); }

WoodSession::WoodSession(const std::string& name) : Session(name) { register_element_types(); }

WoodSession::WoodSession(const WoodSession& other) : Session(other), settings(other.settings), interactions(other.interactions), adjacency(other.adjacency), three_valence(other.three_valence), _edges(other._edges) {
    claim_records();
}

WoodSession::WoodSession(WoodSession&& other) noexcept : Session(std::move(other)), settings(std::move(other.settings)), interactions(std::move(other.interactions)), adjacency(std::move(other.adjacency)), three_valence(std::move(other.three_valence)), _edges(std::move(other._edges)) {
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
    interaction.set_session(this);

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

std::string WoodSession::add_feature(const FeaturePlate& joint) {

    Interaction& interaction = add_interaction(joint.element_a, joint.element_b);
    const bool reversed = _edges[interaction.guid].first != joint.element_a;

    const InteractionContact contact = contact_of(joint);
    InteractionFeature feature(joint);
    feature.guid = joint.guid;
    feature.contact = interaction.add_contact(reversed ? contact.flipped() : contact);
    feature.plate()->sync_features();

    const int index = interaction.add_feature(std::move(feature));
    interaction.features[index].plate()->guid = interaction.features[index].guid;

    return interaction.features[index].guid;
}

/// A plate feature keeps a copy of its contact; the copy must be the stored contact read from the feature's own side.
bool WoodSession::consistent() const {
    for (const auto& [guid, interaction] : interactions) {

        const std::pair<std::string, std::string> ends = edge_of(interaction);
        if (ends.first.empty())
            return false;

        for (const InteractionFeature& feature : interaction.features) {

            if (feature.guid.empty() || feature.contact < 0 || feature.contact >= (int)interaction.contacts.size())
                return false;

            const FeaturePlate* plate = feature.plate();
            if (!plate)
                continue;

            const bool reversed = plate->element_a == ends.second;
            if (plate->guid != feature.guid || (!reversed && plate->element_a != ends.first) || (reversed && plate->element_b != ends.first))
                return false;

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

void WoodSession::add_to_tree(bool with_geometry, bool with_attributes, bool with_contacts, bool with_joints) {
    wood_session::add_to_tree(*this, with_geometry, with_attributes, with_contacts, with_joints);
}

void WoodSession::show_attributes(bool on) {
    wood_session::show_attributes(*this, on);
}

void WoodSession::sync_geometry() const {

    for (const std::shared_ptr<Plate>& plate : plates())
        if (!plate->geometry_synced())
            plate->compute_geometry();

    for (const std::shared_ptr<Beam>& beam : beams())
        if (!beam->geometry_synced())
            beam->compute_geometry();

    for (const std::shared_ptr<Column>& column : columns())
        if (!column->geometry_synced())
            column->compute_geometry();

    for (const std::shared_ptr<Block>& block : blocks())
        if (!block->geometry_synced())
            block->compute_geometry();
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

}  // namespace wood_session
