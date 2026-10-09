#include "wood_session.h"
#include "src/templates/reciprocal/reciprocal_move.h"
#include "src/templates/reciprocal/reciprocal_surface.h"

using namespace session_cpp;
using namespace wood_session;

const int SURFACE = 0; // 0 sinusoidal dome, 1 hypar, 2 pillow dome, 3 disc dome, 4 Annen arch, 5 Scherk saddle
const int GRID = 1; // 0 quads, 1 hexagons, 2 dual hexagons
const double CELL = 2000.0; // mm, the cell size on a surface
const bool FRAME_VERTICAL = true; // the frame upright all round, else tilted with the surface

/// The surface of the case, its boundary curves flattened into planes; none for the sinusoidal dome.
std::optional<NurbsSurface> case_surface() {

    const std::string annen = (config::session_data_dir() / "annen_surfaces.json").string();
    std::optional<NurbsSurface> surface;

    if (SURFACE == 1)
        surface = wood_reciprocal::hypar_surface(12000.0, 10000.0, 3000.0);
    else if (SURFACE == 2)
        surface = wood_reciprocal::pillow_dome_surface(
            12000.0,
            10000.0,
            3000.0,
            1200.0
        );
    else if (SURFACE == 3)
        return wood_reciprocal::disc_dome_surface(8000.0, 3000.0);
    else if (SURFACE == 4)
        surface = wood_reciprocal::annen_surface(annen, 0);
    else if (SURFACE == 5)
        surface = wood_reciprocal::scherk_surface(
            12000.0,
            10000.0,
            2500.0,
            1200.0
        );

    if (!surface)
        return std::nullopt;

    return wood_reciprocal::flatten_surface_sides(*surface);
}

/// The mesh of the case in the chosen grid.
Mesh case_mesh(const std::optional<NurbsSurface>& surface) {

    if (!surface && GRID == 1)
        return wood_reciprocal::sinusoidal_dome_hex_mesh(
            12000.0,
            10000.0,
            3000.0,
            CELL
        );

    if (surface && GRID == 1)
        return wood_reciprocal::hex_mesh_from_surface(*surface, CELL);

    Mesh quads = wood_reciprocal::sinusoidal_dome_mesh(
        12,
        10,
        12000.0,
        10000.0,
        3000.0
    );

    if (surface)
        quads = SURFACE == 3 ? wood_reciprocal::disc_quad_mesh(*surface, 8000.0, CELL) : wood_reciprocal::quad_mesh_from_surface(*surface, CELL);

    return GRID == 2 ? wood_reciprocal::dual_hex_mesh(quads) : quads;
}

/// The reciprocal move frame on the case's mesh: beams shifted past each other inside a butted frame.
int main() {

    const std::optional<NurbsSurface> surface = case_surface();
    const Mesh mesh = case_mesh(surface);
    const std::vector<Vector> upright(4, Vector(0, 0, 1));
    const std::map<std::pair<size_t, size_t>, Vector> frame_ups = surface ? wood_reciprocal::side_tilt_boundary_ups(mesh, *surface) : wood_reciprocal::side_tilt_boundary_ups(mesh, 45.0, FRAME_VERTICAL ? upright : std::vector<Vector>{});
    const std::map<std::pair<size_t, size_t>, int> through = surface ? wood_reciprocal::through_side_priority(mesh, *surface) : wood_reciprocal::through_side_priority(mesh, 45.0);

    ReciprocalMove move(
        mesh,
        100.0,
        100.0,
        400.0,
        1.0,
        frame_ups,
        ReciprocalMove::BeamUp::EdgeAverage,
        wood_reciprocal::CornerJoint::Butt,
        through
    );

    if (surface)
        move.add_nurbssurface(std::make_shared<NurbsSurface>(*surface), move.group_named("mesh"));

    std::cout << fmt::format("reciprocal move: {} faces, {} beams, {} frame beams\n", mesh.number_of_faces(), move.beams().size(), move.frame_beams().size());
    move.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The reciprocal move template on the sinusoidal dome's hexagonal mesh: every other edge of each face becomes a 100 x 400 beam moved 100 sideways so it bears on its neighbours, each end cut flush at the beam it lands on; upright frame beams on the naked edges, butted at the corners. Groups mesh, axes, frame and beams, one per constructor step.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_reciprocal_2_move --parallel 6 && ./build/templates_reciprocal_2_move && ../bash/publish-scene.sh --target templates_reciprocal_2_move

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
