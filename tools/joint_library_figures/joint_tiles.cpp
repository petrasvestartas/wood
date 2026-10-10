// The tiles of docs/joint_library.md: every design the oracle builds, joined on its fixture pair, written as what each plate keeps inside
// the joint's volume box, the plates whole and the key, one text file per design for draw_tile.py and draw_sweep.py.
//   ./build/joint_tiles <out_dir> [family/library/parameters ...]     every oracle variant when no id is given
#define main joint_library_main
#include "../../tests/joint_library.cpp"
#undef main
#include "remesh_cdt.h"

// ═══════════════════════════════════════════════════════════════════════════
// Writing
// ═══════════════════════════════════════════════════════════════════════════

/// The corners of a mesh ring.
static std::vector<Point> ring_points(const Mesh& mesh, const std::vector<size_t>& ring) {

    std::vector<Point> points;
    for (size_t key : ring)
        points.push_back(*mesh.vertex_point(key));

    return points;
}

/// One face line: F x y z x y z ...
static void write_face(std::ofstream& out, const std::vector<Point>& points) {

    out << "F";
    for (const Point& point : points)
        out << " " << point[0] << " " << point[1] << " " << point[2];
    out << "\n";
}

/// A part: every face as its polygon, a face with holes as the triangles of its ring and hole rings in its own plane.
static void write_mesh(std::ofstream& out, const Mesh& mesh, const std::string& kind) {

    out << "P 0 " << kind << "\n";
    for (const std::pair<const size_t, std::vector<size_t>>& entry : mesh.face) {
        const std::vector<Point> outer = ring_points(mesh, entry.second);
        const auto holes = mesh.get_face_holes().find(entry.first);
        if (holes == mesh.get_face_holes().end() || holes->second.empty()) {
            write_face(out, outer);
            continue;
        }

        const Vector normal = Vector::average_normal(outer);
        const Plane plane = Plane::from_point_normal(outer[0], normal);
        const Xform to_plane = Xform::world_to_frame(plane.origin(), plane.x_axis(), plane.y_axis(), plane.z_axis());
        std::vector<Point> corners = outer;
        std::vector<Polyline> rings = {Polyline(outer).transformed(to_plane)};
        for (const std::vector<size_t>& hole : holes->second) {
            std::vector<Point> points = ring_points(mesh, hole);
            if (Vector::average_normal(points).dot(normal) > 0.0)
                std::reverse(points.begin(), points.end());
            rings.push_back(Polyline(points).transformed(to_plane));
            corners.insert(corners.end(), points.begin(), points.end());
        }

        for (const std::array<int, 3>& triangle : RemeshCDT::triangulate(rings)) {
            const Point& a = corners[triangle[0]];
            const Point& b = corners[triangle[1]];
            const Point& c = corners[triangle[2]];
            if ((b - a).cross(c - a).dot(normal) < 0.0)
                write_face(out, {a, c, b});
            else
                write_face(out, {a, b, c});
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Tiles
// ═══════════════════════════════════════════════════════════════════════════

/// One design: its name and parameters, the box, the male and female tiles, the plates and the key.
static void write_tiles(const std::string& id, const std::string& dir) {

    const Built built = build_variant(id, Xform::identity());
    const InteractionFeaturePlate& connection = built.joint->connections.at(0);
    std::string name = id;
    std::replace(name.begin(), name.end(), '/', '_');
    std::ofstream out(dir + "/" + name + ".txt");
    out << "N " << id << " " << connection.name << " divisions " << connection.divisions << " shift " << connection.shift << "\n";

    // the tiles: each plate's cut solid inside the volume box, shrunk 1e-4 so that a plate face lying on the box is not a sheet of the result
    const std::array<std::shared_ptr<Plate>, 2> plates = {built.fixture.target0, built.fixture.target1};
    if (connection.joint_volumes[0] && connection.joint_volumes[1]) {
        for (int side = 0; side < 2; side++) {
            const int first = side == 1 && connection.joint_volumes[2] && connection.joint_volumes[3] ? 2 : 0;
            const Polyline& near = *connection.joint_volumes[first];
            const Polyline& far = *connection.joint_volumes[first + 1];
            Mesh box = Mesh::loft({near}, {far});
            box.transform(Xform::scale_uniform(near.center() + (far.center() - near.center()) * 0.5, 1.0 - 1e-4));
            write_mesh(out, solid_boolean(plates[side]->model_geometry_mesh(), box, SolidOperation::intersect), side == 0 ? "male" : "female");
        }
    }

    // the plates as the solver cuts them, and the joint's own piece
    write_mesh(out, plates[0]->model_geometry_mesh(), "male_plate");
    write_mesh(out, plates[1]->model_geometry_mesh(), "female_plate");
    if (!built.joint->key_mesh().face.empty())
        write_mesh(out, built.joint->key_mesh(), "key");
}

int main(int argc, char** argv) {

    if (argc < 2)
        throw std::invalid_argument("joint_tiles <out_dir> [ids...]");

    std::vector<std::string> ids(argv + 2, argv + argc);
    if (ids.empty())
        ids = VARIANTS;

    for (const std::string& id : ids) {
        try {
            write_tiles(id, argv[1]);
        } catch (const std::exception& e) {
            std::cout << id << ": " << e.what() << std::endl;
        }
    }

    return 0;
}
