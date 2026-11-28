#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace laika_cpp;
using namespace wood_session;

const double SPAN = 5400.0;
const double RISE = 1750.0;
const double DEPTH = 3000.0;
const int COURSES = 16;
const int RINGS = 6;
const double BEAM_HEIGHT = 150.0;
const double BEAM_WIDTH = 75.0;
const double PIN_DIAMETER = 12.0;

/// Leonardo da Vinci reciprocal barrel arch: 96 butt-joined bars inside a radial perimeter frame with centred BRep pins.
int main() {

    const wood_plaited::PlaitedGuide arch_guide(
        wood_plaited::arch_quad_mesh(SPAN, RISE, DEPTH, COURSES, RINGS),
        wood_plaited::PlaitedPattern::Abeille,
        BEAM_HEIGHT
    );
    wood_plaited::PlaitedReciprocal arch(
        arch_guide,
        BEAM_WIDTH,
        BEAM_HEIGHT,
        true,
        PIN_DIAMETER
    );

    std::cout << fmt::format(
        "plaited reciprocal arch: {} beams, {} frame beams, {} pin joints\n",
        arch.beams().size(),
        arch.frame_beams().size(),
        arch.pins().size()
    );
    arch.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Leonardo da Vinci's four-bar reciprocal system on a finely subdivided circular barrel arch (16 courses by 6 rings = 96 timber bars): each bar butts face-to-face against the cheeks of its crossing neighbours or the continuous radial perimeter frame and is fastened by centred cylindrical BRep pins.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_plaited_5_reciprocal_arch --parallel 6 && ./build/templates_plaited_5_reciprocal_arch && ../bash/publish-scene.sh --target templates_plaited_5_reciprocal_arch

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
