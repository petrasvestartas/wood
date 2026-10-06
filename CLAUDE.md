# wood — Agent Instructions

## House style

- Apply `../.claude/agents/session-reviewer.md` to all handwritten wood code. The user's wood scope
overrides that reviewer's default exclusion of wood. Kernel parity and kernel CI
steps apply only when changing the kernels; wood remains a C++ consumer.

- Use explicit types, one concept per file, normal multiline function bodies, short
functions and the standard section banners. Keep one header/source pair for Joint, JointPlate and JointBeam. Plate factories,
parameters, Annen and Vidy stay in the JointPlate pair; use sections and small
functions instead of files per factory or family. Expose library designs by their
actual names, such as `JointPlate::ts_e_p_3`, with their own parameters.
Preserve the user-owned TODO checklist in `examples/1_elements.cpp` and mark its
completion accurately. Generated protobuf files follow the generator's format.

## Kernel first

- Before writing a geometry or scene helper in wood, search the kernel headers
  (`../session/session_cpp/src/*.h`: Point, Vector, Line, Plane, Polyline, Mesh, Xform,
  Intersection, Closest, ConvexHull, BooleanPolyline, Session). If it exists, call it
  directly; never re-implement its math and never wrap it only to rename it.
- If the operation is missing and is not timber-specific, propose it for the kernel
  (cpp, py and rust together) instead of writing it in wood.
- A helper two files need lives once in the module's internal header, never as two
  `static` copies.


## Examples folder

- Examples files must be minimal.
- Where possible, the template classes must be WoodSession and examples must have no or very minimal creation of separate session just to dump geometry for file exchange and vizualization.
- Each example file must follow these instructions to configure cmake, build and publish to the viewer e.g.:
  - First includes:

#include "wood_session.h"
#include "src/templates/floor/floor.h"

  - Use namescapes of session_cpp and wood_session:

using namespace session_cpp;
using namespace wood_session;

  - Add a main function:

/// The guide of the square floor, drawn: every quarter's plan, construction quads and parabolas.
int main() {

    wood_floor::FloorGuide floor({
        Point(-3000.0, -3000.0, 0.0),
        Point(3000.0, -3000.0, 0.0),
        Point(3000.0, 3000.0, 0.0),
        Point(-3000.0, 3000.0, 0.0),
    });
    std::cout << floor << std::endl;
    floor.pb_dump(pb_path("live"));
    return 0;
}

/// At the end provide the description with the commands to configure, build and publish:

/*
|||||||| DESCRIPTION ||||||||
Step 1 of the timber floor: the guide alone, a FloorGuide on the corners of the 6000 x 6000 square with the default parameters, a session that draws its own construction: every quarter's plan polygon and column head with the oculus corners, the plan quad of every member at the floor datum, and the four rib parabolas of every quarter with their two t-section offsets, each kind in its group. No member is built; a Floor builds them from the guide.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_floor_1_floorguide --parallel 6 && ./build/templates_floor_1_floorguide && ../bash/publish-scene.sh --target templates_floor_1_floorguide

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/

## Elements: example, picture and page

- Every element class in `src/joinery_solver/wood_elements/` has all three, added or updated with
  every new element or constructor:
  - `examples/elements/element_<type>.cpp`: minimal like an API, one instance of the element and
    nothing else, registered as `ADD_EXE(element_<type> ...)`; a case that needs other elements (a
    column with glued blocks and cuts, a beam cut by plane elements) is its own example,
    `element_<type>_<case>.cpp`, showing them through `add_interaction(source, target, feature)`.
  - `docs/images/elements/<example>.png`: drawn by `python3 tools/render_element_docs.py` (listed
    in its `EXAMPLES`), which runs the example and renders its `live.pb` with session_viewer's
    selftest (`.claude/skills/wood-film-docs/templates/viewer_render_options.patch` applied): Arctic,
    opacity 0.75, the element features drawn, cropped, with a layer panel of the scene's elements,
    their features and the interaction features between them.
  - `docs/elements/element_<type>.md` (`{#elements_<type>}`, `[TOC]` under the title): one sentence
    on what it is, a `## Constructors` section with the constructors as a code block, then one `##`
    section per case with its picture, one line naming what it shows and the example by
    `\include{lineno} elements/<example>.cpp`; listed in the table and the subpages of
    `docs/elements.md`. The sections fill the page outline on the right.

## Before push to github

- Run each examples locally, if it is not working fix it and the push it.
- After push monitor if ci works.