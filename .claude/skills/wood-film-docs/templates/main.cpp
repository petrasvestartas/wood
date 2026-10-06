#include "docs/floor/movie.h"

using namespace movie;

/// Writes every frame of the floor film to data/output/floor_movie, chapter by chapter in the order the algorithm runs: a scene and its notes per frame, for docs/floor/render.py.
int main() {

    const std::filesystem::path dir = config::output_dir() / "floor_movie";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);


    const FloorGuide guide = FloorGuide::rectangle(3000.0, 3000.0);
    const FloorGuide tied = FloorGuide::rectangle(3000.0, 2400.0);

    Floor members(guide);
    members.add_members();

    Floor connected(guide);
    connected.add_members();
    connected.add_connectors();
    connected.add_screws();

    const Context context{guide, tied, members, connected, dir};
    chapter_00_vocabulary(context);

    return 0;
}
