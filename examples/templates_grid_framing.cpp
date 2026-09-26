#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One bay of the row: its pattern and framing.
struct Case {
    std::string name; // Group name.
    wood_grid::Pattern pattern; // Plan lines.
    wood_grid::Framing framing; // Method, joints and sizes.
};

const double BAY = 6000.0;
const double GAP = 3000.0; // clear distance between the bays
const std::vector<double> ELEVATIONS = {0.0, 4500.0, 9000.0};
const double CLEARANCE = 1.0; // faces closer than this count as touching, not overlapping
const wood_grid::Pattern ONE = wood_grid::Pattern::orthogonal({BAY}, {BAY});
const wood_grid::Framing PURLINS{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.purlin = profile_rectangle(215.0, 456.0)}};
const std::vector<Case> CASES = {
    {"head_section", wood_grid::Pattern::orthogonal({BAY, BAY}, {BAY}), wood_grid::Framing{.system = 1, .span = 0, .node = 0}},
    {"head_conical", ONE, wood_grid::Framing{.system = 0, .node = 0, .reach = 600.0, .capital = 0, .panel = 3000.0}},
    {"head_stepped", ONE, wood_grid::Framing{.system = 0, .node = 0, .reach = 600.0, .capital = 1, .panel = 3000.0}},
    {"node_flush", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1}},
    {"node_through", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 2}},
    {"purlin_flush", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 1, .drop = 0.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}}},
    {"purlin_hung", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 1, .drop = 203.2, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}}},
    {"purlin_stacked", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 1, .drop = 640.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}}},
    {"profile_rectangle", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_round", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_round(360.0), .girder = profile_round(300.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_w", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_w(206.0, 210.0, 14.2, 10.2), .girder = profile_w(250.0, 250.0, 15.0, 10.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_hss", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_hss(178.0, 178.0, 12.7), .girder = profile_hss(250.0, 250.0, 10.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_double", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_double(120.0, 600.0, 60.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_slab_band", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_slab_band(1200.0, 300.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"profile_t", ONE, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_t(300.0, 500.0, 100.0, 80.0), .purlin = profile_rectangle(215.0, 456.0)}}},
}; // heads: the column section where girders run over, conical and stepped under a point supported deck; nodes flush and through; purlins flush, hung and stacked on their girders; the seven profiles
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// The face planes of a closed mesh, normals out.
std::vector<Plane> compute_planes(const Mesh& mesh) {

    const Point centre = mesh.centroid();
    std::vector<Plane> planes;
    for (const size_t face : mesh.faces()) {
        const std::vector<Point> points = mesh.face_polygon(face)->get_points();
        const Vector normal = compute_newell(points).normalized();
        const Point origin = Point::centroid(points);
        planes.push_back(Plane::from_point_normal(origin, (origin - centre).dot(normal) < 0.0 ? -normal : normal));
    }

    return planes;
}

/// True when every vertex of a mesh lies behind every one of its face planes.
bool is_convex(const Mesh& mesh, const std::vector<Plane>& planes) {

    for (const Plane& plane : planes)
        for (const size_t vertex : mesh.vertices())
            if ((*mesh.vertex_point(vertex) - plane.origin()).dot(plane.z_axis()) > CLEARANCE)
                return false;

    return true;
}

/// Volume of other inside a convex mesh, its face planes pulled in by the tolerance so touching faces count for nothing.
double compute_overlap(const Mesh& convex, const Mesh& other) {

    Mesh part = other;
    for (const Plane& plane : compute_planes(convex)) {
        part = part.cut_by_plane(Plane::from_point_normal(plane.origin() - plane.z_axis() * CLEARANCE, -plane.z_axis()));
        if (part.faces().empty())
            return 0.0;
    }

    return std::abs(part.volume());
}

/// The lowest and highest corner of a mesh.
std::pair<Point, Point> compute_box(const Mesh& mesh) {

    std::pair<Point, Point> box(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));
    for (const size_t vertex : mesh.vertices())
        for (int axis = 0; axis < 3; axis++) {
            box.first[axis] = std::min(box.first[axis], (*mesh.vertex_point(vertex))[axis]);
            box.second[axis] = std::max(box.second[axis], (*mesh.vertex_point(vertex))[axis]);
        }

    return box;
}

/// True when two meshes' boxes overlap by more than the tolerance on every axis.
bool is_near(const Mesh& a, const Mesh& b) {

    const std::pair<Point, Point> box_a = compute_box(a);
    const std::pair<Point, Point> box_b = compute_box(b);
    for (int axis = 0; axis < 3; axis++)
        if (box_a.second[axis] <= box_b.first[axis] + CLEARANCE || box_b.second[axis] <= box_a.first[axis] + CLEARANCE)
            return false;

    return true;
}

/// The largest overlap volume over every pair of elements whose boxes overlap, a convex one of the pair cutting the other.
double compute_clash(const WoodSession& session) {

    double worst = 0.0;
    const Collection<std::shared_ptr<Element>>& elements = session.elements();
    for (size_t i = 0; i < elements.size(); i++)
        for (size_t j = i + 1; j < elements.size(); j++) {
            const Mesh& a = elements[i]->model_geometry_mesh();
            const Mesh& b = elements[j]->model_geometry_mesh();
            if (!is_near(a, b))
                continue;

            const bool convex = is_convex(a, compute_planes(a));
            worst = std::max(worst, convex ? compute_overlap(a, b) : compute_overlap(b, a));
        }

    return worst;
}

int main() {

    WoodSession wood_session("templates_grid_framing");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const wood_grid::Building building = wood_grid::Building::from_footprint({}, ELEVATIONS, item.pattern.transformed(Xform::translation(offset, 0.0, 0.0)));
        const std::shared_ptr<TreeNode> group = wood_session.add_group(item.name);
        for (size_t storey = 0; storey + 1 < building.levels.size(); storey++)
            for (const std::shared_ptr<Element>& element : building.to_elements(item.framing, storey))
                wood_session.add(element, group);
        offset += item.pattern.lines.front().length() + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);
    wood_session.pb_dump(pb_path("live"));

    const double clash = compute_clash(wood_session);
    std::cout << "largest overlap " << clash << " mm3" << std::endl;

    return clash > CLEARANCE ? 1 : 0;
}

/*
|||||||| DESCRIPTION ||||||||
The joints and sections of the grid template, fifteen bays side by side over two storeys, one group each: heads that are the column section extruded where the girders run straight over them and conical at the corners, then conical and stepped (a capital under a drop panel) under a point supported deck in strips; columns flush with the datum and running through the levels with the decks notched round them; purlins flush with their girders, hung with the girder top 8 in lower, and stacked over a girder a whole purlin depth lower; then the seven profiles of the library as girders with a matching column: rectangle, round, W, HSS with its hole, double as two members with the same cuts, slab band and T. After the contacts the example cuts every pair of overlapping elements by the convex one's faces, prints the largest overlap volume and fails when it is more than the tolerance: every element touches its neighbours face to face and none overlap. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_framing --parallel 4 && ./build/templates_grid_framing && ../bash/publish-scene.sh --target templates_grid_framing

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
