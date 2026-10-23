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
    const std::string& name
)
    : WoodSession(name),
      mesh(mesh.weld(0.01)),
      depth(depth),
      plate_thickness(plate_thickness),
      beam_width(beam_width) {

    // mesh: the welded input mesh, a plane per face and the face across every face edge
    add_mesh(this->mesh, group_named("mesh"));
    const std::vector<Plane> face_planes = compute_face_planes();
    _face_planes = face_planes;
    const std::vector<std::vector<long long>> neighbours = compute_neighbours();
    _neighbours = neighbours;

    // edge planes: per face edge the plane both cells cut their beams on, its normal out of the face
    const std::vector<std::vector<Plane>> edge_planes = compute_edge_planes();
    _edge_planes = edge_planes;

    // miter planes: per face corner the plane halfway between its two edge planes
    const std::vector<std::vector<Plane>> miter_planes = compute_miter_planes();
    _miter_planes = miter_planes;

    // plates: per cassette a thin top and bottom plate cut by the edge planes
    add_plates();

    // edge beams: per face edge a beam between the plates, outer face on the edge plane, ends on the miter planes
    add_edge_beams();

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

Mesh CassetteShell::house_mesh(
    double length,
    double width,
    double height,
    double ridge
) {

    const Vector up(0.0, 0.0, 1.0);
    const std::vector<double> levels = {0.0, height / 3.0, height * 7.0 / 9.0, height};
    const std::vector<double> long_breaks = {0.0, length / 6.0, length / 3.0, length * 2.0 / 3.0, length * 5.0 / 6.0, length};
    const std::vector<double> gable_breaks = {0.0, width * 0.375, width * 0.625, width};

    // the four walls counter-clockwise from above, each along its own direction so its normal points out; a door and a window in
    // the front, a window in each gable and in the back, every opening a grid cell left out
    const std::array<Point, 4> origins = {Point(0.0, 0.0, 0.0), Point(length, 0.0, 0.0), Point(length, width, 0.0), Point(0.0, width, 0.0)};
    const std::array<Vector, 4> alongs = {Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(-1.0, 0.0, 0.0), Vector(0.0, -1.0, 0.0)};
    const std::array<std::vector<double>, 4> breaks = {long_breaks, gable_breaks, long_breaks, gable_breaks};
    const std::array<std::vector<std::array<size_t, 2>>, 4> openings = {{
        {{1, 0}, {1, 1}, {3, 1}},
        {{1, 1}},
        {{2, 1}},
        {{1, 1}},
    }};
    std::vector<std::vector<Point>> faces;

    for (size_t w = 0; w < 4; w++)
        for (size_t i = 0; i + 1 < breaks[w].size(); i++)
            for (size_t k = 0; k + 1 < levels.size(); k++) {
                const std::array<size_t, 2> cell = {i, k};

                if (std::find(openings[w].begin(), openings[w].end(), cell) != openings[w].end())
                    continue;

                const Point a = origins[w] + alongs[w] * breaks[w][i] + up * levels[k];
                const Point b = origins[w] + alongs[w] * breaks[w][i + 1] + up * levels[k];
                faces.push_back({a, b, b + up * (levels[k + 1] - levels[k]), a + up * (levels[k + 1] - levels[k])});
            }

    // gables: a triangle over each end wall from the ridge through the wall's breaks at the eaves, the ridge first so the first three
    // corners are not in line
    for (const size_t w : {1, 3}) {
        std::vector<Point> gable = {origins[w] + alongs[w] * (0.5 * width) + up * ridge};

        for (const double u : breaks[w])
            gable.push_back(origins[w] + alongs[w] * u + up * height);

        faces.push_back(gable);
    }

    // roof: per long wall a slope of quads from its eaves to the ridge, split at the wall's breaks
    for (const size_t w : {0, 2}) {
        const Vector inward = up.cross(alongs[w]) * (0.5 * width) + up * (ridge - height);

        for (size_t i = 0; i + 1 < breaks[w].size(); i++) {
            const Point a = origins[w] + alongs[w] * breaks[w][i] + up * height;
            const Point b = origins[w] + alongs[w] * breaks[w][i + 1] + up * height;
            faces.push_back({a, b, b + inward, a + inward});
        }
    }

    return Mesh::from_polylines(faces, 1.0);
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
            neighbours[f].push_back(interior ? static_cast<long long>(face_index.at(halfedge_faces.at(twin))) : -1);
        }

    return neighbours;
}

std::vector<std::vector<Plane>> CassetteShell::compute_edge_planes() const {

    const std::vector<size_t> faces = mesh.faces();
    std::vector<std::vector<Plane>> planes(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<std::pair<size_t, size_t>> face_edges = mesh.face_edges(faces[f]).value();

        for (size_t j = 0; j < face_edges.size(); j++) {
            const Point start = mesh.vertex_point(face_edges[j].first).value();
            const Point end = mesh.vertex_point(face_edges[j].second).value();
            const Point middle = Point::mid_point(start, end);
            Vector lean = _face_planes[f].z_axis();

            if (_neighbours[f][j] >= 0)
                lean += _face_planes[static_cast<size_t>(_neighbours[f][j])].z_axis();

            // the normal square to the edge and to the average normal, turned out of the face
            Vector out = (end - start).cross(lean);
            out.normalize_self();

            if ((_face_planes[f].origin() - middle).dot(out) > 0.0)
                out = out * -1.0;

            planes[f].push_back(Plane::from_point_normal(middle, out));
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

            // edges within 5.7 degrees of in line: the plane across the edge at the corner; else the plane halfway between the two edge planes
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

        if (sides.size() > 1 && sides.front().z_axis().angle(sides.back().z_axis(), false, true) <= 0.57)
            sides.pop_back();

        const Polyline top_inner = Polyline::from_planes(sides, face.translate_by_normal(inner));
        const Polyline top_outer = Polyline::from_planes(sides, face.translate_by_normal(outer));
        const Polyline bottom_outer = Polyline::from_planes(sides, face.translate_by_normal(-outer));
        const Polyline bottom_inner = Polyline::from_planes(sides, face.translate_by_normal(-inner));
        add(std::make_shared<Plate>(top_inner, top_outer, fmt::format("top_plate_{}", f)), group);
        add(std::make_shared<Plate>(bottom_outer, bottom_inner, fmt::format("bottom_plate_{}", f)), group);
    }
}

void CassetteShell::add_edge_beams() {

    const double level = 0.5 * depth - plate_thickness;

    for (size_t f = 0; f < _face_planes.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));
        const Plane bottom = _face_planes[f].translate_by_normal(-level);
        const Plane top = _face_planes[f].translate_by_normal(level);
        const size_t n = _edge_planes[f].size();

        for (size_t j = 0; j < n; j++) {
            const Plane& outer = _edge_planes[f][j];
            const Plane inner = outer.translate_by_normal(-beam_width);
            const std::vector<Plane> sides = {outer, _miter_planes[f][j], inner, _miter_planes[f][(j + n - 1) % n]};
            const Polyline bottom_loop = Polyline::from_planes(sides, bottom);
            const Polyline top_loop = Polyline::from_planes(sides, top);
            add(std::make_shared<Plate>(bottom_loop, top_loop, fmt::format("edge_beam_{}_{}", f, j)), group);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

CassetteContacts CassetteShell::add_contacts() {

    CassetteContacts contacts;

    // inside: each plate on each edge beam, every beam on the next at their miter
    for (size_t f = 0; f < _face_planes.size(); f++) {
        const size_t n = _edge_planes[f].size();
        const std::shared_ptr<Plate> top = get_element_by_name<Plate>(fmt::format("top_plate_{}", f));
        const std::shared_ptr<Plate> bottom = get_element_by_name<Plate>(fmt::format("bottom_plate_{}", f));

        for (size_t j = 0; j < n; j++) {
            const std::shared_ptr<Plate> beam = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", f, j));
            const std::shared_ptr<Plate> next = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", f, (j + 1) % n));

            for (const std::shared_ptr<Plate>& other : {top, bottom, next}) {
                const std::shared_ptr<InteractionContactFace> face = add_contact(other, beam);

                if (face)
                    contacts.inside.push_back({other, beam, face});
            }
        }
    }

    // between: the beams of the two cassettes on every interior edge, once per edge
    for (size_t f = 0; f < _face_planes.size(); f++)
        for (size_t j = 0; j < _neighbours[f].size(); j++) {
            const long long g = _neighbours[f][j];

            if (g < static_cast<long long>(f))
                continue;

            const std::vector<long long>& across = _neighbours[static_cast<size_t>(g)];
            const size_t i = static_cast<size_t>(std::find(across.begin(), across.end(), static_cast<long long>(f)) - across.begin());
            const std::shared_ptr<Plate> beam = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", f, j));
            const std::shared_ptr<Plate> other = get_element_by_name<Plate>(fmt::format("edge_beam_{}_{}", g, i));
            const std::shared_ptr<InteractionContactFace> face = add_contact(beam, other);

            if (!face)
                throw std::runtime_error(fmt::format("cassettes {} and {} do not touch across their edge", f, g));

            contacts.between.push_back({beam, other, face});
        }

    return contacts;
}

std::shared_ptr<InteractionContactFace> CassetteShell::add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {

    const std::shared_ptr<InteractionContactFace> face = compute_face_contact(a, b);

    if (!face)
        return nullptr;

    face->name = fmt::format("{}_on_{}", a->name, b->name);
    add_interaction(a, b, face);
    return face;
}

}
