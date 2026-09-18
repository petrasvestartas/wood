#include "wood_session.h"
#include "src/templates/reciprocal_move.h"
#include "src/templates/reciprocal_surface.h"

using namespace session_cpp;
using namespace wood_session;

// ── what to build ────────────────────────────────────────────────────────────
enum class Grid { Quads, Hexagons, DualHexagons };
const int SURFACE = 0;           // 0 sinusoidal dome (straight rectangular boundary), 1 hypar, 2 pillow dome, 3 disc dome, 4 Annen arch, 5 Scherk saddle
const Grid GRID = Grid::Hexagons;
const double CELL = 2000.0;      // cell size, mm
const double SHIFT = 100.0;      // sideways move of every beam, mm; at least half the beam width, the end cuts follow from it
const double WIDTH = 100.0;      // beam section, mm
const double HEIGHT = 400.0;

// ── frame orientation ────────────────────────────────────────────────────────
// Every boundary curve gets one tilt, the sections mirror across each mitre along it, and the corners are butt joints.
// The tilt comes from the surface, or from FRAME_VECTORS when FRAME_FROM_VECTORS is on: one vector per boundary curve,
// for a surface in the order u start, u end, v start, v end; for a mesh alone the sides are split at the corners and
// taken in loop order.
const bool FRAME_FROM_VECTORS = true;  // on with four z vectors: a vertical frame all round, as here on the dome's rectangular boundary
const std::vector<Vector> FRAME_VECTORS = {Vector(0, 0, 1), Vector(0, 0, 1), Vector(0, 0, 1), Vector(0, 0, 1)};

/// The NURBS surface of the case, its boundary curves flattened into planes; none for the sinusoidal dome.
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
        return GRID == Grid::Hexagons ? wood_reciprocal::sinusoidal_dome_hex_mesh(12000.0, 10000.0, 3000.0, CELL)
             : GRID == Grid::DualHexagons ? wood_reciprocal::dual_hex_mesh(quads) : quads;
    }

    if (GRID == Grid::Hexagons)
        return wood_reciprocal::hex_mesh_from_surface(*nurbs, CELL);

    Mesh quads = SURFACE == 3 ? wood_reciprocal::disc_quad_mesh(*nurbs, 8000.0, CELL) : wood_reciprocal::quad_mesh_from_surface(*nurbs, CELL);
    return GRID == Grid::DualHexagons ? wood_reciprocal::dual_hex_mesh(quads) : quads;
}

/// One up per naked edge: the tilt per boundary curve from the surface, or from the given vectors.
static std::map<wood_reciprocal::EdgeKey, Vector> frame_ups(const Mesh& mesh, const std::optional<NurbsSurface>& nurbs) {

    if (nurbs) {
        std::optional<std::array<Vector, 4>> given;
        if (FRAME_FROM_VECTORS)
            given = std::array<Vector, 4>{FRAME_VECTORS[0], FRAME_VECTORS[1], FRAME_VECTORS[2], FRAME_VECTORS[3]};

        return wood_reciprocal::side_tilt_boundary_ups(mesh, *nurbs, given);
    }

    return wood_reciprocal::side_tilt_boundary_ups(mesh, 45.0, FRAME_FROM_VECTORS ? FRAME_VECTORS : std::vector<Vector>{});
}

/// Builds the reciprocal move shell on the chosen surface and writes the surface, the mesh, one plate per beam and one per frame beam to live.
int main() {

    const std::optional<NurbsSurface> surface = case_surface();
    const Mesh mesh = case_mesh(surface);
    const std::map<wood_reciprocal::EdgeKey, int> through = surface ? wood_reciprocal::through_side_priority(mesh, *surface) : wood_reciprocal::through_side_priority(mesh);
    const ReciprocalMove shell(mesh, SHIFT, WIDTH, HEIGHT, 5.0, 1.0, wood_reciprocal::BoundaryTwist::AtBends, frame_ups(mesh, surface),
                               ReciprocalMove::BeamUp::EdgeAverage, wood_reciprocal::CornerJoint::Butt, through);

    WoodSession wood_session("reciprocal_move");
    if (surface)
        wood_session.add_nurbssurface(std::make_shared<NurbsSurface>(*surface));

    wood_session.add_mesh(std::make_shared<Mesh>(shell.dome_mesh));
    for (size_t beam = 0; beam < shell.beam_bottom.size(); beam++)
        wood_session.add(std::make_shared<Plate>(shell.beam_bottom[beam], shell.beam_top[beam], "beam"));

    for (size_t beam = 0; beam < shell.boundary_beam_bottom.size(); beam++)
        wood_session.add(std::make_shared<Plate>(shell.boundary_beam_bottom[beam], shell.boundary_beam_top[beam], "boundary_beam"));

    std::cout << fmt::format("reciprocal move, surface {}: {} faces, {} beams, {} boundary beams\n", SURFACE, shell.dome_mesh.number_of_faces(), shell.beam_bottom.size(), shell.boundary_beam_bottom.size());

    wood_session.add_to_tree(true, true, false, false);
    wood_session.pb_dump(pb_path("live").string());

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reciprocal move template: the edges of a quad or hexagonal mesh on a surface become beams shifted past each other by SHIFT, each end cut flush against the beam it bears on, framed by straight boundary beams with one tilt per boundary curve and butt corners; one plate per beam, written to live for the viewer.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel 4 && ./build/main_reciprocal_move && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target main_reciprocal_move

|||||||| WORKFLOW ||||||||
examples/templates/main_reciprocal_move.cpp
 |
 |-- case_surface -> one of the test surfaces, flatten_surface_sides            src/templates/reciprocal_surface.h
 |-- case_mesh -> quad_mesh_from_surface, hex_mesh_from_surface or dual_hex_mesh
 |-- frame_ups -> side_tilt_boundary_ups, from the surface or from FRAME_VECTORS
 |
 |-- ReciprocalMove(mesh, shift, width, height, ..., boundary_ups, EdgeAverage, Butt, through)   src/templates/reciprocal_move.h
 |    |-- beams: translated edges cut against the beams they bear on; frame: wood_reciprocal::boundary_frame, cut_beam   src/templates/reciprocal_boundary.h
 |
 |-- WoodSession add_nurbssurface, add_mesh, add(plate), add_to_tree, pb_dump   src/joinery_solver/wood_session.cpp -> data/output/pb/live.pb

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
