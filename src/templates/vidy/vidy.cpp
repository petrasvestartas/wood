#include "pch.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_vidy {

namespace {

/// Builds one closed bottom or top outline ring of facet at ring_idx, with optional corner chamfer setback.
Polyline build_miter_outline(const VidyFacet& facet, size_t ring_idx, double chamfer) {

    const std::vector<Point>& ring = facet.miter_rings[ring_idx];
    const size_t m = ring.size();
    std::vector<Point> pts;
    pts.reserve(2 * m + 1);

    if (chamfer <= 1e-6) {
        pts.assign(ring.begin(), ring.end());
        pts.push_back(pts.front());
        return Polyline(pts);
    }

    for (size_t k = 0; k < m; k++) {
        const Point& p0 = ring[k];
        const Point& p1 = ring[(k + 1) % m];
        const Vector dir = p1 - p0;
        const double len = dir.magnitude();
        const double trim = std::min(chamfer, 0.25 * len);
        const Vector unit = len > 1e-9 ? (dir * (1.0 / len)) : Vector(1.0, 0.0, 0.0);
        pts.push_back(p0 + unit * trim);
        pts.push_back(p1 - unit * trim);
    }
    pts.push_back(pts.front());
    return Polyline(pts);
}

/// Finds the side-face index (>= 2) of plate lying on side_plane; returns -1 when none matches.
int find_plate_side_face(const FacePlanarElement& plate, const Plane& side_plane) {

    int best_face = -1;
    double best_dist = 5.0;
    for (size_t face = 2; face < plate.polylines.size() && face < plate.planes.size(); face++) {
        const Point mid = plate.polylines[face].center();
        const double dist = std::abs((mid - side_plane.origin()).dot(side_plane.z_axis()));
        const double align =
            std::abs(plate.planes[face].z_axis().normalized().dot(side_plane.z_axis()));
        if (align > 0.98 && dist < best_dist) {
            best_dist = dist;
            best_face = static_cast<int>(face);
        }
    }
    return best_face;
}

/// Maximum non-negative cotangent of the two interior polygon corner angles flanking directed side s.
double side_corner_max_cotangent(const std::vector<Point>& ring, size_t s) {

    const size_t m = ring.size();
    const size_t next = (s + 1) % m;
    const size_t prev = (s + m - 1) % m;
    const size_t after = (s + 2) % m;

    const Vector e_fwd = (ring[next] - ring[s]).normalized();
    const Vector e_prv = (ring[prev] - ring[s]).normalized();
    const double sin0 = std::max(0.12, e_fwd.cross(e_prv).magnitude());
    const double cot0 = std::max(0.0, e_fwd.dot(e_prv) / sin0);

    const Vector e_bwd = (ring[s] - ring[next]).normalized();
    const Vector e_nxt = (ring[after] - ring[next]).normalized();
    const double sin1 = std::max(0.12, e_bwd.cross(e_nxt).magnitude());
    const double cot1 = std::max(0.0, e_bwd.dot(e_nxt) / sin1);

    return std::max(cot0, cot1);
}

/// Computes the symmetric joint-line end setback (mm) keeping linked shadow mortises clear of facet corners.
double compute_crease_setback(
    const VidyGuide& guide,
    const VidyFacet& fa,
    size_t sa_idx,
    const VidyFacet& fb,
    size_t sb_idx
) {

    const size_t ma = fa.vertices.size();
    const size_t mb = fb.vertices.size();
    double min_len = std::numeric_limits<double>::max();
    for (size_t h = 0; h < 4; h++) {
        min_len = std::min(min_len, fa.miter_rings[h][sa_idx].distance(fa.miter_rings[h][(sa_idx + 1) % ma]));
        min_len = std::min(min_len, fb.miter_rings[h][sb_idx].distance(fb.miter_rings[h][(sb_idx + 1) % mb]));
    }

    const double cos_fold = std::clamp(fa.normal.dot(fb.normal), -0.98, 0.98);
    const double sin_fold = std::max(0.25, std::sqrt(1.0 - cos_fold * cos_fold));
    const double shadow_reach = (guide.total_offset + 0.5 * guide.plate_thickness) / sin_fold;

    double max_cot = 0.0;
    max_cot = std::max(max_cot, side_corner_max_cotangent(fa.miter_rings[0], sa_idx));
    max_cot = std::max(max_cot, side_corner_max_cotangent(fa.miter_rings[3], sa_idx));
    max_cot = std::max(max_cot, side_corner_max_cotangent(fb.miter_rings[0], sb_idx));
    max_cot = std::max(max_cot, side_corner_max_cotangent(fb.miter_rings[3], sb_idx));

    const double raw_setback = shadow_reach * max_cot + 55.0;
    return std::clamp(raw_setback, 55.0, 0.34 * min_len);
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// VidyPavilion
// ═══════════════════════════════════════════════════════════════════════════

VidyPavilion::VidyPavilion(
    const VidyGuide& guide,
    bool with_joints,
    double corner_chamfer,
    double division_length,
    const std::string& name
)
    : WoodSession(name),
      guide(guide),
      with_joints(with_joints),
      corner_chamfer(std::max(0.0, corner_chamfer)),
      division_length(std::max(120.0, division_length)) {

    add_group("inner_walls");
    add_group("outer_walls");
    add_group("inner_roof");
    add_group("outer_roof");

    build_panels();
    draw_panels();
    if (with_joints) {
        solve_vidy_joinery();
    }
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::inner_plates() const {
    return _inner_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::outer_plates() const {
    return _outer_plates;
}

const std::vector<std::shared_ptr<FacePlanarElement>>& VidyPavilion::all_plates() const {
    return _all_plates;
}

size_t VidyPavilion::total_mortise_slots() const {
    return _total_mortise_slots;
}

void VidyPavilion::build_panels() {

    const std::vector<VidyFacet>& facets = guide.facets();
    _inner_plates.clear();
    _outer_plates.clear();
    _all_plates.clear();
    _inner_plates.reserve(facets.size());
    _outer_plates.reserve(facets.size());
    _all_plates.reserve(2 * facets.size());

    for (size_t f = 0; f < facets.size(); f++) {
        const VidyFacet& facet = facets[f];
        const std::string role_tag = (facet.role == VidyFacetRole::LeftWall)
            ? "left_wall"
            : ((facet.role == VidyFacetRole::RightWall) ? "right_wall" : "roof");

        const Polyline in_bot = build_miter_outline(facet, 0, corner_chamfer);
        const Polyline in_top = build_miter_outline(facet, 1, corner_chamfer);
        const std::shared_ptr<FacePlanarElement> inner = std::make_shared<FacePlanarElement>(
            in_bot,
            in_top,
            fmt::format("inner_{}_b{}_f{}", role_tag, facet.bay, facet.local_index)
        );

        const Polyline out_bot = build_miter_outline(facet, 2, corner_chamfer);
        const Polyline out_top = build_miter_outline(facet, 3, corner_chamfer);
        const std::shared_ptr<FacePlanarElement> outer = std::make_shared<FacePlanarElement>(
            out_bot,
            out_top,
            fmt::format("outer_{}_b{}_f{}", role_tag, facet.bay, facet.local_index)
        );

        _inner_plates.push_back(inner);
        _outer_plates.push_back(outer);
        _all_plates.push_back(inner);
        _all_plates.push_back(outer);
    }
}

void VidyPavilion::draw_panels() {

    const std::shared_ptr<TreeNode> inner_walls = group_named("inner_walls");
    const std::shared_ptr<TreeNode> outer_walls = group_named("outer_walls");
    const std::shared_ptr<TreeNode> inner_roof = group_named("inner_roof");
    const std::shared_ptr<TreeNode> outer_roof = group_named("outer_roof");

    const std::vector<VidyFacet>& facets = guide.facets();
    for (size_t f = 0; f < facets.size(); f++) {
        const bool is_roof = (facets[f].role == VidyFacetRole::Roof);
        add(_inner_plates[f], is_roof ? inner_roof : inner_walls);
        add(_outer_plates[f], is_roof ? outer_roof : outer_walls);
    }
}

void VidyPavilion::solve_vidy_joinery() {

    const std::vector<VidyFacet>& facets = guide.facets();
    const std::vector<VidyEdge>& edges = guide.edges();

    settings.search_type = face_to_face;
    settings.dihedral_angle = 175.0;
    settings.distance = 0.5;
    settings.distance_squared = 0.25;
    settings.angle = 0.12;
    settings.joint_parameters[3] = division_length;
    settings.joint_parameters[4] = 0.64;
    settings.joint_parameters[5] = 15.0;
    settings.joint_volume_extension.clear();

    for (const std::shared_ptr<FacePlanarElement>& plate : _all_plates) {
        plate->feature_types.assign(plate->polylines.size(), -1);
    }

    WoodSession::adjacency.clear();
    WoodSession::three_valence = {{1}};

    for (const VidyEdge& edge : edges) {
        if (edge.facet_b < 0 ||
            (edge.kind != VidyEdgeKind::Regular && edge.kind != VidyEdgeKind::BaySeparation)) {
            continue;
        }

        size_t fa_idx = static_cast<size_t>(edge.facet_a);
        size_t sa_idx = static_cast<size_t>(edge.side_a);
        size_t fb_idx = static_cast<size_t>(edge.facet_b);
        size_t sb_idx = static_cast<size_t>(edge.side_b);
        if (fa_idx > fb_idx) {
            std::swap(fa_idx, fb_idx);
            std::swap(sa_idx, sb_idx);
        }

        const VidyFacet& fa = facets[fa_idx];
        const VidyFacet& fb = facets[fb_idx];
        const bool wall_or_bay_seam = (edge.kind == VidyEdgeKind::BaySeparation) ||
            (fa.role != VidyFacetRole::Roof && fb.role != VidyFacetRole::Roof);
        const int joint_code = wall_or_bay_seam ? 16 : 15;

        for (size_t layer = 0; layer < 2; layer++) {
            FacePlanarElement& pa = *_all_plates[2 * fa_idx + layer];
            FacePlanarElement& pb = *_all_plates[2 * fb_idx + layer];
            const int face_a = find_plate_side_face(pa, fa.side_planes[sa_idx]);
            const int face_b = find_plate_side_face(pb, fb.side_planes[sb_idx]);
            if (face_a >= 0) {
                pa.feature_types[static_cast<size_t>(face_a)] = joint_code;
            }
            if (face_b >= 0) {
                pb.feature_types[static_cast<size_t>(face_b)] = joint_code;
            }
        }

        const bool inner_is_concave = (fb.centroid - fa.centroid).dot(fa.normal) < 0.0;
        const int concave_a = static_cast<int>(2 * fa_idx + (inner_is_concave ? 0 : 1));
        const int concave_b = static_cast<int>(2 * fb_idx + (inner_is_concave ? 0 : 1));
        const int convex_b = static_cast<int>(2 * fb_idx + (inner_is_concave ? 1 : 0));
        const int convex_a = static_cast<int>(2 * fa_idx + (inner_is_concave ? 1 : 0));

        const double setback = compute_crease_setback(guide, fa, sa_idx, fb, sb_idx);
        WoodSession::adjacency.push_back({concave_a, concave_b});
        settings.joint_volume_extension.insert(
            settings.joint_volume_extension.end(), {0.0, 0.0, -setback}
        );
        WoodSession::adjacency.push_back({convex_a, convex_b});
        settings.joint_volume_extension.insert(
            settings.joint_volume_extension.end(), {0.0, 0.0, -setback}
        );
        WoodSession::three_valence.push_back({concave_a, concave_b, convex_b, convex_a});
    }

    compute_features();

    _total_mortise_slots = 0;
    for (const std::shared_ptr<FacePlanarElement>& plate : _all_plates) {
        if (plate->features.bottom.size() > 1) {
            _total_mortise_slots += (plate->features.bottom.size() - 1);
        }
    }
}

} // namespace wood_vidy
