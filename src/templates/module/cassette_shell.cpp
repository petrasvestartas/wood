#include "src/templates/module/cassette_shell.h"
#include "src/templates/cross/vda_mesh.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CassetteShell::CassetteShell(
    const Mesh& mesh,
    double depth,
    double plate_thickness,
    double beam_width,
    const std::vector<std::vector<Opening>>& openings,
    double stud_spacing,
    const std::vector<int>& ranks,
    const std::string& name
)
    : WoodSession(name), mesh(mesh.weld(0.01)), depth(depth), plate_thickness(plate_thickness),
      beam_width(beam_width), openings(openings), stud_spacing(stud_spacing), ranks(ranks) {

    // mesh: the welded input mesh, a plane per face and the face across every face edge
    add_mesh(this->mesh, group_named("mesh"));
    const std::vector<Plane> face_planes = compute_face_planes();
    _face_planes = face_planes;
    const std::vector<Plane> face_frames = compute_face_frames();
    _face_frames = face_frames;
    const std::vector<std::vector<long long>> neighbours = compute_neighbours();
    _neighbours = neighbours;

    // edge planes: per face edge the plane its cassette ends on, a miter or a T, its normal out of the face
    const std::vector<std::vector<Plane>> edge_planes = compute_edge_planes();
    _edge_planes = edge_planes;

    // miter planes: per face corner the plane halfway between its two edge planes, where the beams miter
    const std::vector<std::vector<Plane>> miter_planes = compute_miter_planes();
    _miter_planes = miter_planes;

    // plates: per cassette a top and bottom plate over the whole face cut by the edge planes, its openings cut out
    add_plates();

    // edge beams: per face edge a beam between the plates, outer face on the edge plane, ends on the miter planes
    add_edge_beams();

    // opening frames: per window the header and the sill between its jamb studs
    add_opening_frames();

    // studs: the jamb studs of every window, and a stud on the grid of the stud spacing that no post takes, cripples in the windows
    add_studs();

    // contacts: plates on beams and beams on beams inside every cassette, beam on beam between every two cassettes, those drawn
    const CassetteContacts contacts = add_contacts();
    const std::shared_ptr<TreeNode> contact_group = group_named("contacts");

    for (const Contact& contact : contacts.between) {
        Mesh patch = Mesh::from_polylines(std::vector<Polyline>{contact.face->polygon});
        patch.set_objectcolor(Color::red());
        add_mesh(patch, contact_group);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Default mesh
// ═══════════════════════════════════════════════════════════════════════════
Mesh CassetteShell::default_mesh() {
    return wood_cross::VdaMesh::default_mesh();
}

Mesh CassetteShell::house_mesh(double length, double width, double height, double ridge) {

    const double door = length / 6.0;
    const double head = height * 7.0 / 9.0;

    const std::vector<std::vector<Point>> faces = {
        {
            Point(0.0, 0.0, 0.0),
            Point(door, 0.0, 0.0),
            Point(door, 0.0, head),
            Point(2.0 * door, 0.0, head),
            Point(2.0 * door, 0.0, 0.0),
            Point(length, 0.0, 0.0),
            Point(length, 0.0, height),
            Point(0.0, 0.0, height),
        },
        {Point(length, 0.0, 0.0),
         Point(length, width, 0.0),
         Point(length, width, height),
         Point(length, 0.5 * width, ridge),
         Point(length, 0.0, height)},
        {Point(length, width, 0.0),
         Point(0.0, width, 0.0),
         Point(0.0, width, height),
         Point(length, width, height)},
        {Point(0.0, width, 0.0),
         Point(0.0, 0.0, 0.0),
         Point(0.0, 0.0, height),
         Point(0.0, 0.5 * width, ridge),
         Point(0.0, width, height)},
        {Point(0.0, 0.0, height),
         Point(length, 0.0, height),
         Point(length, 0.5 * width, ridge),
         Point(0.0, 0.5 * width, ridge)},
        {Point(length, width, height),
         Point(0.0, width, height),
         Point(0.0, 0.5 * width, ridge),
         Point(length, 0.5 * width, ridge)},
    };

    return Mesh::from_polylines(faces, 1.0);
}

std::vector<int> CassetteShell::house_ranks() {
    return {2, 4, 3, 4, 0, 1};
}

std::vector<std::vector<Opening>> CassetteShell::house_windows() {

    return {
        {{4000.0, 1000.0, 900.0, 2100.0}},
        {{1500.0, 1000.0, 900.0, 2100.0}},
        {{2000.0, 2000.0, 900.0, 2100.0}},
        {{1500.0, 1000.0, 900.0, 2100.0}},
        {},
        {},
    };
}

// ═══════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════
const std::vector<std::vector<Plane>>& CassetteShell::edge_planes() const {
    return _edge_planes;
}

const std::vector<std::vector<Plane>>& CassetteShell::miter_planes() const {
    return _miter_planes;
}

// ═══════════════════════════════════════════════════════════════════════════
// Planes
// ═══════════════════════════════════════════════════════════════════════════
std::vector<Plane> CassetteShell::compute_face_planes() const {

    std::vector<Plane> planes;

    for (const size_t face : mesh.faces()) {
        const Point centroid = mesh.face_centroid(face).value();
        const Vector normal = mesh.face_normal(face).value();
        planes.push_back(Plane::from_point_normal(centroid, normal));
    }

    return planes;
}

std::vector<Plane> CassetteShell::compute_face_frames() const {

    const std::vector<size_t> faces = mesh.faces();
    std::vector<Plane> frames;

    for (size_t f = 0; f < faces.size(); f++) {
        const std::pair<size_t, size_t> first = mesh.face_edges(faces[f]).value()[0];
        const Point origin = mesh.vertex_point(first.first).value();
        const Vector x = (mesh.vertex_point(first.second).value() - origin).normalized();
        const Vector y = _face_planes[f].z_axis().cross(x).normalized();
        frames.push_back(Plane(origin, x, y));
    }

    return frames;
}

std::vector<std::vector<long long>> CassetteShell::compute_neighbours() const {

    const std::vector<size_t> faces = mesh.faces();
    const std::map<std::pair<size_t, size_t>, size_t> halfedge_faces = mesh.edge_face_map();
    std::map<size_t, size_t> face_index;

    for (size_t f = 0; f < faces.size(); f++)
        face_index[faces[f]] = f;

    std::vector<std::vector<long long>> neighbours(faces.size());

    for (size_t f = 0; f < faces.size(); f++)
        for (const std::pair<size_t, size_t>& edge : mesh.face_edges(faces[f]).value()) {
            const std::pair<size_t, size_t> twin = {edge.second, edge.first};
            const bool interior = halfedge_faces.count(twin) > 0;
            neighbours[f].push_back(
                interior ? static_cast<long long>(face_index.at(halfedge_faces.at(twin))) : -1
            );
        }

    return neighbours;
}

std::vector<std::vector<Plane>> CassetteShell::compute_edge_planes() const {

    const std::vector<size_t> faces = mesh.faces();
    std::vector<std::vector<Plane>> planes(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<std::pair<size_t, size_t>> face_edges = mesh.face_edges(faces[f]).value();
        const Plane& face = _face_planes[f];
        std::vector<Point> loop;

        for (const std::pair<size_t, size_t>& edge : face_edges)
            loop.push_back(mesh.vertex_point(edge.first).value());

        loop.push_back(loop[0]);

        // winding: +1 when the corners run counter-clockwise about the face normal, so an edge crossed with the normal points out,
        Point loop_origin;
        Plane loop_plane;
        Polyline(loop).get_fast_plane(loop_origin, loop_plane);
        const double winding = loop_plane.z_axis().dot(face.z_axis()) > 0.0 ? 1.0 : -1.0;

        for (size_t j = 0; j < face_edges.size(); j++) {
            const Point start = mesh.vertex_point(face_edges[j].first).value();
            const Point end = mesh.vertex_point(face_edges[j].second).value();
            const Point middle = Point::mid_point(start, end);
            const long long g = _neighbours[f][j];

            // naked edge: square to the face
            Vector out = (end - start).cross(face.z_axis()) * winding;
            out.normalize_self();

            if (g < 0) {
                planes[f].push_back(Plane::from_point_normal(middle, out));
                continue;
            }

            const Plane& other = _face_planes[static_cast<size_t>(g)];

            // miter: the plane through the edge leaning on the average normal of the two faces
            if (ranks.empty()) {
                Vector lean = (end - start).cross(face.z_axis() + other.z_axis()) * winding;
                lean.normalize_self();

                planes[f].push_back(Plane::from_point_normal(middle, lean));
                continue;
            }

            const bool through =
                ranks[f] < ranks[static_cast<size_t>(g)] ||
                (ranks[f] == ranks[static_cast<size_t>(g)] && static_cast<long long>(f) < g);

            // T: this cassette is cut on the near side of the neighbour where it stops, or ends flush with its far side where it runs through
            const double side =
                (face.origin() - other.origin()).dot(other.z_axis()) > 0.0 ? 1.0 : -1.0;
            const double reach = through ? -side : side;
            const Point surface = other.origin() + other.z_axis() * (reach * 0.5 * depth);
            planes[f].push_back(Plane::from_point_normal(surface, other.z_axis() * -side));
        }
    }

    return planes;
}

std::vector<std::vector<Plane>> CassetteShell::compute_miter_planes() const {

    const std::vector<size_t> faces = mesh.faces();
    std::vector<std::vector<Plane>> planes(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<std::pair<size_t, size_t>> face_edges = mesh.face_edges(faces[f]).value();
        const size_t n = face_edges.size();

        for (size_t j = 0; j < n; j++) {
            const Point start = mesh.vertex_point(face_edges[j].first).value();
            const Point corner = mesh.vertex_point(face_edges[j].second).value();
            const Point end = mesh.vertex_point(face_edges[(j + 1) % n].second).value();
            const Vector current = _edge_planes[f][j].z_axis();
            const Vector next = _edge_planes[f][(j + 1) % n].z_axis();
            const Vector halfway = current - next;
            const double turn = (corner - start).angle(end - corner, false, true);

            if (turn < 5.7)
                planes[f].push_back(Plane::from_point_normal(corner, corner - start));
            else
                planes[f].push_back(Plane::from_point_normal(corner, halfway));
        }
    }

    return planes;
}

// ═══════════════════════════════════════════════════════════════════════════
// Members
// ═══════════════════════════════════════════════════════════════════════════
void CassetteShell::add_plates() {

    const double outer = 0.5 * depth;
    const double inner = 0.5 * depth - plate_thickness;

    for (size_t f = 0; f < _face_planes.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));
        const Plane& face = _face_planes[f];
        std::vector<Plane> sides;

        // an edge plane parallel to the one before it continues that side: no corner between them
        for (const Plane& plane : _edge_planes[f])
            if (sides.empty() || plane.z_axis().angle(sides.back().z_axis(), false, true) > 0.57)
                sides.push_back(plane);

        if (sides.size() > 1 &&
            sides.front().z_axis().angle(sides.back().z_axis(), false, true) <= 0.57)
            sides.pop_back();

        const std::array<std::array<double, 2>, 2> levels = {{{inner, outer}, {-outer, -inner}}};
        const std::array<std::string, 2> names = {
            fmt::format("top_plate_{}", f), fmt::format("bottom_plate_{}", f)
        };

        for (size_t side = 0; side < 2; side++) {
            const Polyline lower =
                Polyline::from_planes(sides, face.translate_by_normal(levels[side][0]));
            const Polyline upper =
                Polyline::from_planes(sides, face.translate_by_normal(levels[side][1]));
            const std::shared_ptr<Plate> plate = std::make_shared<Plate>(lower, upper, names[side]);

            // openings: a hole through both faces of the plate from the sill to the head
            if (f < openings.size() && !openings[f].empty()) {
                Features features;
                features.bottom.push_back(plate->polylines[0]);
                features.top.push_back(plate->polylines[1]);

                for (const Opening& opening : openings[f]) {
                    const double height = opening.head - opening.sill;
                    features.bottom.push_back(face_rectangle(
                        f, levels[side][0], opening.x, opening.sill, opening.width, height
                    ));
                    features.top.push_back(face_rectangle(
                        f, levels[side][1], opening.x, opening.sill, opening.width, height
                    ));
                }

                plate->features = features;
                plate->invalidate_geometry();
            }

            add(plate, group);
        }
    }
}

void CassetteShell::add_edge_beams() {

    const double level = 0.5 * depth - plate_thickness;

    for (size_t f = 0; f < _face_planes.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));
        const Plane bottom = _face_planes[f].translate_by_normal(-level);
        const Plane top = _face_planes[f].translate_by_normal(level);
        const size_t n = _edge_planes[f].size();
        const std::vector<std::pair<size_t, size_t>> face_edges =
            mesh.face_edges(mesh.faces()[f]).value();
        std::vector<double> steepness;

        for (const std::pair<size_t, size_t>& edge : face_edges)
            steepness.push_back(std::abs((mesh.vertex_point(edge.second).value() -
                                          mesh.vertex_point(edge.first).value())
                                             .normalized()
                                             .dot(Vector(0.0, 0.0, 1.0))));

        for (size_t j = 0; j < n; j++) {
            const Plane& outer = _edge_planes[f][j];
            const Plane inner = outer.translate_by_normal(-beam_width);
            const size_t next = (j + 1) % n;
            const size_t previous = (j + n - 1) % n;
            Plane end_plane = _miter_planes[f][j];
            Plane start_plane = _miter_planes[f][previous];

            // butted: the more level beam runs through to the other's outer face, the steeper stops on its inner face
            if (!ranks.empty()) {
                const bool over_next =
                    steepness[j] < steepness[next] - 1e-6 ||
                    (std::abs(steepness[j] - steepness[next]) <= 1e-6 && j < next);
                const bool over_previous =
                    steepness[j] < steepness[previous] - 1e-6 ||
                    (std::abs(steepness[j] - steepness[previous]) <= 1e-6 && j < previous);
                end_plane = over_next ? _edge_planes[f][next]
                                      : _edge_planes[f][next].translate_by_normal(-beam_width);
                start_plane = over_previous
                                  ? _edge_planes[f][previous]
                                  : _edge_planes[f][previous].translate_by_normal(-beam_width);
            }

            const std::vector<Plane> sides = {outer, end_plane, inner, start_plane};
            const Polyline bottom_loop = Polyline::from_planes(sides, bottom);
            const Polyline top_loop = Polyline::from_planes(sides, top);
            add(std::make_shared<Plate>(
                    bottom_loop, top_loop, fmt::format("edge_beam_{}_{}", f, j)
                ),
                group);
        }
    }
}

void CassetteShell::add_opening_frames() {

    const double level = 0.5 * depth - plate_thickness;
    const double w = beam_width;

    for (size_t f = 0; f < openings.size() && f < _face_frames.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));

        for (size_t o = 0; o < openings[f].size(); o++) {
            const Opening& opening = openings[f][o];

            // header: between the jamb studs, over the opening's head
            const Polyline header_bottom =
                face_rectangle(f, -level, opening.x, opening.head, opening.width, w);
            const Polyline header_top =
                face_rectangle(f, level, opening.x, opening.head, opening.width, w);
            add(std::make_shared<Plate>(
                    header_bottom, header_top, fmt::format("header_{}_{}", f, o)
                ),
                group);

            // sill: between the jamb studs under the opening
            const Polyline sill_bottom =
                face_rectangle(f, -level, opening.x, opening.sill - w, opening.width, w);
            const Polyline sill_top =
                face_rectangle(f, level, opening.x, opening.sill - w, opening.width, w);
            add(std::make_shared<Plate>(sill_bottom, sill_top, fmt::format("sill_{}_{}", f, o)),
                group);
        }
    }
}

void CassetteShell::add_studs() {

    if (stud_spacing <= 0.0)
        return;

    const double level = 0.5 * depth - plate_thickness;
    const double w = beam_width;

    for (size_t f = 0; f < _face_frames.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));
        const Plane& frame = _face_frames[f];
        const size_t n = _edge_planes[f].size();
        std::vector<Plane> inner_planes;

        for (const Plane& plane : _edge_planes[f])
            inner_planes.push_back(plane.translate_by_normal(-w));

        const Polyline inner = Polyline::from_planes(inner_planes, _face_planes[f]);
        std::vector<std::array<double, 2>> corners;
        double area = 0.0;
        double length = 0.0;

        for (size_t i = 0; i < n; i++) {
            const Vector offset = inner[i] - frame.origin();
            corners.push_back({offset.dot(frame.x_axis()), offset.dot(frame.y_axis())});
            length = std::max(length, corners.back()[0]);
        }

        for (size_t i = 0; i < n; i++)
            area +=
                corners[i][0] * corners[(i + 1) % n][1] - corners[(i + 1) % n][0] * corners[i][1];

        // posts: the edge beams that run up the face, each on the outer side of its inner face, and the jamb studs of every window
        const std::vector<Opening> face_openings =
            f < openings.size() ? openings[f] : std::vector<Opening>();
        std::vector<std::array<double, 2>> posts;

        for (size_t i = 0; i < n; i++) {
            const std::array<double, 2>& a = corners[(i + n - 1) % n];
            const std::array<double, 2>& b = corners[i];

            if (std::abs(b[0] - a[0]) > 1.0 || std::abs(b[1] - a[1]) < w)
                continue;

            const bool outside_right = (b[1] > a[1]) == (area > 0.0);
            posts.push_back(
                outside_right ? std::array<double, 2>{a[0], a[0] + w}
                              : std::array<double, 2>{a[0] - w, a[0]}
            );
        }

        for (const Opening& opening : face_openings) {
            posts.push_back({opening.x - w, opening.x});
            posts.push_back({opening.x + opening.width, opening.x + opening.width + w});
        }

        // uprights: the jamb studs either side of every window, then a stud on every multiple of the stud spacing from the face's first
        std::vector<double> centres;
        std::vector<std::string> names;
        std::vector<bool> jamb;

        for (size_t o = 0; o < face_openings.size(); o++) {
            const Opening& opening = face_openings[o];
            const std::array<double, 2> places = {
                opening.x - 0.5 * w, opening.x + opening.width + 0.5 * w
            };

            for (size_t side = 0; side < 2; side++) {
                centres.push_back(places[side]);
                names.push_back(fmt::format("jamb_stud_{}_{}_{}", f, o, side));
                jamb.push_back(true);
            }
        }

        size_t count = 0;

        for (double u = stud_spacing; u < length; u += stud_spacing) {
            bool taken = false;

            for (const std::array<double, 2>& post : posts)
                if (u + 0.5 * w > post[0] && u - 0.5 * w < post[1])
                    taken = true;

            for (const Opening& opening : face_openings)
                if (u - 0.5 * w >= opening.x && u + 0.5 * w <= opening.x + opening.width)
                    taken = false;

            if (taken)
                continue;

            centres.push_back(u);
            names.push_back(fmt::format("stud_{}_{}", f, count));
            jamb.push_back(false);
            count++;
        }

        for (size_t s = 0; s < centres.size(); s++) {
            const double u = centres[s];
            std::vector<Plane> tops;
            std::vector<Plane> bottoms;
            std::array<size_t, 2> top_sides = {n, n};
            std::array<size_t, 2> bottom_sides = {n, n};

            for (size_t side = 0; side < 2; side++) {
                const double sample = side == 0 ? u - 0.5 * w + 1e-3 : u + 0.5 * w - 1e-3;
                double low = std::numeric_limits<double>::max();
                double high = std::numeric_limits<double>::lowest();

                for (size_t i = 0; i < n; i++) {
                    const std::array<double, 2>& a = corners[(i + n - 1) % n];
                    const std::array<double, 2>& b = corners[i];

                    if (std::abs(b[0] - a[0]) < 1e-9 || (a[0] - sample) * (b[0] - sample) > 0.0)
                        continue;

                    const double v = a[1] + (sample - a[0]) / (b[0] - a[0]) * (b[1] - a[1]);

                    if (v < low) {
                        low = v;
                        bottom_sides[side] = i;
                    }

                    if (v > high) {
                        high = v;
                        top_sides[side] = i;
                    }
                }
            }

            if (top_sides[0] == n || top_sides[1] == n || bottom_sides[0] == n ||
                bottom_sides[1] == n)
                continue;

            tops.push_back(inner_planes[top_sides[0]]);
            bottoms.push_back(inner_planes[bottom_sides[1]]);

            if (top_sides[1] != top_sides[0])
                tops.push_back(inner_planes[top_sides[1]]);

            if (bottom_sides[0] != bottom_sides[1])
                bottoms.push_back(inner_planes[bottom_sides[0]]);

            // runs: the whole height; a stud inside a window over its header and under its sill
            std::vector<std::array<std::vector<Plane>, 2>> runs = {{bottoms, tops}};

            for (const Opening& opening : face_openings) {
                if (jamb[s] || u < opening.x || u > opening.x + opening.width)
                    continue;

                const std::vector<Plane> over = {Plane::from_point_normal(
                    frame.origin() + frame.y_axis() * (opening.head + w), frame.y_axis()
                )};
                const std::vector<Plane> under = {Plane::from_point_normal(
                    frame.origin() + frame.y_axis() * (opening.sill - w), frame.y_axis()
                )};
                const std::array<std::vector<Plane>, 2> above = {over, tops};
                const std::array<std::vector<Plane>, 2> below = {bottoms, under};
                runs = {above, below};
            }

            const Plane left = Plane::from_point_normal(
                frame.origin() + frame.x_axis() * (u - 0.5 * w), frame.x_axis()
            );
            const Plane right = Plane::from_point_normal(
                frame.origin() + frame.x_axis() * (u + 0.5 * w), frame.x_axis()
            );

            for (size_t r = 0; r < runs.size(); r++) {
                std::vector<Plane> sides = {left};
                sides.insert(sides.end(), runs[r][1].begin(), runs[r][1].end());
                sides.push_back(right);
                sides.insert(sides.end(), runs[r][0].begin(), runs[r][0].end());
                const Polyline bottom =
                    Polyline::from_planes(sides, _face_planes[f].translate_by_normal(-level));
                const Polyline top =
                    Polyline::from_planes(sides, _face_planes[f].translate_by_normal(level));
                const std::string name =
                    runs.size() == 1 ? names[s] : fmt::format("{}_{}", names[s], r);
                add(std::make_shared<Plate>(bottom, top, name), group);
            }
        }
    }
}

Polyline CassetteShell::face_rectangle(
    size_t f, double level, double u, double v, double width, double height
) const {

    const Plane& frame = _face_frames[f];
    const Point corner =
        frame.origin() + frame.x_axis() * u + frame.y_axis() * v + frame.z_axis() * level;
    return Polyline::rectangle(corner, frame.x_axis(), frame.y_axis(), width, height);
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════
CassetteContacts CassetteShell::add_contacts() {

    CassetteContacts contacts;

    // parts: every plate and beam of every cassette, by name
    const std::array<std::string, 7> kinds = {
        "top_plate", "bottom_plate", "edge_beam", "stud", "jamb_stud", "header", "sill"
    };
    std::vector<std::vector<std::shared_ptr<Plate>>> parts(_face_planes.size());

    for (size_t f = 0; f < _face_planes.size(); f++) {
        const std::string place = std::to_string(f);

        for (const std::shared_ptr<Element>& element : *objects.elements)
            for (const std::string& kind : kinds) {
                const std::string prefix = kind + "_" + place;
                const bool named = element && element->name.rfind(prefix, 0) == 0;
                const bool whole = named && (element->name.size() == prefix.size() ||
                                             element->name[prefix.size()] == '_');

                if (whole && std::dynamic_pointer_cast<Plate>(element))
                    parts[f].push_back(std::dynamic_pointer_cast<Plate>(element));
            }
    }

    // inside: every two parts of a cassette that touch
    for (size_t f = 0; f < parts.size(); f++)
        for (size_t a = 0; a < parts[f].size(); a++)
            for (size_t b = a + 1; b < parts[f].size(); b++) {
                const std::shared_ptr<InteractionContactFace> face =
                    add_contact(parts[f][a], parts[f][b]);

                if (face)
                    contacts.inside.push_back({parts[f][a], parts[f][b], face});
            }

    // between: every two parts of the two cassettes on an interior edge that touch, once per edge
    for (size_t f = 0; f < parts.size(); f++)
        for (const long long g : _neighbours[f]) {
            if (g <= static_cast<long long>(f))
                continue;

            const size_t found = contacts.between.size();

            for (const std::shared_ptr<Plate>& a : parts[f])
                for (const std::shared_ptr<Plate>& b : parts[static_cast<size_t>(g)]) {
                    const std::shared_ptr<InteractionContactFace> face = add_contact(a, b);

                    if (face)
                        contacts.between.push_back({a, b, face});
                }

            if (contacts.between.size() == found)
                throw std::runtime_error(
                    fmt::format("cassettes {} and {} do not touch across their edge", f, g)
                );
        }

    return contacts;
}

std::shared_ptr<InteractionContactFace>
CassetteShell::add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {

    const std::shared_ptr<InteractionContactFace> face = compute_face_contact(a, b);

    if (!face)
        return nullptr;

    face->name = fmt::format("{}_on_{}", a->name, b->name);
    add_interaction(a, b, face);
    return face;
}

}
