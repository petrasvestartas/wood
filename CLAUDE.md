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
  plate, pins, a cross lap: where its parts and its pins go is the class's job, distributed over
  the contact, never computed in a template), it is added, and it is passed to each member with `add_interaction`:

```cpp
    const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*contact.a, *contact.b, *contact.face);
    add(pins, group);
    add_interaction(pins, contact.a, pins->interaction(0));
    add_interaction(pins, contact.b, pins->interaction(1));
```

- A model is built with two calls: `add(element, group)` puts an element in the tree, and
  `add_interaction(source, target, interaction)` puts what the source does to the target on their edge: a contact,
  a glued block, a cut, a joint, a connector, pins. No wrapper adds an element and its effects in one call (no
  `add_joint`, `add_connector`, `add_column`-style helpers): the code shows every element and every effect.
- A joint, connector or pin set says what it does to its target i with `interaction(i)`, so it goes on each target
  in its own line, in the joint's target order:

```cpp
    add(wedge, connectors_group(q));
    add_interaction(wedge, seam.a, wedge->interaction(0));
    add_interaction(wedge, seam.b, wedge->interaction(1));
```

- Every screw, dowel or pin is a `Pin`: one cylinder along its axis, a child of its connector. No class, factory,
  field or element name says screw or dowel; they say pin. A connector of pins is a `JointBeam` made by
  `JointBeam::centred_pins` (centred across the contact, bored into both members) or `JointBeam::headed_pins`
  (from a head on the far face of the first member, pre-drilled), its pins laid out on the contact by `PinLayout`.
- An interaction belongs to its target, the element the source acts on: the target hosts the contact, the cut, the
  holes; the source keeps only what it is.
- An element is named where it is made, by its place (`connector_seam_wedge_<q>`, `inner_beams_<i>_<q>`), never
  numbered from the session afterwards.
- Colour and look belong to the element class (a connector and its parts and pins are
  `JointBeam::CONNECTOR_COLOR`); a template does not paint nodes.
- The tree is the model's structure: groups by place and family (`quarter_q` > `outer_ribs_q` > `outer_ribs_<i>_<q>`),
  an element's own parts nested under it, its attributes (base plane) in its `attributes` group. Read elements back
  by name (`get_element_by_name`, `get_elements_numbered`) rather than keeping lists beside the session.

## Templates: the floor pattern

`src/templates/floor` is the reference; every template that builds a model follows it.

- A template is a `WoodSession` subclass built from a guide (`class Floor : public WoodSession`). The guide holds the
  geometry (planes, loops, sizes); the template only turns it into elements and joints. Its fields are the guide and
  `static constexpr` parameters; every element lives in the session, read back by name.
- The constructor is the recipe: members, then contacts, then connectors, nothing else:

```cpp
Floor::Floor(const FloorGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // quarters: every quarter's members, lifted to bay_height and grouped by family
    add_quarters();

    // oculus: the four ring beams, the oculus beams, the bottom wedges and the central plate
    add_oculus();

    // columns: the column at every corner, its head carved by the guide's cutters
    add_columns();

    // contacts: per quarter an interaction between every two members that touch, named by its kind and place
    const std::array<QuarterContacts, 4> contacts = add_contacts();

    // connectors: per quarter its wedges, column plates with their cross lap, centred and headed pins, all built on uncut members
    const std::array<QuarterConnectors, 4> connectors = compute_connectors(contacts);
    add_connectors(connectors, contacts);
}
```

- **Members**: `add_<part>()` adds every element straight into this session under its group: no free function
  returning a session to graft, no sub-session. A member shaped by others (the column: glued blocks, the support seat,
  cutters) is built in its own `add_<part>(i)` with `add` and `add_interaction`. One part alone is read back with
  `get_branch("<group>")`.
- **Groups**: by place, then family, then element: `quarter_q` > `outer_ribs_q` > `outer_ribs_<i>_<q>`, with
  `connectors_q` beside the families. Group functions (`quarter_group(q)`, `connectors_group(q)`) call `group_named`,
  which makes the group on first use.
- **Contacts**: `add_contacts()` finds every contact the design needs (`compute_face_contact`), names it
  `<kind>_<q>_<k>`, stores it with `add_interaction(a, b, face)` and returns them in a fixed-size struct per place
  (`QuarterContacts`). Each field is a `Contact {a, b, face}` named after its two members (`seam_beam_oculus_beam`).
  `a` is the source (the member a pin passes through first) and `b` the target.
- **Connectors**: `compute_connectors(contacts) const` builds each joint element from its contact with a factory of
  the joint class (`JointBeam::wedge`, `rectangle_plate`, `centred_pins`, `headed_pins`), on uncut members, before any
  is added. The result struct (`QuarterConnectors`) mirrors the contacts struct field for field. A factory returning
  null throws, naming the contact. Names are given at the end of the function as `connector_<kind>_<n>`, numbered in
  the order `add_connectors` adds them, so `get_elements_numbered` reads them back in that order.
- **Adding**: `add_connectors(connectors, contacts)` has one explicit block per kind (kind by kind, quarter by
  quarter): `add(joint, group)`, then one `add_interaction(joint, target, joint->interaction(i))` per target, in
  target order. No loop over a table of kinds. Joints that act on other joints (the cross lap on the column plates)
  come last.
- **Names in the code**: a local says what it holds (`quarter_contacts`, `quarter_connectors`, `seam`, `outer`,
  `pins`), never `c`, `made`, `tmp` or `result`.
- **Tests**: read elements back by name and check measured numbers in the message (`20 mm below the rib top`, with a
  tolerance), never through lists kept beside the session.

## Round means BRep

- Every pin, bore, drill and round cut is shown as an exact BRep, never a mesh: a mesh makes them polygons. This
  mistake was made again and again in October 2026 (published scenes, docs pictures, element screenshots); it is the
  first thing to check.
- Scenes: `WoodSession::pb_dump` runs `compute_breps` first; never write or publish a scene any other way.
- Docs pictures: draw members with `Frame::element` or `Frame::brep` (the exact BRep); `Frame::mesh` only for something
  that has no BRep.
- Before calling a picture, a screenshot or a published scene done, look at it: a pin or hole with flat facets is a bug.

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