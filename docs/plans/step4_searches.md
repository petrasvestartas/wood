# Step 4: searches where the geometry is direct

The audit of every template (`src/templates/**`) and of `EdgeCurvedElement` / `EdgeVariableElement` for
marches, bisections, false-position loops and searches, done before any change. Each entry says what is
searched for, what the input really is, and whether the answer can be computed directly.

`EdgeVariableElement` has no search: it cuts its sections by planes and lofts them.

## Replaced: the input is simple

| Where | What it searches for | The input | Direct computation |
|---|---|---|---|
| `floor/floor_guide.cpp:665` scan, `:738` bisection | The horizontal sweep direction r of the central panel where its closure (the start and vertex chords, each shifted along r by a thickness over its face) is parallel: 340 half-degree steps, up to 200 bisections per bracket | Two face normals, two chords and a thickness: the closure numerator is a homogeneous quadratic in (cos, sin) of r's angle | Its roots in closed form, `R cos(2 phi - alpha) = -c`, the root nearest the reference kept |
| `floor/floor_guide.cpp:487` secant | The outer rib's start distance d at which its parabola meets the fan plane at the column level | The parabola is `Polyline::quadratic_points`: its vertex heights do not depend on d, only their spacing along the axis does; the fan plane is a line in the rib's vertical plane | The fan line at the level gives x; the polyline segment holding the level gives the fraction t at it; d from `x = d + (L - d) t` |
| `cylindrical_folding/curved_folded_beam.h:147` bisection | The point a thickness inside a circular profile above plan x | A circle: its inside offset is the concentric circle | The concentric circle's height at x |
| `cylindrical_folding/curved_folded_beam.h:225` bisection | The inner crease's point at a web station x | A NURBS curve and the plane x = station | `Intersection::curve_plane` |
| `cylindrical_folding/cylinder_vault.h:295` bisection | The angle where a generatrix inside the footprint is 1 mm long | A circle of generatrix points and vertical footprint planes: each length is `a sin + b cos + c` | The sinusoid's root |
| `cylindrical_folding/cylinder_vault.h:322-338` 1024-step scan, 50 bisections | The angles where a generatrix enters or leaves the footprint by another plane | Two planes' entry distances are equal where a sinusoid is zero | Every pair's root in the range, kept where the entry or exit plane really changes |
| `chevron/chevron_assembly.h:599-626` 10 mm march | The first step along the insertion vector where a sample point of a sliding plate is inside a placed plate (and back), for every sample, every step and every pair | A straight path against a prism between two parallel faces in an axis-aligned box: the path is inside the box and between the faces on one interval, two slab intersections | The interval first; the steps are visited only inside it, so the reported offsets, depths and counts on the 10 mm grid are the same |
| `vault/vault.cpp:665-673` sampled and interpolated | The panel's edge curve: the surface's image of a straight uv line, sampled at `subdivisions` points and interpolated | The web is a translational surface, cubic across, straight along: the image of a uv line is the profile cubic plus a linear shift along the generator | The cubic with the profile's control points shifted at their Greville abscissae, trimmed to the edge; a line along a generator |

## Kept: the input is general

| Where | What it searches for | Why it stays |
|---|---|---|
| `wood_element_edge_curved.cpp:38` false position, `:861-913` 8-step scans (`bisect_front`, `bisect_back`), `:1083` (`bisect_lap`) | Where a fibre of the beam crosses a cut plane | A fibre is the centreline offset in its frame; every centreline in the examples and templates is an interpolated NURBS (parabolic arches, vault ribs, S-curves), so a fibre is an offset of a general curve, not a NURBS the kernel can intersect |
| `wood_element_edge_curved.cpp:1032` 40-step bisection over v | Where across the width the winning front (or back) cutter changes from the left to the right one | The direct case, two cuts meeting in a line square to a planar beam's width, is already computed from the line (`:1021`); the remaining case is a twisted or 3D beam, whose faces are general |
| `vault/vault.cpp:582` 32 sweeps | The z control points of the cubic web profile that interpolate the circle at the Greville abscissae | The web must stay a polynomial cubic with x linear in u (the panels' uv lines rely on it), so the exact rational arc cannot replace it; the sweeps converge to the interpolant |
| `vault/vault.cpp:866` up to 800 sweeps | The least stop distances of the ribs at a corner head so no two collide | A fixed point of coupled inequalities between neighbours, a handful of ribs, done in a few sweeps |
| `cylindrical_folding/corner_fan.h:212` bisection | Where a fold's inner crease meets a plane | The inner crease is a parabola offset along a normalised, bisected normal: not polynomial |
| `cylindrical_folding/cylinder_carving.h:284-306` 1440-step march and 50 bisections, `:330` 1440 steps, `:417-436` scan and bisections | Where a carver's generator stops keeping a piece, and where what ends a piece changes | Generators against other cylinders with skew axes: the boundaries are quartic intersection curves |
| `chevron/chevron_shifted.h:496-591`, `reciprocal/reciprocal_surface.h:748, 843, 900` `closest_parameters` | A point's parameters on the design surface | The surfaces are the user's NURBS |

## Not searches

- `chevron/chevron.h:415`: the chevron rows grow by `scale` until the rest is below `stop_threshold`: the design rule.
- `vault/vault.cpp:1248`: the walk around a cell of the rib graph.
- `wood_element_edge_curved.cpp:740` (`max_curvature`): the maximum over the stations, not a root.
- `vault/vault.cpp:1130-1159`: the rib centreline sampled on the web and interpolated; the rib axis is half a rib height
  under the web along a tilted up, an offset curve, so it has no exact NURBS.

## Gate

Every example of `examples/` on `main` (`fda9a1ad`) and on the branch, one at a time, every written scene
compared element by element (same names and order): the stored BRep's volume and its vertex set (each
vertex to the nearest of the other). Two runs of `main`'s own code already differ by 1e-14 mm in vertices and
1e-16 relative in volume (`8_beams`, untouched), the noise floor.

| Example | BReps changed | Volume, relative | Vertices, mm | Why |
|---|---|---|---|---|
| `templates_floor_3_contacts`, `_7_contacts_cantilevers` | 24 of 284 | 1.7e-14 | 4.5e-13 | Rib starts and sweep exact instead of to the secant's and bisection's tolerance |
| `templates_floor_8_rectangle` | 41 of 282 | 3.5e-14 | 1.75 on `inner_beams_1_2`, 4.5e-13 elsewhere | The two moved vertices slide along their own straight edge, 1.75 mm further along it, with two more faces: a zero-volume sliver of that beam's boolean, tipped by the now exact rib starts (every other BRep of the floor moved by at most 4.5e-13 mm); the volume is the same to 1e-6 mm3 |
| `templates_cylindrical_folding_8_groin_vault`, `_9_pavilion_vault`, `_12_octagon_vault` | 4 of 4, 4 of 8, 8 of 8 | 1.1e-14 | 8.9e-11 | Plane changes and drawn-in ends at the exact sinusoid roots; distortion and ruling twist in the report change in their noise digits (1e-14, 1e-9) |
| `templates_cylindrical_folding_13_curved_folded_beam` | 54 of 54 | 1.2e-13 | 3.0e-11 | The inner circle and the web stations exact |
| `templates_vault_9_gothic_ribbed_vault` | 155 of 328, all panels | 1.5e-5 by `BRep::volume`, 2.4e-14 by `BRep::volume_exact` | 0 | The panel edges now lie on the web within 1e-11 mm, the interpolated ones were 2.0e-4 mm off it; `BRep::volume` measures a tessellation, which an edge that is now a straight line along a generator instead of a 27-point cubic triangulates differently; the exact volume is unchanged, every panel its plan area times 35 mm |

Every other scene is identical in volume and vertices; stdout is identical once GUIDs are masked and lines sorted
(the threaded contact listings print each pair in either order). The chevron assembly reports are identical.
`ctest`: 15 of 15 pass; `joint_library` 124 of 135 and `reference_datasets` 48 of 49, as on `main`.

## Times

Each example alone, Release on Windows, while other agents built on the same machine, so a time moves by up to
a factor of two between runs: `main` once, the branch the faster of two runs. Every example records its time in
its footer (`|||||||| TIME ||||||||`).

| | `main` | Branch |
|---|---|---|
| All 189 examples | 1261 s | 593 s |
| Examples under a second | 50 | 42 |
| `templates_chevron_4_chevron_assembly` | 169.4 s | 24.9 s |
| `templates_chevron_5_shifted_assembly` | 207.1 s | 27.5 s |
| `templates_chevron_6_max2_assembly` | 208.9 s | 30.1 s |

The assembly's march was the only search that cost time; it now spends what the chevron template itself takes
(15 to 21 s). Every other replaced search took microseconds, so the floor, vault and cylindrical examples run as
before within the noise. The goal of every example under a second is not met: 147 examples take longer, and none of
that time is a template search. Where it goes was not profiled; even `two_plates`, two plates and their joint,
takes half a second.
