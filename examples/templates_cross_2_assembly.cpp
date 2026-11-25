#include "wood_session.h"
#include "src/templates/cross/vda_mesh.h"

using namespace session_cpp;
using namespace wood_session;

/// The vda mesh on its default dome, its connectors turned so every plate slides onto all of them at once.
int main() {

    const Mesh mesh = wood_cross::VdaMesh::default_mesh();
    wood_cross::VdaMesh vda(
        mesh, 40.0, {0.0}, {2}, {}, wood_cross::VdaMesh::assembly_lines(mesh), 300.0, 300.0, 40.0
    );
    std::cout << vda << std::endl;
    vda.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
The vda mesh template on its default fifteen-face dome, made to go together: VdaMesh::assembly_order peels the mesh face by face, always the face its remaining neighbours leave the widest way out of in its own plane, and the assembly is that order reversed; VdaMesh::assembly_lines turns the connectors of every edge along the slide of its later face, so a plate slides onto the connectors of all its earlier neighbours along one vector. 40 thick plates, 300 x 300 x 40 connectors, two per edge, cross slots where they pass through the plates.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_cross_2_assembly --parallel 6 && ./build/templates_cross_2_assembly && ../bash/publish-scene.sh --target templates_cross_2_assembly

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
