#include "src/templates/folding/fold_unfolding.h"

FoldUnfolding::FoldUnfolding(const Mesh& mesh, double gap) : gap(std::max(gap, 0.0)) {

    Mesh oriented = mesh;
    oriented.unify_winding();
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data =
        oriented.to_vertices_and_faces();
    const std::vector<Point>& points = data.first;
    const std::vector<std::vector<size_t>>& loops = data.second;
    const size_t face_count = loops.size();

    if (face_count == 0)
        return;

    // frames: outward unit normal and centroid per 3D face
    std::vector<Vector> normals;
    std::vector<Point> centroids;
    compute_face_frames(points, loops, normals, centroids);

    // dual graph: shared interior edges and per-face adjacency
    const std::vector<DualEdge> dual_edges = compute_dual_edges(points, loops, normals);
    std::vector<std::vector<size_t>> incident(face_count);

    for (size_t e = 0; e < dual_edges.size(); e++) {
        incident[dual_edges[e].face_a].push_back(e);
        incident[dual_edges[e].face_b].push_back(e);
    }

    // sheets: greedy priority hinge forest with exact 2D non-overlap check
    faces.resize(face_count);
    transforms.resize(face_count);
    sheet_of.assign(face_count, 0);
    std::vector<bool> placed(face_count, false);
    double cursor_x = 0.0;

    for (size_t seed = 0; seed < face_count; seed++) {
        if (placed[seed])
            continue;

        const std::vector<size_t> sheet_faces = grow_sheet(
            seed, sheet_count, points, loops, normals, centroids, dual_edges, incident, placed
        );
        cursor_x = finalize_sheet(sheet_faces, points, loops, cursor_x);
        sheet_count++;
    }

    // creases: every interior edge classified as hinge or cut seam
    classify_creases(points, dual_edges);
}

size_t FoldUnfolding::cut_count() const {

    size_t count = 0;

    for (const FoldCrease& crease : creases)
        if (crease.is_cut)
            count++;

    return count;
}

size_t FoldUnfolding::hinge_count() const {

    size_t count = 0;

    for (const FoldCrease& crease : creases)
        if (!crease.is_cut && std::abs(crease.angle) > 1e-4)
            count++;

    return count;
}

std::shared_ptr<FacePlanarElement> FoldUnfolding::flatten_plate(const FacePlanarElement& plate) {

    const std::optional<Plane> base = plate.base_plane();

    if (!base)
        return plate.transformed(Xform::identity());

    const Xform to_xy =
        Xform::world_to_frame(base->origin(), base->x_axis(), base->y_axis(), base->z_axis());
    std::shared_ptr<FacePlanarElement> flat = plate.transformed(to_xy);
    const AABB bounds = flat->aabb(0.0);
    const Xform shift =
        Xform::translation(-bounds.min_point()[0], -bounds.min_point()[1], -bounds.min_point()[2]);
    flat->place(shift);
    return flat;
}

void FoldUnfolding::add_to(WoodSession& session, double beside) const {

    if (sheets.empty())
        return;

    double low_x = std::numeric_limits<double>::infinity();
    double high_x = -std::numeric_limits<double>::infinity();
    double low_y = std::numeric_limits<double>::infinity();
    double high_y = -std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Element>& element : session.elements())
        for (const Point& corner : element->aabb().corners()) {
            low_x = std::min(low_x, corner[0]);
            high_x = std::max(high_x, corner[0]);
            low_y = std::min(low_y, corner[1]);
            high_y = std::max(high_y, corner[1]);
        }

    if (!std::isfinite(high_x)) {
        low_x = 0.0;
        high_x = 0.0;
        low_y = 0.0;
        high_y = 0.0;
    }

    const double margin =
        beside > 0.0 ? beside : std::max(400.0, 0.15 * std::max(high_x - low_x, high_y - low_y));
    const Xform place = Xform::translation(high_x + margin, low_y, 0.0);
    const std::shared_ptr<TreeNode> group = session.group_named("flat");

    for (size_t s = 0; s < sheets.size(); s++) {
        Polyline outline = sheets[s].transformed(place);
        outline.name = fmt::format("flat_sheet_{}", s);
        session.add_polyline(outline, group);
    }

    size_t crease_index = 0;

    for (const FoldCrease& crease : creases) {
        if (crease.is_cut || std::abs(crease.angle) <= 1e-4)
            continue;

        Polyline line({
            place.transform_point(crease.line_2d.start()),
            place.transform_point(crease.line_2d.end()),
        });
        line.name = fmt::format("flat_crease_{}", crease_index);
        session.add_polyline(line, group);
        crease_index++;
    }
}

void FoldUnfolding::compute_face_frames(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& loops,
    std::vector<Vector>& normals,
    std::vector<Point>& centroids
) {

    normals.resize(loops.size());
    centroids.resize(loops.size());

    for (size_t f = 0; f < loops.size(); f++) {
        normals[f] = newell_normal(points, loops[f]);
        Vector sum(0.0, 0.0, 0.0);

        for (const size_t v : loops[f])
            sum = sum + Vector(points[v][0], points[v][1], points[v][2]);

        const double inv = 1.0 / static_cast<double>(loops[f].size());
        centroids[f] = Point(sum[0] * inv, sum[1] * inv, sum[2] * inv);
    }
}

std::vector<FoldUnfolding::DualEdge> FoldUnfolding::compute_dual_edges(
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& loops,
    const std::vector<Vector>& normals
) {

    std::map<std::pair<size_t, size_t>, std::pair<size_t, bool>> seen;
    std::vector<DualEdge> edges;

    for (size_t f = 0; f < loops.size(); f++) {
        const std::vector<size_t>& loop = loops[f];
        const size_t n = loop.size();

        for (size_t i = 0; i < n; i++) {
            const size_t u = loop[i];
            const size_t v = loop[(i + 1) % n];
            const std::pair<size_t, size_t> key = {std::min(u, v), std::max(u, v)};
            const std::map<std::pair<size_t, size_t>, std::pair<size_t, bool>>::const_iterator hit =
                seen.find(key);

            if (hit == seen.end()) {
                seen[key] = {f, u < v};
                continue;
            }

            const size_t face_a = hit->second.first;
            const size_t edge_u = hit->second.second ? key.first : key.second;
            const size_t edge_v = hit->second.second ? key.second : key.first;
            const Vector dir = points[edge_v] - points[edge_u];
            const double length = dir.magnitude();

            if (length < 1e-6)
                continue;

            const Vector axis = dir * (1.0 / length);
            const double cosine = std::clamp(normals[face_a].dot(normals[f]), -1.0, 1.0);
            const double sine = normals[face_a].cross(normals[f]).dot(axis);
            const double angle = std::atan2(sine, cosine);
            const double score = length / (1.0 + std::abs(angle));
            edges.push_back({face_a, f, edge_u, edge_v, length, angle, score});
        }
    }

    return edges;
}

std::vector<size_t> FoldUnfolding::grow_sheet(
    size_t seed,
    size_t piece_index,
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& loops,
    const std::vector<Vector>& normals,
    const std::vector<Point>& centroids,
    const std::vector<DualEdge>& dual_edges,
    const std::vector<std::vector<size_t>>& incident,
    std::vector<bool>& placed
) {

    std::vector<size_t> sheet_faces = {seed};
    transforms[seed] = root_transform(points, loops[seed], normals[seed]);
    faces[seed] = flat_polygon(points, loops[seed], transforms[seed]);
    std::vector<Polyline> sheet_polys = {faces[seed]};
    placed[seed] = true;
    sheet_of[seed] = piece_index;

    for (size_t step = 0; step < loops.size(); step++) {
        int best_edge = -1;
        double best_score = -1.0;
        Xform best_xform;
        Polyline best_poly;
        size_t best_child = 0;

        for (const size_t parent : sheet_faces)
            for (const size_t edge_id : incident[parent]) {
                const DualEdge& edge = dual_edges[edge_id];
                const size_t child = edge.face_a == parent ? edge.face_b : edge.face_a;

                if (placed[child] || edge.score <= best_score)
                    continue;

                const Xform candidate_xform = hinge_transform(
                    points[edge.u],
                    points[edge.v],
                    centroids[child],
                    normals[child],
                    transforms[parent],
                    faces[parent].center()
                );
                const Polyline candidate_poly = flat_polygon(points, loops[child], candidate_xform);

                if (overlaps_sheet(candidate_poly, sheet_polys))
                    continue;

                best_edge = static_cast<int>(edge_id);
                best_score = edge.score;
                best_xform = candidate_xform;
                best_poly = candidate_poly;
                best_child = child;
            }

        if (best_edge < 0)
            break;

        placed[best_child] = true;
        sheet_of[best_child] = piece_index;
        transforms[best_child] = best_xform;
        faces[best_child] = best_poly;
        sheet_faces.push_back(best_child);
        sheet_polys.push_back(best_poly);
    }

    return sheet_faces;
}

double FoldUnfolding::finalize_sheet(
    const std::vector<size_t>& sheet_faces,
    const std::vector<Point>& points,
    const std::vector<std::vector<size_t>>& loops,
    double cursor_x
) {

    double low_x = std::numeric_limits<double>::infinity();
    double high_x = -std::numeric_limits<double>::infinity();
    double low_y = std::numeric_limits<double>::infinity();

    for (const size_t f : sheet_faces)
        for (const Point& pt : faces[f].get_points()) {
            low_x = std::min(low_x, pt[0]);
            high_x = std::max(high_x, pt[0]);
            low_y = std::min(low_y, pt[1]);
        }

    const Xform shift = Xform::translation(cursor_x - low_x, -low_y, 0.0);
    std::vector<Polyline> shifted_polys;
    shifted_polys.reserve(sheet_faces.size());

    for (const size_t f : sheet_faces) {
        transforms[f] = shift * transforms[f];
        faces[f] = faces[f].transformed(shift);
        shifted_polys.push_back(faces[f]);
        distortion = std::max(distortion, face_distortion(points, loops[f], transforms[f]));
    }

    const std::vector<Polyline> merged_rings = union_sheet(shifted_polys);
    sheets.insert(sheets.end(), merged_rings.begin(), merged_rings.end());
    return cursor_x + (high_x - low_x) + gap;
}

void FoldUnfolding::classify_creases(
    const std::vector<Point>& points, const std::vector<DualEdge>& dual_edges
) {

    for (const DualEdge& edge : dual_edges) {
        const Point a0 = transforms[edge.face_a].transform_point(points[edge.u]);
        const Point a1 = transforms[edge.face_a].transform_point(points[edge.v]);
        const Point b0 = transforms[edge.face_b].transform_point(points[edge.u]);
        const Point b1 = transforms[edge.face_b].transform_point(points[edge.v]);
        const bool intact = sheet_of[edge.face_a] == sheet_of[edge.face_b] &&
                            a0.distance(b0) < 1e-2 && a1.distance(b1) < 1e-2;
        creases.push_back({
            edge.face_a,
            edge.face_b,
            Line::from_points(points[edge.u], points[edge.v]),
            Line::from_points(Point(a0[0], a0[1], 0.0), Point(a1[0], a1[1], 0.0)),
            edge.angle,
            !intact,
        });
    }
}

Xform FoldUnfolding::root_transform(
    const std::vector<Point>& points, const std::vector<size_t>& loop, const Vector& normal
) {

    const Point& origin = points[loop[0]];
    Vector x_axis = points[loop[1]] - origin;

    if (!x_axis.normalize_self())
        x_axis = Vector::x_axis();

    const Vector z_axis = normal.normalized();
    const Vector y_axis = z_axis.cross(x_axis).normalized();
    return Xform::world_to_frame(origin, x_axis, y_axis, z_axis);
}

Xform FoldUnfolding::hinge_transform(
    const Point& u_3d,
    const Point& v_3d,
    const Point& child_centre_3d,
    const Vector& child_normal,
    const Xform& parent_xform,
    const Point& parent_centre_2d
) {

    const Vector e_3d = (v_3d - u_3d).normalized();
    const Point u_2d_raw = parent_xform.transform_point(u_3d);
    const Point v_2d_raw = parent_xform.transform_point(v_3d);
    const Point u_2d(u_2d_raw[0], u_2d_raw[1], 0.0);
    const Point v_2d(v_2d_raw[0], v_2d_raw[1], 0.0);
    const Vector e_2d = (v_2d - u_2d).normalized();

    const Vector perp_2d(-e_2d[1], e_2d[0], 0.0);
    const double parent_side = (parent_centre_2d - u_2d).dot(perp_2d);
    const Vector into_child_2d = parent_side >= 0.0 ? perp_2d * -1.0 : perp_2d;

    const Vector perp_3d = child_normal.cross(e_3d).normalized();
    const double child_side = (child_centre_3d - u_3d).dot(perp_3d);
    const Vector into_child_3d = child_side >= 0.0 ? perp_3d : perp_3d * -1.0;

    const Vector z_src = e_3d.cross(into_child_3d).normalized();
    const Vector z_dst = e_2d.cross(into_child_2d).normalized();
    const Xform to_local = Xform::world_to_frame(u_3d, e_3d, into_child_3d, z_src);
    const Xform to_flat = Xform::frame_to_world(u_2d, e_2d, into_child_2d, z_dst);
    return to_flat * to_local;
}

Polyline FoldUnfolding::flat_polygon(
    const std::vector<Point>& points, const std::vector<size_t>& loop, const Xform& xform
) {

    std::vector<Point> ring;
    ring.reserve(loop.size() + 1);

    for (const size_t v : loop) {
        const Point mapped = xform.transform_point(points[v]);
        ring.push_back(Point(mapped[0], mapped[1], 0.0));
    }

    double signed_area = 0.0;

    for (size_t i = 0; i < ring.size(); i++) {
        const Point& a = ring[i];
        const Point& b = ring[(i + 1) % ring.size()];
        signed_area += a[0] * b[1] - b[0] * a[1];
    }

    if (signed_area < 0.0)
        std::reverse(ring.begin(), ring.end());

    ring.push_back(ring.front());
    return Polyline(ring);
}

bool FoldUnfolding::overlaps_sheet(
    const Polyline& candidate, const std::vector<Polyline>& sheet_polys
) {

    const AABB box_a = AABB::from_polyline(candidate);
    const Point min_a = box_a.min_point();
    const Point max_a = box_a.max_point();

    for (const Polyline& existing : sheet_polys) {
        const AABB box_b = AABB::from_polyline(existing);
        const Point min_b = box_b.min_point();
        const Point max_b = box_b.max_point();
        const bool disjoint = max_a[0] <= min_b[0] + 1e-3 || max_b[0] <= min_a[0] + 1e-3 ||
                              max_a[1] <= min_b[1] + 1e-3 || max_b[1] <= min_a[1] + 1e-3;

        if (disjoint)
            continue;

        const std::vector<Polyline> pieces = BooleanPolyline::compute(candidate, existing, 0);
        double area = 0.0;

        for (const Polyline& piece : pieces)
            area += piece.closed().area();

        if (area > 1e-2)
            return true;
    }

    return false;
}

std::vector<Polyline> FoldUnfolding::union_sheet(const std::vector<Polyline>& sheet_polys) {

    std::map<std::pair<long long, long long>, Point> snapped;
    std::vector<Polyline> clean;
    clean.reserve(sheet_polys.size());

    for (const Polyline& poly : sheet_polys) {
        std::vector<Point> pts = poly.open_points();

        if (pts.size() < 3)
            continue;

        for (Point& pt : pts) {
            const std::pair<long long, long long> key = {
                std::llround(pt[0] * 1000.0),
                std::llround(pt[1] * 1000.0),
            };
            const std::map<std::pair<long long, long long>, Point>::const_iterator found =
                snapped.find(key);

            if (found != snapped.end())
                pt = found->second;
            else
                snapped[key] = pt;
        }

        pts.push_back(pts.front());
        clean.push_back(Polyline(pts));
    }

    const std::vector<Polyline> regions = BooleanPolyline::compute_regions(clean, {}, 1);

    if (regions.empty())
        return clean;

    return regions;
}

double FoldUnfolding::face_distortion(
    const std::vector<Point>& points, const std::vector<size_t>& loop, const Xform& xform
) {

    double worst = 0.0;
    const size_t n = loop.size();
    std::vector<Point> flat(n);

    for (size_t i = 0; i < n; i++) {
        const Point mapped = xform.transform_point(points[loop[i]]);
        flat[i] = Point(mapped[0], mapped[1], 0.0);
    }

    for (size_t i = 0; i < n; i++) {
        const size_t next = (i + 1) % n;
        const double edge_3d = points[loop[i]].distance(points[loop[next]]);

        if (edge_3d > 1e-9)
            worst = std::max(worst, std::abs(flat[i].distance(flat[next]) - edge_3d) / edge_3d);

        if (i >= 2 && i + 1 < n) {
            const double diag_3d = points[loop[0]].distance(points[loop[i]]);

            if (diag_3d > 1e-9)
                worst = std::max(worst, std::abs(flat[0].distance(flat[i]) - diag_3d) / diag_3d);
        }
    }

    return worst;
}
