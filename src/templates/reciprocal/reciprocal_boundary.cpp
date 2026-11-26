#include "src/templates/reciprocal/reciprocal_boundary.h"

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// Mesh topology
// ═══════════════════════════════════════════════════════════════════════════
std::pair<size_t, size_t> edge_key(size_t u, size_t v) {
    return {std::min(u, v), std::max(u, v)};
}

std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>
edge_owners(const std::vector<std::vector<size_t>>& faces) {

    std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>> owners;

    for (int fi = 0; fi < (int)faces.size(); fi++) {
        const int n = (int)faces[fi].size();

        for (int j = 0; j < n; j++) {
            const std::pair<size_t, size_t> key = edge_key(faces[fi][j], faces[fi][(j + 1) % n]);
            owners[key].emplace_back(fi, j);
        }
    }

    return owners;
}

size_t half_edge_start(
    const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge
) {
    return faces[half_edge.first][half_edge.second];
}

size_t
half_edge_end(const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge) {
    const std::vector<size_t>& face = faces[half_edge.first];
    return face[(half_edge.second + 1) % face.size()];
}

std::vector<std::pair<int, int>> naked_half_edges(
    const std::vector<std::vector<size_t>>& faces,
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owners
) {

    std::vector<std::pair<int, int>> naked;
    std::map<size_t, size_t>
        leaving_vertex; // boundary vertex -> the naked half-edge starting there.

    for (const std::pair<const std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owner :
         owners) {
        if (owner.second.size() != 1)
            continue;

        leaving_vertex.emplace(half_edge_start(faces, owner.second[0]), naked.size());
        naked.push_back(owner.second[0]);
    }

    std::vector<std::pair<int, int>> ordered;
    std::vector<bool> visited(naked.size(), false);

    for (size_t seed = 0; seed < naked.size(); seed++) {
        size_t current = seed;

        while (!visited[current]) {
            visited[current] = true;
            ordered.push_back(naked[current]);
            const std::map<size_t, size_t>::const_iterator next =
                leaving_vertex.find(half_edge_end(faces, naked[current]));

            if (next == leaving_vertex.end())
                break;

            current = next->second;
        }
    }

    return ordered;
}

std::vector<std::pair<size_t, size_t>> loop_ranges(
    const std::vector<std::pair<int, int>>& naked, const std::vector<std::vector<size_t>>& faces
) {

    std::vector<std::pair<size_t, size_t>> ranges;
    size_t loop_start = 0;

    while (loop_start < naked.size()) {
        size_t loop_end = loop_start + 1;

        while (loop_end < naked.size() &&
               half_edge_start(faces, naked[loop_end]) == half_edge_end(faces, naked[loop_end - 1]))
            loop_end++;

        ranges.emplace_back(loop_start, loop_end);
        loop_start = loop_end;
    }

    return ranges;
}

bool loop_closed(
    const std::vector<std::pair<int, int>>& naked,
    const std::vector<std::vector<size_t>>& faces,
    const std::pair<size_t, size_t>& range
) {
    return range.second - range.first > 1 && half_edge_end(faces, naked[range.second - 1]) ==
                                                 half_edge_start(faces, naked[range.first]);
}

MeshFaces MeshFaces::of(const Mesh& mesh) {

    MeshFaces data;
    const std::vector<size_t> keys = mesh.faces();

    for (size_t key : keys) {
        const std::vector<size_t> face = mesh.face_vertices(key).value();
        data.faces.push_back(face);
        data.normals.push_back(mesh.face_normal(key).value_or(Vector(0, 0, 1)));

        for (size_t vertex : face)
            data.points.emplace(vertex, mesh.vertex_point(vertex).value());
    }

    data.owners = edge_owners(data.faces);
    return data;
}

// ═══════════════════════════════════════════════════════════════════════════
// Vectors
// ═══════════════════════════════════════════════════════════════════════════
Vector perpendicular_part(const Vector& v, const Vector& axis) {

    const Vector part = v - axis * v.dot(axis);

    if (part.is_zero()) {
        Vector any;
        any.perpendicular_to(axis);
        return any.normalized();
    }

    return part.normalized();
}

Plane facing(const Plane& plane, const Point& inside) {

    if ((inside - plane.origin()).dot(plane.z_axis()) >= 0.0)
        return plane;

    return Plane::from_point_normal(plane.origin(), -plane.z_axis());
}

Plane beam_face_towards(
    const Point& point,
    const Vector& dir,
    const Vector& up,
    double width,
    double height,
    const Vector& towards
) {

    const Vector side = up.cross(dir).normalized();
    const Vector rise = up.normalized();
    const std::array<Vector, 4> normals = {side, -side, rise, -rise};
    const std::array<double, 4> offsets = {width * 0.5, width * 0.5, height * 0.5, height * 0.5};
    size_t best = 0;

    for (size_t k = 1; k < 4; k++)
        if (normals[k].dot(towards) > normals[best].dot(towards))
            best = k;

    return Plane::from_point_normal(point + normals[best] * offsets[best], normals[best]);
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════
std::vector<CutFace> unbounded(const Plane& plane) {
    return {CutFace{plane, {}}};
}

std::vector<Vector> boundary_up_directions(
    const std::vector<std::pair<int, int>>& naked,
    const std::vector<std::vector<size_t>>& faces,
    const std::map<size_t, Point>& points,
    const std::vector<Vector>& owner_normals
) {

    std::vector<Vector> ups(naked.size(), Vector(0, 0, 1));
    const std::vector<std::pair<size_t, size_t>> ranges = loop_ranges(naked, faces);

    for (const std::pair<size_t, size_t>& range : ranges) {
        const size_t first = range.first;
        const size_t count = range.second - range.first;

        // directions: the unit edge of every half-edge of the loop
        std::vector<Vector> directions(count);

        for (size_t k = 0; k < count; k++) {
            const Point& start = points.at(half_edge_start(faces, naked[first + k]));
            const Point& end = points.at(half_edge_end(faces, naked[first + k]));
            const Vector edge = end - start;
            const Vector previous = k > 0 ? directions[k - 1] : Vector(1, 0, 0);
            directions[k] = edge.is_zero() ? previous : edge.normalized();
        }

        // average: the owning face normals, turned to one side
        Vector average(0, 0, 0);

        for (size_t k = 0; k < count; k++) {
            const Vector& normal = owner_normals[first + k];
            average += normal.dot(average) < 0.0 ? -normal : normal;
        }

        if (average.is_zero())
            average = Vector(0, 0, 1);

        // transported: the first up reflected across every miter along the loop
        std::vector<Vector> transported(count);
        transported[0] = perpendicular_part(owner_normals[first], directions[0]);

        for (size_t k = 0; k + 1 < count; k++) {
            Vector mirror = directions[k] + directions[k + 1];

            if (mirror.is_zero())
                mirror = directions[k];

            const Vector reflected = transported[k].reflect(mirror.normalized());
            transported[k + 1] = perpendicular_part(reflected, directions[k + 1]);
        }

        // closure: the twist a closed loop's transport returns with
        const bool closed = loop_closed(naked, faces, range);
        double closure = 0.0;

        if (closed) {
            Vector mirror = directions[count - 1] + directions[0];

            if (mirror.is_zero())
                mirror = directions[count - 1];

            const Vector reflected = transported[count - 1].reflect(mirror.normalized());
            const Vector returned = perpendicular_part(reflected, directions[0]);
            closure = std::atan2(
                transported[0].cross(returned).dot(directions[0]), transported[0].dot(returned)
            );
        }

        // bends: the closure spread over the loop by how much it has turned
        std::vector<double> bends(count, 0.0);
        double total_bend = 0.0;

        for (size_t k = 0; k < count; k++) {
            if (k + 1 == count && !closed)
                break;

            const double turn = directions[k].dot(directions[(k + 1) % count]);
            bends[k] = std::acos(std::clamp(turn, -1.0, 1.0));
            total_bend += bends[k];
        }

        double bend_so_far = 0.0;

        for (size_t k = 0; k < count; k++) {
            const double share =
                total_bend > 0.0 ? bend_so_far / total_bend : (double)k / (double)count;
            transported[k] =
                transported[k].transformed(Xform::rotation(directions[k], -closure * share));
            bend_so_far += bends[k];
        }

        // turn: the family about its axes, closest to the average normal
        double along = 0.0;
        double across = 0.0;

        for (size_t k = 0; k < count; k++) {
            along += transported[k].dot(average);
            across += directions[k].cross(transported[k]).dot(average);
        }

        const double turn = std::atan2(across, along);

        for (size_t k = 0; k < count; k++)
            ups[first + k] = transported[k].transformed(Xform::rotation(directions[k], turn));
    }

    return ups;
}

Plane miter_plane(const Point& vertex, const Vector& arriving, const Vector& leaving) {

    const Vector normal = arriving + leaving;

    if (normal.is_zero())
        return Plane::from_point_normal(vertex, arriving);

    return Plane::from_point_normal(vertex, normal);
}

BoundaryFrame boundary_frame(
    const std::vector<std::vector<size_t>>& faces,
    const std::map<std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owners,
    const std::map<size_t, Point>& points,
    const std::vector<Vector>& face_normals,
    double width,
    double height,
    const std::map<std::pair<size_t, size_t>, Vector>& boundary_ups,
    bool butt,
    const std::map<std::pair<size_t, size_t>, int>& through_priority
) {

    BoundaryFrame frame;
    frame.naked = naked_half_edges(faces, owners);
    const size_t count = frame.naked.size();
    std::vector<Vector> owner_normals;

    for (const std::pair<int, int>& half_edge : frame.naked)
        owner_normals.push_back(face_normals[half_edge.first]);

    // ups: transported along the loops, replaced where boundary_ups gives one
    frame.ups = boundary_up_directions(frame.naked, faces, points, owner_normals);

    // directions: start vertex to end vertex, and the edges arriving at and leaving every vertex
    std::map<size_t, Vector> arriving;
    std::map<size_t, Vector> leaving;
    frame.directions.resize(count, Vector(0, 0, 0));

    for (size_t k = 0; k < count; k++) {
        const size_t u = half_edge_start(faces, frame.naked[k]);
        const size_t v = half_edge_end(faces, frame.naked[k]);
        const Vector edge = points.at(v) - points.at(u);

        if (edge.is_zero())
            continue;

        frame.directions[k] = edge.normalized();
        arriving[v] = frame.directions[k];
        leaving[u] = frame.directions[k];

        const std::map<std::pair<size_t, size_t>, Vector>::const_iterator given =
            boundary_ups.find(edge_key(u, v));

        if (given != boundary_ups.end() && !given->second.is_zero())
            frame.ups[k] = perpendicular_part(given->second, frame.directions[k]);
    }

    // miters: both ends of every frame beam
    frame.cut_from.resize(count);
    frame.cut_to.resize(count);

    for (size_t k = 0; k < count; k++) {
        const size_t u = half_edge_start(faces, frame.naked[k]);
        const size_t v = half_edge_end(faces, frame.naked[k]);
        const Vector& dir = frame.directions[k];

        if (dir.is_zero())
            continue;

        frame.cut_from[k] = arriving.count(u) ? miter_plane(points.at(u), arriving[u], dir)
                                              : Plane::from_point_normal(points.at(u), dir);
        frame.cut_to[k] = leaving.count(v) ? miter_plane(points.at(v), dir, leaving[v])
                                           : Plane::from_point_normal(points.at(v), dir);
    }

    // butts: at a corner whose sections do not mirror, the beam with the higher priority runs through
    constexpr double MIRROR_TOLERANCE =
        0.9994; // cos of 2 degrees between the mirrored and the actual next up.
    const std::vector<std::pair<size_t, size_t>> ranges = loop_ranges(frame.naked, faces);

    for (const std::pair<size_t, size_t>& range : ranges) {
        if (!butt)
            break;

        const bool closed = loop_closed(frame.naked, faces, range);

        for (size_t k = range.first; k < range.second; k++) {
            const size_t next = k + 1 < range.second ? k + 1 : range.first;

            if ((k + 1 == range.second && !closed) || frame.directions[k].is_zero() ||
                frame.directions[next].is_zero())
                continue;

            const std::pair<size_t, size_t> key_k = edge_key(
                half_edge_start(faces, frame.naked[k]), half_edge_end(faces, frame.naked[k])
            );
            const std::pair<size_t, size_t> key_next = edge_key(
                half_edge_start(faces, frame.naked[next]), half_edge_end(faces, frame.naked[next])
            );
            const std::map<std::pair<size_t, size_t>, int>::const_iterator found_k =
                through_priority.find(key_k);
            const std::map<std::pair<size_t, size_t>, int>::const_iterator found_next =
                through_priority.find(key_next);
            const int priority_k = found_k == through_priority.end() ? 0 : found_k->second;
            const int priority_next = found_next == through_priority.end() ? 0 : found_next->second;
            const bool side_change = found_k != through_priority.end() &&
                                     found_next != through_priority.end() &&
                                     priority_k != priority_next;
            const Vector mirror = frame.directions[k] + frame.directions[next];
            const Vector mirrored =
                mirror.is_zero() ? frame.ups[k] : frame.ups[k].reflect(mirror.normalized());

            if (!side_change && mirrored.dot(frame.ups[next]) >= MIRROR_TOLERANCE)
                continue;

            const Point& corner = points.at(half_edge_end(faces, frame.naked[k]));
            const double sign = priority_next > priority_k ? -1.0 : 1.0;
            frame.cut_to[k] = beam_face_towards(
                corner,
                frame.directions[next],
                frame.ups[next],
                width,
                height,
                frame.directions[k] * sign
            );
            frame.cut_from[next] = beam_face_towards(
                corner,
                frame.directions[k],
                frame.ups[k],
                width,
                height,
                frame.directions[next] * sign
            );
        }
    }

    // inner faces: the face of every frame beam that looks into the shell, bounded by its two end planes
    for (size_t k = 0; k < count; k++) {
        const size_t u = half_edge_start(faces, frame.naked[k]);
        const size_t v = half_edge_end(faces, frame.naked[k]);
        const Vector& dir = frame.directions[k];

        if (dir.is_zero())
            continue;

        const Point mid = Point::mid_point(points.at(u), points.at(v));
        Vector towards_shell = owner_normals[k].cross(dir);

        if (towards_shell.is_zero())
            towards_shell = frame.ups[k].cross(dir);

        const Plane inner_plane =
            beam_face_towards(mid, dir, frame.ups[k], width, height, towards_shell);
        const CutFace inner{
            inner_plane, {facing(frame.cut_from[k], mid), facing(frame.cut_to[k], mid)}
        };
        frame.inner_faces_at_vertex[u].push_back(inner);
        frame.inner_faces_at_vertex[v].push_back(inner);
    }

    return frame;
}

std::vector<CutFace>
frame_cuts(const BoundaryFrame& frame, size_t vertex, const Vector& dir) {

    constexpr double FLAT_CAP_ALIGNMENT = 0.15; // cos below which a plane runs away along the beam.
    std::vector<CutFace> cuts;
    const std::map<size_t, std::vector<CutFace>>::const_iterator found =
        frame.inner_faces_at_vertex.find(vertex);

    if (found == frame.inner_faces_at_vertex.end())
        return cuts;

    for (const CutFace& face : found->second)
        if (std::abs(face.plane.z_axis().dot(dir)) >= FLAT_CAP_ALIGNMENT)
            cuts.push_back(face);

    return cuts;
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame ups
// ═══════════════════════════════════════════════════════════════════════════
std::map<std::pair<size_t, size_t>, Vector> owner_normal_ups(const Mesh& mesh) {

    const MeshFaces data = MeshFaces::of(mesh);
    std::map<std::pair<size_t, size_t>, Vector> ups;

    for (const std::pair<const std::pair<size_t, size_t>, std::vector<std::pair<int, int>>>& owner :
         data.owners) {
        if (owner.second.size() != 1)
            continue;

        const Vector& normal = data.normals[owner.second[0].first];
        ups[owner.first] = normal[2] < 0.0 ? -normal : normal;
    }

    return ups;
}

MeshBoundary MeshBoundary::of(const Mesh& mesh, double corner_angle) {

    const MeshFaces data = MeshFaces::of(mesh);
    const std::vector<std::pair<int, int>> naked = naked_half_edges(data.faces, data.owners);
    MeshBoundary boundary;

    for (const std::pair<int, int>& half_edge : naked) {
        const size_t u = half_edge_start(data.faces, half_edge);
        const size_t v = half_edge_end(data.faces, half_edge);
        const Vector edge = data.points.at(v) - data.points.at(u);
        const Vector& normal = data.normals[half_edge.first];
        boundary.keys.push_back(edge_key(u, v));
        boundary.starts.push_back(data.points.at(u));
        boundary.ends.push_back(data.points.at(v));
        boundary.directions.push_back(edge.is_zero() ? Vector(1, 0, 0) : edge.normalized());
        boundary.normals.push_back(normal[2] < 0.0 ? -normal : normal);
    }

    const double corner_cos = std::cos(corner_angle * Tolerance::TO_RADIANS);
    const std::vector<std::pair<size_t, size_t>> ranges = loop_ranges(naked, data.faces);
    boundary.sides.assign(naked.size(), -1);

    for (const std::pair<size_t, size_t>& range : ranges) {
        const int first_side = (int)boundary.side_rank.size();
        int side = first_side;
        boundary.side_rank.push_back(0);

        for (size_t k = range.first; k < range.second; k++) {
            boundary.sides[k] = side;

            if (k + 1 < range.second &&
                boundary.directions[k].dot(boundary.directions[k + 1]) < corner_cos) {
                side = (int)boundary.side_rank.size();
                boundary.side_rank.push_back(boundary.side_rank.back() + 1);
            }
        }

        const bool closed = loop_closed(naked, data.faces, range);
        const double closing =
            boundary.directions[range.second - 1].dot(boundary.directions[range.first]);

        if (closed && side != first_side && closing >= corner_cos) {
            for (size_t k = range.first; k < range.second; k++)
                if (boundary.sides[k] == side)
                    boundary.sides[k] = first_side;

            boundary.side_rank.pop_back();
        }
    }

    return boundary;
}

std::map<std::pair<size_t, size_t>, Vector> side_tilt_boundary_ups(
    const Mesh& mesh, double corner_angle, const std::vector<Vector>& side_vectors
) {

    const MeshBoundary boundary = MeshBoundary::of(mesh, corner_angle);
    std::map<std::pair<size_t, size_t>, Vector> ups;

    for (int side = 0; side < (int)boundary.side_rank.size(); side++) {
        std::vector<Point> points;
        std::vector<size_t> members;
        Vector average(0, 0, 0);

        for (size_t k = 0; k < boundary.keys.size(); k++) {
            if (boundary.sides[k] != side)
                continue;

            members.push_back(k);
            points.push_back(boundary.starts[k]);
            points.push_back(boundary.ends[k]);
            average += boundary.normals[k];
        }

        if (members.empty())
            continue;

        if (average.is_zero())
            average = Vector(0, 0, 1);

        // plane: fitted through the side, the plane through its line holding the average normal when straight
        Point a = points.front();
        Point b = points.front();

        for (const Point& p : points)
            for (const Point& q : points)
                if (p.distance(q) > a.distance(b)) {
                    a = p;
                    b = q;
                }

        const Line chord = Line::from_points(a, b);
        double sagitta = 0.0;

        for (const Point& p : points)
            sagitta = std::max(sagitta, p.distance(chord.closest_point(p, true).second));

        Vector n = points.size() >= 3 ? Plane::from_points_pca(points).z_axis() : Vector(0, 0, 0);

        if (sagitta < 1e-6 * a.distance(b) || n.is_zero())
            n = perpendicular_part(average, chord.to_direction());

        if (n.dot(average) < 0.0)
            n = -n;

        // tilt: the mean lean of the face normals, or the given vector, out of the plane
        double sin_sum = 0.0;
        double cos_sum = 0.0;

        for (size_t k : members) {
            const Vector across = n.cross(boundary.directions[k]).normalized();
            const bool given = side < (int)side_vectors.size() && !side_vectors[side].is_zero();
            Vector lean = given ? side_vectors[side] : boundary.normals[k];

            if (lean.dot(boundary.normals[k]) < 0.0)
                lean = -lean;

            sin_sum += lean.dot(across);
            cos_sum += lean.dot(n);
        }

        const double tilt = std::atan2(sin_sum, cos_sum);

        for (size_t k : members) {
            const Vector across = n.cross(boundary.directions[k]).normalized();
            ups[boundary.keys[k]] = (n * std::cos(tilt) + across * std::sin(tilt)).normalized();
        }
    }

    return ups;
}

std::map<std::pair<size_t, size_t>, int>
through_side_priority(const Mesh& mesh, double corner_angle) {

    const MeshBoundary boundary = MeshBoundary::of(mesh, corner_angle);
    std::map<std::pair<size_t, size_t>, int> priority;

    for (size_t k = 0; k < boundary.keys.size(); k++)
        priority[boundary.keys[k]] = boundary.side_rank[boundary.sides[k]] % 2 == 0 ? 1 : 0;

    return priority;
}

// ═══════════════════════════════════════════════════════════════════════════
// Box beam
// ═══════════════════════════════════════════════════════════════════════════
bool within_bounds(const CutFace& face, const Point& point) {

    for (const Plane& bound : face.bounds)
        if ((point - bound.origin()).dot(bound.z_axis()) < -1e-6)
            return false;

    return true;
}

std::pair<Point, int>
ray_exit(const Point& origin, const Vector& out, const std::vector<CutFace>& cuts) {

    double best_t = 0.0;
    double any_t = 0.0;
    int best = -1;
    int any = -1;

    for (int k = 0; k < (int)cuts.size(); k++) {
        const double denominator = cuts[k].plane.z_axis().dot(out);

        if (denominator > -1e-10)
            continue;

        const double t =
            (cuts[k].plane.origin() - origin).dot(cuts[k].plane.z_axis()) / denominator;

        if (any < 0 || t < any_t - 1e-9 * (1.0 + std::abs(any_t))) {
            any_t = t;
            any = k;
        }

        if (!within_bounds(cuts[k], origin + out * t))
            continue;

        if (best < 0 || t < best_t - 1e-9 * (1.0 + std::abs(best_t))) {
            best_t = t;
            best = k;
        }
    }

    if (best >= 0)
        return {origin + out * best_t, best};

    if (any >= 0)
        return {origin + out * any_t, any};

    return {origin, -1};
}

EndCorners end_corners(
    const Point& left,
    const Point& right,
    const Vector& out,
    const std::vector<CutFace>& cuts,
    const Plane& face_plane,
    bool crease_allowed
) {

    const std::pair<Point, int> hit_left = ray_exit(left, out, cuts);
    const std::pair<Point, int> hit_right = ray_exit(right, out, cuts);
    EndCorners end{{hit_left.first, hit_right.first}, hit_left.second, hit_right.second};

    if (!crease_allowed || end.left_plane < 0 || end.right_plane < 0 ||
        end.left_plane == end.right_plane)
        return end;

    const Plane& a = cuts[end.left_plane].plane;
    const Plane& b = cuts[end.right_plane].plane;
    Point crease;
    const bool parallel = std::abs(a.z_axis().dot(b.z_axis())) > 1.0 - 1e-12;

    if (parallel || !Intersection::plane_plane_plane(a, b, face_plane, crease)) {
        end.right_plane = end.left_plane;
        return end;
    }

    const Vector across = hit_right.first - hit_left.first;
    const double along =
        across.is_zero() ? -1.0 : (crease - hit_left.first).dot(across) / across.dot(across);
    const double fold = (crease - hit_left.first - across * along).magnitude();

    if (along < 0.0 || along > 1.0 || fold > across.magnitude()) {
        end.right_plane = end.left_plane;
        return end;
    }

    end.corners = {hit_left.first, crease, hit_right.first};
    return end;
}

Mesh outline_solid(const std::vector<Point>& bottom, const std::vector<Point>& top) {

    const size_t n = bottom.size();
    std::vector<Point> points = bottom;
    points.insert(points.end(), top.begin(), top.end());
    std::vector<std::vector<size_t>> faces;
    std::vector<size_t> bottom_face(n);
    std::vector<size_t> top_face(n);

    for (size_t k = 0; k < n; k++) {
        bottom_face[k] = n - 1 - k;
        top_face[k] = n + k;
        faces.push_back({k, (k + 1) % n, n + (k + 1) % n, n + k});
    }

    faces.push_back(bottom_face);
    faces.push_back(top_face);
    return Mesh::from_vertices_and_faces(points, faces);
}

Polyline closed_outline(const std::vector<Point>& corners) {

    std::vector<Point> closed = corners;
    closed.push_back(corners[0]);
    return Polyline(closed);
}

ReciprocalBeam cut_beam(
    const Point& from_ref,
    const Point& to_ref,
    const Vector& dir,
    const Vector& up,
    double width,
    double height,
    const std::vector<CutFace>& cuts_from,
    const std::vector<CutFace>& cuts_to
) {

    Vector right = dir.cross(up);

    if (right.is_zero())
        right = dir.cross(Vector(0, 0, 1));

    right = right.normalized() * (width * 0.5);
    const Vector rise = up.normalized() * (height * 0.5);
    const Point inside = Point::mid_point(from_ref, to_ref);

    std::vector<CutFace> from_planes;
    std::vector<CutFace> to_planes;

    for (const CutFace& face : cuts_from)
        from_planes.push_back(CutFace{facing(face.plane, inside), face.bounds});

    for (const CutFace& face : cuts_to)
        to_planes.push_back(CutFace{facing(face.plane, inside), face.bounds});

    const Vector face_normal = perpendicular_part(up, dir);
    const Plane bottom_plane = Plane::from_point_normal(inside - rise, face_normal);
    const Plane top_plane = Plane::from_point_normal(inside + rise, face_normal);

    std::array<EndCorners, 4> ends; // start bottom, start top, end bottom, end top.

    for (int pass = 0; pass < 2; pass++) {
        const bool crease = pass == 0;
        const bool start_again = pass == 1 && (ends[0].left_plane != ends[1].left_plane ||
                                               ends[0].right_plane != ends[1].right_plane);
        const bool end_again = pass == 1 && (ends[2].left_plane != ends[3].left_plane ||
                                             ends[2].right_plane != ends[3].right_plane);

        if (crease || start_again) {
            ends[0] = end_corners(
                from_ref - right - rise,
                from_ref + right - rise,
                -dir,
                from_planes,
                bottom_plane,
                crease
            );
            ends[1] = end_corners(
                from_ref - right + rise,
                from_ref + right + rise,
                -dir,
                from_planes,
                top_plane,
                crease
            );
        }

        if (crease || end_again) {
            ends[2] = end_corners(
                to_ref - right - rise, to_ref + right - rise, dir, to_planes, bottom_plane, crease
            );
            ends[3] = end_corners(
                to_ref - right + rise, to_ref + right + rise, dir, to_planes, top_plane, crease
            );
        }
    }

    std::vector<Point> bottom = ends[0].corners;
    bottom.insert(bottom.end(), ends[2].corners.rbegin(), ends[2].corners.rend());
    std::vector<Point> top = ends[1].corners;
    top.insert(top.end(), ends[3].corners.rbegin(), ends[3].corners.rend());
    const std::vector<Point> right_face = {
        ends[0].corners.back(),
        ends[1].corners.back(),
        ends[3].corners.back(),
        ends[2].corners.back()
    };
    const std::vector<Point> left_face = {
        ends[0].corners.front(),
        ends[1].corners.front(),
        ends[3].corners.front(),
        ends[2].corners.front()
    };

    ReciprocalBeam beam;
    beam.direction = dir;
    beam.up = up;
    beam.mesh = outline_solid(bottom, top);
    beam.right = closed_outline(right_face);
    beam.left = closed_outline(left_face);
    beam.bottom = closed_outline(bottom);
    beam.top = closed_outline(top);
    return beam;
}

std::vector<ReciprocalBeam> frame_beams(
    const BoundaryFrame& frame,
    const std::vector<std::vector<size_t>>& faces,
    const std::map<size_t, Point>& points,
    double width,
    double height
) {

    std::vector<ReciprocalBeam> beams;

    for (size_t k = 0; k < frame.naked.size(); k++) {
        if (frame.directions[k].is_zero())
            continue;

        const Point& start = points.at(half_edge_start(faces, frame.naked[k]));
        const Point& end = points.at(half_edge_end(faces, frame.naked[k]));
        const ReciprocalBeam beam = cut_beam(
            start,
            end,
            frame.directions[k],
            frame.ups[k],
            width,
            height,
            unbounded(frame.cut_from[k]),
            unbounded(frame.cut_to[k])
        );
        beams.push_back(beam);
    }

    return beams;
}

}
