#include "pch.h"
#include "wood_session.h"
#include "wood_contact_detection.h"
#include "wood_feature_detection_beam.h"
#include "wood_session.pb.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"

namespace wood_session {

using namespace session_cpp;

const int DRILL_SIDES = 16; // segments of the circles a drill feature draws at both ends of its hole

namespace {

/// Registers the element and interaction factories with the kernel; always returns true so source static can hold the result.
bool register_factories() {

    Plate::register_type();
    Column::register_type();
    BeamVariable::register_type();
    Support::register_type();
    Block::register_type();
    Beam::register_type();
    Joint::register_type();
    InteractionContactFace::register_type();
    InteractionContactAxis::register_type();
    InteractionContactCross::register_type();
    InteractionFeaturePlate::register_type();
    InteractionFeatureBeam::register_type();
    InteractionFeaturePlateBeam::register_type();
    InteractionFeatureSolid::register_type();
    InteractionFeaturePlane::register_type();

    return true;
}

/// Registers the element and interaction factories with the kernel, once.
void register_types() {
    static const bool done = register_factories();
    (void)done;
}

/// The solid a drill runs through: a timber element's stock, with the blocks glued to it, else the element alone.
static Mesh stock_of(const Element& element) {

    if (const WoodElement* member = dynamic_cast<const WoodElement*>(&element))
        return member->stock_mesh();

    return element.element_geometry_mesh();
}

/// The solid features an element hosts: a timber element's or a connector's, none for a support or another element.
std::vector<InteractionFeatureSolid>* get_solid_features(Element& element) {

    if (dynamic_cast<Support*>(&element))
        return nullptr;

    if (WoodElement* member = dynamic_cast<WoodElement*>(&element))
        return &member->solid_features;

    if (JointBeam* connector = dynamic_cast<JointBeam*>(&element))
        return &connector->solid_features;

    return nullptr;
}

bool erase_solid_feature(std::vector<InteractionFeatureSolid>& cuts, const std::string& guid) {

    const size_t count = cuts.size();

    for (auto cut = cuts.begin(); cut != cuts.end();)
        if (cut->source == guid)
            cut = cuts.erase(cut);
        else
            ++cut;

    return count != cuts.size();
}

}  // namespace

const std::vector<InteractionFeatureSolid>* solid_features_of(const Element& element) {
    return get_solid_features(const_cast<Element&>(element));
}

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

/// The kernel reads its fields, the interactions as their registered wood types, and skips the rest; the same bytes read as wood_proto.WoodSession give the settings, the adjacency and the three-valence groups.
WoodSession WoodSession::pb_loads(const std::string& data) {

    register_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::pb_loads(data);

    wood_proto::WoodSession proto;
    if (!proto.ParseFromString(data))
        throw std::runtime_error("Failed to parse WoodSession protobuf data");

    if (proto.has_settings())
        scene.settings = Settings::pb_loads(proto.settings().SerializeAsString());

    for (const wood_proto::PlatePair& pair : proto.adjacency())
        scene.adjacency.push_back({pair.first(), pair.second()});

    for (const wood_proto::PlateGroup& group : proto.three_valence())
        scene.three_valence.push_back(std::vector<int>(group.values().begin(), group.values().end()));

    return scene;
}

WoodSession WoodSession::jsonload(const nlohmann::json& data) {

    register_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::jsonload(data);
    return scene;
}

WoodSession WoodSession::file_json_loads(const std::string& json_string) {
    return jsonload(nlohmann::json::parse(json_string));
}

WoodSession WoodSession::file_json_load(const std::string& filename) {

    register_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::file_json_load(filename);
    return scene;
}

WoodSession WoodSession::from_proto(const session_proto::Session& proto) {

    register_types();

    WoodSession scene;
    static_cast<Session&>(scene) = Session::from_proto(proto);
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

/// A contact as a feature, with face indices relative to its host element.
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

    ElementFeature feature(
        "contact",
        face,
        {outline},
        name
    );
    feature.guid() = contact.guid();

    return feature;
}

/// True when source feature goes: of the type when one is given, else one of the guids.
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
        if (!is_dropped(feature, type, guids)) {
            features.push_back(feature);
            features.back().guid() = feature.guid();
        }

    instance->features = std::move(features);
}

/// drop_features on the one element or instance guid names.
void drop_host_features(
    WoodSession& session,
    const std::string& guid,
    std::string_view type,
    const std::unordered_set<std::string>& guids
) {

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = session.instance_lookup.find(guid);

    if (instance != session.instance_lookup.end())
        drop_instance_features(instance->second, type, guids);
    else
        drop_features(session.get_element<Element>(guid), type, guids);
}

}  // namespace

void WoodSession::clear_features() {

    std::vector<std::string> generated;
    std::unordered_set<std::string> kept_joints;

    for (const std::shared_ptr<Element>& element : *objects.elements) {
        const std::shared_ptr<Joint> joint = std::dynamic_pointer_cast<Joint>(element);
        if (std::dynamic_pointer_cast<JointPlate>(element) || (std::dynamic_pointer_cast<JointBeam>(element) && joint->generated))
            generated.push_back(element->guid());
        else if (joint)
            kept_joints.insert(element->guid());
    }

    // the features a joint the user added put on its edges and its targets stay with it
    std::unordered_set<std::string> kept;

    for (const std::pair<const std::string, std::map<std::string, Edge>>& from : graph.edges)
        for (const std::pair<const std::string, Edge>& to : from.second)
            if (kept_joints.count(from.first) || kept_joints.count(to.first))
                if (const auto found = interactions.find(to.second.guid()); found != interactions.end())
                    for (const std::shared_ptr<Interaction>& interaction : found->second)
                        kept.insert(interaction->guid());

    for (const std::string& id : generated)
        remove_object(id);

    for (const std::shared_ptr<Plate>& plate : plates()) {
        plate->features.top.clear();
        plate->features.bottom.clear();
        plate->invalidate_geometry();
    }

    for (std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions) {

        std::vector<std::shared_ptr<Interaction>> remaining;

        for (const std::shared_ptr<Interaction>& interaction : entry.second)
            if (!dynamic_cast<const InteractionFeature*>(interaction.get()) || kept.count(interaction->guid()))
                remaining.push_back(interaction);

        entry.second = std::move(remaining);
    }

    const auto dropped = [&kept](const std::vector<ElementFeature>& features) {
        std::unordered_set<std::string> guids;
        for (const ElementFeature& feature : features)
            if (feature.feature_type == "joint" && !kept.count(feature.guid()))
                guids.insert(feature.guid());
        return guids;
    };

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (element->features_count() > 0)
            drop_features(element, "", dropped(element->features()));

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        drop_instance_features(instance, "", dropped(instance->features));
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

    for (const std::shared_ptr<Element>& element : *objects.elements)
        drop_features(element, "", erased);

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        drop_instance_features(instance, "", erased);
}

/// Stored elements are lofted first, since source mesh gives their outlines; source view comes with its outlines seeded.
void WoodSession::compute_face_contacts(int level) {

    erase_contacts("face");

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes;
    for (const std::shared_ptr<TreeNode>& node : tree.nodes())
        nodes[node->name] = node;

    std::map<const TreeNode*, std::vector<std::shared_ptr<Element>>> branches;
    for (const std::shared_ptr<Element>& element : world_elements()) {
        if (std::dynamic_pointer_cast<Joint>(element))
            continue;

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

std::shared_ptr<InteractionContactFace> WoodSession::compute_face_contact(std::shared_ptr<Element> source, std::shared_ptr<Element> target){
    if (!source || !target || source->guid() == target->guid())
        return nullptr;
    const std::vector<InteractionContactFace> contacts = face_contacts_for_pair(*source, *target, settings);

    if (contacts.empty())
        return nullptr;

    // the largest: a triangulated face meets its neighbour in many pieces, the face itself the biggest
    const auto largest = std::max_element(contacts.begin(), contacts.end(), [](const InteractionContactFace& x, const InteractionContactFace& y) { return x.polygon.area() < y.polygon.area(); });

    return std::make_shared<InteractionContactFace>(*largest);
}

std::shared_ptr<InteractionContactFace> WoodSession::compute_border_contact(const Plate& plate, int face) {

    if (face < 2 || plate.polylines.size() < 2 || face >= static_cast<int>(plate.polylines.size()) || face >= static_cast<int>(plate.planes.size()))
        return nullptr;

    // the average of the face's two side edges, projected to span them
    const Polyline& bottom = plate.polylines[0];
    const Polyline& top = plate.polylines[1];
    const std::array<Point, 4> ends = {bottom[face - 2], top[face - 2], bottom[face - 1], top[face - 1]};
    Point average_start;
    Point average_end;
    Polyline::line_line_average(ends[0], ends[1], ends[2], ends[3], average_start, average_end);
    Point start;
    Point end;
    if (!Polyline::line_from_projected_points(average_start, average_end, {ends[0], ends[1], ends[2], ends[3]}, start, end))
        return nullptr;

    // x along that line, z the face's normal out of the plate, y across the thickness; the line runs from the end farther from the face's first edge
    const double half = 0.5 * plate.thickness;
    const Vector z = plate.planes[face].z_axis().normalized();
    const Vector x = (end - start).normalized();
    const Vector y = z.cross(x).normalized();
    Point p0 = start;
    Point p1 = end;
    const Point first_edge = Point::mid_point(plate.polylines[face][0], plate.polylines[face][1]);
    if (first_edge.distance(p0) < first_edge.distance(p1))
        std::swap(p0, p1);

    // two rectangles a thickness apart across the plate, each a quarter of the half thickness either way along the normal
    std::array<Polyline, 2> rectangles;
    for (size_t k = 0; k < 2; k++) {
        const Vector across = y * (k == 0 ? -half : half);
        const Vector along = z * (0.25 * half);
        rectangles[k] = Polyline({p0 + across - along, p0 + across + along, p1 + across + along, p1 + across - along, p0 + across - along});
    }

    const Line line = Line::from_points(start, end);
    return std::make_shared<InteractionContactFace>(face, face, ContactType::border, plate.polylines[face], std::array<Line, 2>{line, line}, std::array<Polyline, 4>{rectangles[0], rectangles[1], rectangles[0], rectangles[1]});
}

void WoodSession::compute_axis_contacts(double min_distance) {

    erase_contacts("axis");

    const std::vector<std::shared_ptr<Beam>> beams = world_elements<Beam>();
    for (const auto& [ia, ib, contact] : axis_contacts(beams, min_distance))
        add_interaction(beams[ia], beams[ib], std::make_shared<InteractionContactAxis>(contact));
}

/// Each beam edge's earlier beam features go first, then one per axis contact, oriented to the edge's first beam.
void WoodSession::compute_beam_features(double volume_length, double cross_or_side_to_end, int flip_male) {

    std::vector<std::shared_ptr<JointBeam>> previous;
    for (const auto& element : *objects.elements)
        if (auto joint = std::dynamic_pointer_cast<JointBeam>(element); joint && joint->generated)
            previous.push_back(joint);
    for (const auto& joint : previous) {
        for (const auto& id : joint->targets)
            if (auto target = get_element<Element>(id))
                remove_interaction(joint, target);
        remove_object(joint->guid());
    }

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
        drop_host_features(
            *this,
            edge.v0,
            "",
            erased
        );
        drop_host_features(
            *this,
            edge.v1,
            "",
            erased
        );

        for (const std::shared_ptr<Interaction>& interaction : kept) {

            const InteractionContactAxis* axis = dynamic_cast<const InteractionContactAxis*>(interaction.get());
            if (!axis)
                continue;
            auto joint = JointBeam::from_contact(
                *beam_a,
                *beam_b,
                *axis,
                volume_length,
                cross_or_side_to_end,
                flip_male,
                settings
            );
            if (!joint)
                continue;
            joint->generated = true;

            // the joint in its family's group, then what it does to each beam it joins, in its target order
            add(joint, group_named(joint->element_type_name(), group_named("joints")));
            const std::vector<std::string> targets = joint->targets;
            for (size_t i = 0; i < targets.size(); i++)
                add_interaction(joint, get_element<Element>(targets[i]), joint->interaction(i));
        }
    }
}

void WoodSession::compute_cross_contacts(double angle_tol) {

    erase_contacts("cross");

    const std::vector<std::shared_ptr<Plate>> plates = world_elements<Plate>();
    const auto pairs = adjacency_search(std::vector<std::shared_ptr<Element>>(plates.begin(), plates.end()), settings.distance);
    for (const auto& [i, j] : pairs)
        if (const std::shared_ptr<InteractionContactCross> crossing = compute_cross_contact(plates[i], plates[j], angle_tol))
            add_interaction(plates[i], plates[j], crossing);
}

std::shared_ptr<InteractionContactCross> WoodSession::compute_cross_contact(const std::shared_ptr<Plate>& a, const std::shared_ptr<Plate>& b, double angle_tol) const {

    if (a->polylines.size() < 2 || a->planes.size() < 2 || b->polylines.size() < 2 || b->planes.size() < 2)
        return nullptr;

    InteractionContactCross crossing;
    const bool crosses = plane_to_face(
        a->polylines[0],
        a->polylines[1],
        b->polylines[0],
        b->polylines[1],
        a->planes[0],
        a->planes[1],
        b->planes[0],
        b->planes[1],
        settings.distance_squared,
        crossing,
        angle_tol
    );

    return crosses ? std::make_shared<InteractionContactCross>(crossing) : nullptr;
}

void WoodSession::compute_line_contacts(double tolerance) {

    erase_contacts("axis");

    const double tol = tolerance >= 0.0 ? tolerance : settings.distance;
    const double tol_squared = tol * tol;
    std::vector<std::shared_ptr<Element>> elements = world_elements();
    std::erase_if(elements, [](const auto& element) { return std::dynamic_pointer_cast<Joint>(element) != nullptr; });

    std::vector<std::vector<std::vector<Line>>> lines(elements.size());
    for (size_t source = 0; source < elements.size(); ++source)
        for (const Polyline& loop : elements[source]->polylines())
            lines[source].push_back(loop.get_lines());

    for (size_t source = 0; source < elements.size(); ++source)
        for (size_t target = source + 1; target < elements.size(); ++target)
            for (size_t la = 0; la < lines[source].size(); ++la)
                for (size_t sa = 0; sa < lines[source][la].size(); ++sa)
                    for (size_t lb = 0; lb < lines[target].size(); ++lb)
                        for (size_t sb = 0; sb < lines[target][lb].size(); ++sb) {

                            double t0 = 0.0;
                            double t1 = 0.0;
                            if (!Intersection::line_line_parameters(
                                lines[source][la][sa],
                                lines[target][lb][sb],
                                t0,
                                t1,
                                0.0,
                                true,
                                true
                            ))
                                continue;

                            const Point q0 = lines[source][la][sa].point_at(t0);
                            const Point q1 = lines[target][lb][sb].point_at(t1);
                            if ((q0 - q1).magnitude_squared() > tol_squared)
                                continue;

                            add_interaction(
                                elements[source],
                                elements[target],
                                std::make_shared<InteractionContactAxis>(
                                    Line::from_points(q0, q1),
                                    t0,
                                    t1,
                                    (int)la,
                                    (int)sa,
                                    (int)lb,
                                    (int)sb
                                )
                            );
                        }
}

bool WoodSession::consistent() const {
    for (const auto& pair : graph.get_edges()) {
        const Edge& edge = graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const auto found = interactions.find(edge.guid());
        if (found == interactions.end())
            continue;
        for (const auto& interaction : found->second) {
            const auto* feature = dynamic_cast<const InteractionFeaturePlate*>(interaction.get());
            if (!feature)
                continue;
            if (feature->target_side) {
                const std::string& target = feature->target_side == 1 ? feature->element_a : feature->element_b;
                const std::string source = edge.v0 == target ? edge.v1 : edge.v0;
                if ((edge.v0 != target && edge.v1 != target) || !get_element<Joint>(source))
                    return false;
            } else if (!((feature->element_a == edge.v0 && feature->element_b == edge.v1) ||
                         (feature->element_a == edge.v1 && feature->element_b == edge.v0))) return false;
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
    std::unordered_set<std::string> seen;
    for (const auto& element : *objects.elements)
        if (const auto joint = std::dynamic_pointer_cast<JointPlate>(element))
            for (const auto& connection : joint->connections)
                if (seen.insert(connection.guid()).second)
                    out.push_back(connection);
    for (const auto& entry : interactions)
        for (const auto& interaction : entry.second)
            if (const auto* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get()))
                if (!plate->target_side && seen.insert(plate->guid()).second)
                    out.push_back(*plate);

    return out;
}

/// Side [0] of source plate feature belongs to its element_a, side [1] to its element_b.
std::vector<ElementFeature> WoodSession::get_element_features(const std::string& guid) const {

    std::vector<ElementFeature> features;

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : interactions)
        for (const std::shared_ptr<Interaction>& interaction : entry.second) {

            const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get());

            if (!plate)
                continue;

            const int side = plate->element_a == guid ? 0 : (plate->element_b == guid ? 1 : -1);

            if (side < 0 || (plate->target_side && plate->target_side != side + 1))
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

/// Removes the plane feature with that guid from the features.
static void erase_plane_feature(std::vector<InteractionFeaturePlane>& features, const std::string& guid) {

    for (std::vector<InteractionFeaturePlane>::iterator feature = features.begin(); feature != features.end();)
        if (feature->guid() == guid)
            feature = features.erase(feature);
        else
            ++feature;
}

/// Removes the interaction with that guid from the records of an edge.
static void erase_record(std::vector<std::shared_ptr<Interaction>>& records, const std::string& guid) {

    for (std::vector<std::shared_ptr<Interaction>>::iterator record = records.begin(); record != records.end();)
        if ((*record)->guid() == guid)
            record = records.erase(record);
        else
            ++record;
}

/// Removes every solid feature from the records of an edge.
static void erase_solid_records(std::vector<std::shared_ptr<Interaction>>& records) {

    for (std::vector<std::shared_ptr<Interaction>>::iterator record = records.begin(); record != records.end();)
        if (dynamic_cast<const InteractionFeatureSolid*>(record->get()))
            record = records.erase(record);
        else
            ++record;
}

static void refresh_target(WoodSession& scene, const std::shared_ptr<Element>& target);
static void host_solid_feature(
    WoodSession& scene,
    const Element& source,
    InteractionFeatureSolid cut,
    const std::shared_ptr<Element>& target
);
static void host_plate_joint_side(
    WoodSession& scene,
    const std::shared_ptr<JointPlate>& joint,
    const InteractionFeaturePlate& connection,
    int side,
    const std::shared_ptr<Plate>& plate,
    const std::shared_ptr<InteractionFeaturePlate>& feature
);

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Interactions
// ═══════════════════════════════════════════════════════════════════════════

/// The child group `name` of an element's node, made when missing: an element keeps its attributes and its features in two such groups.
static std::shared_ptr<TreeNode> element_group(Session& session, const std::shared_ptr<TreeNode>& node, const std::string& name) {

    for (TreeNode* child : node->children())
        if (child->name == name && !session.lookup.contains(child->name))
            return child->shared_from_this();

    return session.add_group(name, node);
}

/// Graph ordering stays stable; single-owner geometry belongs to this call's source.
/// Puts the source element's layer in the `features` group under the target's when the two lie side by side in one group, so the elements that put solid or plane features on an element list as its child layers; a source already placed under something else, such as a connector's pin, stays, and a joint that ties two elements is never moved.
static void nest_feature_source(Session& session, const Element& source, const Element& target) {

    const std::shared_ptr<TreeNode> child = session.get_node(source.guid());
    const std::shared_ptr<TreeNode> parent = session.get_node(target.guid());
    if (!child || !parent || child == parent || child->parent() != parent->parent())
        return;

    session.add(child, element_group(session, parent, "features"));
}

std::shared_ptr<Interaction> WoodSession::add_interaction(
    const std::shared_ptr<Element>& source,
    const std::shared_ptr<Element>& target,
    std::shared_ptr<Interaction> interaction
) {

    if (!source || !target || !interaction)
        throw std::invalid_argument("An interaction needs two elements and a payload");

    // a joint on one of its targets: what that kind of joint does there
    const std::shared_ptr<Joint> joint_source = std::dynamic_pointer_cast<Joint>(source);
    const bool joint_payload = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction) || std::dynamic_pointer_cast<InteractionFeatureBeam>(interaction) || std::dynamic_pointer_cast<InteractionFeaturePlateBeam>(interaction);

    if (joint_source && !std::dynamic_pointer_cast<JointPlate>(joint_source) && joint_payload)
        return add_joint_interaction(joint_source, target, interaction);

    const bool feature = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction) || std::dynamic_pointer_cast<InteractionFeaturePlane>(interaction);
    const std::shared_ptr<Joint> tie = std::dynamic_pointer_cast<Joint>(source);
    if (feature && (!tie || tie->targets.size() < 2))
        nest_feature_source(*this, *source, *target);
    if (auto joint = std::dynamic_pointer_cast<JointPlate>(source)) {
        auto feature = std::dynamic_pointer_cast<InteractionFeaturePlate>(interaction);
        if (!feature || feature->target_side < 1 || feature->target_side > 2)
            throw std::invalid_argument("A plate joint interaction must select its target side");
        const int side = feature->target_side - 1;
        auto plate = std::dynamic_pointer_cast<Plate>(target);
        if (!plate)
            throw std::invalid_argument("A plate joint requires a plate target");
        std::vector<InteractionFeaturePlate>::iterator found = joint->connections.end();
        for (std::vector<InteractionFeaturePlate>::iterator connection = joint->connections.begin(); connection != joint->connections.end(); ++connection)
            if (connection->feature_guid(side) == feature->guid())
                found = connection;
        if (found == joint->connections.end())
            throw std::invalid_argument("Feature does not belong to the joint");

        // the side's plate is the one orient designated from the contact; a joint oriented without its plates takes the first target it is given
        std::string& designated = side == 0 ? found->element_a : found->element_b;
        if (designated.empty())
            designated = target->guid();
        else if (designated != target->guid())
            throw std::invalid_argument(fmt::format("interaction({}) of {} is what it does to its {} plate, not to {}", side, joint->name, side == 0 ? "male" : "female", target->name));

        if (std::find(joint->targets.begin(), joint->targets.end(), target->guid()) == joint->targets.end())
            joint->targets.push_back(target->guid());
        *feature = *found;
        feature->target_side = side + 1;
        feature->guid() = found->feature_guid(side);
        host_plate_joint_side(
            *this,
            joint,
            *found,
            side,
            plate,
            feature
        );
        if (!merge_deferred) {
            std::vector<InteractionFeaturePlate> joints = get_plate_features();
            merge_features({plate}, joints);
        }
        return feature;
    }

    const bool known = graph.has_edge({source->guid(), target->guid()});
    const std::string first = known ? graph.edges.at(source->guid()).at(target->guid()).v0 : source->guid();
    const bool reversed = first != source->guid();

    const auto host_on = [&](const std::string& host, ElementFeature feature) {
        const std::unordered_set<std::string> ids{feature.guid()};
        drop_host_features(
            *this,
            source->guid(),
            "",
            ids
        );
        drop_host_features(
            *this,
            target->guid(),
            "",
            ids
        );
        host_feature(host, std::move(feature));
    };
    const auto host_source = [&](ElementFeature feature) {
        host_on(source->guid(), std::move(feature));
    };

    if (const InteractionContact* contact = dynamic_cast<const InteractionContact*>(interaction.get())) {

        const std::shared_ptr<Interaction> oriented = reversed ? contact->flipped() : interaction;
        const InteractionContact& placed = *dynamic_cast<const InteractionContact*>(oriented.get());

        for (const std::shared_ptr<Interaction>& stored : get_interaction(source, target)) {

            const InteractionContact* other = dynamic_cast<const InteractionContact*>(stored.get());

            if (other && other->coincides(placed)) {
                // a contact is what the source does to the target, so the target hosts it, its face index the target's
                host_on(target->guid(), contact_feature(reversed ? *other : *other->flipped()));
                revision++;
                return stored;
            }
        }

        Session::add_interaction(source, target, oriented);
        host_on(target->guid(), contact_feature(*contact->flipped()));

        return oriented;
    }

    if (const std::shared_ptr<InteractionFeaturePlane> plane = std::dynamic_pointer_cast<InteractionFeaturePlane>(interaction)) {

        WoodElement* member = dynamic_cast<WoodElement*>(target.get());
        const std::optional<Xform> local = world_xform(target->guid()).inverse();

        if (!member || !local)
            throw std::invalid_argument("A plane feature needs a placed timber element as its target");

        InteractionFeaturePlane hosted = plane->transformed(*local * world_xform(source->guid()));
        hosted.source = source->guid();
        erase_plane_feature(member->plane_features, plane->guid());
        member->plane_features.push_back(hosted);
        target->invalidate_geometry();

        if (known)
            erase_record(interactions[graph.edges.at(source->guid()).at(target->guid()).guid()], plane->guid());

        Session::add_interaction(source, target, plane);

        return plane;
    }

    if (const std::shared_ptr<InteractionFeatureSolid> cut = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction))
        return add_solid_interaction(source, target, cut);

    if (InteractionFeaturePlate* plate = dynamic_cast<InteractionFeaturePlate*>(interaction.get())) {

        if (plate->element_a.empty()) {
            plate->element_a = source->guid();
            plate->element_b = target->guid();
        }

        plate->sync_features();
        Session::add_interaction(source, target, interaction);
        std::array<ElementFeature, 2> sides = plate->to_features();
        host_feature(plate->element_a, std::move(sides[0]));
        host_feature(plate->element_b, std::move(sides[1]));

        return interaction;
    }

    if (dynamic_cast<InteractionFeatureBeam*>(interaction.get())) {

        // Reusing a joint moves its feature without flipping its stored volumes again.
        std::shared_ptr<Interaction> oriented;
        for (const std::shared_ptr<Interaction>& stored : get_interaction(source, target))
            if (stored->guid() == interaction->guid())
                oriented = stored;

        if (!oriented) {
            oriented = interaction->clone();
            InteractionFeatureBeam& beam = *dynamic_cast<InteractionFeatureBeam*>(oriented.get());

            if (reversed) {
                std::swap(beam.volumes[0], beam.volumes[2]);
                std::swap(beam.volumes[1], beam.volumes[3]);
            }

            Session::add_interaction(source, target, oriented);
        } else {
            revision++;
        }

        const InteractionFeatureBeam& beam = *dynamic_cast<const InteractionFeatureBeam*>(oriented.get());
        ElementFeature side(
            "joint",
            -1,
            std::vector<Polyline>(beam.volumes.begin(), beam.volumes.end()),
            fmt::format("beam_{}", beam.end_type)
        );
        side.guid() = beam.guid();
        host_source(std::move(side));

        return oriented;
    }

    return Session::add_interaction(source, target, interaction);
}

void WoodSession::remove_interaction(const std::shared_ptr<Element>& source, const std::shared_ptr<Element>& target) {

    std::unordered_set<std::string> erased;

    for (const std::shared_ptr<Interaction>& interaction : get_interaction(source, target)) {

        erased.insert(interaction->guid());

        if (const InteractionFeaturePlate* plate = dynamic_cast<const InteractionFeaturePlate*>(interaction.get())) {
            erased.insert(plate->feature_guid(0));
            erased.insert(plate->feature_guid(1));
        }
    }

    for (const ElementFeature& feature : target->features())
        if (feature.feature_type == "drill" && feature.guid().starts_with(source->guid() + "/"))
            erased.insert(feature.guid());

    drop_host_features(
        *this,
        source->guid(),
        "",
        erased
    );
    drop_host_features(
        *this,
        target->guid(),
        "",
        erased
    );
    std::vector<InteractionFeatureSolid>* cuts = get_solid_features(*target);

    if (cuts && erase_solid_feature(*cuts, source->guid()))
        target->invalidate_geometry();

    if (WoodElement* member = dynamic_cast<WoodElement*>(target.get()))
        if (std::erase_if(member->plane_features, [&source](const InteractionFeaturePlane& feature) { return feature.source == source->guid(); }))
            target->invalidate_geometry();

    Session::remove_interaction(source, target);
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Protobuf
// ═══════════════════════════════════════════════════════════════════════════

void WoodSession::pb_dump(const std::string& filename) {
    const std::string data = pb_dumps();
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), data.size());
}

void WoodSession::sync_attributes() {

    for (const std::shared_ptr<Element>& element : *objects.elements) {

        const WoodElement* wood = dynamic_cast<const WoodElement*>(element.get());
        const std::optional<Plane> base = wood ? wood->base_plane() : std::nullopt;
        const std::shared_ptr<TreeNode> node = base ? get_node(element->guid()) : nullptr;
        const std::shared_ptr<TreeNode> parent = node ? node->parent() : nullptr;

        // a feature is no element of its own: it has no base plane
        if (!node || (parent && parent->name == "features" && !lookup.contains(parent->name)))
            continue;

        const std::shared_ptr<TreeNode> attributes = element_group(*this, node, "attributes");
        Plane plane = *base;
        plane.name = "base_plane";
        plane.is_visible = false;
        plane.linecolor = Color::black();
        std::shared_ptr<Plane> drawn;

        for (TreeNode* child : attributes->children())
            if (const std::shared_ptr<Plane> found = get_object<Plane>(child->name))
                drawn = found;

        if (drawn) {
            plane.is_visible = drawn->is_visible;
            plane.is_locked = drawn->is_locked;

            if (*drawn == plane)
                continue;

            remove_object(drawn->guid());
        }

        add_plane(std::make_shared<Plane>(plane), attributes);
    }
}

/// The kernel's bytes, the interactions among them, parse into the superset message field for field; the settings, the adjacency and the three-valence groups follow.
std::string WoodSession::pb_dumps() {

    // exact BReps first: round bores and pins, the mesh only where no BRep is made
    compute_breps();
    sync_attributes();
    wood_proto::WoodSession proto;
    if (!proto.ParseFromString(Session::pb_dumps()))
        throw std::runtime_error("Failed to parse WoodSession protobuf data");

    if (!proto.mutable_settings()->ParseFromString(settings.pb_dumps()))
        throw std::runtime_error("Failed to parse Settings protobuf data");

    for (const std::pair<int, int>& pair : adjacency) {
        wood_proto::PlatePair* stored = proto.add_adjacency();
        stored->set_first(pair.first);
        stored->set_second(pair.second);
    }

    for (const std::vector<int>& row : three_valence)
        proto.add_three_valence()->mutable_values()->Add(row.begin(), row.end());

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

void WoodSession::merge(const WoodSession& other) {
    graft(other, nullptr);
}

/// The guids of the scene's plates in world_elements<Plate>() order, the order adjacency and three_valence count in.
static std::vector<std::string> plate_guids(const WoodSession& scene) {

    std::vector<std::string> guids;
    for (const std::shared_ptr<Plate>& plate : scene.world_elements<Plate>())
        guids.push_back(plate->guid());

    return guids;
}

/// Appends the pairs and three-valence groups of source whose plates all lie in target, renumbered from source's plate order to target's; the instruction row comes first when target has none.
static void append_plate_lists(
    WoodSession& target,
    const std::vector<std::string>& before,
    const std::vector<std::pair<int, int>>& adjacency,
    const std::vector<std::vector<int>>& three_valence
) {

    const std::vector<std::string> after = plate_guids(target);
    std::vector<int> moved(before.size(), -1);

    for (size_t i = 0; i < before.size(); i++) {
        const std::vector<std::string>::const_iterator found = std::find(after.begin(), after.end(), before[i]);
        if (found != after.end())
            moved[i] = static_cast<int>(found - after.begin());
    }

    const auto kept = [&moved](int index) {
        return index >= 0 && index < static_cast<int>(moved.size()) && moved[index] >= 0;
    };

    for (const std::pair<int, int>& pair : adjacency)
        if (kept(pair.first) && kept(pair.second))
            target.adjacency.push_back({moved[pair.first], moved[pair.second]});

    for (size_t row = 1; row < three_valence.size(); row++) {
        if (!std::all_of(three_valence[row].begin(), three_valence[row].end(), kept))
            continue;

        if (target.three_valence.empty())
            target.three_valence.push_back(three_valence[0]);

        std::vector<int> group;
        for (int index : three_valence[row])
            group.push_back(moved[index]);

        target.three_valence.push_back(group);
    }
}

void WoodSession::graft(const WoodSession& other, std::shared_ptr<TreeNode> parent) {

    const std::vector<std::string> mine = plate_guids(*this);
    const std::vector<std::string> theirs = plate_guids(other);
    const bool search = (adjacency.empty() && !mine.empty()) || (other.adjacency.empty() && !theirs.empty());
    const std::vector<std::pair<int, int>> pairs = adjacency;
    const std::vector<std::vector<int>> groups = three_valence;

    Session::graft(other, parent);
    adjacency.clear();
    three_valence.clear();
    append_plate_lists(
        *this,
        mine,
        pairs,
        groups
    );
    append_plate_lists(
        *this,
        theirs,
        other.adjacency,
        other.three_valence
    );

    if (search)
        adjacency.clear();
}

WoodSession WoodSession::get_branch(const std::string& name) const {

    WoodSession part;
    static_cast<Session&>(part) = Session::get_branch(name);
    part.settings = settings;
    append_plate_lists(
        part,
        plate_guids(*this),
        adjacency,
        three_valence
    );

    // a pre-drill connector grouped elsewhere that drills one of the branch's members comes along at the root, so pre_drill_lines reads its holes in the branch too
    for (const std::shared_ptr<JointBeam>& connector : get_elements<JointBeam>())
        if (connector->pre_drill && !part.get_element<JointBeam>(connector->guid()))
            if (std::any_of(connector->targets.begin(), connector->targets.end(), [&part](const std::string& target) { return part.get_element<Element>(target) != nullptr; })) {
                part.add(connector->clone());
                part.set_xform(connector->guid(), world_xform(connector->guid()));
            }

    return part;
}

std::vector<Line> WoodSession::pre_drill_lines(const std::string& guid) const {

    std::vector<Line> lines;

    for (const std::shared_ptr<JointBeam>& connector : get_elements<JointBeam>()) {
        if (!connector->pre_drill || std::find(connector->targets.begin(), connector->targets.end(), guid) == connector->targets.end())
            continue;

        const Xform world = world_xform(connector->guid());

        for (const Line& line : connector->drill_lines)
            lines.push_back(line.transformed(world));
    }

    return lines;
}

}  // namespace wood_session

namespace wood_session {

/// A connector's holes in one target, in the connector's frame: the pins that pass through the target as cut so far, each end run on by the overshoot where the pin leaves the target there, tested just beyond the pin's own end in the target's frame, so a blind hole stops at its pin.
static std::vector<Line> target_drills(const WoodSession& scene, const JointBeam& joint, const Element& target) {

    const std::optional<Xform> local = scene.world_xform(target.guid()).inverse();

    if (!local)
        throw std::invalid_argument("Drill target has a singular placement");

    const Xform to_target = *local * scene.world_xform(joint.guid());
    const std::vector<PlanarFace> solid = planar_faces(stock_of(target));
    std::vector<Line> drills;

    for (const Line& pin : joint.drill_lines) {
        const Vector d = pin.to_vector().normalized();
        const Line placed = pin.transformed(to_target);
        const Vector e = placed.to_vector().normalized();

        // a pin that never enters the target as cut so far, even run on by the overshoot, is not its hole
        const std::vector<std::array<double, 2>> inside = inside_stretches(target.model_geometry_mesh(), placed);
        const double low = -joint.drill_overshoot;
        const double high = placed.length() + joint.drill_overshoot;
        const bool touches = std::any_of(inside.begin(), inside.end(), [low, high](const std::array<double, 2>& stretch) { return stretch[1] > low + 1e-9 && stretch[0] < high - 1e-9; });

        if (!touches)
            continue;

        if (joint.drill_overshoot <= 0.0) {
            drills.push_back(pin);
            continue;
        }

        const bool blind_start = is_inside(solid, placed.start() - e * 1.0);
        const bool blind_end = is_inside(solid, placed.end() + e * 1.0);
        const Point start = blind_start ? pin.start() : pin.start() - d * joint.drill_overshoot;
        const Point end = blind_end ? pin.end() : pin.end() + d * joint.drill_overshoot;
        drills.push_back(Line::from_points(start, end));
    }

    return drills;
}

/// Gives a cut connector's part children the connector's cuts again, each by its place among them, so a slot stored on the connector after nesting reaches the part that draws it.
static void sync_parts(WoodSession& scene, const JointBeam& connector) {

    const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector.guid());
    size_t index = 0;

    if (!node)
        return;

    for (TreeNode* child : node->children())
        if (const std::shared_ptr<ConnectorPart> part = scene.get_element<ConnectorPart>(child->name)) {
            part->solid_features = connector.part_features(index++);
            part->invalidate_geometry();
        }
}

/// Drops the target's cached solids after a cut, and a connector's part children's too.
static void refresh_target(WoodSession& scene, const std::shared_ptr<Element>& target) {

    target->invalidate_geometry();

    if (const std::shared_ptr<JointBeam> connector = std::dynamic_pointer_cast<JointBeam>(target))
        sync_parts(scene, *connector);
}

/// Nests a connector's parts and pins under its node as child elements, once.
static void nest_children(WoodSession& scene, const JointBeam& connector) {

    const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector.guid());

    if (!node || !node->children().empty())
        return;

    for (const std::shared_ptr<Joint>& child : connector.children())
        scene.add(child, node);

    scene.set_node_color(node, JointBeam::CONNECTOR_COLOR, true);
}

/// The holes a joint's lines make in a target as drill features in the target's frame: per stretch of a line inside the target's solid, the circles of the radius where the hole enters and leaves it, named by the joint and the diameter; each feature's guid starts with the joint's, so hosting them again replaces them.
static void host_drills(
    WoodSession& scene,
    const Joint& joint,
    const Element& target,
    const std::vector<Line>& lines,
    double radius
) {

    std::unordered_set<std::string> old;

    for (const ElementFeature& feature : target.features())
        if (feature.feature_type == "drill" && feature.guid().starts_with(joint.guid()))
            old.insert(feature.guid());

    drop_host_features(
        scene,
        target.guid(),
        "",
        old
    );
    const Xform world = scene.world_xform(joint.guid());
    const Mesh solid = stock_of(target).transformed(scene.world_xform(target.guid()));
    const std::optional<Xform> local = scene.world_xform(target.guid()).inverse();

    if (!local)
        throw std::invalid_argument("Drill target has a singular placement");

    for (size_t i = 0; i < lines.size(); i++) {
        const Line line = lines[i].transformed(world);
        const Vector d = line.to_direction();
        const Plane frame = Plane::from_point_normal(line.start(), d);
        size_t stretch = 0;

        for (const std::array<double, 2>& inside : inside_stretches(solid, line)) {
            const double a = std::max(inside[0], 0.0);
            const double b = std::min(inside[1], line.length());

            if (b - a < 1e-6)
                continue;

            std::vector<Polyline> circles;

            for (double t : {a, b}) {
                const Polyline circle = Polyline::from_sides(DRILL_SIDES, radius, true);
                const Xform placement = Xform::frame_to_world(
                    line.start() + d * t,
                    frame.x_axis(),
                    frame.y_axis(),
                    d
                );
                circles.push_back(circle.transformed(*local * placement));
            }

            ElementFeature feature(
                "drill",
                -1,
                circles,
                fmt::format("{} d{:g}", joint.name, 2.0 * radius)
            );
            feature.guid() = fmt::format("{}/{}/{}", joint.guid(), i, stretch++);
            scene.host_feature(target.guid(), std::move(feature));
        }
    }
}

/// Hosts the source's cut on the target in the target's frame, replacing the one the source hosted before, and redraws the target.
static void host_solid_feature(
    WoodSession& scene,
    const Element& source,
    InteractionFeatureSolid cut,
    const std::shared_ptr<Element>& target
) {

    std::vector<InteractionFeatureSolid>* cuts = get_solid_features(*target);

    if (!cuts)
        throw std::invalid_argument("Solid cutters require a plate, beam, column, block or connector");

    const bool solid = cut.mesh.number_of_faces() > 0;

    if ((solid && !cut.mesh.is_closed()) || (!solid && cut.drills.empty()))
        throw std::invalid_argument("Missing closed cutter solid");

    cut.source = source.guid();

    const std::optional<Xform> local = scene.world_xform(target->guid()).inverse();

    if (!local)
        throw std::invalid_argument("Cutter target has a singular placement");

    cut = cut.transformed(*local * scene.world_xform(source.guid()));
    bool replaced = false;

    for (InteractionFeatureSolid& stored : *cuts)
        if (stored.source == source.guid()) {
            stored = cut;
            replaced = true;
        }

    if (!replaced)
        cuts->push_back(std::move(cut));

    refresh_target(scene, target);
}

/// The solid one side of a plate joint takes out of its plate, in the joint's frame: every outline pair of a solid type (slice, mill, cut, conic) lofted into a piece and every drill line as an axis, a pair its builder repeats counted once, a pair whose loft Manifold does not take as a solid or that takes nothing out of the stock left out. Empty for a side that only merges into the outline.
static InteractionFeatureSolid plate_joint_cutter(
    const InteractionFeaturePlate& connection,
    int side,
    double radius,
    double chord_tolerance,
    const Mesh& stock
) {

    InteractionFeatureSolid cut;
    cut.drill_radius = radius;
    cut.drill_tolerance = chord_tolerance;
    cut.drills = JointPlate::side_drills(connection, side);

    // a piece that only touches the stock, as the side slab a builder pushes into the neighbour does, is no cut: its coincident faces would only trouble the boolean
    std::vector<Mesh> pieces;
    for (const Mesh& piece : JointPlate::side_solids(connection, side)) {
        const Mesh taken = solid_boolean(stock, piece, SolidOperation::intersect);
        if (taken.number_of_faces() > 0 && compute_volume(taken) > 1e-9 * compute_volume(piece))
            pieces.push_back(piece);
    }

    // the pieces of one design touch, overlap and nest; the boolean unites their bodies itself, and does so exactly, where a union read back first does not
    cut.mesh = shells_side_by_side(pieces);

    return cut;
}

/// Hosts one side of a plate joint on its plate: the side's feature on the edge and on the plate, replacing the one hosted before, then what the side takes out as a solid, its slice, mill, cut and conic pairs with its drills as the joint's solid feature on the plate and a drill feature per hole, or nothing where an earlier one was.
static void host_plate_joint_side(
    WoodSession& scene,
    const std::shared_ptr<JointPlate>& joint,
    const InteractionFeaturePlate& connection,
    int side,
    const std::shared_ptr<Plate>& plate,
    const std::shared_ptr<InteractionFeaturePlate>& feature
) {

    drop_host_features(
        scene,
        plate->guid(),
        "",
        {feature->guid()}
    );

    if (scene.graph.has_edge({joint->guid(), plate->guid()}))
        erase_record(scene.interactions[scene.graph.edges.at(joint->guid()).at(plate->guid()).guid()], feature->guid());

    scene.Session::add_interaction(joint, plate, feature);
    scene.host_feature(plate->guid(), connection.to_features()[side]);

    const std::optional<Xform> to_joint = scene.world_xform(joint->guid()).inverse();

    if (!to_joint)
        throw std::invalid_argument("Cutter source has a singular placement");

    const Mesh stock = plate->element_geometry_mesh().transformed(*to_joint * scene.world_xform(plate->guid()));
    InteractionFeatureSolid cut = plate_joint_cutter(
        connection,
        side,
        joint->line_radius,
        joint->chord_tolerance,
        stock
    );

    // a top-top pin joint records on each side the line through the other plate, as 2024 did: the plate is bored with the other side's lines
    if (connection.name.starts_with("tt_e_p"))
        cut.drills = plate_joint_cutter(connection, 1 - side, joint->line_radius, joint->chord_tolerance, stock).drills;
    const std::vector<Line> drills = cut.drills;

    if (cut.mesh.number_of_faces() == 0 && drills.empty()) {
        if (erase_solid_feature(plate->solid_features, joint->guid()))
            plate->invalidate_geometry();
    } else {
        host_solid_feature(
            scene,
            *joint,
            std::move(cut),
            plate
        );
    }

    host_drills(
        scene,
        *joint,
        *plate,
        drills,
        joint->line_radius
    );
}

static void add_plane_cut(const Joint& joint, Element& target) {

    if (Beam* beam = dynamic_cast<Beam*>(&target))
        beam->cuts.insert(beam->cuts.end(), joint.cuts.begin(), joint.cuts.end());
    else if (Column* column = dynamic_cast<Column*>(&target))
        column->cuts.insert(column->cuts.end(), joint.cuts.begin(), joint.cuts.end());
    else if (BeamVariable* beam = dynamic_cast<BeamVariable*>(&target))
        beam->cuts.insert(beam->cuts.end(), joint.cuts.begin(), joint.cuts.end());
    else if (Block* block = dynamic_cast<Block*>(&target))
        block->cuts.insert(block->cuts.end(), joint.cuts.begin(), joint.cuts.end());
    else
        throw std::invalid_argument("Plane cutters require a beam, column or block");
}

size_t WoodSession::next_number(const std::string& prefix) const {

    size_t next = 0;

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (numbered(element->name, prefix))
            next = std::max(next, static_cast<size_t>(std::stoul(element->name.substr(prefix.size() + 1))) + 1);

    return next;
}

bool WoodSession::numbered(const std::string& name, const std::string& prefix) {
    return name.size() > prefix.size() + 1 && name.compare(0, prefix.size() + 1, prefix + "_") == 0 && std::all_of(name.begin() + prefix.size() + 1, name.end(), ::isdigit);
}

bool WoodSession::placed(const std::string& name, const std::string& prefix) {

    if (name.size() <= prefix.size() + 1 || name.compare(0, prefix.size() + 1, prefix + "_") != 0)
        return false;

    const std::string place = name.substr(prefix.size() + 1);

    return std::isdigit(place.front()) && std::isdigit(place.back()) && place.find("__") == std::string::npos
        && std::all_of(place.begin(), place.end(), [](char c) { return std::isdigit(c) || c == '_'; });
}

void WoodSession::compute_breps() {

    for (const std::shared_ptr<Element>& element : *objects.elements) {
        // a pin or a connector part, which a connector draws on its own, a support with its round parts, or a member its joints cut
        const bool connector_child = std::dynamic_pointer_cast<Pin>(element) || std::dynamic_pointer_cast<ConnectorPart>(element);

        if (connector_child || std::dynamic_pointer_cast<Support>(element) || (!std::dynamic_pointer_cast<Joint>(element) && element->model_geometry_mesh().number_of_vertices() != element->element_geometry_mesh().number_of_vertices()))
            element->compute_geometry_brep();
    }
}

std::shared_ptr<Interaction> WoodSession::add_solid_interaction(const std::shared_ptr<Element>& source, const std::shared_ptr<Element>& target, const std::shared_ptr<InteractionFeatureSolid>& cut) {

    const bool known = graph.has_edge({source->guid(), target->guid()});
    host_solid_feature(
        *this,
        *source,
        *cut,
        target
    );

    if (known)
        erase_solid_records(interactions[graph.edges.at(source->guid()).at(target->guid()).guid()]);

    Session::add_interaction(source, target, cut);

    return cut;
}

std::shared_ptr<Interaction> WoodSession::add_joint_interaction(const std::shared_ptr<Joint>& joint, const std::shared_ptr<Element>& target, std::shared_ptr<Interaction> interaction) {

    // the target among the joint's, joining them when the caller names a new one
    if (std::find(joint->targets.begin(), joint->targets.end(), target->guid()) == joint->targets.end())
        joint->targets.push_back(target->guid());

    const std::shared_ptr<JointBeam> beam_joint = std::dynamic_pointer_cast<JointBeam>(joint);
    const std::shared_ptr<InteractionFeatureSolid> cut = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction);

    // a connector: its parts and pins nest under it once; pre-drilled pins record their holes, which the targets read through pre_drill_lines
    if (beam_joint && beam_joint->is_connector()) {
        nest_children(*this, *beam_joint);
        Session::remove_interaction(joint, target);

        if (beam_joint->pre_drill) {
            Session::add_interaction(joint, target, interaction);
            host_drills(
                *this,
                *joint,
                *target,
                joint->drill_lines,
                joint->line_radius
            );

            // the pre-drilled holes in the target's solid too, each pin's stretch inside it bored, exact in the BRep
            InteractionFeatureSolid holes;
            holes.drills = target_drills(*this, *beam_joint, *target);
            holes.drill_radius = joint->line_radius;
            holes.drill_tolerance = joint->chord_tolerance;
            if (!holes.drills.empty())
                host_solid_feature(*this, *joint, std::move(holes), target);

            return interaction;
        }

        // its cutters and every pin as a hole, run on past the target where the pin leaves it
        if (!cut)
            throw std::invalid_argument("A connector's interaction is its InteractionFeatureSolid");

        cut->drills = target_drills(*this, *beam_joint, *target);

        if (joint->targets.size() < 2)
            nest_feature_source(*this, *joint, *target);

        add_solid_interaction(joint, target, cut);
        host_drills(
            *this,
            *joint,
            *target,
            cut->drills,
            joint->line_radius
        );

        return cut;
    }

    // a beam-to-beam joint: its volumes on the edge, hosted on the target as a joint feature
    if (beam_joint) {
        const std::shared_ptr<InteractionFeatureBeam> volumes = std::dynamic_pointer_cast<InteractionFeatureBeam>(interaction);

        if (!volumes)
            throw std::invalid_argument("A beam joint's interaction is its InteractionFeatureBeam");

        if (has_interaction(joint, target))
            remove_interaction(joint, target);

        Session::add_interaction(joint, target, volumes);
        ElementFeature hosted(
            "joint",
            -1,
            {volumes->volumes[0], volumes->volumes[1]},
            joint->name
        );
        hosted.guid() = volumes->guid();
        host_feature(target->guid(), std::move(hosted));

        // and the cut the plate joint on its box leaves the target, as a solid it loses
        const size_t side = static_cast<size_t>(std::find(joint->targets.begin(), joint->targets.end(), target->guid()) - joint->targets.begin());
        if (side < 2 && (beam_joint->member_cuts[side].number_of_faces() > 0 || !beam_joint->member_drills[side].empty())) {
            InteractionFeatureSolid member_cut;
            member_cut.mesh = beam_joint->member_cuts[side];
            member_cut.drills = beam_joint->member_drills[side];
            member_cut.drill_radius = joint->line_radius;
            member_cut.drill_tolerance = joint->chord_tolerance;
            host_solid_feature(*this, *joint, std::move(member_cut), target);
        }

        return volumes;
    }

    // a cutter: its solid with its drills, or only its planes, which the target takes in itself
    Session::remove_interaction(joint, target);

    if (cut) {
        if (joint->targets.size() < 2)
            nest_feature_source(*this, *joint, *target);

        add_solid_interaction(joint, target, cut);

        if (!joint->drill_axes().empty())
            host_drills(
                *this,
                *joint,
                *target,
                joint->drill_axes(),
                joint->line_radius
            );

        return cut;
    }

    add_plane_cut(*joint, *target);
    refresh_target(*this, target);
    Session::add_interaction(joint, target, interaction);

    return interaction;
}


}
