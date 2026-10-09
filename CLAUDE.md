# wood — Agent Instructions

## House style

- Apply `../.claude/agents/session-reviewer.md` to all handwritten wood code. The user's wood scope
overrides that reviewer's default exclusion of wood. Kernel parity and kernel CI
steps apply only when changing the kernels; wood remains a C++ consumer.

- No function that does not earn its place: a helper exists only when it is used in more than one place or names a
  real step of the computation. Never one that only renames or wraps a single call, one called once that a reader
  would follow faster inline, or one kept "for later"; write the code where it is used instead.

- Use explicit types, one concept per file, normal multiline function bodies, short
functions and the standard section banners. Keep one header/source pair for Joint, JointPlate and JointBeam. Plate factories,
parameters, Annen and Vidy stay in the JointPlate pair; use sections and small
functions instead of files per factory or family. Expose library designs by their
actual names, such as `JointPlate::ts_e_p_3`, with their own parameters.
Preserve the user-owned TODO checklist in `examples/1_elements.cpp` and mark its
completion accurately. Generated protobuf files follow the generator's format.

## Class constructors

- A class that computes something has one clear constructor, and the constructor is the code flow: it takes the
  inputs (parameters as arguments with their defaults, kept as const fields) and runs the computation as a chain of
  short blocks, each one step, each under a one-line comment saying what it makes:

```cpp
    // construction planes, a pair per member
    for (size_t q = 0; q < 4; q++) {
        const ConstructionPlanes planes = compute_construction_planes(q);
        _construction_planes[q] = planes;
    }
```

- Each step's work lives in a private `compute_<result>` function; the constructor only calls them in order, stores
  each result in a named, fixed-size variable before keeping it, and reads top to bottom as the recipe. No separate
  `compute()`, no setters that ask for a recompute, no validity checks in between.
- Public methods only return what the constructor stored. The docs page of the class follows the same blocks, one
  snippet and one picture per block (`wood-agent`, Protocol 5).

## Using the session

- **The key pattern: a joint is an element, and what it does is an interaction.** Every connection between members
  is made the same way: a joint element class computes itself from the contact of the members it joins (a wedge, a
  plate, dowels, screws, a cross lap: where its parts and its dowels or screws go is the class's job, distributed over
  the contact, never computed in a template), it is added, and it is passed to each member with `add_interaction`:

```cpp
    const std::shared_ptr<JointBeam> dowels = JointBeam::dowels(*contact.a, *contact.b, *contact.face);
    add(dowels, group);
    add_interaction(dowels, contact.a, dowels->interaction(0));
    add_interaction(dowels, contact.b, dowels->interaction(1));
```

- A model is built with two calls: `add(element, group)` puts an element in the tree, and
  `add_interaction(source, target, interaction)` puts what the source does to the target on their edge: a contact,
  a glued block, a cut, a joint, a connector, screws. No wrapper adds an element and its effects in one call (no
  `add_joint`, `add_connector`, `add_column`-style helpers): the code shows every element and every effect.
- A joint, connector or screw set says what it does to its target i with `interaction(i)`, so it goes on each target
  in its own line, in the joint's target order:

```cpp
    add(wedge, connectors_group(q));
    add_interaction(wedge, seam.a, wedge->interaction(0));
    add_interaction(wedge, seam.b, wedge->interaction(1));
```

- An interaction belongs to its target, the element the source acts on: the target hosts the contact, the cut, the
  holes; the source keeps only what it is.
- An element is named where it is made, by its place (`connector_seam_wedge_<q>`, `inner_beams_<i>_<q>`), never
  numbered from the session afterwards.
- Colour and look belong to the element class (a connector and its parts and dowels are
  `JointBeam::CONNECTOR_COLOR`); a template does not paint nodes.
- The tree is the model's structure: groups by place and family (`quarter_q` > `outer_ribs_q` > `outer_ribs_<i>_<q>`),
  an element's own parts nested under it, its attributes (base plane) in its `attributes` group. Read elements back
  by name (`get_element_by_name`, `get_elements_numbered`) rather than keeping lists beside the session.

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
  - `docs/images/elements/<example>.png`: a screenshot of the real viewer by
    `bash wood/tools/screenshot_element_docs.sh <example> ...`: it publishes the example to the live
    viewer and opens it with `?cmd=Layers All;Element Interactions On;Element Attributes On;Arctic On;View Isometric;View
    Orthographic;Fit` in a Chrome window on the GPU (`tools/screenshot_viewer.mjs`, which starts
    Chrome with the radeon Vulkan env of the user's launcher; any other Chrome loses its WebGPU
    device). The layer panel is expanded, the base plane shows its pink, yellow-green and blue axes.
  - `docs/elements/element_<type>.md` (`{#elements_<type>}`, `[TOC]` under the title): one sentence
    on what it is, a `## Constructors` section with the constructors as a code block, then one `##`
    section per case with its picture, one line naming what it shows and the example by
    `\include{lineno} elements/<example>.cpp`; listed in the table and the subpages of
    `docs/elements.md`. The sections fill the page outline on the right.

## Before push to github

- Run each examples locally, if it is not working fix it and the push it.
- After push monitor if ci works.