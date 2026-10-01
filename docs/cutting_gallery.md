# Cutting examples for the viewer

| Target | Cases |
| --- | --- |
| `13_profile_cuts` | Concave L profile, profile with a hole, a slot producing two separate pieces, concave pocket |
| `14_drill_solids` | Radius 35 with chord tolerances 2, 0.2 and 0.02; exact cylindrical BReps; a tilted radius 28 drill through a block |
| `15_solid_cuts` | Difference, intersection and union of sloped solids; arbitrary tetrahedral mesh cutter |
| `16_cutting_gallery` | Loads the three saved scenes and places them side by side |
| `17_plate_joint_library` | Six side-to-top joints created by named library factories |

Each cutting row reads **stock → cutter → result** from left to right. The cutter
is moved to the middle column after application for display. The target stores
its own applied cutter definition, so this display move does not change the cut.
Reapplying that moved joint would use its new location.

## Generate

Run from `wood/`. The combined gallery depends on the three named scene files.

```bash
~/.local/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
buildslot ~/.local/bin/cmake --build build --target 13_profile_cuts 14_drill_solids 15_solid_cuts 16_cutting_gallery --parallel 4
tools/run_guarded.sh -t 10 -m 4 -- build/13_profile_cuts
tools/run_guarded.sh -t 10 -m 4 -- build/14_drill_solids
tools/run_guarded.sh -t 10 -m 4 -- build/15_solid_cuts
tools/run_guarded.sh -t 10 -m 4 -- build/16_cutting_gallery
```

Files are written to `data/output/pb/<target>.pb`. Each example also writes
`live.pb`; running the combined gallery last leaves it in that slot.

## Publish to Cloudflare

The existing script publishes one file to the viewer's live slot. Publishing
replaces that slot. The named local scene files remain available.

```bash
../bash/publish-scene.sh "$PWD/data/output/pb/16_cutting_gallery.pb" --no-notify
```

Use another named scene file, including `17_plate_joint_library.pb`, to show that scene. Passing an explicit
file uses the already generated artifact, with no unguarded solver invocation.
Open [the viewer](https://petrasvestartas.github.io/session/).

## API and geometry

```cpp
auto profile = std::make_shared<Joint>(
    std::vector<Polyline>{outer, hole}, extrusion, SolidOperation::intersection);
auto drill = Joint::drill(axis, radius, chord_tolerance);
auto solid = std::make_shared<Joint>(closed_mesh, SolidOperation::difference);
auto stock = std::make_shared<Block>(stock_mesh);
profile->targets = {plate->guid()};
scene.add_joint(profile);
```

The two-argument `Joint(profile, extrusion)` keeps the profile's interior.
Profiles must be simple, closed, coplanar rings, perpendicular to their extrusion.
The first ring is the exterior and subsequent rings are holes. Concave rings are
supported. The extrusion is finite, so its start and end control through cuts
versus pockets.

Compatible through cuts use a planar polygon boolean and loft each resulting
region, including holes and disconnected pieces. Other closed polyhedral solids
go to [Manifold](https://github.com/elalish/manifold) (v3.5.4, double precision),
which always returns a closed solid and rejects an input that is not one. The
difference cuts of an element that follow each other are subtracted in one batch
and only the largest resulting solid is kept, so offcuts a cut separates fall away,
as compas_tf does; intersections and unions run one at a time and keep every piece.
The triangles Manifold returns are merged back into one polygon per input face, so
contact detection still sees whole faces.

Drill mesh sections are chosen from the radius and maximum chord deviation
(default `0.05` model units); there is no fixed 16-side limit. The minimum is eight
segments and requests exceeding 65,536 segments are rejected. Drill BReps use an
exact NURBS cylindrical surface with circular edges. Rigid transforms and uniform
scaling preserve this representation; nonuniform scaling of a drill is rejected.

General solid operations operate on meshes, so the mesh of a drilled element has
faceted bores. Drills are kept on the element's cut as axes, though, and its BRep
makes them exact: every planar face a drill crosses gets an exact circle or
ellipse loop, the bore is an exact cylindrical face between them, and a drill that
ends inside the solid gets a flat bottom. This needs each bore to stay clear of
every edge and every other drill (the two stretches of one drill across a slot are
fine); where it does not, the BRep is the faceted one of the mesh. The cutter
itself keeps its exact cylindrical BRep.

Applied cutters, operations, profiles, extrusion vectors and tolerance are stored
on Plate, Beam, Column and Block in protobuf. Original element geometry, including an imported Block source mesh, remains
available separately from model geometry. Reapplying a joint updates its stored
cut; removing its interaction removes the solid cut. Legacy plane cuts retain
their existing behavior.
