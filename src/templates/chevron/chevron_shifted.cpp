#include "src/templates/chevron/chevron_shifted.h"

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ChevronShifted::ChevronShifted(const ChevronShiftedGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // surface: the NURBS surface the cells are laid on, turned so its u runs across the shell as the 2018 definition read it
    add_nurbssurface(guide.surface, add_group("surface"));
    _across = guide.surface;
    _across.transpose();

    // points: iso curves across the surface divided at equal arc length, the interior points shifted along the rows
    _points = compute_points();

    // cells: hexagons over three rows in staggered columns, a half-cell quad at one end of every column, as one mesh
    _cells = compute_cells();
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // edge_planes: through every edge along the surface normal, the even cells' long sides merged and turned
    const std::map<std::pair<size_t, size_t>, Plane> edge_planes = compute_edge_planes();
    _edge_planes = compute_merged_edge_planes(edge_planes);
    const std::shared_ptr<TreeNode> planes = add_group("edge_planes");

    for (const std::pair<const std::pair<size_t, size_t>, Plane>& plane : _edge_planes)
        add_plane(plane.second, planes);

    // sides: a plane at every cell's centre and its loop of side planes with the neighbours' cross planes inserted
    _face_planes = compute_face_planes();
    _sides = compute_sides();

    // plates: per cell a box of a top and a bottom plate stepped in where its own walls stand, then its walls, a merged long side one wall, mitred at the box corners, plate_<i> in box_<cell>
    const wood_chevron::ShiftedPlates plates = compute_plates();
    const std::shared_ptr<TreeNode> plates_group = add_group("plates");

    for (size_t cell = 0; cell < plates.boxes.size(); cell++) {
        const std::shared_ptr<TreeNode> box = add_group(fmt::format("box_{}", cell), plates_group);

        for (const int plate : plates.boxes[cell])
            add(plates.plates[plate], box);

        for (const std::array<int, 2>& wall : plates.walls)
            if (wall[1] == static_cast<int>(cell))
                add(plates.plates[wall[0]], box);
    }

    // joinery: insertion vectors, joint types, divisions, adjacency and three-valence groups for the solver, an insertion line per cell
    const std::shared_ptr<TreeNode> insertion = add_group("insertion");
    const std::vector<Line> insertion_lines = compute_joinery(plates);

    for (Line line : insertion_lines) {
        line.width = 2.5;
        line.linecolor = Color(206, 16, 66, 255);
        line.arrowhead = Arrowhead::END;
        add_line(line, insertion);
    }
    set_node_color(insertion, Color(206, 16, 66, 255), true);

    // joints: the wood solver reads the tables and cuts the definition's tenons and their slots into the plates and the Annen fingers into the box corners; a contact is at least a wall thickness square, so the slivers where a wall's slanted end crosses the middle of a merged side are none; an edge over long_edge takes twice the steps, one under short_edge no tenons
    settings.clipper_area = guide.plate_thickness * guide.plate_thickness;
    settings.joint_parameters[6] = guide.long_edge;
    settings.limit_min_joint_length = guide.short_edge;
    WoodSession::adjacency = _adjacency;
    WoodSession::three_valence = {{0}};

    for (const std::array<int, 4>& group : _three_valence)
        WoodSession::three_valence.push_back(std::vector<int>(group.begin(), group.end()));

    for (size_t i = 0; i < plates.plates.size(); i++) {
        plates.plates[i]->insertion_vectors() = _insertion_vectors[i];
        plates.plates[i]->feature_axes = _feature_axes[i];
        plates.plates[i]->feature_types = _joints_per_face[i];
        plates.plates[i]->feature_divisions = _divisions_per_face[i];
    }

    compute_features();

    // slots: the plates across a wall slide into it along their mover's vector, which leaves their own plane by the turn between two cells
    if (guide.closed_boxes)
        open_slots(plates, insertion_lines);

    // assembly: every even cell's box from its inward walls on their corner fingers, its two plates along its one vector, its outward
    // wall last, every odd cell's shelf of three plates; the boxes and shelves cell by cell along their vectors
    std::vector<AssemblyModule> boxes;
    for (size_t cell = 0; cell < plates.boxes.size(); cell++) {
        const Vector along = insertion_lines[cell].to_vector().normalized();

        // a wall comes on its fingers in the plane of the wall it meets: its own normal laid into that plane, or square to the box's up
        std::vector<Vector> square = {along, along * -1.0};
        for (const std::array<int, 2>& wall : plates.walls)
            for (const std::array<int, 2>& met : plates.walls) {
                if (wall[0] == met[0] || wall[1] != static_cast<int>(cell) || met[1] != static_cast<int>(cell))
                    continue;
                const Vector normal = plates.plates[wall[0]]->planes[0].z_axis();
                const Vector other = plates.plates[met[0]]->planes[0].z_axis();
                const Vector laid = normal - other * normal.dot(other);
                if (laid.magnitude() < 1e-3)
                    continue;
                square.push_back(laid.normalized());
                square.push_back(laid.normalized() * -1.0);
                square.push_back(other.cross(_face_planes[cell].z_axis()).normalized());
                square.push_back(square.back() * -1.0);
            }
        std::vector<AssemblyModule> walls;
        std::vector<AssemblyModule> outward;
        for (const std::array<int, 2>& wall : plates.walls)
            if (wall[1] == static_cast<int>(cell))
                (plates.outward.count(wall[0]) ? outward : walls).push_back(AssemblyModule::part(plates.plates[wall[0]], square));
        for (const int plate : plates.boxes[cell])
            walls.push_back(AssemblyModule::part(plates.plates[plate], {along, along * -1.0}));
        walls.insert(walls.end(), outward.begin(), outward.end());
        boxes.push_back(AssemblyModule::module(fmt::format("{}_{}", walls.size() > 3 ? "box" : "shelf", cell), walls, {along, along * -1.0}));
    }
    AssemblyModule whole = AssemblyModule::module(name, boxes);
    whole.play = 0.15;
    assemblies = {whole};
}

// ═══════════════════════════════════════════════════════════════════════════
// Slots
// ═══════════════════════════════════════════════════════════════════════════
void ChevronShifted::open_slots(const wood_chevron::ShiftedPlates& plates, const std::vector<Line>& insertion_lines) {

    // every plate's solid, its cell and whether it is a wall
    std::vector<std::shared_ptr<SolidBody>> bodies;
    for (const std::shared_ptr<FacePlanarElement>& plate : plates.plates)
        bodies.push_back(std::make_shared<SolidBody>(plate->model_geometry_mesh()));
    std::vector<std::vector<int>> boxes(plates.boxes.size());
    std::set<int> walls;
    for (size_t cell = 0; cell < plates.boxes.size(); cell++)
        boxes[cell].assign(plates.boxes[cell].begin(), plates.boxes[cell].end());
    for (const std::array<int, 2>& wall : plates.walls) {
        boxes[wall[1]].push_back(wall[0]);
        walls.insert(wall[0]);
    }

    // per pair what a box shares, on its way in, with the plates of the boxes before it, where it lies in the plate that loses it
    std::map<std::pair<int, int>, std::vector<Mesh>> shared;
    for (size_t cell = 0; cell < boxes.size(); cell++) {
        const Vector along = insertion_lines[cell].to_vector().normalized();

        for (const int moving : boxes[cell]) {
            const AABB way = AABB::merge(bodies[moving]->box(), bodies[moving]->box_at(Xform::translation(-along[0] * OPEN_REACH, -along[1] * OPEN_REACH, -along[2] * OPEN_REACH)));

            for (size_t before = 0; before < cell; before++)
                for (const int standing : boxes[before]) {
                    if (!way.intersects(bodies[standing]->box()))
                        continue;
                    const bool cut_moving = walls.count(moving) && !walls.count(standing);

                    for (double distance = 0.0; distance <= OPEN_REACH; distance += OPEN_STEP) {
                        const Xform back = Xform::translation(-along[0] * distance, -along[1] * distance, -along[2] * distance);
                        if (!bodies[moving]->box_at(back).intersects(bodies[standing]->box()) || bodies[moving]->shared_with(*bodies[standing], back).volume < 1.0)
                            continue;
                        const Mesh common = solid_boolean(plates.plates[moving]->model_geometry_mesh().transformed(back), plates.plates[standing]->model_geometry_mesh(), SolidOperation::intersect);
                        for (const Mesh& piece : solid_pieces(common))
                            shared[{cut_moving ? moving : standing, cut_moving ? standing : moving}].push_back(
                                cut_moving ? piece.transformed(Xform::translation(along[0] * distance, along[1] * distance, along[2] * distance)) : piece
                            );
                    }
                }
        }
    }

    // each pair's pieces that run on from each other one hull, the pair's hulls one cut
    for (const std::pair<const std::pair<int, int>, std::vector<Mesh>>& pair : shared) {
        // a run's box is that of its last piece
        std::vector<std::pair<AABB, std::vector<Point>>> runs;
        for (const Mesh& piece : pair.second) {
            const std::vector<Point> points = piece.to_vertices_and_faces().first;
            const AABB box = AABB::from_points(points, 1.0);
            size_t run = 0;
            while (run < runs.size() && !runs[run].first.intersects(box))
                run++;
            if (run == runs.size())
                runs.push_back({box, {}});
            runs[run].first = box;
            runs[run].second.insert(runs[run].second.end(), points.begin(), points.end());
        }
        std::vector<Mesh> hulls;
        for (const std::pair<AABB, std::vector<Point>>& run : runs)
            hulls.push_back(solid_hull(run.second));
        add_interaction(plates.plates[pair.first.second], plates.plates[pair.first.first], std::make_shared<InteractionFeatureSolid>(shells_side_by_side(hulls), SolidOperation::subtract));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& ChevronShifted::mesh() const {
    return _mesh;
}

const wood_chevron::ShiftedCells& ChevronShifted::cells() const {
    return _cells;
}

const std::vector<std::vector<Vector>>& ChevronShifted::insertion_vectors() const {
    return _insertion_vectors;
}

const std::vector<std::vector<Vector>>& ChevronShifted::feature_axes() const {
    return _feature_axes;
}

const std::vector<std::vector<int>>& ChevronShifted::joints_per_face() const {
    return _joints_per_face;
}

const std::vector<std::vector<int>>& ChevronShifted::divisions_per_face() const {
    return _divisions_per_face;
}

const std::vector<std::array<int, 4>>& ChevronShifted::three_valence() const {
    return _three_valence;
}

const std::vector<std::pair<int, int>>& ChevronShifted::adjacency() const {
    return _adjacency;
}

// ═══════════════════════════════════════════════════════════════════════════
// Points and cells
// ═══════════════════════════════════════════════════════════════════════════
std::vector<std::vector<Point>> ChevronShifted::compute_points() const {

    const std::pair<double, double> domain_u = _across.domain(0);
    const int steps = guide.iso_count - 2;
    const NurbsCurve middle = _across.iso_curve(1, 0.5 * (domain_u.first + domain_u.second));
    const std::vector<double> stations = middle.divide_by_count(2 * steps + 1, true).second;
    std::vector<double> rows = {stations.front()};

    for (int i = 1; i <= steps; i++)
        rows.push_back(stations[2 * i - 1]);

    rows.push_back(stations.back());

    std::vector<std::vector<std::pair<double, double>>> parameters;
    std::vector<std::vector<Point>> points;

    for (const double v : rows) {
        const std::vector<double> columns =
            _across.iso_curve(0, v).divide_by_count(guide.point_count, true).second;
        std::vector<std::pair<double, double>> row;
        std::vector<Point> row_points;

        for (const double u : columns) {
            row.emplace_back(u, v);
            row_points.push_back(_across.point_at(u, v));
        }

        parameters.push_back(row);
        points.push_back(row_points);
    }

    for (size_t i = 1; i + 1 < rows.size(); i++)
        for (int j = 0; j < guide.point_count; j++) {
            const std::pair<double, double>& here = parameters[i][j];
            const std::pair<double, double>& there = parameters[j % 2 == 0 ? i + 1 : i - 1][j];
            points[i][j] = _across.point_at(
                here.first + (there.first - here.first) * guide.shift,
                here.second + (there.second - here.second) * guide.shift
            );
        }

    return points;
}

wood_chevron::ShiftedCells ChevronShifted::compute_cells() const {

    const int columns = guide.point_count - 1;
    wood_chevron::ShiftedCells draft;

    for (int i = 0; i < guide.iso_count - 2; i++)
        for (int j = 0; j < columns; j++) {
            if (i % 2 == 0 && j % 2 == 0) {
                add_hexagon(
                    draft,
                    i,
                    j,
                    j % 4 == 2 ? 2 : 20
                );

                if (i == guide.iso_count - 4)
                    add_quad(
                        draft,
                        i + 2,
                        j,
                        3
                    );
            }

            if (i % 2 == 1 && j % 2 == 1) {
                if (i == 1)
                    add_quad(
                        draft,
                        0,
                        j,
                        -1
                    );

                add_hexagon(
                    draft,
                    i,
                    j,
                    -2
                );

                if (i == guide.iso_count - 4)
                    add_quad(
                        draft,
                        i + 2,
                        j,
                        -3
                    );
            }
        }

    std::vector<std::vector<int>> sequence(columns);

    for (size_t cell = 0; cell < draft.columns.size(); cell++)
        sequence[draft.columns[cell]].push_back(static_cast<int>(cell));

    wood_chevron::ShiftedCells cells;

    for (int j = 0; j < columns; j++) {
        std::vector<int> order = sequence[j];

        if (j % 2 == 1)
            std::reverse(order.begin(), order.end());

        for (const int cell : order)
            add_cell(
                cells,
                draft.faces.vertices[cell],
                draft.masks[cell],
                draft.columns[cell]
            );
    }

    for (size_t cell = 0; cell < cells.faces.vertices.size(); cell++) {
        const std::vector<size_t>& keys = cells.faces.vertices[cell];

        for (size_t k = 0; k < keys.size(); k++) {
            const size_t a = keys[k];
            const size_t b = keys[(k + 1) % keys.size()];
            cells.faces.edge_faces[{std::min(a, b), std::max(a, b)}].push_back(
                static_cast<int>(cell)
            );
        }
    }

    return cells;
}

Mesh ChevronShifted::compute_mesh() const {

    std::vector<std::vector<Point>> polygons;

    for (const std::vector<size_t>& keys : _cells.faces.vertices) {
        std::vector<Point> polygon;

        for (const size_t key : keys)
            polygon.push_back(point(key));

        polygons.push_back(polygon);
    }

    return Mesh::from_polylines(polygons);
}

const Point& ChevronShifted::point(size_t key) const {
    return _points[key / guide.point_count][key % guide.point_count];
}

size_t ChevronShifted::vertex_key(int i, int j) const {
    return static_cast<size_t>(i * guide.point_count + j);
}

void ChevronShifted::add_cell(
    wood_chevron::ShiftedCells& cells,
    std::vector<size_t> keys,
    int mask,
    int column
) {

    cells.faces.vertices.push_back(std::move(keys));
    cells.masks.push_back(mask);
    cells.columns.push_back(column);
}

void ChevronShifted::add_hexagon(
    wood_chevron::ShiftedCells& cells,
    int i,
    int j,
    int mask
) const {

    add_cell(
        cells,
        {vertex_key(i, j),
         vertex_key(i, j + 1),
         vertex_key(i + 1, j + 1),
         vertex_key(i + 2, j + 1),
         vertex_key(i + 2, j),
         vertex_key(i + 1, j)},
        mask,
        j
    );
}

void ChevronShifted::add_quad(
    wood_chevron::ShiftedCells& cells,
    int i,
    int j,
    int mask
) const {

    add_cell(
        cells,
        {vertex_key(i, j), vertex_key(i, j + 1), vertex_key(i + 1, j + 1), vertex_key(i + 1, j)},
        mask,
        j
    );
}

std::pair<size_t, size_t> ChevronShifted::edge_key(int cell, int edge) const {

    const std::vector<size_t>& keys = _cells.faces.vertices[cell];
    const size_t a = keys[edge];
    const size_t b = keys[(edge + 1) % keys.size()];
    return {std::min(a, b), std::max(a, b)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════
std::map<std::pair<size_t, size_t>, Plane> ChevronShifted::compute_edge_planes() const {

    const std::pair<double, double> domain_u = _across.domain(0);
    const std::array<NurbsCurve, 2> sides = {
        _across.iso_curve(1, domain_u.first), _across.iso_curve(1, domain_u.second)
    };
    std::map<std::pair<size_t, size_t>, Plane> planes;

    for (const std::pair<const std::pair<size_t, size_t>, std::vector<int>>& edge :
         _cells.faces.edge_faces) {
        const Point& start = point(edge.first.first);
        const Point& end = point(edge.first.second);
        const Point middle = Point::mid_point(start, end);
        const std::pair<double, double> uv = _across.closest_parameters(middle);
        Vector y = _across.normal_at(uv.first, uv.second);

        if (edge.second.size() == 1) {
            double nearest = std::numeric_limits<double>::infinity();
            Vector curvature(0.0, 0.0, 0.0);

            for (const NurbsCurve& side : sides) {
                const double t = side.closest_parameter(middle);
                const double distance = Point::distance(side.point_at(t), middle);

                if (distance < nearest) {
                    nearest = distance;
                    curvature = side.evaluate(t, 2)[2];
                }
            }

            if (curvature.magnitude() > Tolerance::ZERO_TOLERANCE)
                y = -curvature;
        }

        planes.emplace(edge.first, Plane(middle, end - start, y));
    }

    return planes;
}

std::map<std::pair<size_t, size_t>, Plane> ChevronShifted::compute_merged_edge_planes(
    std::map<std::pair<size_t, size_t>, Plane> planes
) const {

    if (!guide.average_planes)
        return planes;

    const double angle = guide.edge_rotation * Tolerance::TO_RADIANS;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];

        if (mask != 2 && mask != 20)
            continue;

        const double turn = mask == 2 ? -angle : angle;

        for (const std::array<int, 2> pair : {std::array<int, 2>{1, 2}, std::array<int, 2>{4, 5}}) {
            const std::pair<size_t, size_t> first = edge_key(static_cast<int>(cell), pair[0]);
            const std::pair<size_t, size_t> second = edge_key(static_cast<int>(cell), pair[1]);
            const Plane& a = planes.at(first);
            const Plane& b = planes.at(second);
            const Point middle = Point::mid_point(a.origin(), b.origin());
            Plane merged = Plane(middle, a.x_axis(), a.y_axis());

            if (_cells.faces.edge_faces.at(first).size() != 1) {
                const std::pair<double, double> uv = _across.closest_parameters(middle);
                merged = turned_about_y(
                    Plane(middle, a.x_axis() + b.x_axis(), _across.normal_at(uv.first, uv.second)),
                    turn
                );
            }

            planes.at(first) = merged;
            planes.at(second) = merged;
        }
    }

    return planes;
}

Plane ChevronShifted::turned_about_y(const Plane& plane, double angle) {

    const Vector x =
        (plane.x_axis() * std::cos(angle) - plane.z_axis() * std::sin(angle)).normalized();
    const Vector z =
        (plane.x_axis() * std::sin(angle) + plane.z_axis() * std::cos(angle)).normalized();
    return Plane::from_frame(plane.origin(), x, plane.y_axis(), z);
}

std::vector<Plane> ChevronShifted::compute_face_planes() const {

    std::vector<Plane> planes;

    for (const std::vector<size_t>& keys : _cells.faces.vertices) {
        Vector weighted(0.0, 0.0, 0.0);
        double perimeter = 0.0;

        for (size_t k = 0; k < keys.size(); k++) {
            const Point& a = point(keys[k]);
            const Point& b = point(keys[(k + 1) % keys.size()]);
            const double length = Point::distance(a, b);
            weighted =
                weighted + (Vector(a[0], a[1], a[2]) + Vector(b[0], b[1], b[2])) * (0.5 * length);
            perimeter += length;
        }

        const Vector centre = weighted * (1.0 / perimeter);
        const std::pair<double, double> uv =
            _across.closest_parameters(Point(centre[0], centre[1], centre[2]));
        const std::vector<Vector> derivatives = _across.evaluate(uv.first, uv.second, 1);
        const Vector normal = _across.normal_at(uv.first, uv.second);
        const Vector x = derivatives[1].normalized();
        planes.push_back(
            Plane::from_frame(
                _across.point_at(uv.first, uv.second), x, x.cross(normal).normalized(), -normal
            )
        );
    }

    return planes;
}

std::vector<std::vector<wood_chevron::ShiftedSide>> ChevronShifted::compute_sides() const {

    std::vector<std::vector<wood_chevron::ShiftedSide>> sides;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const std::vector<size_t>& keys = _cells.faces.vertices[static_cast<int>(cell)];
        const int count = static_cast<int>(keys.size());
        std::vector<wood_chevron::ShiftedSide> loop;

        for (int j = 0; j < count; j++) {
            const std::pair<size_t, size_t> key = edge_key(static_cast<int>(cell), j);
            loop.push_back({_edge_planes.at(key), j});

            if (count != 6 || (j != 1 && j != 4))
                continue;

            const std::vector<int>& on_edge = _cells.faces.edge_faces.at(key);

            if (on_edge.size() != 1) {
                // the neighbour's edge before the shared one: its cross plane through this cell's middle vertex
                const int neighbour =
                    on_edge[0] != static_cast<int>(cell) ? on_edge[0] : on_edge[1];
                const int neighbour_count =
                    static_cast<int>(_cells.faces.vertices[neighbour].size());
                int local = 0;

                while (edge_key(neighbour, local) != key)
                    local++;

                const int previous = (local + neighbour_count - 1) % neighbour_count;
                loop.push_back({_edge_planes.at(edge_key(neighbour, previous)), -1});
            } else {
                const Plane& plane = _edge_planes.at(key);
                const Plane& next =
                    _edge_planes.at(edge_key(static_cast<int>(cell), (j + 1) % count));
                loop.push_back(
                    {Plane(
                         Point::mid_point(plane.origin(), next.origin()),
                         plane.z_axis(),
                         plane.y_axis()
                     ),
                     -1}
                );
            }
        }

        sides.push_back(loop);
    }

    // the odd hexagons' step after edge 1, where it steps into the slab of the lower even neighbour's long wall: the plane through the step's corner on that wall, along the neighbour's insertion vector, so the wall slides along the cut on its way in; a step the other way is in nobody's way
    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        if (_cells.masks[cell] != -2)
            continue;

        const std::pair<size_t, size_t> key = edge_key(static_cast<int>(cell), 1);
        const std::vector<int>& on_edge = _cells.faces.edge_faces.at(key);

        if (on_edge.size() != 2)
            continue;

        const int neighbour = on_edge[0] != static_cast<int>(cell) ? on_edge[0] : on_edge[1];
        const Plane& lower = sides[cell][1].plane;
        const Plane& upper = sides[cell][3].plane;
        const Plane& across = sides[cell][2].plane;
        Point on_lower;
        Point on_upper;
        const std::optional<Line> step = Intersection::plane_plane(across, lower);

        if (!step ||
            !Intersection::plane_plane_plane(across, lower, _face_planes[cell], on_lower) ||
            !Intersection::plane_plane_plane(across, upper, _face_planes[cell], on_upper))
            continue;

        const double into = (on_upper - on_lower).dot(lower.z_axis());
        const double slab = (_face_planes[neighbour].origin() - on_lower).dot(lower.z_axis());

        if (into * slab <= 0.0)
            continue;

        const Vector along =
            direction_of(sides[neighbour], _face_planes[neighbour], _cells.masks[neighbour]);
        sides[cell][2].plane = Plane(step->start(), along, step->to_vector());
    }

    // the odd hexagons' step after edge 4, between the two even neighbours' outward walls standing inside the cell, where the upper wall stands further out than the lower one: the plane through the step's corner on the lower wall's inner face, along this cell's own insertion vector, so the plate's upper part slides along the lower wall's end on its way in
    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        if (_cells.masks[cell] != -2)
            continue;

        const Plane& face = _face_planes[cell];
        const Plane lower = stepped(sides[cell][7].plane, guide.plate_thickness, face.origin(), true);
        const Plane upper = stepped(sides[cell][5].plane, guide.plate_thickness, face.origin(), true);
        const Plane& across = sides[cell][6].plane;
        Point on_lower;
        Point on_upper;
        const std::optional<Line> step = Intersection::plane_plane(across, lower);

        if (!step || !Intersection::plane_plane_plane(across, lower, face, on_lower) ||
            !Intersection::plane_plane_plane(across, upper, face, on_upper))
            continue;

        const double out = (on_upper - on_lower).dot(lower.z_axis());
        const double inside = (face.origin() - on_lower).dot(lower.z_axis());

        if (out * inside >= 0.0)
            continue;

        const Vector along = direction_of(sides[cell], face, _cells.masks[cell]);
        sides[cell][6].plane = Plane(step->start(), along, step->to_vector());
    }

    return sides;
}

// ═══════════════════════════════════════════════════════════════════════════
// Plates
// ═══════════════════════════════════════════════════════════════════════════
std::vector<int> ChevronShifted::step_flags(int mask) {

    if (mask == 2 || mask == 20 || mask == -2)
        return {-1, 0, 0, 0, 0, -1, 0, -1};

    if (mask == -1)
        return {0, 0, 0, -1};

    return {-1, 0, 0, -1};
}

std::vector<std::pair<int, bool>> ChevronShifted::wall_sides(int mask) {

    if (mask == 2 || mask == 20)
        return {{0, false}, {1, true}, {3, true}, {5, false}, {7, false}};

    if (mask == 3)
        return {{2, true}, {0, false}, {1, true}, {3, false}};

    if (mask == -1)
        return {{2, true}};

    if (mask == -2)
        return {{4, true}};

    return {};
}

Plane
ChevronShifted::stepped(const Plane& plane, double distance, const Point& centre, bool inward) {

    const Plane in = plane.translate_by_normal(-distance);
    const Plane out = plane.translate_by_normal(distance);
    const double to_in = Point::distance(centre, in.origin());
    const double to_out = Point::distance(centre, out.origin());

    if (inward)
        return to_in > to_out ? out : in;

    return to_in < to_out ? out : in;
}

std::optional<Plane>
ChevronShifted::mitre(const Plane& a, const Plane& b, const Point& centre) {

    const std::optional<Line> seam = Intersection::plane_plane(a, b);

    if (!seam)
        return std::nullopt;

    Vector away_a = a.z_axis();
    Vector away_b = b.z_axis();

    if (away_a.dot(centre - a.origin()) > 0.0)
        away_a = -away_a;

    if (away_b.dot(centre - b.origin()) > 0.0)
        away_b = -away_b;

    Vector normal = away_a - away_b;

    if (!normal.normalize_self())
        return std::nullopt;

    return Plane::from_point_normal(seam->start(), normal);
}

std::vector<int>
ChevronShifted::compact(Polyline& first, Polyline& second, const std::vector<int>& edges) {

    const std::vector<Point> corners = first.get_points();
    const std::vector<Point> others = second.get_points();
    const size_t count = edges.size();
    std::vector<size_t> kept;

    for (size_t k = 0; k < count; k++)
        if (Point::distance(corners[k], corners[(k + count - 1) % count]) > 0.01)
            kept.push_back(k);

    std::vector<Point> points;
    std::vector<Point> other_points;
    std::vector<int> sides;

    for (size_t s = 0; s < kept.size(); s++) {
        points.push_back(corners[kept[s]]);
        other_points.push_back(others[kept[s]]);
        sides.push_back(edges[kept[(s + 1) % kept.size()]]);
    }

    points.push_back(points.front());
    other_points.push_back(other_points.front());
    first = Polyline(points);
    second = Polyline(other_points);
    return sides;
}

wood_chevron::ShiftedPlates ChevronShifted::compute_plates() const {

    wood_chevron::ShiftedPlates plates;
    const double half = guide.plate_thickness * 0.5;

    // where a wall stands inside a cell: a cell's own inward sides and its neighbours' outward ones, so a wall of another cell ending on that edge stops a wall short of it
    std::set<std::pair<std::pair<size_t, size_t>, int>> occupied;

    for (size_t cell = 0; cell < _cells.masks.size(); cell++)
        for (const std::pair<int, bool>& wall : wall_sides(_cells.masks[cell])) {
            const std::pair<size_t, size_t> key =
                edge_key(static_cast<int>(cell), _sides[cell][wall.first].edge);

            for (const int on_edge : _cells.faces.edge_faces.at(key))
                if ((on_edge == static_cast<int>(cell)) != wall.second)
                    occupied.insert({key, on_edge});
        }

    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];
        const Plane& face = _face_planes[cell];
        const std::vector<wood_chevron::ShiftedSide>& sides = _sides[cell];
        const std::vector<int> flags = step_flags(mask);
        std::array<int, 2> box = {0, 0};

        // top and bottom plate: the face plane moved by the offset, its outline stepped in by a wall on the flagged sides
        for (const double sign : {1.0, -1.0}) {
            const Plane near = face.translate_by_normal(sign * guide.plate_offset - half);
            const Plane far = face.translate_by_normal(sign * guide.plate_offset + half);
            std::vector<Plane> loop;
            std::vector<int> edges;

            for (size_t k = 0; k < sides.size(); k++) {
                loop.push_back(
                    flags[k] == 0
                        ? sides[k].plane
                        : stepped(sides[k].plane, guide.plate_thickness, near.origin(), flags[k] < 0)
                );
                edges.push_back(sides[k].edge);
            }

            Polyline first = Polyline::from_planes(loop, near);
            Polyline second = Polyline::from_planes(loop, far);
            const std::vector<int> side_edges = compact(first, second, edges);
            box[sign > 0.0 ? 0 : 1] = static_cast<int>(plates.plates.size());
            plates.plates.push_back(
                std::make_shared<FacePlanarElement>(
                    first, second, fmt::format("plate_{}", plates.plates.size())
                )
            );
            plates.edges.push_back(side_edges);
        }

        plates.boxes.push_back(box);

        // wall runs: the cell's wall sides in order, the next side taken into the run when only cross planes lie between and it stands on the same plane the same way, so the halves of a merged long side make one wall
        std::vector<wood_chevron::ShiftedWallRun> runs;

        for (const std::pair<int, bool>& wall : wall_sides(mask)) {
            bool merged = !runs.empty() && runs.back().last < wall.first &&
                          runs.back().outward == wall.second;

            for (int k = merged ? runs.back().last + 1 : wall.first; k < wall.first; k++)
                merged = merged && sides[k].edge < 0;

            if (merged) {
                const Plane& a = sides[runs.back().first].plane;
                const Plane& b = sides[wall.first].plane;
                merged = Point::distance(a.origin(), b.origin()) < 1e-6 &&
                         a.z_axis().dot(b.z_axis()) > 1.0 - 1e-9;
            }

            if (merged)
                runs.back().last = wall.first;
            else
                runs.push_back({wall.first, wall.first, wall.second});
        }

        // walls: each run between the outer box faces, a wall thick inward or outward from its plane, ended on the mitre between the outer faces where it meets another wall of the box, a wall short of a side where a neighbour's wall stands inside the cell, else on the side's plane
        const Plane top = face.translate_by_normal(guide.plate_offset + half + guide.wall_extend);
        const Plane bottom = face.translate_by_normal(-guide.plate_offset - half - guide.wall_extend);
        const int count = static_cast<int>(sides.size());
        const int base = static_cast<int>(plates.plates.size());
        std::vector<Plane> outers;

        for (const wood_chevron::ShiftedWallRun& run : runs)
            outers.push_back(
                run.outward ? stepped(sides[run.first].plane, guide.plate_thickness, top.origin(), false)
                            : sides[run.first].plane
            );

        // the bounds: per run the plane after it and the one before it; a corner's mitre is made once, between the two walls' outer faces, so both walls end on the same plane; two walls too nearly parallel for a mitre end on each other's side planes and get no corner
        std::vector<std::array<Plane, 2>> bounds(runs.size());

        for (size_t r = 0; r < runs.size(); r++)
            for (int end = 0; end < 2; end++) {
                const int side =
                    end == 0 ? (runs[r].last + 1) % count : (runs[r].first + count - 1) % count;
                bounds[r][end] = sides[side].plane;

                if (sides[side].edge >= 0 &&
                    occupied.count(
                        {edge_key(static_cast<int>(cell), sides[side].edge), static_cast<int>(cell)}
                    ))
                    bounds[r][end] =
                        stepped(sides[side].plane, guide.plate_thickness, top.origin(), true);
            }

        for (size_t r = 0; r < runs.size(); r++)
            for (size_t q = 0; q < runs.size(); q++) {
                if (runs[q].first != (runs[r].last + 1) % count)
                    continue;

                const std::optional<Plane> corner = mitre(outers[r], outers[q], top.origin());

                if (!corner)
                    continue;

                bounds[r][0] = *corner;
                bounds[q][1] = *corner;
                plates.corners.push_back(
                    {base + static_cast<int>(r), base + static_cast<int>(q), static_cast<int>(cell)}
                );
            }

        for (size_t r = 0; r < runs.size(); r++) {
            const Plane& plane = sides[runs[r].first].plane;
            const Plane moved = stepped(plane, guide.plate_thickness, top.origin(), !runs[r].outward);
            std::vector<std::pair<size_t, size_t>> keys;

            for (int k = runs[r].first; k <= runs[r].last; k++)
                if (sides[k].edge >= 0)
                    keys.push_back(edge_key(static_cast<int>(cell), sides[k].edge));

            const std::vector<Plane> loop = {top, bounds[r][0], bottom, bounds[r][1]};
            plates.walls.push_back(
                {static_cast<int>(plates.plates.size()), static_cast<int>(cell)}
            );
            plates.wall_edges.push_back(keys);
            if (runs[r].outward)
                plates.outward.insert(static_cast<int>(plates.plates.size()));
            plates.plates.push_back(
                std::make_shared<FacePlanarElement>(
                    Polyline::from_planes(loop, plane),
                    Polyline::from_planes(loop, moved),
                    fmt::format("plate_{}", plates.plates.size())
                )
            );
            plates.edges.push_back({-1, -1, -1, -1});
        }
    }

    return plates;
}

// ═══════════════════════════════════════════════════════════════════════════
// Joinery
// ═══════════════════════════════════════════════════════════════════════════
std::array<int, 2> ChevronShifted::insertion_edges(int mask) {

    if (mask == 2 || mask == 20)
        return {5, 0};

    if (mask == -2)
        return {3, 4};

    if (mask == 3)
        return {3, 0};

    return {2, 3};
}

Vector ChevronShifted::direction_of(
    const std::vector<wood_chevron::ShiftedSide>& sides, const Plane& face, int mask
) {

    std::vector<Plane> loop;

    for (const wood_chevron::ShiftedSide& side : sides)
        loop.push_back(side.plane);

    const std::vector<Point> corners = Polyline::from_planes(loop, face).get_points();
    std::array<Vector, 2> segments;

    for (size_t which = 0; which < 2; which++) {
        size_t k = 0;

        while (sides[k].edge != insertion_edges(mask)[which])
            k++;

        segments[which] = (corners[k] - corners[(k + loop.size() - 1) % loop.size()]).normalized();
    }

    return segments[0] - segments[1];
}

std::vector<int> ChevronShifted::owned_edges(int mask) {

    if (mask == 2 || mask == 20)
        return {5, 0, 4};

    if (mask == -2)
        return {3, 4, 5};

    if (mask == 3)
        return {3, 0};

    return {2, 3};
}

bool ChevronShifted::tenon_side(int mask, int edge) {

    if (mask == -1)
        return edge == 1 || edge == 2;

    return edge >= 0;
}

std::vector<Line>
ChevronShifted::compute_joinery(const wood_chevron::ShiftedPlates& plates) {

    _insertion_vectors.assign(plates.plates.size(), {});
    _feature_axes.assign(plates.plates.size(), {});
    _joints_per_face.assign(plates.plates.size(), {});
    _divisions_per_face.assign(plates.plates.size(), {});
    std::map<std::pair<size_t, size_t>, Vector> directions;
    std::map<std::pair<size_t, size_t>, Vector> axes;
    std::vector<Vector> cell_directions;
    std::vector<Vector> cell_axes;
    std::vector<Line> lines;

    // per cell its direction, the bisector of two of its own sides on its outline, and its axis, the mean of those sides' surface normals, which the definition's cut planes follow, both on the edges whose walls it slides into
    for (size_t cell = 0; cell < _cells.masks.size(); cell++) {
        const int mask = _cells.masks[cell];
        const std::vector<wood_chevron::ShiftedSide>& sides = _sides[cell];
        Vector axis(0.0, 0.0, 0.0);

        for (size_t which = 0; which < 2; which++) {
            size_t k = 0;

            while (sides[k].edge != insertion_edges(mask)[which])
                k++;

            axis = axis + sides[k].plane.y_axis() * 0.5;
        }

        const Vector direction = direction_of(sides, _face_planes[cell], mask);
        cell_directions.push_back(direction);
        cell_axes.push_back(axis);

        for (const int edge : owned_edges(mask)) {
            directions[edge_key(static_cast<int>(cell), edge)] = direction;
            axes[edge_key(static_cast<int>(cell), edge)] = axis;
        }

        const Point centre = _face_planes[cell].translate_by_normal(guide.plate_offset).origin();
        lines.push_back(Line::from_points(centre, centre + direction * 300.0));
    }

    // a cell's own walls: its plates enter them along its own direction, so its box closes along one vector; the plates across an
    // outward wall still along the direction of the cell that slides into it
    std::map<std::pair<int, std::pair<size_t, size_t>>, std::pair<Vector, Vector>> own;

    if (guide.closed_boxes)
        for (size_t w = 0; w < plates.walls.size(); w++)
            for (const std::pair<size_t, size_t>& key : plates.wall_edges[w])
                own[{plates.walls[w][1], key}] = {cell_directions[plates.walls[w][1]], cell_axes[plates.walls[w][1]]};

    // top and bottom plates: the tenon type on every side the definition's outer loop moved out by a wall, with the direction and axis of its edge where the edge has them; the first quad's side under its neighbour's wall, which the definition left degenerate, gets neither
    for (size_t cell = 0; cell < _cells.masks.size(); cell++)
        for (const int plate : plates.boxes[cell]) {
            const std::vector<int>& edges = plates.edges[plate];
            std::vector<Vector> vectors(2 + edges.size(), Vector(0.0, 0.0, 0.0));
            std::vector<Vector> plate_axes(2 + edges.size(), Vector(0.0, 0.0, 0.0));
            std::vector<int> types(2 + edges.size(), 0);

            for (size_t k = 0; k < edges.size(); k++) {
                if (!tenon_side(_cells.masks[cell], edges[k]))
                    continue;

                types[2 + k] = 23;
                const std::map<std::pair<size_t, size_t>, Vector>::const_iterator found =
                    directions.find(edge_key(static_cast<int>(cell), edges[k]));

                const std::map<std::pair<int, std::pair<size_t, size_t>>, std::pair<Vector, Vector>>::const_iterator mine =
                    own.find({static_cast<int>(cell), edge_key(static_cast<int>(cell), edges[k])});

                if (mine != own.end()) {
                    vectors[2 + k] = mine->second.first;
                    plate_axes[2 + k] = mine->second.second;
                    continue;
                }

                if (found == directions.end())
                    continue;

                vectors[2 + k] = found->second;
                plate_axes[2 + k] = axes.at(found->first);
            }

            _insertion_vectors[plate] = vectors;
            _feature_axes[plate] = plate_axes;
            _joints_per_face[plate] = types;
            _divisions_per_face[plate] = std::vector<int>(2 + edges.size(), guide.divisions);
        }

    // walls: every wall first in its pairs with the top and bottom plates whose side on one of its edges has a direction, edge by edge, so a plate with a side on both halves of a merged long side is paired twice and gets a joint per half, its own cell first, so the solver meets the wall's face before a plate's kink side meets the wall's end
    for (size_t w = 0; w < plates.walls.size(); w++) {
        const int wall = plates.walls[w][0];
        _insertion_vectors[wall] = std::vector<Vector>(6, Vector(0.0, 0.0, 0.0));
        _feature_axes[wall] = std::vector<Vector>(6, Vector(0.0, 0.0, 0.0));
        _joints_per_face[wall] = std::vector<int>(6, 0);
        _divisions_per_face[wall] = std::vector<int>(6, 0);

        for (const std::pair<size_t, size_t>& key : plates.wall_edges[w]) {
            std::vector<int> cells = _cells.faces.edge_faces.at(key);
            std::rotate(
                cells.begin(),
                std::find(cells.begin(), cells.end(), plates.walls[w][1]),
                cells.end()
            );

            for (const int cell : cells)
                for (const int plate : plates.boxes[cell]) {
                    const std::vector<int>& edges = plates.edges[plate];

                    for (size_t k = 0; k < edges.size(); k++)
                        if (edges[k] >= 0 && edge_key(cell, edges[k]) == key &&
                            _insertion_vectors[plate][2 + k].magnitude() > 0.0)
                            _adjacency.emplace_back(wall, plate);
                }

            if (cells.size() == 2) {
                _three_valence.push_back(
                    {plates.boxes[cells[0]][0], wall, plates.boxes[cells[1]][0], wall}
                );
                _three_valence.push_back(
                    {plates.boxes[cells[0]][1], wall, plates.boxes[cells[1]][1], wall}
                );
            }
        }
    }

    // corners: the Annen fingers on the two mitre faces of every box corner, the wall ending on the mitre the axis wall with 17 on its end face, the wall starting there 10 on its start face, both inserted along the box's up, cut in finger_divisions parts
    for (const std::array<int, 3>& corner : plates.corners) {

        // an outward wall goes on last, along the cell's own direction: its corners are plain mitres
        if (guide.closed_boxes && (plates.outward.count(corner[0]) || plates.outward.count(corner[1])))
            continue;

        const Vector up = -_face_planes[corner[2]].z_axis();
        _joints_per_face[corner[0]][2] = 17;
        _joints_per_face[corner[1]][4] = 10;
        _divisions_per_face[corner[0]][2] = guide.finger_divisions;
        _divisions_per_face[corner[1]][4] = guide.finger_divisions;
        _insertion_vectors[corner[0]][2] = up;
        _insertion_vectors[corner[1]][4] = up;
        _adjacency.emplace_back(corner[0], corner[1]);
    }

    return lines;
}
