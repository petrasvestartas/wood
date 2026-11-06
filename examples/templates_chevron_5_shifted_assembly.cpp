#include "wood_session.h"
#include "src/templates/chevron/chevron_shifted.h"
#include "src/templates/chevron/chevron_assembly.h"

using namespace session_cpp;
using namespace wood_session;

/// The shifted chevron shell on the first Annen surface assembled cell by cell: the order its tenons allow, every cell swept along its insertion vector, and the 4 x 4 block at the crown.
int main() {

    const std::filesystem::path path = config::session_data_dir() / "annen_surfaces.json";
    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces(path.string());
    ChevronShifted shifted(surfaces[0]);
    const wood_chevron::Assembly assembly(shifted);

    // the 4 x 4 block at the crown: the four middle columns, four cells about the middle of each, the odd columns stored from their far end
    const std::vector<int>& columns = shifted.cells().columns;
    const int column_count = shifted.point_count - 1;
    std::vector<std::vector<int>> per_column(column_count);

    for (size_t cell = 0; cell < columns.size(); cell++)
        per_column[columns[cell]].push_back(static_cast<int>(cell));

    std::vector<int> block;

    for (int column = column_count / 2 - 2; column < column_count / 2 + 2; column++) {
        const std::vector<int>& cells = per_column[column];
        const int middle = static_cast<int>(cells.size()) / 2;

        for (int row = middle - 2; row < middle + 2; row++)
            block.push_back(column % 2 == 0 ? cells[row] : cells[cells.size() - 1 - row]);
    }

    std::cout << assembly.report() << std::endl;
    std::cout << fmt::format("block: {}\n", wood_chevron::Assembly::groups(block));
    assembly.write_json(
        (std::filesystem::path(pb_path("live")).parent_path() / "assembly.json").string(), block
    );
    shifted.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The shifted chevron template on the first Annen surface assembled cell by cell: every cell's box slides in along its one insertion vector, the bisector of two of its own sides, into the boxes already placed; the order comes from the through-tenons, a box after the boxes whose walls its tenons enter along its vector and after the boxes whose tenons its own walls take along it, the odd columns after their even neighbours; every box is swept along its vector against the plates already placed, points just inside its faces and along its edges stepped along the way, and the report lists the plate pairs that meet with how deep. The 4 x 4 block at the crown, four cells of the four middle columns, is written with the order and the vectors to data/output/pb/assembly.json for tools/render_assembly_gif.py, which draws the block's assembly frame by frame.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_chevron_5_shifted_assembly --parallel 6 && ./build/templates_chevron_5_shifted_assembly && ../bash/publish-scene.sh --target templates_chevron_5_shifted_assembly

|||||||| TIME ||||||||
27.52 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
