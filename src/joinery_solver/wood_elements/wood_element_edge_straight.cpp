#include "pch.h"
#include "wood_serialization.h"
#include "wood_element_edge_straight.h"
#include "wood_element_geometry.h"
#include "wood_profile.h"
#include "wood_brep_drill.h"
#include "wood_element_edge_variable.h"
#include "element_edge_straight.pb.h"
#include "element_column.pb.h"
using namespace session_cpp;

namespace wood_session {

EdgeStraightElement::EdgeStraightElement() : EdgeElement("beam") {
}

EdgeStraightElement::EdgeStraightElement(
    const Polyline& axis,
    const std::vector<Polyline>& profile,
    const std::vector<Vector>& directions,
    const std::string& name
)
    : EdgeElement(name), axis(axis), radii(axis.segment_count(), compute_size(profile).first / 2.0),
      directions(directions), profile(profile) {
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════
std::shared_ptr<EdgeStraightElement>
EdgeStraightElement::square(const Polyline& axis, double radius, const std::string& name) {

    const std::shared_ptr<EdgeStraightElement> edge = std::make_shared<EdgeStraightElement>();
    edge->name = name;
    edge->axis = axis;
    edge->radii.assign(axis.segment_count(), radius);

    return edge;
}

std::shared_ptr<EdgeStraightElement> EdgeStraightElement::corner_square(
    const Line& axis, const Plane& corner, double side, const std::string& name
) {

    const Point base = corner.origin() + corner.z_axis() * corner.signed_distance(axis.start());
    const Vector x = corner.x_axis() * side;
    const Vector y = corner.y_axis() * side;

    return from_section(axis, Polyline({base, base + x, base + x + y, base + y}).closed(), name);
}

std::shared_ptr<EdgeStraightElement> EdgeStraightElement::from_section(
    const Line& axis, const Polyline& section, const std::string& name
) {

    const Vector along = axis.to_vector().normalized();
    if (!is_square_to(section, along))
        throw std::invalid_argument("from_section: the section is not square to the axis");

    const Vector x = (section.get_point(1) - section.get_point(0)).normalized();
    const Vector y = along.cross(x);

    std::vector<Point> loop;
    for (const Point& point : section.get_points()) {
        const Vector offset = point - axis.start();
        loop.emplace_back(offset.dot(x), offset.dot(y), 0.0);
    }

    return std::make_shared<EdgeStraightElement>(
        Polyline({axis.start(), axis.end()}),
        std::vector<Polyline>{Polyline(loop)},
        std::vector<Vector>{y},
        name
    );
}

std::shared_ptr<EdgeStraightElement> EdgeStraightElement::round(
    const Line& axis, double radius, double chord_tolerance, const std::string& name
) {

    const Plane frame = Plane::from_point_normal(axis.start(), axis.to_vector());
    const int chords = circle_segments(radius, chord_tolerance);
    std::vector<Point> ring;
    for (int i = 0; i < chords; i++) {
        const double turn = Tolerance::TWO_PI * i / chords;
        ring.emplace_back(radius * std::cos(turn), radius * std::sin(turn), 0.0);
    }
    ring.push_back(ring.front());

    const std::shared_ptr<EdgeStraightElement> edge = std::make_shared<EdgeStraightElement>(
        Polyline({axis.start(), axis.end()}),
        std::vector<Polyline>{Polyline(ring)},
        std::vector<Vector>{frame.y_axis()},
        name
    );
    edge->round_radius = radius;

    return edge;
}

std::array<std::shared_ptr<EdgeStraightElement>, 2>
EdgeStraightElement::pair_crossing(double angle_degrees, double radius, double length) {

    const double turn = angle_degrees * Tolerance::PI / 180.0;
    const Vector along(std::cos(turn), std::sin(turn), 0.0);
    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<EdgeStraightElement> first = EdgeStraightElement::square(
        Polyline({Point(-length * 0.5, 0.0, 0.0), Point(length * 0.5, 0.0, 0.0)}), radius, "first"
    );
    const std::shared_ptr<EdgeStraightElement> second = EdgeStraightElement::square(
        Polyline({origin - along * (length * 0.5), origin + along * (length * 0.5)}),
        radius,
        "second"
    );
    return {first, second};
}

std::array<std::shared_ptr<EdgeStraightElement>, 2>
EdgeStraightElement::pair_tee(double angle_degrees, double radius, double length) {

    const double turn = angle_degrees * Tolerance::PI / 180.0;
    const Vector along(std::cos(turn), std::sin(turn), 0.0);
    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<EdgeStraightElement> first = EdgeStraightElement::square(
        Polyline({Point(-length * 0.5, 0.0, 0.0), Point(length * 0.5, 0.0, 0.0)}), radius, "first"
    );
    const std::shared_ptr<EdgeStraightElement> second =
        EdgeStraightElement::square(Polyline({origin, origin + along * length}), radius, "second");
    second->extension = radius;
    return {first, second};
}

std::array<std::shared_ptr<EdgeStraightElement>, 2>
EdgeStraightElement::pair_end_to_end(double angle_degrees, double radius, double length, double extension) {

    const double turn = angle_degrees * Tolerance::PI / 180.0;
    const Vector along(std::cos(turn), std::sin(turn), 0.0);
    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<EdgeStraightElement> first =
        EdgeStraightElement::square(Polyline({Point(-length, 0.0, 0.0), origin}), radius, "first");
    const std::shared_ptr<EdgeStraightElement> second =
        EdgeStraightElement::square(Polyline({origin, origin + along * length}), radius, "second");
    first->extension = extension;
    second->extension = extension;
    return {first, second};
}

std::shared_ptr<EdgeStraightElement> EdgeStraightElement::from_element(Element e) {

    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<EdgeStraightElement> beam = std::make_shared<EdgeStraightElement>();
    static_cast<Element&>(*beam) = std::move(e);

    if (!bytes.empty() && bytes.front() == '{') {
        try {
            const nlohmann::json payload = nlohmann::json::parse(bytes);
            if (payload.contains("axis") && !payload["axis"].is_null())
                beam->axis = Polyline::jsonload(payload["axis"]);
            if (payload.contains("radii"))
                beam->radii = payload["radii"].get<std::vector<double>>();
            if (payload.contains("directions"))
                for (const nlohmann::json& direction : payload["directions"])
                    beam->directions.push_back(Vector::jsonload(direction));
            beam->allowed_type = payload.value("allowed_type", -1);
        } catch (const std::exception&) {
        }
        return beam;
    }

    wood_proto::EdgeStraightElement proto;
    if (!proto.ParseFromString(bytes))
        return beam;

    if (proto.has_axis())
        beam->axis = Polyline::pb_loads(proto.axis().SerializeAsString());
    beam->radii.assign(proto.radii().begin(), proto.radii().end());
    for (const session_proto::Vector& direction : proto.directions())
        beam->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));
    beam->allowed_type = proto.allowed_type();
    beam->extension = proto.extension();
    beam->round_radius = proto.round_radius();
    for (const session_proto::Plane& cut : proto.cuts())
        beam->cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));
    for (const session_proto::Polyline& ring : proto.profile())
        beam->profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));

    for (const auto& cut : proto.solid_features())
        beam->solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));
    for (const wood_proto::InteractionFeaturePlane& feature : proto.plane_features())
        beam->plane_features.push_back(InteractionFeaturePlane::pb_loads(feature.SerializeAsString()
        ));
    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
Line EdgeStraightElement::chord() const {

    if (axis.point_count() < 2)
        return Line::from_points(Point(0, 0, 0), Point(0, 0, 0));

    return Line::from_points(axis.get_point(0), axis.get_point(axis.point_count() - 1));
}

double EdgeStraightElement::radius(int segment) const {
    return segment >= 0 && segment < (int)radii.size() ? radii[segment] : 0.0;
}

bool EdgeStraightElement::has_direction(int segment) const {
    return segment >= 0 && segment < (int)directions.size();
}

std::vector<Polyline> EdgeStraightElement::sections() const {

    std::vector<Polyline> sections;
    const int segments = static_cast<int>(axis.segment_count());
    if (segments < 1 || radii.empty())
        return sections;

    const std::vector<Point> points = axis.get_points();
    for (int i = 0; i <= segments; i++) {

        const int segment = std::min(i, segments - 1);
        Vector along = points[segment + 1] - points[segment];
        if (i > 0 && i < segments)
            along = along.normalized() + (points[i] - points[i - 1]).normalized();

        const Vector up = has_direction(segment) ? directions[segment] : Vector::z_axis();
        Point station = points[i];
        if (i == 0)
            station = station - along.normalized() * extension;
        if (i == segments)
            station = station + along.normalized() * extension;
        if (profile.empty())
            sections.push_back(square_section(station, along, up, radius(segment)));
        else
            sections.push_back(profile_section(station, along, up, profile[0]));
    }

    return sections;
}

/// Every profile loop placed at both ends of a one-segment axis: what a hollow beam lofts between.
static std::pair<std::vector<Polyline>, std::vector<Polyline>> profile_ends(const EdgeStraightElement& beam) {

    const Vector along = beam.axis.get_point(1) - beam.axis.get_point(0);
    const Vector up = beam.has_direction(0) ? beam.directions[0] : Vector::z_axis();
    const Vector reach = along.normalized() * beam.extension;

    std::pair<std::vector<Polyline>, std::vector<Polyline>> ends;
    for (const Polyline& ring : beam.profile) {
        ends.first.push_back(profile_section(beam.axis.get_point(0) - reach, along, up, ring));
        ends.second.push_back(profile_section(beam.axis.get_point(1) + reach, along, up, ring));
    }

    return ends;
}

/// The turn about the column's axis that puts its cylinder's seam in the middle of the widest run of the cut, at mid height of the cut, so none of the seam is left in the head.
static double seam_turn(const Mesh& cutter, const Line& axis, double radius) {

    const Vector along = axis.to_vector().normalized();
    const Plane base = Plane::from_point_normal(axis.start(), along);
    double lowest = std::numeric_limits<double>::max();
    double highest = std::numeric_limits<double>::lowest();
    for (const Point& point : cutter.to_vertices_and_faces().first) {
        lowest = std::min(lowest, (point - axis.start()).dot(along));
        highest = std::max(highest, (point - axis.start()).dot(along));
    }
    const Point centre = axis.start() + along * (0.5 * (lowest + std::min(highest, axis.length())));

    const std::vector<PlanarFace> faces = planar_faces(cutter);
    const size_t steps = 720;
    std::vector<bool> inside(steps);
    for (size_t i = 0; i < steps; i++) {
        const double turn = Tolerance::TWO_PI * i / steps;
        inside[i] = is_inside(
            faces,
            centre + base.x_axis() * (radius * std::cos(turn)) +
                base.y_axis() * (radius * std::sin(turn))
        );
    }

    size_t best_start = 0;
    size_t best_run = 0;
    for (size_t i = 0; i < steps; i++) {
        if (!inside[i] || inside[(i + steps - 1) % steps])
            continue;
        size_t run = 0;
        while (run < steps && inside[(i + run) % steps])
            run++;
        if (run > best_run) {
            best_run = run;
            best_start = i;
        }
    }

    return Tolerance::TWO_PI * (best_start + 0.5 * best_run) / steps;
}

/// A prism along the column as a BRep from its upper outline, both loops in one point order, as the kernel's boolean needs them.
static BRep cut_prism(const Mesh& piece, const Vector& along) {

    const std::vector<PlanarFace> faces = planar_faces(piece);
    const PlanarFace* top = nullptr;
    double low = std::numeric_limits<double>::max();
    double high = std::numeric_limits<double>::lowest();
    for (const PlanarFace& face : faces) {
        if (std::abs(face.normal.dot(along)) < 0.999)
            continue;
        const double level = Vector(face.points[0][0], face.points[0][1], face.points[0][2]).dot(along);
        low = std::min(low, level);
        if (level > high) {
            high = level;
            top = &face;
        }
    }
    const Polyline upper = Polyline(top->points).closed();

    return brep_between_loops({upper.translated(along * (low - high))}, {upper});
}

const BRep& EdgeStraightElement::model_geometry_brep() const {

    if (round_radius <= 0.0 || solid_features.empty())
        return WoodElement::model_geometry_brep();

    if (!_model_brep_cache) {
        const Line line = chord();

        // the prisms of the cut, a floor each
        std::vector<Mesh> prisms;
        Mesh cutter;
        for (const InteractionFeatureSolid& feature : solid_features)
            if (feature.operation == SolidOperation::subtract)
                for (const Mesh& piece : solid_pieces(feature.mesh)) {
                    prisms.push_back(piece);
                    cutter = solid_boolean(cutter, piece, SolidOperation::add);
                }

        // the exact cylinder, its seam in the cut, less each prism by the kernel's boolean
        BRep solid = element_geometry_brep().transformed(
            Xform::rotation_around_line(line, seam_turn(cutter, line, round_radius))
        );
        for (const Mesh& prism : prisms)
            solid = solid.boolean_difference(cut_prism(prism, line.to_vector().normalized()));
        _model_brep_cache = solid;
    }

    return *_model_brep_cache;
}

const Mesh& EdgeStraightElement::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        if (!profile.empty() && axis.segment_count() == 1) {
            const std::pair<std::vector<Polyline>, std::vector<Polyline>> ends =
                profile_ends(*this);
            _element_geometry_mesh = Mesh::loft(ends.first, ends.second, true);
        } else
            _element_geometry_mesh = sweep_sections(sections());
    }

    return *_element_geometry_mesh;
}

const BRep& EdgeStraightElement::element_geometry_brep() const {

    if (!_element_geometry_brep) {
        if (round_radius > 0.0)
            _element_geometry_brep = drill_brep(chord(), round_radius);
        else if (!profile.empty() && axis.segment_count() == 1) {
            const std::pair<std::vector<Polyline>, std::vector<Polyline>> ends =
                profile_ends(*this);
            _element_geometry_brep = brep_between_loops(ends.first, ends.second);
        } else
            _element_geometry_brep = brep_sections(sections());
    }

    return *_element_geometry_brep;
}

/// The up directions moved by xform.
static std::vector<Vector>
transformed_directions(const std::vector<Vector>& directions, size_t segments, const Xform& xform) {

    std::vector<Vector> moved = transformed_list(directions, xform);
    const Vector up = Vector::z_axis().transformed(xform);

    if (up != Vector::z_axis() && moved.size() < segments)
        moved.resize(segments, up);

    return moved;
}

std::shared_ptr<EdgeStraightElement> EdgeStraightElement::transformed(const Xform& xform) const {

    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<EdgeStraightElement> beam = std::make_shared<EdgeStraightElement>();
    beam->name = name;
    beam->axis = axis.transformed(xform);
    beam->radii = radii;
    beam->directions = transformed_directions(directions, axis.segment_count(), xform);
    beam->allowed_type = allowed_type;
    beam->guid() = guid();
    beam->cuts = transformed_list(cuts, xform);
    for (const auto& cut : solid_features)
        beam->solid_features.push_back(cut.transformed(xform));
    for (const InteractionFeaturePlane& feature : plane_features)
        beam->plane_features.push_back(feature.transformed(xform));
    beam->profile = profile;
    beam->extension = extension;
    beam->round_radius = round_radius;
    beam->set_features(transformed_features(_features, xform));
    beam->set_insertion_vectors(transformed_list(_insertion_vectors, xform));

    return beam;
}

void EdgeStraightElement::place(const Xform& xform) {

    Element::place(xform);
    directions = transformed_directions(directions, axis.segment_count(), xform);
    axis.transform(xform);
    cuts = transformed_list(cuts, xform);
    for (auto& cut : solid_features)
        cut = cut.transformed(xform);
    for (InteractionFeaturePlane& feature : plane_features)
        feature = feature.transformed(xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
}

void EdgeStraightElement::compute_geometry_mesh_impl() {

    const std::vector<Polyline> rings = sections();
    if (!rings.empty()) {
        set_geometry(model_geometry_mesh());
    }
    compute_geometry_features();
}

void EdgeStraightElement::compute_geometry_brep_impl() {

    const std::vector<Polyline> rings = sections();
    if (!rings.empty()) {
        set_geometry(model_geometry_brep());
    }
    compute_geometry_features();
}

void EdgeStraightElement::compute_geometry_features() {

    const std::pair<Polyline, std::vector<Polyline>> trimmed = trim_to_cuts(axis, sections(), cuts);

    std::vector<ElementFeature> next;
    next.push_back(polyline_feature("axis", trimmed.first));

    for (const Polyline& ring : trimmed.second)
        if (ring.point_count() > 0)
            next.push_back(polyline_feature("section", ring));

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB EdgeStraightElement::aabb(double inflate) const {

    const Mesh& solid = geometry_mesh();
    if (solid.number_of_vertices() > 0)
        return AABB::from_mesh(solid, inflate);

    double reach = 0.0;
    for (const double radius : radii)
        reach = std::max(reach, radius);
    for (const Polyline& ring : profile)
        for (const Point& point : ring.get_points())
            reach = std::max(reach, std::hypot(point[0], point[1]));

    return AABB::from_polyline(axis, inflate + reach + extension);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════
nlohmann::ordered_json EdgeStraightElement::element_data_jsondump() const {

    wood_proto::EdgeStraightElement proto;
    if (!proto.ParseFromString(element_data_dumps()))
        throw std::runtime_error("Failed to parse EdgeStraightElement protobuf data");

    return json_of(proto);
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════
std::string EdgeStraightElement::element_data_dumps() const {

    wood_proto::EdgeStraightElement proto;
    if (!proto.mutable_axis()->ParseFromString(axis.pb_dumps()))
        throw std::runtime_error("Failed to parse Line protobuf data");
    proto.mutable_radii()->Add(radii.begin(), radii.end());
    for (const Vector& direction : directions)
        if (!proto.add_directions()->ParseFromString(direction.pb_dumps()))
            throw std::runtime_error("Failed to parse Vector protobuf data");
    proto.set_allowed_type(allowed_type);
    proto.set_extension(extension);
    proto.set_round_radius(round_radius);
    for (const Plane& cut : cuts)
        if (!proto.add_cuts()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Failed to parse Plane protobuf data");
    for (const Polyline& ring : profile)
        if (!proto.add_profile()->ParseFromString(ring.pb_dumps()))
            throw std::runtime_error("Failed to parse Polyline protobuf data");

    for (const auto& cut : solid_features)
        if (!proto.add_solid_features()->ParseFromString(cut.pb_dumps()))
            throw std::runtime_error("Invalid solid cut");
    for (const InteractionFeaturePlane& feature : plane_features)
        if (!proto.add_plane_features()->ParseFromString(feature.pb_dumps()))
            throw std::runtime_error("Invalid plane feature");
    return proto.SerializeAsString();
}

/// The element factory of a serialized beam: the protobuf bytes decoded as an Element and promoted to a EdgeStraightElement.
static std::shared_ptr<Element> beam_from_protobuf(const std::string& data) {
    return EdgeStraightElement::from_element(Element::pb_loads(data));
}

/// What a scene saved under the tag Column holds: an edge, or a variable edge when its level section leans off the axis.
static std::shared_ptr<Element> column_from_protobuf(const std::string& data) {

    Element element = Element::pb_loads(data);
    const std::string bytes = element.element_data_dumps();
    Line axis = Line::from_points(Point(0, 0, 0), Point(0, 0, 0));
    Polyline section;
    std::vector<Polyline> profile;
    double rotation = 0.0;
    std::vector<Plane> cuts;
    std::vector<InteractionFeatureSolid> solid_features;
    std::vector<InteractionFeaturePlane> plane_features;

    if (!bytes.empty() && bytes.front() == '{') {
        const nlohmann::json payload = nlohmann::json::parse(bytes, nullptr, false);
        if (payload.is_discarded())
            throw std::runtime_error("Invalid column JSON");
        if (payload.contains("axis") && !payload["axis"].is_null())
            axis = Line::jsonload(payload["axis"]);
        if (payload.contains("section") && !payload["section"].is_null())
            section = Polyline::jsonload(payload["section"]);
    } else {
        wood_proto::Column proto;
        if (proto.ParseFromString(bytes)) {
            if (proto.has_axis())
                axis = Line::pb_loads(proto.axis().SerializeAsString());
            if (proto.has_section())
                section = Polyline::pb_loads(proto.section().SerializeAsString());
            for (const session_proto::Plane& cut : proto.cuts())
                cuts.push_back(Plane::pb_loads(cut.SerializeAsString()));
            for (const session_proto::Polyline& ring : proto.profile())
                profile.push_back(Polyline::pb_loads(ring.SerializeAsString()));
            rotation = proto.rotation();
            for (const wood_proto::InteractionFeatureSolid& cut : proto.solid_features())
                solid_features.push_back(InteractionFeatureSolid::pb_loads(cut.SerializeAsString()));
            for (const wood_proto::InteractionFeaturePlane& feature : proto.plane_features())
                plane_features.push_back(
                    InteractionFeaturePlane::pb_loads(feature.SerializeAsString())
                );
        }
    }

    std::shared_ptr<EdgeElement> edge;
    if (!profile.empty() && axis.length() > 0.0)
        edge = std::make_shared<EdgeStraightElement>(
            Polyline({axis.start(), axis.end()}),
            profile,
            std::vector<Vector>{compute_up(axis, rotation)},
            element.name
        );
    else if (section.point_count() < 3 || axis.length() <= 0.0)
        edge = std::make_shared<EdgeStraightElement>();
    else if (is_square_to(section, axis.to_vector()))
        edge = EdgeStraightElement::from_section(axis, section, element.name);
    else
        edge = EdgeVariableElement::between(
            section, section.translated(axis.to_vector()), element.name
        );

    static_cast<Element&>(*edge) = std::move(element);
    edge->cuts = cuts;
    edge->solid_features = solid_features;
    edge->plane_features = plane_features;

    return edge;
}

void EdgeStraightElement::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_from_protobuf);
    Element::register_type(std::string(LEGACY_ELEMENT_TYPE), beam_from_protobuf);
    Element::register_type("Column", column_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════
std::string EdgeStraightElement::str() const {

    std::ostringstream os;
    os << "EdgeStraightElement(name=" << name << ", segments=" << axis.segment_count()
       << ", radii=" << radii.size() << ", is_visible=" << std::boolalpha << is_visible
       << ", is_locked=" << is_locked << ")";

    return os.str();
}

std::optional<Plane> EdgeStraightElement::base_plane() const {

    const std::vector<Polyline> rings = sections();

    if (axis.point_count() < 2 || rings.empty() || rings[0].point_count() < 2)
        return std::nullopt;

    const Polyline& first_ring = rings[0];
    const Vector ring_edge = first_ring.get_point(1) - first_ring.get_point(0);
    const Vector axis_direction = axis.get_point(1) - axis.get_point(0);
    return frame_along(axis.get_point(0), ring_edge, axis_direction);
}

}
