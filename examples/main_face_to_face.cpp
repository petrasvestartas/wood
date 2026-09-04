#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/session.h"
#include "../src/polyline.h"
#include "../src/mesh.h"

#include <fmt/core.h>
#include <filesystem>
#include <string>
#include <vector>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Choose
// ═══════════════════════════════════════════════════════════════════════════

const bool plates_or_blocks = true;

// ═══════════════════════════════════════════════════════════════════════════
// A — plates built in code
// ═══════════════════════════════════════════════════════════════════════════
static std::vector<WoodElement> plates() {
    const double h = 174.0;
    const double width = 1000.0;
    const double height = 500.0;
    const double thickness = 15.0;
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);

    // Folded plate: unnormalized edge, its true length is the height.
    const Vector fold(0.0, height, -h);
    const double fold_length = fold.magnitude();

    std::vector<WoodElement> elements;

    // Coplanar pair, sharing the edge y = 0.
    elements.emplace_back(
        Polyline::rectangle(Point(-500, 0, 0), x, y, width, height),
        Polyline::rectangle(Point(-500, 0, -thickness), x, y, width, height));
    elements.emplace_back(
        Polyline::rectangle(Point(-500, -500, 0), x, y, width, height),
        Polyline::rectangle(Point(-500, -500, -thickness), x, y, width, height));

    // Folded pair, sharing the edge x = 1000..2000, y = 0.
    elements.emplace_back(
        Polyline::rectangle(Point(1000, 0, 0), x, y, width, height),
        Polyline::rectangle(Point(1000, 0, -thickness), x, y, width, height));
    elements.emplace_back(
        Polyline::rectangle(Point(1000, -500, h), x, fold, width, fold_length),
        Polyline::rectangle(Point(1000, -500, h - thickness), x, fold, width, fold_length));
    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// B — the compas_tf floor system, as closed polylines
// ═══════════════════════════════════════════════════════════════════════════

static std::vector<BlockElement> blocks() {
    const std::filesystem::path pb = internal::session_data_dir() / "floor_model.pb";

    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\ngenerate it with:\n"
                           "  cd data/face_to_face_detection && .venv/bin/python step_to_pb.py\n",
                   pb.string());
        return {};
    }

    Session session = Session::pb_load(pb.string());
    std::vector<BlockElement> elements;
    for (const std::vector<Polyline>& loops : session.select_by_type<Polyline>()) {
        elements.emplace_back(loops);
    }
    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// Run and write
// ═══════════════════════════════════════════════════════════════════════════

// Contact areas come out of the boolean as one closed ring, which the CDT meshes directly;
// `false` keeps that ring as the boundary rather than picking one by bounding box.
template <class Element>
static int write_live(const std::vector<Element>& elements,
                      const std::vector<FaceContact>& contacts) {
    Session session("wood - face to face contacts");

    // Inputs keep Polyline's default black pen. The contacts carry their colour on the mesh
    // itself, not on the group - the viewer draws an object from its own colour.
    auto g_inputs = session.add_group("Inputs");
    size_t outlines = 0;
    for (size_t i = 0; i < elements.size(); ++i) {
        for (size_t f = 0; f < elements[i].polylines.size(); ++f) {
            auto pl = std::make_shared<Polyline>(elements[i].polylines[f]);
            pl->name = fmt::format("element_{}_face_{}", i, f);
            session.add_polyline(pl, g_inputs);
            ++outlines;
        }
    }

    auto g_contacts = session.add_group("Contacts");
    for (const FaceContact& c : contacts) {
        auto mesh = std::make_shared<Mesh>(Mesh::from_polygon_with_holes({c.area.get_points()}, false));
        if (mesh->number_of_faces() == 0) { continue; }   // degenerate ring, nothing to draw
        mesh->name = fmt::format("contact_{}_{}", c.element_a, c.element_b);
        // Color takes 0..1 FLOATS and clamps. Passing 0..255 makes every channel clamp to
        // 1.0 - white - which is what shipped, and why the contacts were invisible against
        // the background. One objectcolor per mesh is enough: to_render only prefers
        // per-vertex colors when color_mode is POINTCOLORS, so no vertex colors are needed.
        mesh->set_objectcolor(Color(1.0f, 0.0f, 0.0f, 1.0f, "red"));
        session.add_mesh(mesh, g_contacts);
    }

    const std::filesystem::path dir = internal::output_dir() / "pb";
    std::filesystem::create_directories(dir);
    const std::string path = (dir / "live.pb").string();
    session.pb_dump(path);
    return outlines > 0 ? 0 : 1;
}

int main() {
    globals::globals_yaml("hello");

    if constexpr (plates_or_blocks) {
        const std::vector<WoodElement> elements = plates();
        const std::vector<FaceContact> contacts =
            face_contacts(elements, globals::DISTANCE, globals::ANGLE, globals::DISTANCE_SQUARED);

        // The reason this case is a WoodElement and case B is not: only a known
        // (bottom, top) pair lets the joints be classified.
        std::vector<WoodElement> mutable_elements = elements;
        const std::vector<WoodJoint> joints = get_connection_zones(mutable_elements, face_to_face);

        return write_live(elements, contacts);
    } else {
        const std::vector<BlockElement> elements = blocks();
        if (elements.empty()) { return 1; }
        const std::vector<FaceContact> contacts =
            face_contacts(elements, globals::DISTANCE, globals::ANGLE, globals::DISTANCE_SQUARED);
        return write_live(elements, contacts);
    }
}
