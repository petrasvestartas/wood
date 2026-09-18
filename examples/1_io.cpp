#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const int SESSION = 0;   // config::SESSION_NAMES
const std::string DATASET{config::Dataset::hex_block_rossiniere};  // config::Dataset::<name> autocompletes

int main() {

    WoodSession wood_session = WoodSession::pb_load(config::session_pb(SESSION));
    std::cout << wood_session << std::endl;
    wood_session.pb_dump(pb_path(config::SESSION_NAMES[SESSION]).string());

    WoodSession session_plates = WoodSession::yaml_load(DATASET);
    std::cout << session_plates << std::endl;
    session_plates.add_to_tree(true, true, false, false);
    session_plates.pb_dump(pb_path("live").string()); // Live will be seen in the viewer.

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
WoodSession serialization example.

|||||||| DIRECTORY ||||||||
cd wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --config Release --parallel && ./build/1_io && bash "$(git rev-parse --show-toplevel)/../bash/publish-scene.sh" --target 1_io

|||||||| WORKFLOW ||||||||
examples/1_io.cpp
 |
 |-- WoodSession::pb_load(name)                 src/joinery_solver/wood_session.cpp
 |    |-- register_element_types()               Plate/Column/Block::register_type      wood_element_*.cpp
 |    |-- config::dataset_path(name, ".pb")     wood_config.cpp    ->  data/<name>.pb
 |    '-- Session::pb_load(file)                  ../session/session_cpp/src/session.cpp
 |         '-- Element registry -> Plate::from_element (outline payload back to a Plate)
 |
 |-- operator<< / str()                          wood_session.cpp
 |
 |-- WoodSession::yaml_load(dataset)             wood_session.cpp
 |    |-- config::load_yaml(path)             wood_config.cpp   ->  data/<dataset>.yml (all tunables)
 |    |-- WoodSession::obj_load(obj)            config::load_obj -> data/<obj>.obj
 |    |    |-- file_obj::read_file_obj_polylines  ../session/session_cpp/src/file_obj.cpp
 |    |    '-- Plate(bottom, top)                 wood_element_plate.cpp (outlines and planes only, no loft)
 |    '-- WoodSession::add(plate)                 Session::add_element  (tree node + graph node)
 |
 |-- add_to_tree(geometry, outlines, contacts, joints)   wood_session.cpp
 |    '-- node_of, element_outlines, child_group  one group per plate: the plate, then "outlines"
 |
 '-- pb_dump(pb_path("live"))                    wood_session.cpp
      |-- sync_geometry() -> Plate::compute_geometry() -> Mesh::loft   (first and only loft, here)
      '-- Session::pb_dump(file)                  ../session/session_cpp  ->  data/output/pb/live.pb

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
