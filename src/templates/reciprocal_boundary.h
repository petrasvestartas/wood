#pragma once
#include "session.h"
#include "intersection.h"
#include "mesh.h"
#include "plane.h"
#include "polyline.h"
#include "tolerance.h"
#include "line.h"
#include "xform.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

using namespace session_cpp;

/// The boundary frame shared by the reciprocal templates: the naked edges walked into loops, one straight beam per naked edge with a section that mirrors across every mitre, and the box beam cut by any number of planes at each end so an interior beam that reaches a boundary kink stops flush against both frame beams there.
namespace wood_reciprocal {

using EdgeKey = std::pair<size_t, size_t>;                                    // sorted vertex pair of a mesh edge
using EdgeOwners = std::map<EdgeKey, std::vector<std::pair<int, int>>>;       // edge → [(face index, local edge index), …]

/// The sorted vertex pair of the edge u-v.
inline EdgeKey edge_key(size_t u, size_t v)
{
    return {std::min(u, v), std::max(u, v)};
}

/// Every mesh edge mapped to the faces that own it, as (face index, local edge index) pairs.
inline EdgeOwners edge_owners(const std::vector<std::vector<size_t>>& faces)
{

    EdgeOwners owners;
    for (int fi = 0; fi < (int)faces.size(); fi++) {
        int n = (int)faces[fi].size();
        for (int j = 0; j < n; j++)
            owners[edge_key(faces[fi][j], faces[fi][(j + 1) % n])].emplace_back(fi, j);
    }

    return owners;
}

/// The naked half-edges as (face, local edge) in boundary-loop order, each walked in its owning face's winding; a loop's half-edges stay consecutive.
inline std::vector<std::pair<int, int>> naked_half_edges(const std::vector<std::vector<size_t>>& faces,
                                                         const EdgeOwners& owners)
{

    std::vector<std::pair<int, int>> naked;
    std::map<size_t, size_t> leaving_vertex;  // boundary vertex → the naked half-edge starting there
    for (const auto& [key, edge_owners] : owners) {
        if (edge_owners.size() != 1)
            continue;

        const auto& [fi, fj] = edge_owners[0];
        if (!leaving_vertex.emplace(faces[fi][fj], naked.size()).second)
            std::cerr << fmt::format("  WARNING: reciprocal boundary: two naked edges leave vertex {} - the boundary is not a simple loop there.\n", faces[fi][fj]);

        naked.push_back(edge_owners[0]);
    }

    std::vector<std::pair<int, int>> ordered;
    std::vector<bool> visited(naked.size(), false);
    for (size_t seed = 0; seed < naked.size(); seed++) {
        size_t current = seed;
        while (!visited[current]) {
            visited[current] = true;
            ordered.push_back(naked[current]);

            const auto& [fi, fj] = naked[current];
            std::map<size_t, size_t>::const_iterator next = leaving_vertex.find(faces[fi][(fj + 1) % faces[fi].size()]);
            if (next == leaving_vertex.end())
                break;

            current = next->second;
        }
    }

    return ordered;
}

/// The start vertex of the naked half-edge.
inline size_t half_edge_start(const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge)
{
    return faces[half_edge.first][half_edge.second];
}

/// The end vertex of the naked half-edge.
inline size_t half_edge_end(const std::vector<std::vector<size_t>>& faces, const std::pair<int, int>& half_edge)
{
    return faces[half_edge.first][(half_edge.second + 1) % faces[half_edge.first].size()];
}

/// The [start, end) ranges of the boundary loops in the ordered naked half-edges: a new loop starts wherever a half-edge does not leave the previous one's end vertex.
inline std::vector<std::pair<size_t, size_t>> loop_ranges(const std::vector<std::pair<int, int>>& naked,
                                                          const std::vector<std::vector<size_t>>& faces)
{

    std::vector<std::pair<size_t, size_t>> ranges;
    size_t loop_start = 0;
    while (loop_start < naked.size()) {
        size_t loop_end = loop_start + 1;
        while (loop_end < naked.size() && half_edge_start(faces, naked[loop_end]) == half_edge_end(faces, naked[loop_end - 1]))
            loop_end++;

        ranges.emplace_back(loop_start, loop_end);
        loop_start = loop_end;
    }

    return ranges;
}

/// The signed angle from a to b about the unit axis, both perpendicular to it.
inline double signed_angle_about(const Vector& a, const Vector& b, const Vector& axis)
{
    return std::atan2(a.cross(b).dot(axis), a.dot(b));
}

/// The part of v perpendicular to the unit axis, unit length; a perpendicular of the axis when v is along it.
inline Vector perpendicular_part(const Vector& v, const Vector& axis)
{

    Vector part = v - axis * v.dot(axis);
    if (part.is_zero()) {
        Vector any;
        any.perpendicular_to(axis);
        return any.normalized();
    }

    return part.normalized();
}

/// v rotated about the unit axis by the angle in radians.
inline Vector rotated_about(const Vector& v, const Vector& axis, double angle)
{
    return v.transformed(Xform::rotation(axis, angle));
}

/// Where the closure twist of a non-planar boundary loop goes: at the bends keeps every straight run one exact prism and puts the whole twist at the corners; even spreads it over all joints, so the sections turn a little from beam to beam along straight runs too.
enum class BoundaryTwist { AtBends, Even };

/// The up per naked half-edge: transported along each loop by reflection in the mitre planes, so consecutive sections mirror across their shared mitre and the mitred faces coincide; the closure twist of a non-planar loop spread over the joints as boundary_twist says, and the whole family turned about its own axes to lie closest to the loop's average face normal, which leaves a planar loop's frame exactly as one shared up.
inline std::vector<Vector> boundary_up_directions(const std::vector<std::pair<int, int>>& naked,
                                                  const std::vector<std::vector<size_t>>& faces,
                                                  const std::map<size_t, Point>& vertex_points,
                                                  const std::vector<Vector>& owner_normals,
                                                  BoundaryTwist boundary_twist)
{

    std::vector<Vector> ups(naked.size(), Vector(0, 0, 1));
    for (const auto& [loop_start, loop_end] : loop_ranges(naked, faces)) {
        size_t count = loop_end - loop_start;
        std::vector<Vector> directions(count);
        for (size_t k = 0; k < count; k++) {
            Vector edge = vertex_points.at(half_edge_end(faces, naked[loop_start + k])) - vertex_points.at(half_edge_start(faces, naked[loop_start + k]));
            directions[k] = edge.is_zero() ? (k > 0 ? directions[k - 1] : Vector(1, 0, 0)) : edge.normalized();
        }

        Vector average(0, 0, 0);
        for (size_t k = 0; k < count; k++)
            average += owner_normals[loop_start + k].dot(average) < 0.0 ? -owner_normals[loop_start + k] : owner_normals[loop_start + k];

        if (average.is_zero())
            average = Vector(0, 0, 1);

        std::vector<Vector> transported(count);
        transported[0] = perpendicular_part(owner_normals[loop_start], directions[0]);
        for (size_t k = 0; k + 1 < count; k++) {
            Vector mirror = directions[k] + directions[k + 1];
            if (mirror.is_zero())
                mirror = directions[k];

            transported[k + 1] = perpendicular_part(transported[k].reflect(mirror.normalized()), directions[k + 1]);
        }

        bool closed = count > 1 && half_edge_end(faces, naked[loop_end - 1]) == half_edge_start(faces, naked[loop_start]);
        double closure = 0.0;
        if (closed) {
            Vector mirror = directions[count - 1] + directions[0];
            if (mirror.is_zero())
                mirror = directions[count - 1];

            Vector returned = perpendicular_part(transported[count - 1].reflect(mirror.normalized()), directions[0]);
            closure = signed_angle_about(transported[0], returned, directions[0]);
        }

        // the closure twist goes to the joints in proportion to their bend, so a straight run keeps one section, or evenly
        std::vector<double> bends(count, 0.0);
        double total_bend = 0.0;
        for (size_t k = 0; k < count; k++) {
            if (k + 1 == count && !closed)
                break;

            bends[k] = std::acos(std::clamp(directions[k].dot(directions[(k + 1) % count]), -1.0, 1.0));
            total_bend += bends[k];
        }

        double bend_so_far = 0.0;
        for (size_t k = 0; k < count; k++) {
            bool at_bends = boundary_twist == BoundaryTwist::AtBends && total_bend > 0.0;
            double share = at_bends ? bend_so_far / total_bend : (double)k / (double)count;
            transported[k] = rotated_about(transported[k], directions[k], -closure * share);
            bend_so_far += bends[k];
        }

        double along = 0.0, across = 0.0;
        for (size_t k = 0; k < count; k++) {
            along  += transported[k].dot(average);
            across += directions[k].cross(transported[k]).dot(average);
        }

        double turn = std::atan2(across, along);
        for (size_t k = 0; k < count; k++)
            ups[loop_start + k] = rotated_about(transported[k], directions[k], turn);
    }

    return ups;
}

/// The mitre at a boundary vertex: the bisector plane whose normal is the sum of the arriving and leaving unit edge directions, the arriving one alone when they fold back.
inline Plane mitre_plane(const Point& vertex, const Vector& arriving, const Vector& leaving)
{

    Vector normal = arriving + leaving;
    if (normal.is_zero())
        return Plane::from_point_normal(vertex, arriving);

    return Plane::from_point_normal(vertex, normal);
}

/// The boundary beam's face that looks into the shell: of its four long faces the one whose normal points most towards the owning face, which lies to the left of its own half-edge, half the width away when it is a side face and half the height when the beam lies flat and the shell meets its top or bottom.
inline Plane boundary_inner_plane(const Point& from, const Point& to, const Vector& dir, const Vector& up,
                                  const Vector& owner_normal, double beam_w, double beam_h)
{

    Vector towards_shell = owner_normal.cross(dir);  // left of the half-edge, in the owning face
    if (towards_shell.is_zero())
        towards_shell = up.cross(dir);

    Vector side = up.cross(dir).normalized();
    Vector rise = up.normalized();
    std::array<std::pair<Vector, double>, 4> faces = {std::make_pair(side, beam_w * 0.5), std::make_pair(-side, beam_w * 0.5),
                                                      std::make_pair(rise, beam_h * 0.5), std::make_pair(-rise, beam_h * 0.5)};
    const std::pair<Vector, double>* best = &faces[0];
    for (const std::pair<Vector, double>& face : faces)
        if (face.first.dot(towards_shell) > best->first.dot(towards_shell))
            best = &face;

    return Plane::from_point_normal(Point::mid_point(from, to) + best->first * best->second, best->first);
}

/// A plane a beam end is cut by, valid only where the hit lies on the positive side of every bound: a frame beam's inner face between its two mitres, or an unbounded plane when bounds is empty.
struct CutFace {
    Plane plane;                // the cutting plane
    std::vector<Plane> bounds;  // half-spaces the hit must lie in, positive side; empty for a plane that cuts everywhere
};

/// The plane as a cut that is valid everywhere.
inline std::vector<CutFace> unbounded(const Plane& plane)
{
    return {CutFace{plane, {}}};
}

/// The plane turned so that the point lies on its positive side.
inline Plane facing(const Plane& plane, const Point& inside)
{

    if ((inside - plane.origin()).dot(plane.z_axis()) >= 0.0)
        return plane;

    return Plane::from_point_normal(plane.origin(), -plane.z_axis());
}

/// How two frame beams whose sections do not mirror across their mitre, as at a corner between two sides of different tilt, are joined: mitred anyway, with a step, or butted, one beam running through to the far face of the other, which is cut against it; the beam with the higher through_priority runs through, the arriving one when they tie.
enum class CornerJoint { Mitre, Butt };

/// The face plane of a beam on the axis through vertex along dir with the given up and section, the one of its four long faces whose normal points most along towards.
inline Plane beam_face_towards(const Point& vertex, const Vector& dir, const Vector& up, double beam_w, double beam_h, const Vector& towards)
{

    Vector side = up.cross(dir).normalized();
    Vector rise = up.normalized();
    std::array<std::pair<Vector, double>, 4> faces = {std::make_pair(side, beam_w * 0.5), std::make_pair(-side, beam_w * 0.5),
                                                      std::make_pair(rise, beam_h * 0.5), std::make_pair(-rise, beam_h * 0.5)};
    const std::pair<Vector, double>* best = &faces[0];
    for (const std::pair<Vector, double>& face : faces)
        if (face.first.dot(towards) > best->first.dot(towards))
            best = &face;

    return Plane::from_point_normal(vertex + best->first * best->second, best->first);
}

/// One straight beam per naked edge: its axis, up and the mitre planes at its two ends, plus its inner face, bounded by those mitres, gathered per boundary vertex for the interior beams that end there.
struct BoundaryFrame {
    std::vector<std::pair<int, int>> naked;                        // (face, local edge) per boundary beam, in loop order
    std::vector<Vector> directions;                                // unit axis per boundary beam, start vertex to end vertex
    std::vector<Vector> ups;                                       // up per boundary beam
    std::vector<Plane> cut_from;                                   // mitre plane at the start vertex per boundary beam
    std::vector<Plane> cut_to;                                     // mitre plane at the end vertex per boundary beam
    std::map<size_t, std::vector<CutFace>> inner_faces_at_vertex;  // boundary vertex → the inner faces of the frame beams meeting there, each bounded by its mitres
};

/// One up per naked edge from the owning face's normal, turned to point up: the frame that follows the surface, with a small step at every mitre where the normal changes.
inline std::map<EdgeKey, Vector> owner_normal_ups(const EdgeOwners& owners, const std::vector<Vector>& face_normals)
{

    std::map<EdgeKey, Vector> ups;
    for (const auto& [key, edge_owners] : owners) {
        if (edge_owners.size() != 1)
            continue;

        const Vector& normal = face_normals[edge_owners[0].first];
        ups[key] = normal[2] < 0.0 ? -normal : normal;
    }

    return ups;
}

/// One up per naked edge of the mesh from its owning face's normal, keyed by the sorted vertex keys of the edge.
inline std::map<EdgeKey, Vector> owner_normal_ups(const Mesh& mesh)
{

    std::vector<size_t> fkeys = mesh.faces();
    std::vector<std::vector<size_t>> faces(fkeys.size());
    std::vector<Vector> face_normals(fkeys.size());
    for (size_t fi = 0; fi < fkeys.size(); fi++) {
        faces[fi] = mesh.face_vertices(fkeys[fi]).value();
        face_normals[fi] = mesh.face_normal(fkeys[fi]).value_or(Vector(0, 0, 1));
    }

    return owner_normal_ups(edge_owners(faces), face_normals);
}

/// One up per naked edge of the mesh from the average of the vertex normals at its two ends, turned to point up: the normals already average the faces around each vertex, so the frame turns gradually along a curved boundary with smaller steps at the mitres than the owning face normals give.
inline std::map<EdgeKey, Vector> vertex_normal_boundary_ups(const Mesh& mesh)
{

    std::map<EdgeKey, Vector> ups;
    for (const auto& [u, v] : mesh.edges_on_boundary()) {
        Vector sum = mesh.vertex_normal(u).value_or(Vector(0, 0, 0)) + mesh.vertex_normal(v).value_or(Vector(0, 0, 0));
        if (sum.is_zero())
            continue;

        Vector up = sum.normalized();
        ups[edge_key(u, v)] = up[2] < 0.0 ? -up : up;
    }

    return ups;
}

/// The boundary frame of the mesh given as faces of vertex keys, their points and one normal per face, for beams beam_w wide and beam_h high. A naked edge listed in boundary_ups takes that vector, made perpendicular to the edge, instead of the transported one; the others keep the transport.
inline BoundaryFrame boundary_frame(const std::vector<std::vector<size_t>>& faces,
                                    const EdgeOwners& owners,
                                    const std::map<size_t, Point>& vertex_points,
                                    const std::vector<Vector>& face_normals,
                                    double beam_w,
                                    double beam_h,
                                    BoundaryTwist boundary_twist = BoundaryTwist::AtBends,
                                    const std::map<EdgeKey, Vector>& boundary_ups = {},
                                    CornerJoint corner_joint = CornerJoint::Mitre,
                                    const std::map<EdgeKey, int>& through_priority = {})
{

    BoundaryFrame frame;
    frame.naked = naked_half_edges(faces, owners);
    std::vector<Vector> owner_normals;
    for (const auto& [fi, fj] : frame.naked)
        owner_normals.push_back(face_normals[fi]);

    frame.ups = boundary_up_directions(frame.naked, faces, vertex_points, owner_normals, boundary_twist);
    for (size_t k = 0; k < frame.naked.size(); k++) {
        size_t u = half_edge_start(faces, frame.naked[k]), v = half_edge_end(faces, frame.naked[k]);
        std::map<EdgeKey, Vector>::const_iterator given = boundary_ups.find(edge_key(u, v));
        Vector edge = vertex_points.at(v) - vertex_points.at(u);
        if (given == boundary_ups.end() || given->second.is_zero() || edge.is_zero())
            continue;

        frame.ups[k] = perpendicular_part(given->second, edge.normalized());
    }
    std::map<size_t, Vector> arriving, leaving;  // boundary vertex → unit direction of the naked edge ending / starting there
    frame.directions.resize(frame.naked.size(), Vector(0, 0, 0));
    for (size_t k = 0; k < frame.naked.size(); k++) {
        size_t u = half_edge_start(faces, frame.naked[k]), v = half_edge_end(faces, frame.naked[k]);
        Vector edge = vertex_points.at(v) - vertex_points.at(u);
        if (edge.is_zero())
            continue;

        frame.directions[k] = edge.normalized();
        arriving[v] = frame.directions[k];
        leaving[u]  = frame.directions[k];
    }

    frame.cut_from.resize(frame.naked.size());
    frame.cut_to.resize(frame.naked.size());
    for (size_t k = 0; k < frame.naked.size(); k++) {
        size_t u = half_edge_start(faces, frame.naked[k]), v = half_edge_end(faces, frame.naked[k]);
        const Vector& dir = frame.directions[k];
        if (dir.is_zero())
            continue;

        const Point& pu = vertex_points.at(u);
        const Point& pv = vertex_points.at(v);
        frame.cut_from[k] = arriving.count(u) ? mitre_plane(pu, arriving[u], dir) : Plane::from_point_normal(pu, dir);
        frame.cut_to[k]   = leaving.count(v)  ? mitre_plane(pv, dir, leaving[v])  : Plane::from_point_normal(pv, dir);
    }

    // when asked, a joint between two sides, or one whose sections do not mirror across the mitre, is butted: one beam runs
    // through to the far face of the other, which starts at the through beam's face
    constexpr double MIRROR_TOLERANCE = 0.9994;  // cos of 2 degrees between the mirrored and the actual next up
    if (corner_joint == CornerJoint::Butt)
        for (const auto& [loop_start, loop_end] : loop_ranges(frame.naked, faces)) {
            bool closed = loop_end - loop_start > 1 && half_edge_end(faces, frame.naked[loop_end - 1]) == half_edge_start(faces, frame.naked[loop_start]);
            for (size_t k = loop_start; k < loop_end; k++) {
                size_t next = k + 1 < loop_end ? k + 1 : loop_start;
                if ((k + 1 == loop_end && !closed) || frame.directions[k].is_zero() || frame.directions[next].is_zero())
                    continue;

                std::map<EdgeKey, int>::const_iterator pk = through_priority.find(edge_key(half_edge_start(faces, frame.naked[k]), half_edge_end(faces, frame.naked[k])));
                std::map<EdgeKey, int>::const_iterator pn = through_priority.find(edge_key(half_edge_start(faces, frame.naked[next]), half_edge_end(faces, frame.naked[next])));
                int priority_k = pk == through_priority.end() ? 0 : pk->second;
                int priority_next = pn == through_priority.end() ? 0 : pn->second;
                bool side_change = pk != through_priority.end() && pn != through_priority.end() && priority_k != priority_next;  // consecutive sides alternate priority, so a change marks a corner
                Vector mirror = frame.directions[k] + frame.directions[next];
                Vector mirrored = mirror.is_zero() ? frame.ups[k] : frame.ups[k].reflect(mirror.normalized());
                if (!side_change && mirrored.dot(frame.ups[next]) >= MIRROR_TOLERANCE)
                    continue;

                const Point& corner = vertex_points.at(half_edge_end(faces, frame.naked[k]));
                if (priority_next > priority_k) {
                    // the leaving beam runs through, back to the far face of the arriving one, which ends at the leaving beam's near face
                    frame.cut_from[next] = beam_face_towards(corner, frame.directions[k], frame.ups[k], beam_w, beam_h, -frame.directions[next]);
                    frame.cut_to[k] = beam_face_towards(corner, frame.directions[next], frame.ups[next], beam_w, beam_h, -frame.directions[k]);
                    continue;
                }

                frame.cut_to[k] = beam_face_towards(corner, frame.directions[next], frame.ups[next], beam_w, beam_h, frame.directions[k]);
                frame.cut_from[next] = beam_face_towards(corner, frame.directions[k], frame.ups[k], beam_w, beam_h, frame.directions[next]);
            }
        }

    for (size_t k = 0; k < frame.naked.size(); k++) {
        size_t u = half_edge_start(faces, frame.naked[k]), v = half_edge_end(faces, frame.naked[k]);
        const Vector& dir = frame.directions[k];
        if (dir.is_zero())
            continue;

        const Point& pu = vertex_points.at(u);
        const Point& pv = vertex_points.at(v);
        Point mid = Point::mid_point(pu, pv);
        CutFace inner{boundary_inner_plane(pu, pv, dir, frame.ups[k], owner_normals[k], beam_w, beam_h), {facing(frame.cut_from[k], mid), facing(frame.cut_to[k], mid)}};
        frame.inner_faces_at_vertex[u].push_back(inner);
        frame.inner_faces_at_vertex[v].push_back(inner);
    }

    return frame;
}

/// The naked half-edges of the mesh in loop order with, per half-edge, its sorted edge key, unit direction, owning face normal turned up, and the side it belongs to: a new side starts after every boundary vertex where the loop bends by more than corner_angle degrees, sides numbered in loop order and the closing run of a loop merged into its first side when the loop does not bend there.
struct MeshBoundary {
    std::vector<EdgeKey> keys;          // per naked half-edge
    std::vector<Point> starts, ends;    // per naked half-edge, its two vertices
    std::vector<Vector> directions;     // per naked half-edge, unit start to end
    std::vector<Vector> normals;        // per naked half-edge, the owning face normal turned to point up
    std::vector<int> sides;             // per naked half-edge, its side index
    std::vector<int> side_of_loop;      // per side, the loop it belongs to
    std::vector<int> side_rank;         // per side, its rank within its loop, 0 first

    /// The boundary of the mesh split into sides at bends over corner_angle degrees.
    static MeshBoundary of(const Mesh& mesh, double corner_angle = 45.0)
    {

        std::vector<size_t> fkeys = mesh.faces();
        std::vector<std::vector<size_t>> faces(fkeys.size());
        std::vector<Vector> face_normals(fkeys.size());
        std::map<size_t, Point> points;
        for (size_t fi = 0; fi < fkeys.size(); fi++) {
            faces[fi] = mesh.face_vertices(fkeys[fi]).value();
            face_normals[fi] = mesh.face_normal(fkeys[fi]).value_or(Vector(0, 0, 1));
            for (size_t vk : faces[fi])
                points.emplace(vk, mesh.vertex_point(vk).value());
        }

        MeshBoundary boundary;
        std::vector<std::pair<int, int>> naked = naked_half_edges(faces, edge_owners(faces));
        for (const auto& [fi, fj] : naked) {
            size_t u = half_edge_start(faces, {fi, fj}), v = half_edge_end(faces, {fi, fj});
            boundary.keys.push_back(edge_key(u, v));
            boundary.starts.push_back(points.at(u));
            boundary.ends.push_back(points.at(v));
            Vector edge = points.at(v) - points.at(u);
            boundary.directions.push_back(edge.is_zero() ? Vector(1, 0, 0) : edge.normalized());
            boundary.normals.push_back(face_normals[fi][2] < 0.0 ? -face_normals[fi] : face_normals[fi]);
        }

        const double corner_cos = std::cos(corner_angle * Tolerance::TO_RADIANS);
        boundary.sides.assign(naked.size(), -1);
        std::vector<std::pair<size_t, size_t>> loops = loop_ranges(naked, faces);
        for (size_t loop = 0; loop < loops.size(); loop++) {
            const auto& [loop_start, loop_end] = loops[loop];
            int first_side = (int)boundary.side_of_loop.size();
            int side = first_side;
            boundary.side_of_loop.push_back((int)loop);
            boundary.side_rank.push_back(0);
            for (size_t k = loop_start; k < loop_end; k++) {
                boundary.sides[k] = side;
                if (k + 1 < loop_end && boundary.directions[k].dot(boundary.directions[k + 1]) < corner_cos) {
                    side = (int)boundary.side_of_loop.size();
                    boundary.side_of_loop.push_back((int)loop);
                    boundary.side_rank.push_back(boundary.side_rank.back() + 1);
                }
            }

            bool closed = loop_end - loop_start > 1 && half_edge_end(faces, naked[loop_end - 1]) == half_edge_start(faces, naked[loop_start]);
            if (closed && side != first_side && boundary.directions[loop_end - 1].dot(boundary.directions[loop_start]) >= corner_cos) {
                for (size_t k = loop_start; k < loop_end; k++)
                    if (boundary.sides[k] == side)
                        boundary.sides[k] = first_side;  // the closing run continues the first side

                boundary.side_of_loop.pop_back();
                boundary.side_rank.pop_back();
            }
        }

        return boundary;
    }
};

/// One up per naked edge with one tilt per side of the mesh boundary, the sides split at bends over corner_angle degrees and numbered in loop order: in the plane fitted through each side every beam's up is the plane normal turned towards the shell by the side's mean tilt of the face normals out of that plane, or by the tilt of the side's entry in side_vectors when one is given, so consecutive sections mirror across every mitre on the side; a straight side takes the plane through its line that holds its average face normal.
inline std::map<EdgeKey, Vector> side_tilt_boundary_ups(const Mesh& mesh, double corner_angle = 45.0,
                                                        const std::vector<Vector>& side_vectors = {})
{

    MeshBoundary boundary = MeshBoundary::of(mesh, corner_angle);
    std::map<EdgeKey, Vector> ups;
    for (int side = 0; side < (int)boundary.side_of_loop.size(); side++) {
        std::vector<Point> points;
        Vector average(0, 0, 0);
        std::vector<size_t> members;
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

        Point a = points.front(), b = points.front();
        for (const Point& p : points)
            for (const Point& q : points)
                if (p.distance(q) > a.distance(b)) {
                    a = p;
                    b = q;
                }

        Line chord = Line::from_points(a, b);
        double sagitta = 0.0;
        for (const Point& p : points)
            sagitta = std::max(sagitta, p.distance(chord.closest_point(p, true).second));

        Vector n = points.size() >= 3 ? Plane::from_points_pca(points).z_axis() : Vector(0, 0, 0);
        if (sagitta < 1e-6 * a.distance(b) || n.is_zero())
            n = perpendicular_part(average, chord.to_direction());

        if (n.dot(average) < 0.0)
            n = -n;

        double sin_sum = 0.0, cos_sum = 0.0;
        for (size_t k : members) {
            Vector across = n.cross(boundary.directions[k]).normalized();
            Vector lean = side < (int)side_vectors.size() && !side_vectors[side].is_zero() ? side_vectors[side] : boundary.normals[k];  // a given vector stands in for the face normal on its side
            if (lean.dot(boundary.normals[k]) < 0.0)
                lean = -lean;

            sin_sum += lean.dot(across);
            cos_sum += lean.dot(n);
        }

        double tilt = std::atan2(sin_sum, cos_sum);
        for (size_t k : members) {
            Vector across = n.cross(boundary.directions[k]).normalized();
            ups[boundary.keys[k]] = (n * std::cos(tilt) + across * std::sin(tilt)).normalized();
        }
    }

    return ups;
}

/// The through priority per naked edge for the frame's butt corners from the mesh alone: the sides of each boundary loop, split at bends over corner_angle degrees, alternate 1, 0, 1, 0 in loop order, so around a four-sided boundary opposite sides share a role.
inline std::map<EdgeKey, int> through_side_priority(const Mesh& mesh, double corner_angle = 45.0)
{

    MeshBoundary boundary = MeshBoundary::of(mesh, corner_angle);
    std::map<EdgeKey, int> priority;
    for (size_t k = 0; k < boundary.keys.size(); k++)
        priority[boundary.keys[k]] = boundary.side_rank[boundary.sides[k]] % 2 == 0 ? 1 : 0;

    return priority;
}

/// The frame's inner faces at the vertex that a beam along dir can be cut by, or the fallback alone when there is none or every one is nearly parallel to the beam, where a cut would run away along it.
inline std::vector<CutFace> boundary_cuts_or(const BoundaryFrame& frame, size_t vertex, const Vector& dir, const Plane& fallback)
{

    constexpr double FLAT_CAP_ALIGNMENT_THRESHOLD = 0.15;  // cos of the angle between the plane normal and the beam below which the plane is skipped
    std::vector<CutFace> cuts;
    std::map<size_t, std::vector<CutFace>>::const_iterator it = frame.inner_faces_at_vertex.find(vertex);
    if (it != frame.inner_faces_at_vertex.end())
        for (const CutFace& face : it->second)
            if (std::abs(face.plane.z_axis().dot(dir)) >= FLAT_CAP_ALIGNMENT_THRESHOLD)
                cuts.push_back(face);

    if (cuts.empty())
        return unbounded(fallback);

    return cuts;
}

/// One box beam: its mesh and the four side outlines, bottom and top with corner i of the top above corner i of the bottom so the plate loft pairs them; an end cut by two planes carries one more corner on the crease between them.
struct BeamGeom {
    Mesh mesh;                        // the closed beam solid
    std::vector<Point> side0;         // right face corners (+right of the axis)
    std::vector<Point> side1;         // left face corners (-right of the axis)
    std::vector<Point> beam_bottom;   // bottom face corners (-up): start left, [start crease], start right, end right, [end crease], end left
    std::vector<Point> beam_top;      // top face corners (+up), each above beam_bottom's corner of the same index
};

/// The corners as a closed outline: the first corner repeated at the end.
inline Polyline closed_outline(const std::vector<Point>& corners)
{

    std::vector<Point> closed = corners;
    closed.push_back(corners[0]);
    return Polyline(closed);
}

/// Appends one beam's mesh and its four closed outlines to the given lists.
inline void store_beam(BeamGeom& bg, std::vector<Mesh>& meshes,
                       std::vector<Polyline>& right_faces, std::vector<Polyline>& left_faces,
                       std::vector<Polyline>& bottom_faces, std::vector<Polyline>& top_faces)
{

    meshes.push_back(std::move(bg.mesh));
    right_faces.push_back(closed_outline(bg.side0));
    left_faces.push_back(closed_outline(bg.side1));
    bottom_faces.push_back(closed_outline(bg.beam_bottom));
    top_faces.push_back(closed_outline(bg.beam_top));
}

/// Whether the point lies within every bound of the face, a hair of tolerance for a hit right on a mitre.
inline bool within_bounds(const CutFace& face, const Point& point)
{

    for (const Plane& bound : face.bounds)
        if ((point - bound.origin()).dot(bound.z_axis()) < -1e-6)
            return false;

    return true;
}

/// Where the ray from origin along out leaves the cut faces: the nearest crossing among the faces the ray heads out of and hits within their bounds, with the index of that face; the nearest crossing regardless of bounds when it hits none within them, and origin itself with -1 when it heads out of none.
inline std::pair<Point, int> ray_exit(const Point& origin, const Vector& out, const std::vector<CutFace>& cuts)
{

    double best_t = 0.0, best_any_t = 0.0;
    int best = -1, best_any = -1;
    for (int k = 0; k < (int)cuts.size(); k++) {
        double denom = cuts[k].plane.z_axis().dot(out);
        if (denom > -1e-10)
            continue;  // heading along or into the half-space: this plane never stops the ray

        double t = (cuts[k].plane.origin() - origin).dot(cuts[k].plane.z_axis()) / denom;
        if (best_any < 0 || t < best_any_t - 1e-9 * (1.0 + std::abs(best_any_t))) {  // a tie keeps the first plane, so coincident planes cut every corner alike
            best_any_t = t;
            best_any = k;
        }

        if (!within_bounds(cuts[k], origin + out * t))
            continue;

        if (best < 0 || t < best_t - 1e-9 * (1.0 + std::abs(best_t))) {
            best_t = t;
            best = k;
        }
    }

    if (best < 0 && best_any < 0)
        return {origin, -1};

    if (best < 0)
        return {origin + out * best_any_t, best_any};

    return {origin + out * best_t, best};
}

/// The solid between the two outlines: bottom, top and one quad per outline edge.
inline Mesh outline_solid(const std::vector<Point>& bottom, const std::vector<Point>& top)
{

    size_t n = bottom.size();
    std::vector<Point> points = bottom;
    points.insert(points.end(), top.begin(), top.end());
    std::vector<std::vector<size_t>> faces;
    std::vector<size_t> bottom_face(n), top_face(n);
    for (size_t k = 0; k < n; k++) {
        bottom_face[k] = n - 1 - k;
        top_face[k] = n + k;
        faces.push_back({k, (k + 1) % n, n + (k + 1) % n, n + k});
    }

    faces.push_back(bottom_face);
    faces.push_back(top_face);
    return Mesh::from_vertices_and_faces(points, faces);
}

/// One end of the beam on one face: the corners from left to right and the planes the left and right corner rays left through, -1 for none.
struct EndCorners {
    std::vector<Point> corners;  // left corner, the crease when there is one, right corner
    int left_plane = -1;         // index of the plane the left corner ray left through
    int right_plane = -1;        // index of the plane the right corner ray left through
};

/// One end of the beam on one face: the two corner rays from the left and right origins along out leave the cut faces; when crease is allowed and they leave through two different, non-parallel planes the crease between those planes, on the face plane, becomes a corner between them.
inline EndCorners end_corners(const Point& left, const Point& right, const Vector& out,
                              const std::vector<CutFace>& cuts, const Plane& face_plane, bool crease_allowed)
{

    std::pair<Point, int> hit_left  = ray_exit(left, out, cuts);
    std::pair<Point, int> hit_right = ray_exit(right, out, cuts);
    EndCorners end{{hit_left.first, hit_right.first}, hit_left.second, hit_right.second};
    if (!crease_allowed || end.left_plane < 0 || end.right_plane < 0 || end.left_plane == end.right_plane)
        return end;

    const Plane& a = cuts[end.left_plane].plane;
    const Plane& b = cuts[end.right_plane].plane;
    if (std::abs(a.z_axis().dot(b.z_axis())) > 1.0 - 1e-12) {
        end.right_plane = end.left_plane;  // the same plane twice: no crease, the right corner is on it within rounding
        return end;
    }

    Point crease;
    if (!Intersection::plane_plane_plane(a, b, face_plane, crease)) {
        end.right_plane = end.left_plane;
        return end;
    }

    // the crease is real only when the two planes meet between the corners; two nearly coincident planes,
    // as on a straight run whose sections carry a hair of closure twist, meet far away and the end stays a quad
    Vector across = hit_right.first - hit_left.first;
    double along = across.is_zero() ? -1.0 : (crease - hit_left.first).dot(across) / across.dot(across);
    double fold = (crease - hit_left.first - across * along).magnitude();
    if (along < 0.0 || along > 1.0 || fold > across.magnitude()) {
        end.right_plane = end.left_plane;
        return end;
    }

    end.corners = {hit_left.first, crease, hit_right.first};
    return end;
}

/// Whether the bottom and the top of one end left through the same planes, so their outlines pair corner by corner.
inline bool same_planes(const EndCorners& bottom, const EndCorners& top)
{
    return bottom.left_plane == top.left_plane && bottom.right_plane == top.right_plane;
}

/// The box beam on the axis through from_ref and to_ref along dir, w wide across right = dir x up and h high along up, its ends cut by the given faces: every plane turned to face the axis midpoint, and each corner ray stopped at the first face it leaves through within that face's bounds. An end whose two corners leave through different planes gets the crease between them as an extra corner, on the bottom and the top alike; when the bottom and the top disagree every corner keeps its own first exit and no crease is added, so the end is a slightly folded quad that stays clear of every frame beam.
inline BeamGeom cut_beam(const Point& from_ref, const Point& to_ref, const Vector& dir, const Vector& up,
                         double w, double h, const std::vector<CutFace>& cuts_from, const std::vector<CutFace>& cuts_to)
{

    Vector right = dir.cross(up);
    if (right.is_zero())
        right = dir.cross(Vector(0, 0, 1));

    right = right.normalized() * (w * 0.5);
    Vector rise = up.normalized() * (h * 0.5);
    Point inside = Point::mid_point(from_ref, to_ref);

    std::vector<CutFace> from_planes, to_planes;
    for (const CutFace& face : cuts_from)
        from_planes.push_back(CutFace{facing(face.plane, inside), face.bounds});

    for (const CutFace& face : cuts_to)
        to_planes.push_back(CutFace{facing(face.plane, inside), face.bounds});

    Vector face_normal = perpendicular_part(up, dir);  // the bottom and top faces are spanned by dir and right, so their normal is up less its part along dir
    Plane bottom_plane = Plane::from_point_normal(inside - rise, face_normal);
    Plane top_plane    = Plane::from_point_normal(inside + rise, face_normal);

    // start end, walked left to right on the bottom and the top; end end, walked right to left
    EndCorners start_bottom = end_corners(from_ref - right - rise, from_ref + right - rise, -dir, from_planes, bottom_plane, true);
    EndCorners start_top    = end_corners(from_ref - right + rise, from_ref + right + rise, -dir, from_planes, top_plane, true);
    EndCorners end_bottom   = end_corners(to_ref - right - rise, to_ref + right - rise, dir, to_planes, bottom_plane, true);
    EndCorners end_top      = end_corners(to_ref - right + rise, to_ref + right + rise, dir, to_planes, top_plane, true);

    if (!same_planes(start_bottom, start_top)) {
        start_bottom = end_corners(from_ref - right - rise, from_ref + right - rise, -dir, from_planes, bottom_plane, false);
        start_top    = end_corners(from_ref - right + rise, from_ref + right + rise, -dir, from_planes, top_plane, false);
    }

    if (!same_planes(end_bottom, end_top)) {
        end_bottom = end_corners(to_ref - right - rise, to_ref + right - rise, dir, to_planes, bottom_plane, false);
        end_top    = end_corners(to_ref - right + rise, to_ref + right + rise, dir, to_planes, top_plane, false);
    }

    BeamGeom bg;
    bg.beam_bottom = start_bottom.corners;
    bg.beam_bottom.insert(bg.beam_bottom.end(), end_bottom.corners.rbegin(), end_bottom.corners.rend());
    bg.beam_top = start_top.corners;
    bg.beam_top.insert(bg.beam_top.end(), end_top.corners.rbegin(), end_top.corners.rend());
    bg.side0 = {start_bottom.corners.back(), start_top.corners.back(), end_top.corners.back(), end_bottom.corners.back()};      // right face
    bg.side1 = {start_bottom.corners.front(), start_top.corners.front(), end_top.corners.front(), end_bottom.corners.front()};  // left face
    bg.mesh = outline_solid(bg.beam_bottom, bg.beam_top);

    return bg;
}

}  // namespace wood_reciprocal
