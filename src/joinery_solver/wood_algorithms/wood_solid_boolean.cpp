#include "pch.h"
#include "wood_element_geometry.h"

namespace wood_session {

using namespace session_cpp;

namespace {
struct Polygon {
    std::vector<Point> points;
    Vector normal;
    explicit Polygon(std::vector<Point> p) : points(std::move(p)), normal(compute_newell(points)) {
    }
    void invert() {
        std::reverse(points.begin(), points.end());
        normal = -normal;
    }
};

void append_polygon(const std::vector<Point>& points, std::vector<Polygon>& polygons, double tolerance) {

    std::vector<Point> clean;

    for (const Point& point : points)
        if (clean.empty() || Point::distance(clean.back(), point) > tolerance)
            clean.push_back(point);

    if (clean.size() > 1 && Point::distance(clean.front(), clean.back()) <= tolerance)
        clean.pop_back();

    if (clean.size() < 3)
        return;

    Polygon polygon(std::move(clean));

    if (polygon.normal.magnitude_squared() > 0.5)
        polygons.push_back(std::move(polygon));
}

struct SplitPlane {
    Point origin;
    Vector normal;
    double tolerance;
    void split(const Polygon& p, std::vector<Polygon>& aligned, std::vector<Polygon>& opposed,
               std::vector<Polygon>& front, std::vector<Polygon>& back) const {
        std::vector<int> type;
        std::vector<double> distances;
        int sides = 0;
        for (const Point& point : p.points) {
            const double d = (point - origin).dot(normal);
            const int t = d > tolerance ? 1 : d < -tolerance ? 2
                                                             : 0;
            type.push_back(t);
            distances.push_back(d);
            sides |= t;
        }
        if (sides == 0)
            (normal.dot(p.normal) >= 0 ? aligned : opposed).push_back(p);
        else if (sides == 1)
            front.push_back(p);
        else if (sides == 2)
            back.push_back(p);
        else {
            std::vector<Point> f, b;
            for (size_t i = 0; i < p.points.size(); ++i) {
                const size_t j = (i + 1) % p.points.size();
                if (type[i] != 2)
                    f.push_back(p.points[i]);
                if (type[i] != 1)
                    b.push_back(p.points[i]);
                if ((type[i] | type[j]) == 3) {
                    const Point hit = p.points[i] + (p.points[j] - p.points[i]) * (distances[i] / (distances[i] - distances[j]));
                    f.push_back(hit);
                    b.push_back(hit);
                }
            }
            append_polygon(f, front, tolerance);
            append_polygon(b, back, tolerance);
        }
    }
};

class Bsp {
    std::optional<SplitPlane> plane;
    std::vector<Polygon> polygons;
    std::unique_ptr<Bsp> front, back;
    double tolerance;

public:
    explicit Bsp(double t) : tolerance(t) {
    }
    void build(const std::vector<Polygon>& input, int depth = 0) {
        if (input.empty())
            return;
        if (depth > 1024 || input.size() > 200000)
            throw std::runtime_error("Solid split complexity limit exceeded");
        if (!plane) {
            const Polygon& p = input[input.size() / 2];
            plane = SplitPlane{p.points[0], p.normal, tolerance};
        }
        std::vector<Polygon> f, b;
        for (const Polygon& p : input)
            plane->split(p, polygons, polygons, f, b);
        if (!f.empty()) {
            if (!front)
                front = std::make_unique<Bsp>(tolerance);
            front->build(f, depth + 1);
        }
        if (!b.empty()) {
            if (!back)
                back = std::make_unique<Bsp>(tolerance);
            back->build(b, depth + 1);
        }
    }
    std::vector<Polygon> clip(const std::vector<Polygon>& input) const {
        if (!plane)
            return input;
        std::vector<Polygon> f, b;
        for (const Polygon& p : input)
            plane->split(p, f, b, f, b);
        if (front)
            f = front->clip(f);
        if (back)
            b = back->clip(b);
        else
            b.clear();
        f.insert(f.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
        return f;
    }
    void clip_to(const Bsp& other) {

        std::array<Bsp*, 1025> stack;
        size_t count = 1;
        stack[0] = this;

        while (count) {
            Bsp* node = stack[--count];
            node->polygons = other.clip(node->polygons);

            if (count + 2 > stack.size())
                throw std::runtime_error("Solid traversal depth exceeded");

            if (node->back)
                stack[count++] = node->back.get();

            if (node->front)
                stack[count++] = node->front.get();
        }
    }

    void invert() {

        std::array<Bsp*, 1025> stack;
        size_t count = 1;
        stack[0] = this;

        while (count) {
            Bsp* node = stack[--count];

            for (Polygon& polygon : node->polygons)
                polygon.invert();

            if (node->plane)
                node->plane->normal = -node->plane->normal;

            std::swap(node->front, node->back);

            if (count + 2 > stack.size())
                throw std::runtime_error("Solid traversal depth exceeded");

            if (node->back)
                stack[count++] = node->back.get();

            if (node->front)
                stack[count++] = node->front.get();
        }
    }

    std::vector<Polygon> all() const {

        std::vector<Polygon> polygons;
        std::array<const Bsp*, 1025> stack;
        size_t count = 1;
        stack[0] = this;

        while (count) {
            const Bsp* node = stack[--count];
            polygons.insert(polygons.end(), node->polygons.begin(), node->polygons.end());

            if (count + 2 > stack.size())
                throw std::runtime_error("Solid traversal depth exceeded");

            if (node->back)
                stack[count++] = node->back.get();

            if (node->front)
                stack[count++] = node->front.get();
        }

        return polygons;
    }
};

std::vector<Polygon> polygons_of(const Mesh& mesh) {
    std::vector<Polygon> result;
    for (const std::pair<const size_t, std::vector<size_t>>& entry : mesh.face) {
        const size_t id = entry.first;
        const std::vector<size_t>& face = entry.second;
        if (face.size() < 3)
            continue;
        const auto cached = mesh.get_triangulation().find(id);
        if (cached != mesh.get_triangulation().end()) {
            for (const std::array<size_t, 3>& tri : cached->second)
                result.emplace_back(std::vector<Point>{mesh.vertex.at(tri[0]).position(), mesh.vertex.at(tri[1]).position(), mesh.vertex.at(tri[2]).position()});
        } else {
            std::vector<Point> points;
            for (size_t v : face)
                points.push_back(mesh.vertex.at(v).position());
            const Vector normal = compute_newell(points);
            std::vector<std::vector<Point>> rings{points};
            if (auto holes = mesh.face_holes.find(id); holes != mesh.face_holes.end())
                for (const std::vector<size_t>& ids : holes->second) {
                    rings.emplace_back();
                    for (size_t v : ids)
                        rings.back().push_back(mesh.vertex.at(v).position());
                }
            const Mesh patch = Mesh::from_polygon_with_holes(rings);
            for (const std::pair<const size_t, std::vector<size_t>>& entry : patch.face) {
                const std::vector<size_t>& ids = entry.second;
                std::vector<Point> triangle;
                for (size_t v : ids)
                    triangle.push_back(patch.vertex.at(v).position());
                Polygon p(std::move(triangle));
                if (p.normal.dot(normal) < 0)
                    p.invert();
                result.push_back(std::move(p));
            }
        }
    }
    double volume = 0;
    for (const Polygon& p : result) {
        const Vector a = p.points[0] - Point(0, 0, 0);
        for (size_t i = 1; i + 1 < p.points.size(); ++i)
            volume += a.dot((p.points[i] - Point(0, 0, 0)).cross(p.points[i + 1] - Point(0, 0, 0)));
    }
    if (volume < 0)
        for (Polygon& p : result)
            p.invert();
    return result;
}

Mesh stitched_mesh(const std::vector<Polygon>& polygons, double tolerance) {
    std::vector<Polyline> rings;
    for (const Polygon& p : polygons)
        rings.push_back(Polyline(p.points).closed());
    const Mesh welded = Mesh::from_polylines(rings, tolerance);
    std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> data = welded.to_vertices_and_faces();
    const std::vector<Point>& vertices = data.first;
    std::vector<std::vector<size_t>>& faces = data.second;
    for (std::vector<size_t>& face : faces) {
        std::vector<size_t> split;
        for (size_t i = 0; i < face.size(); ++i) {
            const size_t a = face[i], b = face[(i + 1) % face.size()];
            const Vector direction = vertices[b] - vertices[a];
            const double length2 = direction.magnitude_squared();
            if (length2 <= tolerance * tolerance)
                continue;
            std::vector<std::pair<double, size_t>> along{{0, a}};
            for (size_t k = 0; k < vertices.size(); ++k) {
                if (k == a || k == b)
                    continue;
                const double t = (vertices[k] - vertices[a]).dot(direction) / length2;
                if (t <= 0 || t >= 1)
                    continue;
                if ((vertices[k] - (vertices[a] + direction * t)).magnitude_squared() <= tolerance * tolerance)
                    along.emplace_back(t, k);
            }
            std::sort(along.begin(), along.end());
            for (const std::pair<double, size_t>& vertex : along)
                if (split.empty() || split.back() != vertex.second)
                    split.push_back(vertex.second);
        }
        face = std::move(split);
    }
    Mesh result = Mesh::from_vertices_and_faces(vertices, faces);
    if (result.number_of_faces() && !result.is_closed())
        throw std::runtime_error("Solid boolean produced an open boundary");
    return result;
}

}

Mesh solid_boolean(const Mesh& source, const Mesh& cutter, SolidOperation operation, double tolerance) {
    if (!std::isfinite(tolerance) || tolerance <= 0)
        throw std::invalid_argument("Solid tolerance must be positive");
    if (!source.number_of_faces())
        return operation == SolidOperation::unite ? cutter : Mesh();
    if (!cutter.number_of_faces())
        return operation == SolidOperation::intersection ? Mesh() : source;
    if (!source.is_closed() || !cutter.is_closed())
        throw std::invalid_argument("Solid booleans require closed meshes");
    Bsp a(tolerance), b(tolerance);
    a.build(polygons_of(source));
    b.build(polygons_of(cutter));
    if (operation == SolidOperation::difference) {
        a.invert();
        a.clip_to(b);
        b.clip_to(a);
        b.invert();
        b.clip_to(a);
        b.invert();
        a.build(b.all());
        a.invert();
    } else if (operation == SolidOperation::intersection) {
        a.invert();
        b.clip_to(a);
        b.invert();
        a.clip_to(b);
        b.clip_to(a);
        a.build(b.all());
        a.invert();
    } else {
        a.clip_to(b);
        b.clip_to(a);
        b.invert();
        b.clip_to(a);
        b.invert();
        a.build(b.all());
    }
    return stitched_mesh(a.all(), tolerance);
}

}
