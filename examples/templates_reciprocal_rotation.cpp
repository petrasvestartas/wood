#include "wood_session.h"
#include "src/templates/reciprocal/reciprocal_rotation.h"
#include "src/templates/reciprocal/reciprocal_surface.h"

using namespace session_cpp;
using namespace wood_session; // ═══════════════════════════════════════════════════════════════════════════
// What to build
// ═══════════════════════════════════════════════════════════════════════════
const int SURFACE = 0; // 0 sinusoidal dome, 1 hypar, 2 pillow dome, 3 disc dome, 4 Annen arch, 5 Scherk saddle
const int GRID = 0; // 0 quads, 1 hexagons, 2 dual hexagons; the rotation template is at home on quads; on hexagons its beams can cross each other
const double CELL = 2000.0; // cell size, mm
const double ANGLE = 0.35; // rotation of every beam about its midpoint, radians
const double STRETCH = 1.4; // lengthening of every beam before the cuts
const double WIDTH = 100.0; // beam section, mm
const double HEIGHT = 400.0; // ═══════════════════════════════════════════════════════════════════════════
// Frame orientation
// ═══════════════════════════════════════════════════════════════════════════
const bool FRAME_FROM_VECTORS = false;
const std::vector<Vector> FRAME_VECTORS = {Vector(0, 0, 1), Vector(0, 0, 1), Vector(0, 0, 1), Vector(0, 0, 1)}; /// The NURBS surface of the case, its boundary curves flattened into planes; none for the sinusoidal dome.
static std::optional<NurbsSurface> case_surface() {

    const std::string annen = (config::session_data_dir() / "annen_surfaces.json").string();
    std::optional<NurbsSurface> nurbs;
    switch (SURFACE) {
        case 1: nurbs = wood_reciprocal::hypar_surface(12000.0, 10000.0, 3000.0); break;
        case 2: nurbs = wood_reciprocal::pillow_dome_surface(12000.0, 10000.0, 3000.0, 1200.0); break;
        case 3: return wood_reciprocal::disc_dome_surface(8000.0, 3000.0);
        case 4: nurbs = wood_reciprocal::annen_surface(annen, 0); break;
        case 5: nurbs = wood_reciprocal::scherk_surface(12000.0, 10000.0, 2500.0, 1200.0); break;
        default: return std::nullopt;
    }

    return wood_reciprocal::flatten_surface_sides(*nurbs);
}

/// The mesh of the case in the chosen grid.
static Mesh case_mesh(const std::optional<NurbsSurface>& nurbs) {

    if (!nurbs) {
        Mesh quads = wood_reciprocal::sinusoidal_dome_mesh(12, 10, 12000.0, 10000.0, 3000.0);
        return GRID == 1 ? wood_reciprocal::sinusoidal_dome_hex_mesh(12000.0, 10000.0, 3000.0, CELL)
             : GRID == 2 ? wood_reciprocal::dual_hex_mesh(quads) : quads;
    }

    if (GRID == 1)
        return wood_reciprocal::hex_mesh_from_surface(*nurbs, CELL);

    Mesh quads = SURFACE == 3 ? wood_reciprocal::disc_quad_mesh(*nurbs, 8000.0, CELL) : wood_reciprocal::quad_mesh_from_surface(*nurbs, CELL);
    return GRID == 2 ? wood_reciprocal::dual_hex_mesh(quads) : quads;
}

/// One up per naked edge: the tilt per boundary curve from the surface, or from the given vectors.
static std::map<std::pair<size_t, size_t>, Vector> frame_ups(const Mesh& mesh, const std::optional<NurbsSurface>& nurbs) {

    if (nurbs) {
        std::optional<std::array<Vector, 4>> given;
        if (FRAME_FROM_VECTORS)
            given = std::array<Vector, 4>{FRAME_VECTORS[0], FRAME_VECTORS[1], FRAME_VECTORS[2], FRAME_VECTORS[3]};

        return wood_reciprocal::side_tilt_boundary_ups(mesh, *nurbs, given);
    }

    return wood_reciprocal::side_tilt_boundary_ups(mesh, 45.0, FRAME_FROM_VECTORS ? FRAME_VECTORS : std::vector<Vector>{});
}

/// Builds the reciprocal rotation shell on the chosen surface and writes the surface, the mesh, one plate per interior beam and one per frame beam to live.
int main() {

    const std::optional<NurbsSurface> surface = case_surface();
    const Mesh mesh = case_mesh(surface);
    const std::map<std::pair<size_t, size_t>, int> through = surface ? wood_reciprocal::through_side_priority(mesh, *surface) : wood_reciprocal::through_side_priority(mesh);
    const ReciprocalRotation shell(mesh, ANGLE, STRETCH, WIDTH, HEIGHT, 5.0, 1.0, wood_reciprocal::BoundaryTwist::AtBends, frame_ups(mesh, surface),
                                   wood_reciprocal::CornerJoint::Butt, through);

    WoodSession wood_session("reciprocal_rotation");
    if (surface)
        wood_session.add_nurbssurface(std::make_shared<NurbsSurface>(*surface));

    wood_session.add_mesh(std::make_shared<Mesh>(shell.dome_mesh));

    const std::vector<std::pair<size_t, size_t>> edges = shell.dome_mesh.edges(); // beams[i] sits on edges[i]
    size_t interior = 0;
    for (size_t beam = 0; beam < shell.beam_bottom.size(); beam++) {
        if (shell.dome_mesh.is_edge_on_boundary(edges[beam].first, edges[beam].second))
            continue; // the rotated naked-edge beam: the straight frame beam replaces it

        wood_session.add(std::make_shared<Plate>(shell.beam_bottom[beam], shell.beam_top[beam], "beam"));
        interior++;
    }

    for (size_t beam = 0; beam < shell.boundary_beam_bottom.size(); beam++)
        wood_session.add(std::make_shared<Plate>(shell.boundary_beam_bottom[beam], shell.boundary_beam_top[beam], "boundary_beam"));

    std::cout << fmt::format("reciprocal rotation, surface {}: {} faces, {} beams, {} boundary beams\n", SURFACE, shell.dome_mesh.number_of_faces(), interior, shell.boundary_beam_bottom.size());

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reciprocal rotation template: the edges of a mesh on a surface become beams rotated about their midpoints and cut against their neighbours, framed by straight boundary beams with one tilt per boundary curve and butt corners; one plate per beam, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_reciprocal_rotation --parallel 4 && ./build/templates_reciprocal_rotation && ../bash/publish-scene.sh --target templates_reciprocal_rotation

|||||||| WORKFLOW ||||||||
examples/templates_reciprocal_rotation.cpp
 |
 |-- case_surface -> one of the test surfaces, flatten_surface_sides            src/templates/reciprocal/reciprocal_surface.h
 |-- case_mesh -> quad_mesh_from_surface, hex_mesh_from_surface or dual_hex_mesh
 |-- frame_ups -> side_tilt_boundary_ups, from the surface or from FRAME_VECTORS
 |
 |-- ReciprocalRotation(mesh, angle, scale, width, height, ..., boundary_ups, Butt, through)   src/templates/reciprocal/reciprocal_rotation.h
 |    |-- Reciprocal::from_mesh -> rotated centre lines; frame: wood_reciprocal::boundary_frame, cut_beam   src/templates/reciprocal/reciprocal_boundary.h
 |
 |-- WoodSession add_nurbssurface, add_mesh, add(plate), pb_dump   src/joinery_solver/wood_session.cpp -> data/output/pb/live.pb

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
