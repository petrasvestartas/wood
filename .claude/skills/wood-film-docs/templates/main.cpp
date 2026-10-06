#include "docs/floor/movie.h"

using namespace movie;

/// Writes every frame of the floor film to data/output/floor_movie, chapter by chapter in the order the algorithm runs: a scene and its notes per frame, for docs/floor/render.py.
int main() {

    const std::filesystem::path dir = config::output_dir() / "floor_movie";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    FloorParameters tied_parameters;
    tied_parameters.seam_through_ribs = false;
    const FloorGuide guide = FloorGuide::rectangle(3000.0, 3000.0);
    const FloorGuide tied = FloorGuide::rectangle(3000.0, 2400.0, tied_parameters);

    Floor members(guide);
    members.add_members();

    Floor connected(guide);
    connected.add_members();
    connected.add_connectors();
    connected.add_screws();

    const Context context{guide, tied, members, connected, dir};
    chapter_01_bay(context);
    chapter_02_quarter_planes(context);
    chapter_03_parabolas(context);
    chapter_04_central_panel(context);
    chapter_05_rib_outlines(context);
    chapter_06_outlines(context);
    chapter_07_elements(context);
    chapter_08_relationships(context);
    chapter_09_connectors(context);
    chapter_10_screws(context);
    chapter_11_checks(context);

    return 0;
}
