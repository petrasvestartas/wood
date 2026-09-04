#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/session.h"
#include "../src/polyline.h"
#include "../src/plane.h"
#include "../src/mesh.h"

#include <fmt/core.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Choose
// ═══════════════════════════════════════════════════════════════════════════

enum class Case { Plates, Blocks };

static constexpr Case PUBLISH = Case::Plates;

// ═══════════════════════════════════════════════════════════════════════════
// A — plates built in code
// ═══════════════════════════════════════════════════════════════════════════
//
// Plates are 15 mm thick, given as the outline at z = 0 and its copy at z = -15, in
// (bottom, top) order. Two coplanar plates meeting along y = 0 give a side-to-side IN-PLANE
// joint; the folded pair at x >= 1000 shares an edge and gives an OUT-OF-PLANE one.
//
// Plates must TOUCH, not interpenetrate: a wall sunk into a slab shares no coplanar face
// pair and is detected as nothing at all.

static std::vector<WoodElement> plates() {
    const double h = 174.0;
    const Vector x(1.0, 0.0, 0.0);
    const Vector y(0.0, 1.0, 0.0);

    // The folded plate leans: its second edge runs +500 in y while dropping h in z, so it is
    // handed to Plane unnormalized and its true length becomes the rectangle height.
    const Vector fold(0.0, 500.0, -h);
    const double fold_length = fold.magnitude();

    std::vector<WoodElement> elements;

    // Coplanar pair, sharing the edge y = 0.
    elements.emplace_back(
        Polyline::rectangle(Plane(Point(-500,   0,   0), x, y), 1000.0, 500.0),
        Polyline::rectangle(Plane(Point(-500,   0, -15), x, y), 1000.0, 500.0));
    elements.emplace_back(
        Polyline::rectangle(Plane(Point(-500, -500,   0), x, y), 1000.0, 500.0),
        Polyline::rectangle(Plane(Point(-500, -500, -15), x, y), 1000.0, 500.0));

    // Folded pair, sharing the edge x = 1000..2000, y = 0.
    elements.emplace_back(
        Polyline::rectangle(Plane(Point(1000,   0,   0), x, y), 1000.0, 500.0),
        Polyline::rectangle(Plane(Point(1000,   0, -15), x, y), 1000.0, 500.0));
    elements.emplace_back(
        Polyline::rectangle(Plane(Point(1000, -500,      h), x, fold), 1000.0, fold_length),
        Polyline::rectangle(Plane(Point(1000, -500, h - 15), x, fold), 1000.0, fold_length));
    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// B — the compas_tf floor system, as closed polylines
// ═══════════════════════════════════════════════════════════════════════════
//
// A list of lists: each child of the tree root is one solid, its children are that solid's
// closed face loops, unordered and unlabelled.

static std::vector<BlockElement> blocks() {
    const std::filesystem::path pb = internal::session_data_dir() / "floor_model.pb";

    // pb_load answers a missing file with an EMPTY session rather than throwing, which would
    // make a bad path look like a run that found nothing. Check first.
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr, "not found: {}\ngenerate it with:\n"
                           "  cd data/face_to_face_detection && .venv/bin/python step_to_pb.py\n",
                   pb.string());
        return {};
    }

    Session session = Session::pb_load(pb.string());
    std::vector<BlockElement> elements;
    if (std::shared_ptr<TreeNode> root = session.tree.root()) {
        for (TreeNode* group : root->children()) {
            std::vector<Polyline> loops;
            for (TreeNode* leaf : group->children()) {
                if (auto pl = session.get_object<Polyline>(leaf->name)) { loops.push_back(*pl); }
            }
            if (!loops.empty()) { elements.emplace_back(loops); }
        }
    }
    return elements;
}

// ═══════════════════════════════════════════════════════════════════════════
// Run and write
// ═══════════════════════════════════════════════════════════════════════════

static double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// Works on either element type: both carry outlines and planes.
template <class Element>
static std::vector<FaceContact> contacts_of(const std::vector<Element>& elements) {
    auto t0 = std::chrono::steady_clock::now();
    const std::vector<std::pair<int, int>> pairs = adjacency_search(elements, globals::DISTANCE);
    const double t_broad = ms_since(t0);

    // face_contacts runs the broad phase again internally; the call above is only so the two
    // phases can be reported apart.
    t0 = std::chrono::steady_clock::now();
    std::vector<FaceContact> contacts =
        face_contacts(elements, globals::DISTANCE, globals::ANGLE, globals::DISTANCE_SQUARED);
    const double t_both = ms_since(t0);

    size_t faces = 0;
    for (const Element& e : elements) { faces += e.polylines.size(); }
    fmt::print("elements    : {} ({} faces)\n", elements.size(), faces);
    fmt::print("broad phase : {} candidate pairs            {:8.3f} ms\n", pairs.size(), t_broad);
    fmt::print("narrow phase: {} face pairs in real contact {:8.3f} ms\n",
               contacts.size(), std::max(0.0, t_both - t_broad));
    return contacts;
}

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
    size_t meshed = 0;
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
        ++meshed;
    }

    const std::filesystem::path dir = internal::output_dir() / "pb";
    std::filesystem::create_directories(dir);
    const std::string path = (dir / "live.pb").string();
    session.pb_dump(path);

    fmt::print("wrote       : {} polylines, {} meshes\n", outlines, meshed);
    fmt::print("              {}\n", path);
    return outlines > 0 ? 0 : 1;
}

int main() {
    globals::globals_yaml("hello");

    if constexpr (PUBLISH == Case::Plates) {
        const std::vector<WoodElement> elements = plates();
        const std::vector<FaceContact> contacts = contacts_of(elements);

        // The reason this case is a WoodElement and case B is not: only a known
        // (bottom, top) pair lets the joints be classified.
        std::vector<WoodElement> mutable_elements = elements;
        const std::vector<WoodJoint> joints = get_connection_zones(mutable_elements, face_to_face);

        return write_live(elements, contacts);
    } else {
        const std::vector<BlockElement> elements = blocks();
        if (elements.empty()) { return 1; }
        const std::vector<FaceContact> contacts = contacts_of(elements);
        return write_live(elements, contacts);
    }
}
