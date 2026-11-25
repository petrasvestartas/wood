#include "wood_session.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Copies all inner, outer and border plates of pavilion translated by delta into grp.
void append_pavilion_copy(
    WoodSession& session,
    const std::shared_ptr<TreeNode>& grp,
    const wood_vidy::VidyPavilion& pavilion,
    const Vector& delta = {0.0, 0.0, 0.0}
) {

    std::map<std::string, std::string> copies;
    for (const std::shared_ptr<FacePlanarElement>& plate : pavilion.all_plates()) {
        std::shared_ptr<FacePlanarElement> copy =
            std::dynamic_pointer_cast<FacePlanarElement>(plate->clone());
        copies[plate->guid()] = copy->guid();

        if (!delta.is_zero())
            copy->place(Xform::translation(delta[0], delta[1], delta[2]));

        session.add(copy, grp);
    }

    // the pavilion's own assembly, on its copies where they stand
    if (delta.is_zero() && grp->name == pavilion.name)
        session.assemblies.push_back(pavilion.assemblies[0].of_copies(copies));
}

}

/// Parametric Vidy variations: compact symmetric 3-bay, tapered 5-bay hall and wide 4-bay arch.
int main() {

    const wood_vidy::VidyGuide g_compact(
        3,
        2400.0,
        12800.0,
        12800.0,
        480.0,
        4800.0,
        4800.0,
        7200.0,
        7200.0,
        3900.0,
        1850.0,
        0.0,
        40.0,
        240.0
    );
    const wood_vidy::VidyPavilion compact(g_compact, true, 1.0, 420.0, "compact_symmetric");

    const wood_vidy::VidyGuide g_tapered(
        5,
        2800.0,
        21000.0,
        14800.0,
        760.0,
        7800.0,
        6000.0,
        11600.0,
        9000.0,
        6200.0,
        2450.0,
        820.0,
        45.0,
        320.0
    );
    const wood_vidy::VidyPavilion tapered(g_tapered, true, 1.0, 500.0, "tapered_hall");

    const wood_vidy::VidyGuide g_wide(
        4,
        2500.0,
        22000.0,
        19500.0,
        580.0,
        5200.0,
        4800.0,
        8400.0,
        7800.0,
        7000.0,
        2350.0,
        0.0,
        45.0,
        280.0
    );
    const wood_vidy::VidyPavilion wide(g_wide, true, 1.0, 480.0, "wide_shallow");

    WoodSession scene("vidy_parametric");
    append_pavilion_copy(scene, scene.add_group("compact_symmetric"), compact);
    append_pavilion_copy(scene, scene.add_group("tapered_hall"), tapered);
    append_pavilion_copy(scene, scene.add_group("wide_shallow"), wide);

    const std::shared_ptr<TreeNode> family = scene.add_group("parametric_family");
    append_pavilion_copy(scene, family, compact, {-25000.0, 0.0, 0.0});
    append_pavilion_copy(scene, family, tapered, {0.0, 0.0, 0.0});
    append_pavilion_copy(scene, family, wide, {30000.0, 0.0, 0.0});

    std::cout << fmt::format(
        "vidy 5 parametric: compact={}/{}p, tapered={}/{}p, wide={}/{}p\n",
        compact.all_plates().size(),
        compact.total_pins(),
        tapered.all_plates().size(),
        tapered.total_pins(),
        wide.all_plates().size(),
        wide.total_pins()
    );
    scene.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vidy 5 parametric: compact 3-bay (12 m, 240 mm, 126 plates), tapered 5-bay (21/14.8 m, 320 mm, 210 plates), wide 4-bay (22 m, 280 mm, 168 plates).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_vidy_5_parametric --parallel 6 && ./build/templates_vidy_5_parametric && ../bash/publish-scene.sh --target templates_vidy_5_parametric

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
