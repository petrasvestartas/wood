#include "pch.h"
#include "src/templates/floor/floor_geometry.h"
#include "wood_brep_drill.h"
#include <numeric>

using namespace session_cpp;

namespace wood_floor {

const double SAMPLE_STEP = 0.25; // mm between the points a screw is sampled at against a pocket
const double NEAR = 30.0; // mm a solid's box is inflated by before a screw is measured against it

/// A convex solid of another connector a screw must keep out of: its mesh as triangles and its box.
struct KeepOut {
    std::string name; // The connector it belongs to.
    std::vector<wood_session::PlanarFace> faces; // The solid's faces, for the inside test.
    std::vector<std::array<Point, 3>> triangles; // Its faces fanned into triangles.
    AABB box; // Its box, inflated by NEAR.
    std::optional<std::vector<wood_session::PlanarFace>> within; // For a cutter, its target's solid faces: only the screw's points inside the target meet the pocket.
    std::vector<std::string> targets; // The guids of the connector's members.
};

// ═══════════════════════════════════════════════════════════════════════════
// Distances
// ═══════════════════════════════════════════════════════════════════════════

/// The distance between two segments.
static double segment_distance(const Line& s, const Line& t) {

    const Vector d1 = s.to_vector();
    const Vector d2 = t.to_vector();
    const Vector r = s.start() - t.start();
    const double a = d1.dot(d1);
    const double e = d2.dot(d2);
    const double f = d2.dot(r);
    const double c = d1.dot(r);
    const double b = d1.dot(d2);
    const double denominator = a * e - b * b;
    double u = denominator > 1e-12 ? std::clamp((b * f - c * e) / denominator, 0.0, 1.0) : 0.0;
    double v = (b * u + f) / e;

    if (v < 0.0) {
        v = 0.0;
        u = std::clamp(-c / a, 0.0, 1.0);
    } else if (v > 1.0) {
        v = 1.0;
        u = std::clamp((b - c) / a, 0.0, 1.0);
    }

    return ((s.start() + d1 * u) - (t.start() + d2 * v)).magnitude();
}

/// The distance from a point to the segment a b.
static double point_distance(const Point& p, const Point& a, const Point& b) {
    return (p - Line::from_points(a, b).closest_point(p).second).magnitude();
}

/// The distance from a point to a triangle.
static double triangle_distance(const Point& p, const std::array<Point, 3>& tri) {

    const Vector ab = tri[1] - tri[0];
    const Vector ac = tri[2] - tri[0];
    const Vector n = ab.cross(ac);
    const double area = n.magnitude();

    if (area < 1e-12)
        return std::min({point_distance(p, tri[0], tri[1]), point_distance(p, tri[1], tri[2]), point_distance(p, tri[2], tri[0])});

    const Vector unit = n * (1.0 / area);
    const double height = (p - tri[0]).dot(unit);
    const Point q = p - unit * height;
    bool inside = true;

    for (size_t i = 0; i < 3; i++)
        inside = inside && (tri[(i + 1) % 3] - tri[i]).cross(q - tri[i]).dot(unit) >= 0.0;

    if (inside)
        return std::abs(height);

    double best = 1e300;

    for (size_t i = 0; i < 3; i++)
        best = std::min(best, point_distance(p, tri[i], tri[(i + 1) % 3]));

    return best;
}

/// The keep-out of one closed solid.
static KeepOut keep_out(const std::string& name, const Mesh& mesh) {

    KeepOut solid;
    solid.name = name;
    solid.faces = wood_session::planar_faces(mesh);
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = mesh.to_vertices_and_faces();

    for (const std::vector<size_t>& face : data.second)
        for (size_t i = 1; i + 1 < face.size(); i++)
            solid.triangles.push_back({data.first[face[0]], data.first[face[i]], data.first[face[i + 1]]});

    solid.box = AABB::from_mesh(mesh, NEAR);

    return solid;
}

/// The signed distance from a screw's surface to a solid: the closest sampled axis point's distance less the radius, negative where the screw enters it, a pocket's only over the points inside its target; 1e300 when the solid is out of reach.
static double solid_clearance(const Line& screw, double radius, const KeepOut& solid) {

    const AABB reach = AABB::from_line(screw);

    if (std::abs(reach.cx - solid.box.cx) > reach.hx + solid.box.hx || std::abs(reach.cy - solid.box.cy) > reach.hy + solid.box.hy || std::abs(reach.cz - solid.box.cz) > reach.hz + solid.box.hz)
        return 1e300;

    const size_t samples = static_cast<size_t>(std::ceil(screw.length() / SAMPLE_STEP));
    double best = 1e300;

    for (size_t i = 0; i <= samples; i++) {
        const Point p = screw.start() + screw.to_vector() * (static_cast<double>(i) / samples);
        double distance = 1e300;

        if (solid.within && !wood_session::is_inside(*solid.within, p))
            continue;

        for (const std::array<Point, 3>& triangle : solid.triangles)
            distance = std::min(distance, triangle_distance(p, triangle));

        best = std::min(best, wood_session::is_inside(solid.faces, p) ? -distance - radius : distance - radius);
    }

    return best;
}

// ═══════════════════════════════════════════════════════════════════════════
// Check
// ═══════════════════════════════════════════════════════════════════════════

/// Every bore and every pocket or part of the connectors that are not screws, each part once from its connector, not again from its part child: the bores as lines run on by their overshoot, with their radius, each pocket within its target.
static void collect(const wood_session::WoodSession& session, std::vector<std::pair<Line, double>>& bores, std::vector<KeepOut>& solids) {

    for (const std::shared_ptr<wood_session::JointBeam>& connector : session.get_elements<wood_session::JointBeam>()) {
        if (connector->pre_drill || std::dynamic_pointer_cast<wood_session::ConnectorPart>(connector))
            continue;

        for (const Line& dowel : connector->drill_lines) {
            const Vector d = dowel.to_vector().normalized() * connector->drill_overshoot;
            bores.push_back({Line::from_points(dowel.start() - d, dowel.end() + d), connector->line_radius});
        }

        for (size_t i = 0; i < connector->parts.size(); i++) {
            solids.push_back(keep_out(connector->name, connector->part_mesh(i)));
            solids.back().targets = connector->targets;
        }

        for (size_t side = 0; side < connector->cutters.size() && side < connector->targets.size(); side++) {
            const std::vector<wood_session::PlanarFace> target = wood_session::planar_faces(uncut(*session.get_element<Element>(connector->targets[side]))->element_geometry_mesh());

            for (const std::array<Polyline, 2>& cutter : connector->cutters[side]) {
                solids.push_back(keep_out(connector->name, Mesh::loft({cutter[0]}, {cutter[1]}, true)));
                solids.back().within = target;
                solids.back().targets = connector->targets;
            }
        }
    }
}

/// How much of a screw each member it names holds, their solids before any cut, the stretches clipped to the screw.
static std::vector<double> held(const wood_session::WoodSession& session, const wood_session::JointBeam& connector, const Line& screw) {

    std::vector<double> lengths;

    for (const std::string& guid : connector.targets) {
        const std::shared_ptr<Element> target = session.get_element<Element>(guid);
        lengths.push_back(0.0);

        for (const std::array<double, 2>& stretch : wood_session::inside_stretches(uncut(*target)->element_geometry_mesh(), screw))
            lengths.back() += std::max(0.0, std::min(stretch[1], screw.length()) - std::max(stretch[0], 0.0));
    }

    return lengths;
}

/// Measures one screw against the other screws after it, the bores and the solids, and how its members hold it; a screw drilled from a seam face before the wedge goes in, its member drilled_from, may cross the parts and pockets of the connectors on that member.
static void check_screw(const wood_session::WoodSession& session, const wood_session::JointBeam& connector, size_t index, const std::vector<std::pair<Line, double>>& bores, const std::vector<KeepOut>& solids, const std::string& drilled_from, ScrewCheck& check) {

    const Line& screw = connector.drill_lines[index];
    const std::string name = fmt::format("{} screw {}", connector.name, index);
    const std::vector<double> lengths = held(session, connector, screw);
    const double total = std::accumulate(lengths.begin(), lengths.end(), 0.0);
    check.embedded_min_mm = std::min(check.embedded_min_mm, total);
    check.member_min_mm = std::min({check.member_min_mm, lengths[0], lengths[1]});

    if (lengths[0] < 1e-6 || lengths[1] < 1e-6 || std::abs(total - screw.length()) > 1e-3)
        check.misfits.push_back(fmt::format("{}: held {:.3f} + {:.3f} of {:.3f} mm over {} members", name, lengths[0], lengths[1], screw.length(), lengths.size()));

    for (const std::pair<Line, double>& bore : bores) {
        const double clearance = segment_distance(screw, bore.first) - connector.line_radius - bore.second;
        check.screw_bore_mm = std::min(check.screw_bore_mm, clearance);

        if (clearance < 0.0)
            check.misfits.push_back(fmt::format("{}: into a bore, {:.3f} mm", name, clearance));
    }

    for (const KeepOut& solid : solids) {
        if (!drilled_from.empty() && std::find(solid.targets.begin(), solid.targets.end(), drilled_from) != solid.targets.end())
            continue;

        const double clearance = solid_clearance(screw, connector.line_radius, solid);
        check.screw_pocket_mm = std::min(check.screw_pocket_mm, clearance);

        if (clearance < 0.0)
            check.misfits.push_back(fmt::format("{}: into {}, {:.3f} mm", name, solid.name, clearance));
    }
}

ScrewCheck check_screws(const wood_session::WoodSession& session, const FloorGuide& guide, const std::vector<std::shared_ptr<wood_session::JointBeam>>& screws) {

    ScrewCheck check;
    std::vector<std::pair<Line, double>> bores;
    std::vector<KeepOut> solids;
    std::vector<std::pair<std::string, Line>> all;
    collect(session, bores, solids);
    std::vector<std::string> drilled_from;
    const std::vector<Relationship> rows = relationships(guide);
    const size_t screwed = static_cast<size_t>(std::count_if(rows.begin(), rows.end(), [](const Relationship& row) { return !row.screws.empty(); }));

    if (screws.size() != screwed)
        throw std::invalid_argument(fmt::format("check_screws reads one screw connector per screw row in relationship order: {} connectors for {} rows", screws.size(), screwed));

    for (const Relationship& row : rows)
        if (!row.screws.empty()) {
            check.counts[row.kind] += screws[drilled_from.size()]->drill_lines.size();
            drilled_from.push_back(row.kind == Relation::screw_rib_beam && guide.seam_through_ribs ? screws[drilled_from.size()]->targets[1] : "");
        }

    for (size_t c = 0; c < screws.size(); c++) {
        const wood_session::JointBeam& connector = *screws[c];

        for (size_t i = 0; i < connector.drill_lines.size(); i++) {
            check_screw(session, connector, i, bores, solids, drilled_from[c], check);
            all.push_back({fmt::format("{} screw {}", connector.name, i), connector.drill_lines[i]});
        }
    }

    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++) {
            const double distance = segment_distance(all[i].second, all[j].second);
            check.screw_screw_mm = std::min(check.screw_screw_mm, distance);

            if (distance < SCREW_SPACING)
                check.misfits.push_back(fmt::format("{} and {}: {:.3f} mm apart", all[i].first, all[j].first, distance));
        }

    return check;
}

std::string ScrewCheck::str() const {

    std::string text = "screws:";
    size_t total = 0;

    for (const std::pair<const Relation, size_t>& count : counts) {
        text += fmt::format(" {} {},", relation_name(count.first), count.second);
        total += count.second;
    }

    text += fmt::format(" {} in all\n", total);
    text += fmt::format("  closest: screw to screw {:.3f} mm (axes), screw to bore {:.3f} mm, screw to pocket or part {:.3f} mm (surfaces)\n", screw_screw_mm, screw_bore_mm, screw_pocket_mm);
    text += fmt::format("  held: every screw {:.3f} mm in the members it names at the least, {:.3f} mm in one member of its joint\n", embedded_min_mm, member_min_mm);
    text += fmt::format("  misfits: {}", misfits.size());

    for (const std::string& misfit : misfits)
        text += "\n    " + misfit;

    return text;
}

}
