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
    const std::string& name
)
    : WoodSession(name),
      mesh(mesh.weld(0.01)),
      depth(depth),
      plate_thickness(plate_thickness),
      beam_width(beam_width),
      openings(openings),
      stud_spacing(stud_spacing) {

    // mesh: the welded input mesh, a plane per face and the face across every face edge
    add_mesh(this->mesh, group_named("mesh"));
    const std::vector<Plane> face_planes = compute_face_planes();
    _face_planes = face_planes;
    const std::vector<Plane> face_frames = compute_face_frames();
    _face_frames = face_frames;
    const std::vector<std::vector<long long>> neighbours = compute_neighbours();
    _neighbours = neighbours;

    // edge planes: per face edge the plane both cells cut their beams on, its normal out of the face
    const std::vector<std::vector<Plane>> edge_planes = compute_edge_planes();
    _edge_planes = edge_planes;

    // miter planes: per face corner the plane halfway between its two edge planes
    const std::vector<std::vector<Plane>> miter_planes = compute_miter_planes();
    _miter_planes = miter_planes;

    // plates: per cassette a top and bottom plate over the whole face cut by the edge planes, its openings cut out
    add_plates();

    // edge beams: per face edge a beam between the plates, outer face on the edge plane, ends on the miter planes
    add_edge_beams();

    // opening frames: per opening two jambs, a header and under a window a sill
    add_opening_frames();

    // studs: at the stud spacing between the edge beams, split by the openings
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

Mesh CassetteShell::house_mesh(
    double length,
    double width,
    double height,
    double ridge
) {

    // the front, the right gable end, the back and the left gable end counter-clockwise from above, each from its foot, then the front
    // and the back roof slope from their eaves
    const std::vector<std::vector<Point>> faces = {
        {Point(0.0, 0.0, 0.0), Point(length, 0.0, 0.0), Point(length, 0.0, height), Point(0.0, 0.0, height)},
        {Point(length, 0.0, 0.0), Point(length, width, 0.0), Point(length, width, height), Point(length, 0.5 * width, ridge), Point(length, 0.0, height)},
        {Point(length, width, 0.0), Point(0.0, width, 0.0), Point(0.0, width, height), Point(length, width, height)},
        {Point(0.0, width, 0.0), Point(0.0, 0.0, 0.0), Point(0.0, 0.0, height), Point(0.0, 0.5 * width, ridge), Point(0.0, width, height)},
        {Point(0.0, 0.0, height), Point(length, 0.0, height), Point(length, 0.5 * width, ridge), Point(0.0, 0.5 * width, ridge)},
        {Point(length, width, height), Point(0.0, width, height), Point(0.0, 0.5 * width, ridge), Point(length, 0.5 * width, ridge)},
    };

    return Mesh::from_polylines(faces, 1.0);
}

std::vector<std::vector<Opening>> CassetteShell::house_openings() {

    return {
        {{1000.0, 1000.0, 0.0, 2100.0}, {4000.0, 1000.0, 900.0, 2100.0}},
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

        const std::array<std::array<double, 2>, 2> levels = {{{inner, outer}, {-outer, -inner}}};
        const std::array<std::string, 2> names = {fmt::format("top_plate_{}", f), fmt::format("bottom_plate_{}", f)};

        for (size_t side = 0; side < 2; side++) {
            const Polyline lower = Polyline::from_planes(sides, face.translate_by_normal(levels[side][0]));
            const Polyline upper = Polyline::from_planes(sides, face.translate_by_normal(levels[side][1]));
            const std::shared_ptr<Plate> plate = std::make_shared<Plate>(lower, upper, names[side]);

            // openings: a hole through both faces of the plate, a door's from the top of the first edge's beam
            if (f < openings.size() && !openings[f].empty()) {
                Features features;
                features.bottom.push_back(plate->polylines[0]);
                features.top.push_back(plate->polylines[1]);

                for (const Opening& opening : openings[f]) {
                    const double foot = opening.sill > 0.0 ? opening.sill : beam_width;
                    features.bottom.push_back(face_rectangle(f, levels[side][0], opening.x, foot, opening.width, opening.head - foot));
                    features.top.push_back(face_rectangle(f, levels[side][1], opening.x, foot, opening.width, opening.head - foot));
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

void CassetteShell::add_opening_frames() {

    const double level = 0.5 * depth - plate_thickness;
    const double w = beam_width;

    for (size_t f = 0; f < openings.size() && f < _face_frames.size(); f++) {
        const std::shared_ptr<TreeNode> group = group_named(fmt::format("cassette_{}", f));

        for (size_t o = 0; o < openings[f].size(); o++) {
            const Opening& opening = openings[f][o];
            const double foot = opening.sill > 0.0 ? opening.sill - w : w;
            const double height = opening.head + w - foot;

            // jambs: either side of the opening from the sill's underside, or a door's foot, to the header's top
            const std::array<double, 2> jambs = {opening.x - w, opening.x + opening.width};

            for (size_t side = 0; side < 2; side++) {
                const Polyline bottom = face_rectangle(f, -level, jambs[side], foot, w, height);
                const Polyline top = face_rectangle(f, level, jambs[side], foot, w, height);
                add(std::make_shared<Plate>(bottom, top, fmt::format("jamb_{}_{}_{}", f, o, side)), group);
            }

            // header: across the opening over its head
            const Polyline header_bottom = face_rectangle(f, -level, opening.x, opening.head, opening.width, w);
            const Polyline header_top = face_rectangle(f, level, opening.x, opening.head, opening.width, w);
            add(std::make_shared<Plate>(header_bottom, header_top, fmt::format("header_{}_{}", f, o)), group);

            // sill: across a window under it
            if (opening.sill > 0.0) {
                const Polyline sill_bottom = face_rectangle(f, -level, opening.x, opening.sill - w, opening.width, w);
                const Polyline sill_top = face_rectangle(f, level, opening.x, opening.sill - w, opening.width, w);
                add(std::make_shared<Plate>(sill_bottom, sill_top, fmt::format("sill_{}_{}", f, o)), group);
            }
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

        // the inner faces of the edge beams in the face frame, corner i where beam i meets beam i + 1, so beam i runs from corner i - 1 to i
        const Polyline inner = Polyline::from_planes(inner_planes, _face_planes[f]);
        std::vector<std::array<double, 2>> corners;

        for (size_t i = 0; i < n; i++) {
            const Vector offset = inner[i] - frame.origin();
            corners.push_back({offset.dot(frame.x_axis()), offset.dot(frame.y_axis())});
        }

        const std::pair<size_t, size_t> first = mesh.face_edges(mesh.faces()[f]).value()[0];
        const double length = (mesh.vertex_point(first.second).value() - frame.origin()).magnitude();
        const std::vector<Opening> face_openings = f < openings.size() ? openings[f] : std::vector<Opening>();
        size_t count = 0;

        for (double u = stud_spacing; u + 0.5 * w < length - w; u += stud_spacing) {
            if (u - 0.5 * w < w)
                continue;

            // the beams the stud's centre line meets lowest and highest, their inner faces its ends
            double low = std::numeric_limits<double>::max();
            double high = std::numeric_limits<double>::lowest();
            size_t low_side = 0;
            size_t high_side = 0;

            for (size_t i = 0; i < n; i++) {
                const std::array<double, 2>& a = corners[(i + n - 1) % n];
                const std::array<double, 2>& b = corners[i];

                if ((a[0] - u) * (b[0] - u) >= 0.0)
                    continue;

                const double v = a[1] + (u - a[0]) / (b[0] - a[0]) * (b[1] - a[1]);

                if (v < low) {
                    low = v;
                    low_side = i;
                }

                if (v > high) {
                    high = v;
                    high_side = i;
                }
            }

            if (low >= high)
                continue;

            // runs: the whole height; around an opening it stands in, above the header and below the sill; none beside the jambs
            std::vector<std::array<Plane, 2>> runs = {{inner_planes[low_side], inner_planes[high_side]}};

            for (const Opening& opening : face_openings) {
                const bool inside = u - 0.5 * w > opening.x && u + 0.5 * w < opening.x + opening.width;
                const bool beside = u + 0.5 * w > opening.x - w && u - 0.5 * w < opening.x + opening.width + w;

                if (!beside)
                    continue;

                runs.clear();

                if (!inside)
                    break;

                const Plane over = Plane::from_point_normal(frame.origin() + frame.y_axis() * (opening.head + w), frame.y_axis());
                runs.push_back({over, inner_planes[high_side]});

                if (opening.sill > 0.0) {
                    const Plane under = Plane::from_point_normal(frame.origin() + frame.y_axis() * (opening.sill - w), frame.y_axis());
                    runs.push_back({inner_planes[low_side], under});
                }

                break;
            }

            const Plane left = Plane::from_point_normal(frame.origin() + frame.x_axis() * (u - 0.5 * w), frame.x_axis());
            const Plane right = Plane::from_point_normal(frame.origin() + frame.x_axis() * (u + 0.5 * w), frame.x_axis());

            for (const std::array<Plane, 2>& run : runs) {
                const std::vector<Plane> sides = {left, run[1], right, run[0]};
                const Polyline bottom = Polyline::from_planes(sides, _face_planes[f].translate_by_normal(-level));
                const Polyline top = Polyline::from_planes(sides, _face_planes[f].translate_by_normal(level));
                add(std::make_shared<Plate>(bottom, top, fmt::format("stud_{}_{}", f, count)), group);
                count++;
            }
        }
    }
}

Polyline CassetteShell::face_rectangle(
    size_t f,
    double level,
    double u,
    double v,
    double width,
    double height
) const {

    const Plane& frame = _face_frames[f];
    const Point corner = frame.origin() + frame.x_axis() * u + frame.y_axis() * v + frame.z_axis() * level;
    return Polyline::rectangle(corner, frame.x_axis(), frame.y_axis(), width, height);
}

// ═══════════════════════════════════════════════════════════════════════════
// Contacts
// ═══════════════════════════════════════════════════════════════════════════

CassetteContacts CassetteShell::add_contacts() {

    CassetteContacts contacts;

    // inside: every two plates and beams of a cassette that touch
    const std::array<std::string, 7> kinds = {"top_plate", "bottom_plate", "edge_beam", "stud", "jamb", "header", "sill"};

    for (size_t f = 0; f < _face_planes.size(); f++) {
        const std::string place = std::to_string(f);
        std::vector<std::shared_ptr<Plate>> parts;

        for (const std::shared_ptr<Element>& element : *objects.elements)
            for (const std::string& kind : kinds) {
                const std::string prefix = kind + "_" + place;
                const bool named = element && element->name.rfind(prefix, 0) == 0;
                const bool whole = named && (element->name.size() == prefix.size() || element->name[prefix.size()] == '_');

                if (whole && std::dynamic_pointer_cast<Plate>(element))
                    parts.push_back(std::dynamic_pointer_cast<Plate>(element));
            }

        for (size_t a = 0; a < parts.size(); a++)
            for (size_t b = a + 1; b < parts.size(); b++) {
                const std::shared_ptr<InteractionContactFace> face = add_contact(parts[a], parts[b]);

                if (face)
                    contacts.inside.push_back({parts[a], parts[b], face});
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
