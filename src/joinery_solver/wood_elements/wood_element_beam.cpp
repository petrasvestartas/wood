#include "pch.h"
#include "wood_element_beam.h"
#include "element_beam.pb.h"
#include "wood_session.h"
#include "wood_feature_detection.h"
using namespace session_cpp;

namespace wood_session {

constexpr bool TRACE = false;

namespace {

/// Closest points of two axis segments and where they sit on their polylines.
struct Closest {
    double dist_sq;
    int pid0;
    int sid0;
    int pid1;
    int sid1;
    double t0;
    double t1;
};

bool has_valid_frame(const Vector& direction, const Vector& normal) {
    const double area = direction.cross(normal).magnitude_squared();
    return area > 0.0 && std::isfinite(area);
}

/// Whether a pair's end-type sum (0 cross, 1 side-to-end, 2 end-to-end) passes the dataset's allowed type: 0, 1 or -1 for any.
bool type_allowed(const int sum, const int allowed) {
    switch (allowed) {
        case 0: return sum == 0;
        case 1: return sum == 1 || sum == 2;
        case -1: return true;
        default: return false;
    }
}

/// The closest segment pair of every two axes within min_distance, keyed by axis pair.
std::map<uint64_t, Closest> compute_closest(const std::vector<std::vector<Line>>& lines, const double min_distance) {

    std::map<uint64_t, Closest> contacts;
    for (size_t a = 0; a < lines.size(); a++) {
        for (size_t sa = 0; sa < lines[a].size(); sa++) {

            const Line& la = lines[a][sa];
            if (!(la.squared_length() > 0.0))
                continue;

            for (size_t b = a + 1; b < lines.size(); b++) {
                for (size_t sb = 0; sb < lines[b].size(); sb++) {

                    const Line& lb = lines[b][sb];
                    if (!(lb.squared_length() > 0.0))
                        continue;

                    double t0;
                    double t1;
                    if (!Intersection::line_line_parameters(la, lb, t0, t1, 0.0, true, true))
                        continue;
                    if (!std::isfinite(t0) || !std::isfinite(t1))
                        continue;

                    const Point q0 = la.point_at(t0);
                    const Point q1 = lb.point_at(t1);
                    const double d2 = (q0 - q1).magnitude_squared();
                    if (!std::isfinite(d2) || d2 > min_distance * min_distance)
                        continue;

                    const uint64_t id = ((uint64_t)b << 32) | (uint64_t)a;
                    const Closest c{d2, (int)a, (int)sa, (int)b, (int)sb, t0, t1};
                    const auto it = contacts.find(id);
                    if (it == contacts.end() || d2 < it->second.dist_sq)
                        contacts[id] = c;
                }
            }
        }
    }

    return contacts;
}

}  // namespace

Beam::Beam() : Element("beam") {}

Beam::Beam(const Polyline& axis, double radius, const std::string& name)
    : Element(name), axis(axis), radii(axis.segment_count(), radius) {}

Beam::Beam(const Polyline& axis, const std::vector<double>& radii, const std::vector<Vector>& directions, int allowed_type, const std::string& name)
    : Element(name), axis(axis), radii(radii), directions(directions), allowed_type(allowed_type) {}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<Beam> Beam::from_element(const Element& e) {

    std::shared_ptr<Beam> beam = std::make_shared<Beam>();
    static_cast<Element&>(*beam) = e;
    beam->guid() = e.guid();

    const std::string bytes = e.element_data_dumps();
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

    wood_proto::Beam proto;
    if (!proto.ParseFromString(bytes))
        return beam;

    if (proto.has_axis())
        beam->axis = Polyline::pb_loads(proto.axis().SerializeAsString());
    beam->radii.assign(proto.radii().begin(), proto.radii().end());
    for (const session_proto::Vector& direction : proto.directions())
        beam->directions.push_back(Vector::pb_loads(direction.SerializeAsString()));
    beam->allowed_type = proto.allowed_type();

    return beam;
}

WoodSession Beam::joint_volumes(
    const std::vector<std::shared_ptr<Beam>>& beams,
    double min_distance,
    double volume_length,
    double cross_or_side_to_end,
    int flip_male
) {

    using namespace wood_session::config;

    WoodSession session("WoodF2F");
    const std::shared_ptr<TreeNode> g_axes = session.add_group("BeamAxes");
    const std::shared_ptr<TreeNode> g_vols = session.add_group("JointVolumes");
    g_axes->color = Color(0.70f, 0.70f, 0.70f, 1.0f, "grey");
    g_vols->color = Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta");

    const double zero_length_squared = 1e-6; // A joint line no longer than 1 mm is degenerate.
    std::vector<std::vector<Line>> lines;
    lines.reserve(beams.size());
    for (size_t i = 0; i < beams.size(); i++) {
        session.add(beams[i]);
        std::shared_ptr<Polyline> pl = std::make_shared<Polyline>(beams[i]->axis);
        pl->name = fmt::format("axis_{}", i);
        session.add_polyline(pl, g_axes);
        lines.push_back(beams[i]->axis.get_lines());
    }

    const std::map<uint64_t, Closest> contacts = compute_closest(lines, min_distance);

    int n_pairs = 0;
    int n_success = 0;
    int n_failed = 0;
    int counts[6] = {0, 0, 0, 0, 0, 0};

    for (const std::pair<const uint64_t, Closest>& entry : contacts) {
        const Closest& c = entry.second;
        n_pairs++;
        const Beam& beam0 = *beams[c.pid0];
        const Beam& beam1 = *beams[c.pid1];
        const Polyline& pa_pts = beam0.axis;
        const Polyline& pb_pts = beam1.axis;
        const Line s0 = lines[c.pid0][c.sid0];
        const Line s1 = lines[c.pid1][c.sid1];

        Point p0;
        Point p1;
        Vector v0;
        Vector v1;
        Vector normal;
        bool type0 = false;
        bool type1 = false;
        bool is_parallel = false;
        bool ok = Intersection::line_line_classified(
            s0, s1,
            (int)(pa_pts.point_count() - 1), (int)(pb_pts.point_count() - 1),
            c.sid0, c.sid1,
            cross_or_side_to_end,
            p0, p1, v0, v1, normal,
            type0, type1, is_parallel
        );
        if (!ok) {
            n_failed++;
            continue;
        }

        const int sum = (int)type0 + (int)type1;
        if (!type_allowed(sum, beam0.allowed_type) || !type_allowed(sum, beam1.allowed_type))
            continue;

        const Vector sn0 = beam0.has_direction(c.sid0) ? beam0.directions[c.sid0] : normal;
        const Vector sn1 = beam1.has_direction(c.sid1) ? beam1.directions[c.sid1] : normal;
        const double r0 = beam0.radius(c.sid0);
        const double r1 = beam1.radius(c.sid1);
        if (!(r0 > 0.0) || !(r1 > 0.0) || !std::isfinite(r0) || !std::isfinite(r1) ||
            !(volume_length > 0.0) || !std::isfinite(volume_length) ||
            !has_valid_frame(v0, sn0) || !has_valid_frame(v1, sn1)) {
            n_failed++;
            continue;
        }

        std::array<Polyline, 4> beam_vol;
        Polyline::two_rects_from_frame(p0, v0, sn0, type0 == 1, r0, volume_length, flip_male, beam_vol[0], beam_vol[1]);
        Polyline::two_rects_from_frame(p1, v1, sn1, type1 == 1, r1, volume_length, flip_male, beam_vol[2], beam_vol[3]);

        if (sum == 0) {
            const Point pm = Point::mid_point(p0, p1);
            const Vector bisector = is_parallel ? v0 : v0 - v1;
            const Plane cp = Plane::from_point_normal(pm, bisector);
            const bool toward_v0 = !cp.has_on_negative_side(pm + v0);
            const Vector npos = cp.z_axis();
            const Vector nneg = -npos;
            const Plane cut_plane0 = Plane::from_point_normal(pm, toward_v0 ? npos : nneg);
            const Plane cut_plane1 = Plane::from_point_normal(pm, toward_v0 ? nneg : npos);
            for (int lid = 0; lid < 2; lid++) {
                const int shift = lid == 0 ? 0 : 2;
                const Plane& cutpl = lid == 0 ? cut_plane0 : cut_plane1;
                if (!Polyline::trim_rectangles_by_plane(beam_vol[shift], beam_vol[shift + 1], cutpl)) {
                    ok = false;
                    break;
                }
            }
        } else if (sum == 1) {
            int closer_rect;
            int farrer_rect;
            if (type0 == 0) {
                const Point pp = p0 + v0;
                const bool closer = Point::distance(pp, beam_vol[2].get_point(0)) < Point::distance(pp, beam_vol[3].get_point(0));
                closer_rect = closer ? 2 : 3;
                farrer_rect = closer ? 3 : 2;
            } else {
                const Point pp = p1 + v1;
                const bool closer = Point::distance(pp, beam_vol[0].get_point(0)) < Point::distance(pp, beam_vol[1].get_point(0));
                closer_rect = closer ? 0 : 1;
                farrer_rect = closer ? 1 : 0;
            }
            const Polyline& qc = beam_vol[closer_rect];
            const Vector rv0 = qc[1] - qc[0];
            const Vector rv1 = qc[2] - qc[0];
            const Vector rnrm = rv0.cross(rv1);
            Plane cutpl = Plane::from_point_normal(qc[0], rnrm);
            if (!cutpl.has_on_negative_side(beam_vol[farrer_rect][0]))
                cutpl = Plane::from_point_normal(qc[0], -rnrm);
            const int shift = type0 == 0 ? 0 : 2;
            ok = Polyline::trim_rectangles_by_plane(beam_vol[shift], beam_vol[shift + 1], cutpl);
        }

        if (!ok) {
            n_failed++;
            continue;
        }

        for (int k = 0; k < 4; k++) {
            std::shared_ptr<Polyline> rect = std::make_shared<Polyline>(beam_vol[k]);
            rect->name = fmt::format("beam_{}_{}_rect{}", c.pid0, c.pid1, k);
            session.add_polyline(rect, g_vols);
        }

        Interaction& interaction = session.add_interaction(beam0.guid(), beam1.guid());
        InteractionFeature feature(FeatureBeam{sum, beam_vol});
        feature.contact = interaction.add_contact(InteractionContact(ContactAxis(Line::from_points(s0.point_at(c.t0), s1.point_at(c.t1)), c.t0, c.t1, 0, c.sid0, 0, c.sid1)));
        interaction.add_feature(std::move(feature));

        Plate el0(beam_vol[0], beam_vol[1]);
        Plate el1(beam_vol[2], beam_vol[3]);

        FeaturePlate jt;
        bool swap_planes_1 = false;
        const bool jok = face_to_face_wood(
            el0,
            el1,
            {c.pid0, c.pid1},
            JOINT_VOLUME_EXTENSION,
            0.0,
            zero_length_squared,
            DISTANCE_SQUARED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_DIHEDRAL_ANGLE,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ALL_TREATED_AS_ROTATED,
            FACE_TO_FACE_SIDE_TO_SIDE_JOINTS_ROTATED_JOINT_AS_AVERAGE,
            (sum == 2 ? 1 : 0),
            jt,
            swap_planes_1
        );
        if (!jok) {
            n_failed++;
            continue;
        }

        n_success++;
        switch (jt.joint_type) {
            case 11: counts[0]++; break;
            case 12: counts[1]++; break;
            case 13: counts[2]++; break;
            case 20: counts[3]++; break;
            case 30: counts[4]++; break;
            case 40: counts[5]++; break;
            default: break;
        }
    }

    if (TRACE) {
        std::cout << fmt::format("\n=== Beam::joint_volumes ===\n");
        std::cout << fmt::format("{} axes -> {} contacts -> {} volumes ({} failed)\n", beams.size(), n_pairs, n_success, n_failed);
        std::cout << fmt::format("  by type: 11={} 12={} 13={} 20={} 30={} 40={}\n", counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
    }

    return session;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

double Beam::radius(int segment) const {
    return segment >= 0 && segment < (int)radii.size() ? radii[segment] : 0.0;
}

bool Beam::has_direction(int segment) const {
    return segment >= 0 && segment < (int)directions.size();
}

AABB Beam::aabb(double inflate) const {

    const double reach = radii.empty() ? 0.0 : *std::max_element(radii.begin(), radii.end());

    return AABB::from_polyline(axis, inflate + reach);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json Beam::element_data_jsondump() const {

    nlohmann::ordered_json ups = nlohmann::ordered_json::array();
    for (const Vector& direction : directions)
        ups.push_back(direction.jsondump());

    return nlohmann::ordered_json{
        {"allowed_type", allowed_type},
        {"axis", axis.jsondump()},
        {"directions", ups},
        {"radii", radii},
        {"type", std::string(ELEMENT_TYPE)},
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Protobuf
// ═══════════════════════════════════════════════════════════════════════════

std::string Beam::element_data_dumps() const {

    wood_proto::Beam proto;
    proto.mutable_axis()->ParseFromString(axis.pb_dumps());
    proto.mutable_radii()->Add(radii.begin(), radii.end());
    for (const Vector& direction : directions)
        proto.add_directions()->ParseFromString(direction.pb_dumps());
    proto.set_allowed_type(allowed_type);

    return proto.SerializeAsString();
}

/// The element factory of a serialized beam: the protobuf bytes decoded as an Element and promoted to a Beam.
static std::shared_ptr<Element> beam_from_protobuf(const std::string& data) {
    return Beam::from_element(Element::pb_loads(data));
}

void Beam::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string Beam::str() const {

    std::ostringstream os;
    os << "Beam(name=" << name << ", segments=" << axis.segment_count() << ", radii=" << radii.size() << ")";

    return os.str();
}

} // namespace wood_session
