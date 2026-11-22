#include "wood_session.h"
#include "wood_joint_library.h"
#include <fmt/ranges.h>

using namespace session_cpp;
using namespace wood_session;

/// The outlines of a tile's pairs as JSON: each pair its two outlines, each outline its points.
static nlohmann::ordered_json pairs_json(const std::vector<std::array<Polyline, 2>>& pairs) {

    nlohmann::ordered_json written = nlohmann::ordered_json::array();
    for (const std::array<Polyline, 2>& pair : pairs) {
        nlohmann::ordered_json both = nlohmann::ordered_json::array();
        for (const Polyline& outline : pair) {
            nlohmann::ordered_json points = nlohmann::ordered_json::array();
            for (const Point& point : outline.get_points())
                points.push_back({point[0], point[1], point[2]});
            both.push_back(points);
        }
        written.push_back(both);
    }

    return written;
}

/// Every joint of the YAML library as its joinery tile: per member its cuts as outline pairs in the unit box, then its parts.
int main() {

    nlohmann::ordered_json tiles;
    for (const std::string& design : library_designs()) {
        const LibraryTile tile = library_tile(design);
        nlohmann::ordered_json members = nlohmann::ordered_json::array();
        for (const std::vector<std::array<Polyline, 2>>& cuts : tile.cuts)
            members.push_back(pairs_json(cuts));
        tiles[design] = {{"members", members}, {"parts", pairs_json(tile.parts)}, {"unsupported", tile.unsupported}, {"size", {tile.size[0], tile.size[1], tile.size[2]}}, {"divisions", tile.divisions}, {"axes", tile.axes}};
        std::cout << fmt::format("{} {} pairs, {} unsupported, divisions {}", design, tile.cuts[0].size() + tile.cuts[1].size(), tile.unsupported, fmt::join(tile.divisions, " ")) << std::endl;
    }
    const std::string path = pb_path("joint_library_tiles");
    std::ofstream(path.substr(0, path.size() - 3) + ".json") << tiles.dump(1);

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Every joint of the beam joint library as its joinery tile: the wood each cut takes away from each member, the male a and the female b, as pairs of outlines (the two parallel faces of a prism the solver lofts) in the unit box of the joint, and its pegs, keys and wedges likewise, written to data/output/pb/joint_library_tiles.json for tools/render_joint_library.py.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_joint_beam_library_tiles --parallel 6 && ./build/element_joint_beam_library_tiles

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
