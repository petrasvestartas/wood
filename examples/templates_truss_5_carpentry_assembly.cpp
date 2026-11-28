#include "wood_session.h"
#include "src/templates/truss/carpentry_truss.h"
#include "wood_assembly_sequence.h"

using namespace laika_cpp;
using namespace wood_session;

/// A king post truss of carpentry joints, its assembly found by taking it apart.
int main() {

    wood_truss::CarpentryTruss truss(wood_truss::TrussGuide(wood_truss::KingPostCarpentry{}));
    const AssemblySequence sequence(truss, "chord_0_0");
    std::cout << sequence.str() << std::endl;
    sequence.json_dump(std::filesystem::path(pb_path("joint_assembly")).replace_extension(".json").string());
    truss.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A king post truss drawn for carpentry joints (KingPostCarpentry: the tie running past the heels, the king post past the ridge, the struts seated on the king post) and built by CarpentryTruss: every pair of members classified from its axes, a notched oblique thrust joint (Stirnversatz) where an end meets a side at an angle, a blind mortise and tenon where it meets it square. AssemblySequence takes it apart member by member along the directions its joints give, each member swept clear of the ones still in, and prints the assembly: the tie first, then every member with the direction it goes in along.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_truss_5_carpentry_assembly --parallel 6 && ./build/templates_truss_5_carpentry_assembly && ../bash/publish-scene.sh --target templates_truss_5_carpentry_assembly

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
