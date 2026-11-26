#include "src/templates/developable_folding/space_curve_ridge.h"
#include "src/templates/template_chamfer.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
SpaceCurveRidge::SpaceCurveRidge(const Polyline& curve, double width, double angle, double zigzag) {

    // crease: the curve's vertices, zig-zagged along their curvature vectors when asked
    _crease = compute_crease(curve, zigzag);

    // ribbon: the discrete ribbon along the crease, its rulings by Eq. 6 from the middle edge out
    _ribbon = compute_ribbon(width);

    // ridge: the ribbon replaced by two half-planes per edge at half the opening to it, trimmed where neighbours meet
    _ridge = compute_ridge(width, angle);

    // pattern: the ridge developed flat
    _pattern = compute_pattern();
}

const std::vector<Point>& SpaceCurveRidge::crease() const {
    return _crease;
}

const QuadGrid& SpaceCurveRidge::ribbon() const {
    return _ribbon;
}

const QuadGrid& SpaceCurveRidge::ridge() const {
    return _ridge;
}

const QuadGrid& SpaceCurveRidge::pattern() const {
    return _pattern;
}

// ═══════════════════════════════════════════════════════════════════════════
// Ribbon, ridge and pattern
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Point> SpaceCurveRidge::compute_crease(const Polyline& curve, double zigzag) {

    const std::vector<Point> points = curve.get_points();
    if (zigzag == 0.0)
        return points;

    const size_t count = points.size();
    std::vector<Point> crease;

    for (size_t i = 0; i < count; i++) {
        const size_t inner = std::clamp<size_t>(i, 1, count - 2);
        const Vector bend = (points[inner + 1] - points[inner]).normalized() -
                            (points[inner] - points[inner - 1]).normalized();
        const double side = i % 2 == 0 ? -1.0 : 1.0;
        crease.push_back(points[i] + bend.normalized() * (side * zigzag));
    }

    return crease;
}

QuadGrid SpaceCurveRidge::compute_ribbon(double width) const {

    const size_t edges = _crease.size() - 1;
    const size_t middle = edges / 2;
    const double half = 0.5 * width;
    std::vector<Vector> tangents;
    for (size_t i = 0; i < edges; i++)
        tangents.push_back((_crease[i + 1] - _crease[i]).normalized());

    // frame of the middle edge: its principal normal from its neighbours' tangents, as on the smooth curve
    std::vector<Vector> normals(edges);
    std::vector<Vector> binormals(edges);
    const Vector bend = tangents[middle + 1] - tangents[middle - 1];
    normals[middle] = (bend - tangents[middle] * bend.dot(tangents[middle])).normalized();
    binormals[middle] = tangents[middle].cross(normals[middle]);

    // rulings by Eq. 6: normal to the last frame's normal, at equal angles to both tangents, half the width along its binormal
    std::vector<Vector> rulings(edges + 1);

    for (size_t i = middle; i + 1 < edges; i++) {
        const Vector solved = normals[i].cross(tangents[i + 1] - tangents[i]);
        rulings[i + 1] = solved * (half / binormals[i].dot(solved));
        normals[i + 1] = rulings[i + 1].cross(tangents[i + 1]).normalized();
        binormals[i + 1] = tangents[i + 1].cross(normals[i + 1]);
    }

    for (size_t i = middle; i > 0; i--) {
        const Vector solved = normals[i].cross(tangents[i] - tangents[i - 1]);
        rulings[i] = solved * (half / binormals[i].dot(solved));
        normals[i - 1] = rulings[i].cross(tangents[i - 1]).normalized();
        binormals[i - 1] = tangents[i - 1].cross(normals[i - 1]);
    }

    rulings[0] = binormals[0] * half;
    rulings[edges] = binormals[edges - 1] * half;

    // rows: one edge, the crease, the other edge
    QuadGrid ribbon = {{}, _crease, {}};

    for (size_t i = 0; i <= edges; i++) {
        ribbon[0].push_back(_crease[i] + rulings[i]);
        ribbon[2].push_back(_crease[i] - rulings[i]);
    }

    return ribbon;
}

QuadGrid SpaceCurveRidge::compute_ridge(double width, double angle) const {

    const size_t edges = _crease.size() - 1;
    const double c = std::cos(0.5 * angle);
    const double s = std::sin(0.5 * angle);

    // arms: per edge its two in-plane directions off the crease, the ribbon's binormal turned by half the opening either way
    std::vector<Vector> tangents;
    std::array<std::vector<Vector>, 2> arms;

    for (size_t i = 0; i < edges; i++) {
        const Vector tangent = (_crease[i + 1] - _crease[i]).normalized();
        const Vector ruling = _ribbon[0][i] - _crease[i];
        const Vector binormal = (ruling - tangent * ruling.dot(tangent)).normalized();
        const Vector normal = binormal.cross(tangent);
        tangents.push_back(tangent);
        arms[0].push_back(binormal * c + normal * s);
        arms[1].push_back(binormal * c - normal * s);
    }

    // rulings: where an edge's half-plane meets the next one's, half the width out on average
    QuadGrid ridge = {{}, _crease, {}};

    for (size_t side = 0; side < 2; side++) {
        const std::vector<Vector>& arm = arms[side];
        std::vector<Point>& row = ridge[2 * side];
        row.push_back(_crease[0] + arm[0] * (0.5 * width));

        for (size_t i = 1; i < edges; i++) {
            const Vector meet = tangents[i - 1].cross(arm[i - 1]).cross(tangents[i].cross(arm[i]));
            row.push_back(_crease[i] + meet * (width / meet.dot(arm[i - 1] + arm[i])));
        }

        row.push_back(_crease[edges] + arm[edges - 1] * (0.5 * width));
    }

    return ridge;
}

QuadGrid SpaceCurveRidge::compute_pattern() const {

    const size_t count = _crease.size();
    QuadGrid pattern(3, std::vector<Point>(count, Point(0.0, 0.0, 0.0)));
    Vector next(1.0, 0.0, 0.0);

    // vertex by vertex: arm a counterclockwise from the crease behind, the crease ahead after it, arm b clockwise from behind
    for (size_t i = 0; i < count; i++) {
        const Vector arm_a = _ridge[0][i] - _ridge[1][i];
        const Vector arm_b = _ridge[2][i] - _ridge[1][i];
        Vector toward_a = next;
        Vector toward_b = next;

        if (i == 0) {
            const Vector ahead = _ridge[1][1] - _ridge[1][0];
            toward_a = next.transformed(Xform::rotation_z(-angle(arm_a, ahead)));
            toward_b = next.transformed(Xform::rotation_z(angle(arm_b, ahead)));
        } else {
            const Vector behind = _ridge[1][i - 1] - _ridge[1][i];
            const Vector flat_behind = (pattern[1][i - 1] - pattern[1][i]).normalized();
            toward_a = flat_behind.transformed(Xform::rotation_z(angle(behind, arm_a)));
            toward_b = flat_behind.transformed(Xform::rotation_z(-angle(behind, arm_b)));
            if (i + 1 < count)
                next = toward_a.transformed(
                    Xform::rotation_z(angle(arm_a, _ridge[1][i + 1] - _ridge[1][i]))
                );
        }

        pattern[0][i] = pattern[1][i] + toward_a * arm_a.magnitude();
        pattern[2][i] = pattern[1][i] + toward_b * arm_b.magnitude();
        if (i + 1 < count)
            pattern[1][i + 1] =
                pattern[1][i] + next * (_ridge[1][i + 1] - _ridge[1][i]).magnitude();
    }

    return pattern;
}

// ═══════════════════════════════════════════════════════════════════════════
// Folding
// ═══════════════════════════════════════════════════════════════════════════
QuadGrid SpaceCurveRidge::folded(double opening) const {

    const size_t count = _crease.size();
    const size_t middle = (count - 1) / 2;
    QuadGrid folded = _ridge;

    // middle: arm a's quad kept, arm b's turned about their crease edge to the opening
    const Point start = _ridge[1][middle];
    const Vector edge = (_ridge[1][middle + 1] - start).normalized();
    const Vector across_a = _ridge[0][middle] - start;
    const Line axis = Line::from_points(start, _ridge[1][middle + 1]);
    const std::function<double(const Point&)> opening_to = [&](const Point& point) {
        const Vector across_b = point - start;
        return angle(across_a - edge * across_a.dot(edge), across_b - edge * across_b.dot(edge));
    };
    const double current = opening_to(_ridge[2][middle]);
    Xform turn = Xform::rotation_around_line(axis, opening - current);
    if (std::abs(opening_to(_ridge[2][middle].transformed(turn)) - opening) > 1e-9)
        turn = Xform::rotation_around_line(axis, current - opening);
    folded[2][middle] = _ridge[2][middle].transformed(turn);
    folded[2][middle + 1] = _ridge[2][middle + 1].transformed(turn);

    // outwards: every next vertex's crease ray at its flat sector angles to the two rulings, its quads placed on it
    for (size_t i = middle + 1; i + 1 < count; i++) {
        const std::array<Point, 3> step = compute_step(folded, i, i + 1);
        for (size_t k = 0; k < 3; k++)
            folded[k][i + 1] = step[k];
    }

    for (size_t i = middle; i > 0; i--) {
        const std::array<Point, 3> step = compute_step(folded, i, i - 1);
        for (size_t k = 0; k < 3; k++)
            folded[k][i - 1] = step[k];
    }

    return folded;
}

std::array<Point, 3>
SpaceCurveRidge::compute_step(const QuadGrid& folded, size_t vertex, size_t next) const {

    const Point& corner = folded[1][vertex];
    const Vector arm_a = (folded[0][vertex] - corner).normalized();
    const Vector arm_b = (folded[2][vertex] - corner).normalized();
    const Vector flat_a = _pattern[0][vertex] - _pattern[1][vertex];
    const Vector flat_b = _pattern[2][vertex] - _pattern[1][vertex];
    const Vector flat_next = _pattern[1][next] - _pattern[1][vertex];

    // the branch the ridge is on: the crease ray on the side of arm a x arm b it takes there
    const Vector design = _ridge[1][next] - _ridge[1][vertex];
    const Vector design_a = _ridge[0][vertex] - _ridge[1][vertex];
    const Vector design_b = _ridge[2][vertex] - _ridge[1][vertex];
    const double side = design.dot(design_a.cross(design_b)) > 0.0 ? 1.0 : -1.0;

    const Vector ray =
        compute_ray(arm_a, angle(flat_a, flat_next), arm_b, angle(flat_b, flat_next), side);

    return {
        place(_pattern[0][next], _pattern[1][vertex], flat_a, flat_next, corner, arm_a, ray),
        corner + ray * flat_next.magnitude(),
        place(_pattern[2][next], _pattern[1][vertex], flat_b, flat_next, corner, arm_b, ray),
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Stages
// ═══════════════════════════════════════════════════════════════════════════
FoldMeasures
SpaceCurveRidge::measure(const std::vector<QuadGrid>& stages, const QuadGrid& pattern) {

    FoldMeasures measures;
    measures.openings = {
        std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()
    };

    for (const QuadGrid& grid : stages) {
        const size_t rows = grid.size();
        const size_t count = grid[0].size();
        std::vector<std::array<Point, 4>> quads;
        std::vector<std::array<size_t, 2>> places;

        // quads: a corner off the plane of the diagonals, and every edge against the pattern's
        for (size_t k = 0; k + 1 < rows; k++)
            for (size_t i = 0; i + 1 < count; i++) {
                const Vector first = grid[k + 1][i + 1] - grid[k][i];
                const Vector second = grid[k + 1][i] - grid[k][i + 1];
                const double off =
                    (grid[k][i + 1] - grid[k][i]).dot(first.cross(second).normalized());
                measures.planarity = std::max(
                    measures.planarity,
                    std::abs(off) / std::max(first.magnitude(), second.magnitude())
                );
                quads.push_back({grid[k][i], grid[k][i + 1], grid[k + 1][i + 1], grid[k + 1][i]});
                places.push_back({k, i});
            }

        for (size_t k = 0; k < rows; k++)
            for (size_t i = 0; i < count; i++) {
                if (i + 1 < count) {
                    const double along = (grid[k][i + 1] - grid[k][i]).magnitude();
                    const double flat = (pattern[k][i + 1] - pattern[k][i]).magnitude();
                    measures.stretch = std::max(measures.stretch, std::abs(along - flat));
                }
                if (k + 1 < rows) {
                    const double across = (grid[k + 1][i] - grid[k][i]).magnitude();
                    const double flat = (pattern[k + 1][i] - pattern[k][i]).magnitude();
                    measures.stretch = std::max(measures.stretch, std::abs(across - flat));
                }
            }

        // inner rows: the opening across every crease edge, Kawasaki and the angle sum at every inner vertex
        for (size_t k = 1; k + 1 < rows; k++)
            for (size_t i = 0; i < count; i++) {
                const Point& vertex = grid[k][i];

                if (i + 1 < count) {
                    const Vector edge = (grid[k][i + 1] - vertex).normalized();
                    const Vector before = grid[k - 1][i] - vertex;
                    const Vector after = grid[k + 1][i] - vertex;
                    const double opening =
                        angle(before - edge * before.dot(edge), after - edge * after.dot(edge));
                    measures.openings[0] = std::min(measures.openings[0], opening);
                    measures.openings[1] = std::max(measures.openings[1], opening);
                }

                if (i == 0 || i + 1 == count)
                    continue;

                const std::array<Vector, 4> rays = {
                    grid[k][i - 1] - vertex,
                    grid[k + 1][i] - vertex,
                    grid[k][i + 1] - vertex,
                    grid[k - 1][i] - vertex,
                };
                std::array<double, 4> sectors;
                for (size_t j = 0; j < 4; j++)
                    sectors[j] = angle(rays[j], rays[(j + 1) % 4]);
                const double sum = sectors[0] + sectors[1] + sectors[2] + sectors[3];
                const double alternating = sectors[0] - sectors[1] + sectors[2] - sectors[3];
                measures.developability =
                    std::max(measures.developability, std::abs(sum - 2.0 * Tolerance::PI));
                measures.kawasaki = std::max(measures.kawasaki, std::abs(alternating));
            }

        // crossings: every two quads sharing no corner, an edge of one through the other
        const std::function<bool(const std::array<Point, 4>&, const std::array<Point, 4>&)>
            through = [](const std::array<Point, 4>& a, const std::array<Point, 4>& b) {
                for (size_t j = 0; j < 4; j++) {
                    const Point& start = a[j];
                    const Point& end = a[(j + 1) % 4];
                    if (pierces(start, end, b[0], b[1], b[2]) ||
                        pierces(start, end, b[0], b[2], b[3]))
                        return true;
                }
                return false;
            };

        for (size_t p = 0; p < quads.size(); p++)
            for (size_t q = p + 1; q < quads.size(); q++) {
                const size_t rows_apart =
                    std::max(places[p][0], places[q][0]) - std::min(places[p][0], places[q][0]);
                const size_t points_apart =
                    std::max(places[p][1], places[q][1]) - std::min(places[p][1], places[q][1]);
                if (rows_apart <= 1 && points_apart <= 1)
                    continue;
                if (through(quads[p], quads[q]) || through(quads[q], quads[p]))
                    measures.crossings++;
            }
    }

    return measures;
}

void SpaceCurveRidge::add_curve(
    WoodSession& session, const Polyline& curve, const SpaceCurveRidge& ridge
) {

    const std::shared_ptr<TreeNode> group = session.group_named("curve");
    Polyline given = curve;
    given.name = "curve";
    session.add_polyline(given, group);

    if (ridge.crease() != curve.get_points()) {
        Polyline zigzag(ridge.crease());
        zigzag.name = "zigzag";
        session.add_polyline(zigzag, group);
    }

    session.add_mesh(to_mesh(ridge.ribbon(), "ribbon"), session.group_named("ribbon"));
}

std::vector<Vector> SpaceCurveRidge::add_quads(
    WoodSession& session, const std::vector<QuadGrid>& stages, const QuadGrid& pattern
) {

    // shifts: every stage after the last along x, a quarter of the widest apart
    const bool sequence = stages.size() > 1;
    std::vector<AABB> boxes;
    double widest = 0.0;

    for (const QuadGrid& grid : stages) {
        AABB box = AABB::empty();
        for (const std::vector<Point>& row : grid)
            box.union_with(AABB::from_points(row));
        widest = std::max(widest, box.max_point()[0] - box.min_point()[0]);
        boxes.push_back(box);
    }

    std::vector<Vector> shifts = {Vector(0.0, 0.0, 0.0)};
    double cursor = boxes[0].max_point()[0];

    for (size_t s = 1; s < stages.size(); s++) {
        shifts.push_back(Vector(cursor + 0.25 * widest - boxes[s].min_point()[0], 0.0, 0.0));
        cursor += 0.25 * widest + boxes[s].max_point()[0] - boxes[s].min_point()[0];
    }

    // stages: the quads as one mesh, every inner row a crease
    const std::shared_ptr<TreeNode> creases = session.group_named("creases");
    AABB box = AABB::empty();

    for (size_t s = 0; s < stages.size(); s++) {
        const std::string prefix = sequence ? fmt::format("stage_{}_", s) : "";
        const Xform shift = Xform::translation(shifts[s][0], shifts[s][1], shifts[s][2]);
        const Mesh mesh = to_mesh(stages[s], prefix + "quads").transformed(shift);
        box.union_with(AABB::from_mesh(mesh));
        session.add_mesh(mesh, session.group_named(prefix + "quads"));

        for (size_t k = 1; k + 1 < stages[s].size(); k++) {
            Polyline crease = Polyline(stages[s][k]).transformed(shift);
            crease.name = fmt::format("{}crease_{}", prefix, k);
            session.add_polyline(crease, creases);
        }
    }

    // pattern: its rows and its columns under the stages, a quarter of the widest below
    const std::shared_ptr<TreeNode> group = session.group_named("pattern");
    AABB flat = AABB::empty();
    for (const std::vector<Point>& row : pattern)
        flat.union_with(AABB::from_points(row));
    const Xform down = Xform::translation(
        box.min_point()[0] - flat.min_point()[0],
        box.min_point()[1] - 0.25 * widest - flat.max_point()[1],
        0.0
    );

    for (size_t k = 0; k < pattern.size(); k++) {
        Polyline row = Polyline(pattern[k]).transformed(down);
        row.name = fmt::format("pattern_row_{}", k);
        session.add_polyline(row, group);
    }

    for (size_t i = 0; i < pattern[0].size(); i++) {
        std::vector<Point> points;
        for (const std::vector<Point>& row : pattern)
            points.push_back(row[i]);
        Polyline column = Polyline(points).transformed(down);
        column.name = fmt::format("pattern_column_{}", i);
        session.add_polyline(column, group);
    }

    return shifts;
}

void SpaceCurveRidge::add_plates(
    WoodSession& session,
    const std::vector<QuadGrid>& stages,
    const std::vector<Vector>& shifts,
    double thickness
) {

    const bool sequence = stages.size() > 1;
    std::vector<std::shared_ptr<FacePlanarElement>> first;

    for (size_t s = 0; s < stages.size(); s++) {
        const std::shared_ptr<TreeNode> group =
            session.group_named(sequence ? fmt::format("stage_{}", s) : "plates");
        const std::string prefix = sequence ? fmt::format("stage_{}_plate", s) : "plate";
        const Xform shift = Xform::translation(shifts[s][0], shifts[s][1], shifts[s][2]);
        const std::vector<std::shared_ptr<FacePlanarElement>> plates = mitered_plates(
            to_mesh(stages[s], "quads").transformed(shift), thickness, 0.0, 0.0, 180.0
        );

        for (size_t i = 0; i < plates.size(); i++) {
            plates[i]->name = fmt::format("{}_{}", prefix, i);
            session.add(plates[i], group);
        }

        if (s == 0)
            first = plates;
    }

    // assembly: the first stage's plates put in one by one, arm by arm along the crease
    session.assemblies.push_back(
        AssemblyModule::module(session.name, AssemblyModule::parts_of(first))
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
Mesh SpaceCurveRidge::to_mesh(const QuadGrid& grid, const std::string& name) {

    const size_t count = grid[0].size();
    std::vector<Point> vertices;
    std::vector<std::vector<size_t>> faces;

    for (const std::vector<Point>& row : grid)
        vertices.insert(vertices.end(), row.begin(), row.end());

    for (size_t k = 0; k + 1 < grid.size(); k++)
        for (size_t i = 0; i + 1 < count; i++)
            faces.push_back(
                {k * count + i, k * count + i + 1, (k + 1) * count + i + 1, (k + 1) * count + i}
            );

    Mesh mesh = Mesh::from_vertices_and_faces(vertices, faces);
    mesh.name = name;
    return mesh;
}

double SpaceCurveRidge::angle(const Vector& a, const Vector& b) {
    return std::atan2(a.cross(b).magnitude(), a.dot(b));
}

Vector SpaceCurveRidge::compute_ray(
    const Vector& a, double first, const Vector& b, double second, double sign
) {

    const double c = a.dot(b);
    const double det = 1.0 - c * c;
    const double along_a = (std::cos(first) - c * std::cos(second)) / det;
    const double along_b = (std::cos(second) - c * std::cos(first)) / det;
    const Vector base = a * along_a + b * along_b;
    const double height = std::sqrt(std::max(0.0, 1.0 - base.dot(base)) / det);

    return base + a.cross(b) * (sign * height);
}

Point SpaceCurveRidge::place(
    const Point& point,
    const Point& at,
    const Vector& from,
    const Vector& to,
    const Point& corner,
    const Vector& along,
    const Vector& toward
) {

    const Vector u = from.normalized();
    const Vector v = (to - u * u.dot(to)).normalized();
    const Vector x = along.normalized();
    const Vector y = (toward - x * x.dot(toward)).normalized();
    const Vector offset = point - at;

    return corner + x * offset.dot(u) + y * offset.dot(v);
}

bool SpaceCurveRidge::pierces(
    const Point& start, const Point& end, const Point& a, const Point& b, const Point& c
) {

    const Vector normal = (b - a).cross(c - a);
    const double from = (start - a).dot(normal);
    const double to = (end - a).dot(normal);
    const double tolerance = 1e-9 * normal.magnitude() * (end - start).magnitude();

    if (!((from < -tolerance && to > tolerance) || (from > tolerance && to < -tolerance)))
        return false;

    const Point hit = start + (end - start) * (from / (from - to));

    return (b - a).cross(hit - a).dot(normal) > 0.0 && (c - b).cross(hit - b).dot(normal) > 0.0 &&
           (a - c).cross(hit - c).dot(normal) > 0.0;
}

}
