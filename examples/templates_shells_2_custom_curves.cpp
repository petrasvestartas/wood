#include "wood_session.h"
#include "src/templates/shells/translation_shell.h"

using namespace session_cpp;
using namespace wood_session;

/// A translation shell from a cross section and a profile of your own: the section swept along the profile, one mitered plate per quad.
int main() {

    const Polyline cross_section({
        {1200.0, 0.0,   0.0},
        { 900.0, 0.0, 260.0},
        { 600.0, 0.0, 360.0},
        { 300.0, 0.0, 260.0},
        {   0.0, 0.0,   0.0},
    });
    const Polyline profile({
        {0.0,     0.0,   0.0},
        {0.0,  -500.0, 240.0},
        {0.0, -1000.0, 340.0},
        {0.0, -1500.0, 240.0},
        {0.0, -2000.0,   0.0},
    });

    TranslationShell shell(cross_section, profile, 12.0, 1.5, 180.0, "custom_translation_shell");
    shell.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
A translation shell from your own cross section and profile curves: the section swept along the profile, one mitered plate per quad.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_shells_2_custom_curves --parallel 6 && ./build/templates_shells_2_custom_curves && ../bash/publish-scene.sh --target templates_shells_2_custom_curves

|||||||| TIME ||||||||
0.52 s, Release on Windows, measured 2026-11-06

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
