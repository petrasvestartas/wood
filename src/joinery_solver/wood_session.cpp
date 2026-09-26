#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection_beam.h"
#include "wood_session.pb.h"
#include "wood_element_geometry.h"

namespace wood_session {

using namespace session_cpp;

namespace {

/// Registers the element and interaction factories with the kernel; always returns true so a static can hold the result.
bool register_factories() {

    Plate::register_type();
    Column::register_type();
    Block::register_type();
    Beam::register_type();
    InteractionContactFace::register_type();
    InteractionContactAxis::register_type();
    InteractionContactCross::register_type();
    InteractionFeaturePlate::register_type();
    InteractionFeatureBeam::register_type();
    InteractionFeaturePlateBeam::register_type();

    return true;
}

/// Registers the element and interaction factories with the kernel, once.
void register_types() {
    static const bool done = register_factories();
    (void)done;
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession
// ═══════════════════════════════════════════════════════════════════════════

WoodSession::WoodSession() {
    register_types();
}

WoodSession::WoodSession(const std::string& name) : Session(name) {
    register_types();
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Static constructors
// ═══════════════════════════════════════════════════════════════════════════

WoodSession WoodSession::pb_load(const std::filesystem::path& path) {

    register_types();

    const std::filesystem::path file = config::dataset_path(path.string(), ".pb");
    if (!std::filesystem::exists(file)) {
        std::cerr << fmt::format("not found: {}\n", file.string());
        return WoodSession();
    }

    std::ifstream in(file, std::ios::binary);
    const std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    return pb_loads(data);
}

/// The kernel reads its fields, the interactions as their registered wood types, and skips the settings; the same bytes read as wood_proto.WoodSession give the settings.
WoodSession WoodSession::pb_loads(const std::string& data) {

    register_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::pb_loads(data);

    wood_proto::WoodSession proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse WoodSession protobuf data");

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

std::ostream& operator<<(std::ostream& os, const WoodSession& scene) {
    return os << scene.str();
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Geometry
// ═══════════════════════════════════════════════════════════════════════════

namespace {

/// A contact as the feature its edge's first element carries: the face or cross polygon, or the axis segment, named by its class.
ElementFeature contact_feature(const InteractionContact& contact) {

    Polyline outline;
    int face = -1;
    std::string name = "axis";

    if (const InteractionContactFace* touch = dynamic_cast<const InteractionContactFace*>(&contact)) {
        outline = touch->polygon;
        face = touch->face_a;
        name = std::string(contact_type_name(touch->type));
    } else if (const InteractionContactCross* cross = dynamic_cast<const InteractionContactCross*>(&contact)) {
        outline = cross->polygon;
        name = "cross";
    } else if (const InteractionContactAxis* axis = dynamic_cast<const InteractionContactAxis*>(&contact)) {
        outline = Polyline({axis->segment.start(), axis->segment.end()});
    }

    ElementFeature feature("contact", face, {outline}, name);
    feature.guid() = contact.guid();

    return feature;
}

/// True when a feature goes: of the type when one is given, else one of the guids.
bool is_dropped(const ElementFeature& feature, std::string_view type, const std::unordered_set<std::string>& guids) {
    return type.empty() ? guids.count(feature.guid()) > 0 : feature.feature_type == type;
}

/// Every feature of the element but the ones is_dropped picks, guids and visibility kept; an element without features is not read, so it does not loft.
void drop_features(const std::shared_ptr<Element>& element, std::string_view type, const std::unordered_set<std::string>& guids) {

    if (!element || element->features_count() == 0)
        return;

    std::vector<ElementFeature> features;
    for (const ElementFeature& feature : element->features()) {

        if (is_dropped(feature, type, guids))
            continue;

        features.push_back(feature);
        features.back().guid() = feature.guid();
    }

    element->set_features(std::move(features));
}

/// The same over an instance's own features.
void drop_instance_features(const std::shared_ptr<InstanceRef>& instance, std::string_view type, const std::unordered_set<std::string>& guids) {

    std::vector<ElementFeature> features;
    for (const ElementFeature& feature : instance->features)
        if (!is_dropped(feature, type, guids))
            features.push_back(feature);

    instance->features = std::move(features);
}

/// drop_features on the one element or instance guid names.
void drop_host_features(WoodSession& session, const std::string& guid, std::string_view type, const std::unordered_set<std::string>& guids) {

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = session.instance_lookup.find(guid);

    if (instance != session.instance_lookup.end())
        drop_instance_features(instance->second, type, guids);
    else
        drop_features(session.get_element<Element>(guid), type, guids);
}

}  // namespace

void WoodSession::clear_features() {

    for (std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions) {

        std::vector<std::shared_ptr<Interaction>> kept;

        for (const std::shared_ptr<Interaction>& interaction : entry.second)
            if (!dynamic_cast<const InteractionFeature*>(interaction.get()))
                kept.push_back(interaction);

        entry.second = std::move(kept);
    }

    const std::unordered_set<std::string> none;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        drop_features(element, "joint", none);

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        drop_instance_features(instance, "joint", none);
}

void WoodSession::erase_contacts(std::string_view kind) {

    std::unordered_set<std::string> erased;

    for (std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions) {

        std::vector<std::shared_ptr<Interaction>> kept;

        for (const std::shared_ptr<Interaction>& interaction : entry.second) {

            const InteractionContact* contact = dynamic_cast<const InteractionContact*>(interaction.get());

            if (contact && contact->kind() == kind)
                erased.insert(contact->guid());
            else
                kept.push_back(interaction);
        }

        entry.second = std::move(kept);
    }

    for (std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second) {

            InteractionFeature* feature = dynamic_cast<InteractionFeature*>(interaction.get());

            if (feature && erased.count(feature->contact_guid))
                feature->contact_guid.clear();
        }

    for (const std::shared_ptr<Element>& element : *objects.elements)
        drop_features(element, "", erased);

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        drop_instance_features(instance, "", erased);
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
            add_interaction(elements[ia], elements[ib], std::make_shared<InteractionContactFace>(contact));
}

void WoodSession::compute_axis_contacts(double min_distance) {

    erase_contacts("axis");

    const std::vector<std::shared_ptr<Beam>> beams = world_elements<Beam>();
    for (const auto& [ia, ib, contact] : axis_contacts(beams, min_distance))
        add_interaction(beams[ia], beams[ib], std::make_shared<InteractionContactAxis>(contact));
}

/// Each beam edge's earlier beam features go first, then one per axis contact, oriented to the edge's first beam.
void WoodSession::compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male) {

    std::unordered_map<std::string, std::shared_ptr<Beam>> beams;

    for (const std::shared_ptr<Beam>& beam : world_elements<Beam>())
        beams[beam->guid()] = beam;

    for (const std::tuple<std::string, std::string>& pair : graph.get_edges()) {

        const Edge& edge = graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::map<std::string, std::vector<std::shared_ptr<Interaction>>>::iterator found = interactions.find(edge.guid());

        if (found == interactions.end() || !beams.count(edge.v0) || !beams.count(edge.v1))
            continue;

        const std::shared_ptr<Beam> beam_a = beams.at(edge.v0);
        const std::shared_ptr<Beam> beam_b = beams.at(edge.v1);
        std::unordered_set<std::string> erased;
        std::vector<std::shared_ptr<Interaction>> kept;

        for (const std::shared_ptr<Interaction>& interaction : found->second)
            if (dynamic_cast<const InteractionFeatureBeam*>(interaction.get()))
                erased.insert(interaction->guid());
            else
                kept.push_back(interaction);

        found->second = kept;
        drop_host_features(*this, edge.v0, "", erased);

        for (const std::shared_ptr<Interaction>& interaction : kept) {

            const InteractionContactAxis* axis = dynamic_cast<const InteractionContactAxis*>(interaction.get());
            std::shared_ptr<InteractionFeatureBeam> feature = std::make_shared<InteractionFeatureBeam>();

            if (!axis || !beam_to_beam(*beam_a, *beam_b, *axis, volume_length, cross_or_side_to_end, flip_male, *feature))
                continue;

            feature->contact_guid = axis->guid();
            add_interaction(beam_a, beam_b, feature);
        }
    }
}

void WoodSession::compute_contacts(int level) {
    compute_face_contacts(level);
}

void WoodSession::compute_cross_contacts(double angle_tol) {

    erase_contacts("cross");

    const std::vector<std::shared_ptr<Plate>> plates = world_elements<Plate>();
    for (size_t i = 0; i < plates.size(); ++i) {

        if (plates[i]->polylines.size() < 2 || plates[i]->planes.size() < 2)
            continue;

        for (size_t j = i + 1; j < plates.size(); ++j) {

            if (plates[j]->polylines.size() < 2 || plates[j]->planes.size() < 2)
                continue;

            InteractionContactCross crossing;
            const bool crossed = plane_to_face(
                plates[i]->polylines[0], plates[i]->polylines[1],
                plates[j]->polylines[0], plates[j]->polylines[1],
                plates[i]->planes[0], plates[i]->planes[1],
                plates[j]->planes[0], plates[j]->planes[1],
                settings.distance_squared, crossing, angle_tol
            );
            if (crossed)
                add_interaction(plates[i], plates[j], std::make_shared<InteractionContactCross>(crossing));
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

                            add_interaction(elements[a], elements[b], std::make_shared<InteractionContactAxis>(Line::from_points(q0, q1), t0, t1, (int)la, (int)sa, (int)lb, (int)sb));
                        }
}

/// A plate feature keeps a copy of its contact; the copy must be the stored contact read from the feature's own side.
bool WoodSession::consistent() const {

    for (const std::tuple<std::string, std::string>& pair : graph.get_edges()) {

        const Edge& edge = graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::map<std::string, std::vector<std::shared_ptr<Interaction>>>::const_iterator found = interactions.find(edge.guid());

        if (found == interactions.end())
            continue;

        std::unordered_map<std::string, const InteractionContact*> contacts;

        for (const std::shared_ptr<Interaction>& interaction : found->second)
            if (const InteractionContact* contact = dynamic_cast<const InteractionContact*>(interaction.get()))
                contacts[contact->guid()] = contact;

        for (const std::shared_ptr<Interaction>& interaction : found->second) {

            const InteractionFeature* feature = dynamic_cast<const InteractionFeature*>(interaction.get());

            if (!feature || feature->contact_guid.empty())
                continue;

            if (!contacts.count(feature->contact_guid))
                return false;

            const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(feature);

            if (!plate)
                continue;

            const bool reversed = plate->element_a == edge.v1;

            if ((!reversed && plate->element_a != edge.v0) || (reversed && plate->element_b != edge.v0))
                return false;

            const InteractionContact& contact = *contacts.at(feature->contact_guid);

            if (!plate->to_contact()->coincides(reversed ? *contact.flipped() : contact))
                return false;
        }
    }

    return true;
}

std::vector<std::shared_ptr<InteractionContact>> WoodSession::get_contacts() const {

    std::vector<std::shared_ptr<InteractionContact>> out;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second)
            if (const std::shared_ptr<InteractionContact> contact = std::dynamic_pointer_cast<InteractionContact>(interaction))
                out.push_back(contact);

    return out;
}

std::vector<std::shared_ptr<InteractionFeature>> WoodSession::get_features() const {

    std::vector<std::shared_ptr<InteractionFeature>> out;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second)
            if (const std::shared_ptr<InteractionFeature> feature = std::dynamic_pointer_cast<InteractionFeature>(interaction))
                out.push_back(feature);

    return out;
}

std::vector<InteractionFeaturePlate> WoodSession::get_plate_features() const {

    std::vector<InteractionFeaturePlate> out;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second)
            if (const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get()))
                out.push_back(*plate);

    return out;
}

/// Side [0] of a plate feature belongs to its element_a, side [1] to its element_b.
std::vector<ElementFeature> WoodSession::get_element_features(const std::string& guid) const {

    std::vector<ElementFeature> features;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second) {

            const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get());

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

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Interactions
// ═══════════════════════════════════════════════════════════════════════════

/// The edge's first element is v0 of the stored edge, or a when the pair has no edge yet.
std::shared_ptr<Interaction> WoodSession::add_interaction(
    const std::shared_ptr<Element>& a,
    const std::shared_ptr<Element>& b,
    std::shared_ptr<Interaction> interaction
) {

    const bool known = graph.has_edge({a->guid(), b->guid()});
    const std::string first = known ? graph.edges.at(a->guid()).at(b->guid()).v0 : a->guid();
    const bool reversed = first != a->guid();

    if (const InteractionContact* contact = dynamic_cast<const InteractionContact*>(interaction.get())) {

        const std::shared_ptr<Interaction> oriented = reversed ? contact->flipped() : interaction;
        const InteractionContact& placed = *dynamic_cast<const InteractionContact*>(oriented.get());

        for (const std::shared_ptr<Interaction>& stored : get_interaction(a, b)) {

            const InteractionContact* other = dynamic_cast<const InteractionContact*>(stored.get());

            if (other && other->coincides(placed))
                return stored;
        }

        Session::add_interaction(a, b, oriented);
        host_feature(first, contact_feature(placed));

        return oriented;
    }

    if (InteractionFeaturePlate* plate = dynamic_cast<InteractionFeaturePlate*>(interaction.get())) {

        if (plate->element_a.empty()) {
            plate->element_a = a->guid();
            plate->element_b = b->guid();
        }

        plate->sync_features();
        Session::add_interaction(a, b, interaction);
        std::array<ElementFeature, 2> sides = plate->to_features();
        host_feature(plate->element_a, std::move(sides[0]));
        host_feature(plate->element_b, std::move(sides[1]));

        return interaction;
    }

    if (InteractionFeatureBeam* beam = dynamic_cast<InteractionFeatureBeam*>(interaction.get())) {

        if (reversed) {
            std::swap(beam->volumes[0], beam->volumes[2]);
            std::swap(beam->volumes[1], beam->volumes[3]);
        }

        Session::add_interaction(a, b, interaction);
        ElementFeature side("joint", -1, std::vector<Polyline>(beam->volumes.begin(), beam->volumes.end()), fmt::format("beam_{}", beam->end_type));
        side.guid() = beam->guid();
        host_feature(first, std::move(side));

        return interaction;
    }

    return Session::add_interaction(a, b, interaction);
}

void WoodSession::remove_interaction(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {

    std::unordered_set<std::string> erased;

    for (const std::shared_ptr<Interaction>& interaction : get_interaction(a, b)) {

        erased.insert(interaction->guid());

        if (const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get())) {
            erased.insert(plate->feature_guid(0));
            erased.insert(plate->feature_guid(1));
        }
    }

    drop_host_features(*this, a->guid(), "", erased);
    drop_host_features(*this, b->guid(), "", erased);
    Session::remove_interaction(a, b);
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::pb_dump(const std::string& filename) {
    const std::string data = pb_dumps();
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), data.size());
}

/// The kernel's bytes, the interactions among them, parse into the superset message field for field; the settings follow.
std::string WoodSession::pb_dumps() {

    wood_proto::WoodSession proto;
    if (!proto.ParseFromString(Session::pb_dumps()))
        throw std::runtime_error("Failed to parse WoodSession protobuf data");
    if (!proto.mutable_settings()->ParseFromString(settings.pb_dumps()))
        throw std::runtime_error("Failed to parse Settings protobuf data");

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

    for (const std::tuple<std::string, std::string>& pair : graph.get_edges()) {

        const Edge& edge = graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::map<std::string, std::vector<std::shared_ptr<Interaction>>>::const_iterator found = interactions.find(edge.guid());

        if (found == interactions.end())
            continue;

        size_t contacts = 0;
        size_t features = 0;
        std::set<std::string_view> kinds;

        for (const std::shared_ptr<Interaction>& interaction : found->second) {

            if (dynamic_cast<const InteractionContact*>(interaction.get()))
                ++contacts;

            if (const InteractionFeature* feature = dynamic_cast<const InteractionFeature*>(interaction.get())) {
                ++features;
                kinds.insert(feature->kind());
            }
        }

        os << name_of(*this, edge.v0) << " -- " << name_of(*this, edge.v1) << " : " << contacts << " contacts, " << features << " features";

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
