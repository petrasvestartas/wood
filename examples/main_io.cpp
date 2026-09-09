#include "wood_session.h"
#include "../src/session.h"

const char* DATASET = "data/floor_model.pb";
const char* DIR = "data/";

const std::vector<std::string> DATASET_NAMES = {
    "annen_box",
    "annen_box_pair",
    "annen_corner",
    "annen_grid_full_arch",
    "annen_grid_small",
    "cross_and_sides_corner",
    "cross_brussels_sports_tower",
    "cross_corners",
    "cross_ibois_pavilion",
    "cross_square_reciprocal_iseya",
    "cross_square_reciprocal_two_sides",
    "cross_vda_corner",
    "cross_vda_hexshell",
    "cross_vda_hexshell_reciprocal",
    "cross_vda_shell", // slows down the viewer
    "cross_vda_single_arch",
    "hello",
    "hex_block_rossiniere",
    "hexbox_and_corner",
    "hexboxes",
    "inplane_butterflies",
    "inplane_differentdirections",
    "inplane_hexshell",
    "inplane_hilti",
    "outofplane_box",
    "outofplane_box_miter",
    "outofplane_dodecahedron",
    "outofplane_icosahedron",
    "outofplane_octahedron",
    "outofplane_tetra",
    "phanomema_node",
    "simple_corners",
    "simple_corners_combined",
    "simple_corners_diff_lengths",
    "top_to_side_box",
    "top_to_side_corners",
    "top_to_top_pairs",
    "vda_floor_0",
    "vda_floor_2",
    "vidy_corner",
    "vidy_folding",
    "vidy_full",
    "vidy_one_axis_two_layers",
    "vidy_one_layer",
    "top_to_side_snap_fit",
    "top_to_side_test",
    "vda_floor_1",
    "cross_brg_slab_0",
};


int main() {
    const std::shared_ptr<session_cpp::Session> session = session_cpp::Session::pb_load(DATASET);
    const wood_session::WoodSession scene = wood_session::WoodSession::from_session(session);
    wood_session::pb_dump(*scene.to_session(), "live");

    const wood_session::WoodSession plates = wood_session::WoodSession::yaml_load(DIR + DATASET_NAMES[14] + ".yml");
    std::cout << "Loaded WoodSession from YAML: " << DATASET_NAMES[14] << std::endl;
    const std::shared_ptr<session_cpp::Session> plates_session = plates.to_session();
    wood_session::pb_dump(*plates_session, "live");
    return 0;
}

/*
description: load a .pb session -> convert to WoodSession -> dump to "live.pb"; load a dataset .yml (obj + txt files it names) as a WoodSession -> its session -> dump to "hexboxes.pb".

directory: cd ~/code/code_cpp/wood_research/wood
run: cmake --build build --target main_io -j8 && ./build/main_io
cloudflare: ../bash/publish-scene.sh --target main_io
view: https://petrasvestartas.github.io/session/
*/
