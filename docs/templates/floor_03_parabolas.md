# Floor 03: Construction quads, run-in and parabolas {#templates_floor_03_parabolas}

This chapter follows `compute_quarter` in `src/templates/floor/floor.cpp` (lines 360-375) from the first `construction_quads` call to `boundary_parabolas`. It takes the construction planes of chapter 2 (`geometry.planes`, whose block far faces `cp.wedges[i][1]` are still the provisional ones from `wedge_fan`) and the column corner. It gives the plan quads `geometry.quads`, the solved run-ins `geometry.run_in`, the final block far faces in `cp.wedges[i][1]` and `column.wedge_fan[i][1]`, and the four `geometry.parabolas` with their +t and +2t layers, which chapter 4 reads to solve the central panel. The pictures show quarter 0 of the 6000 x 6000 bay `FloorGuide::rectangle(3000.0, 3000.0)`, except frames 43 and 44, which use `FloorGuide::rectangle(3000.0, 2400.0)`, a 6000 x 4800 bay called the 3000 x 2400 bay below after its half spans, because only there does the run-in solve change anything. Its quarter 0 spans x from -3000 to 0 and y from -2400 to 0.

Example: [templates_floor_1_floorguide.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_1_floorguide.cpp) draws this chapter's result on the square bay, quarter 0's plan quads at the datum and the three parabolas of every rib; [templates_floor_8_rectangle.cpp](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/examples/templates_floor_8_rectangle.cpp) builds the 3000 x 2400 bay, whose short outer ribs end at the shared level with their run-in solved to 187.667.

![](floor/film_03_parabolas.webp)

The order of the calls in `compute_quarter` (floor.cpp:366-374):

```mermaid
flowchart LR
    A["construction_planes"] --> B["column_seats"]
    B --> C["construction_quads<br/>first pass, 368"]
    C --> D["run_ins, 369"]
    D --> E["block_planes, 370"]
    E --> F["construction_quads<br/>second pass, 371"]
    F --> G["boundary_parabolas, 372"]
    G --> H["central_panel<br/>chapter 4"]
```

## 35. quad() rule and outer rib quads (first pass)

![](floor/035_outer_rib_quads.webp)

<span style="color:#2196EA">■ built</span> `quads.outer_ribs[0]`, `quads.outer_ribs[1]` and the corners of [0]   <span style="color:#EB7721">■ variable</span> the rib axis `get_point(0)` -> `get_point(1)`   <span style="color:#455B6B">■ input</span> datum traces of the four planes of `quads.outer_ribs[0]`, dashed   <span style="color:#8C969E">■ context</span> quarter 0 polygon

Every member footprint in the quarter comes from `quad(planes)`, which takes four planes. It intersects pairs of them with the datum plane `xy = level(0.0)` by `plane_plane_plane`: <span style="color:#2196EA">corner 0</span> = `planes[0]` x `planes[3]`, <span style="color:#2196EA">corner 1</span> = `planes[0]` x `planes[1]`, <span style="color:#2196EA">corner 2</span> = `planes[1]` x `planes[2]`, <span style="color:#2196EA">corner 3</span> = `planes[2]` x `planes[3]`. `planes[0]` and `planes[2]` are the member's two faces and `planes[1]` and `planes[3]` are the planes it ends on, so side 0-1 lies on the first face, side 2-3 on the second face, and sides 1-2 and 3-0 on the two end planes. `quads()` applies `quad` to every 4-plane array of a family. `quad` calls `.value()` on each optional corner, so two parallel planes in one array throw `std::bad_optional_access` and are not reported as an error of their own. <span style="color:#2196EA">Outer rib k</span> uses <span style="color:#455B6B">`{cp.outer_ribs[k][0], cp.inner_beams[0 or 2][0], cp.outer_ribs[k][1], cp.wedges[0 or 2][0]}`</span>: the bay-edge face, the seam plane, the inner face and the fan plane. So <span style="color:#EB7721">`get_point(0)`</span> is where the bay-edge face meets the fan plane at the datum and <span style="color:#EB7721">`get_point(1)`</span> is where it meets the seam plane. That pair is the <span style="color:#EB7721">rib axis</span> `outer_parabola` uses in frame 40.

```cpp
const Plane xy = level(0.0);
return Polyline({
    plane_plane_plane(xy, planes[0], planes[3]).value(),
    plane_plane_plane(xy, planes[0], planes[1]).value(),
    plane_plane_plane(xy, planes[1], planes[2]).value(),
    plane_plane_plane(xy, planes[2], planes[3]).value(),
});
```

| Variable | Value | Meaning |
|---|---|---|
| `xy` | `level(0.0)`, z = 0 | The datum every quad is cut on. |
| `quads.outer_ribs[0]` | (-2780,-3000), (0,-3000), (0,-2900), (-2780,-2900) | Outer rib 0 footprint. Its axis `get_point(0)` to `get_point(1)` is 2780 long. |
| `quads.outer_ribs[1]` | (-3000,-2780), (-3000,0), (-2900,0), (-2900,-2780) | Outer rib 1 footprint, the mirror in x = y. |

Code: `quad`, `quads`, `construction_quads`, [floor.cpp:25-46](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L25-L46), [218-225](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L218-L225) (called at 368).

## 36. Inner beam quads

![](floor/036_inner_beam_quads.webp)

<span style="color:#2196EA">■ built</span> `quads.inner_beams[0]`, `[1]`, `[2]`   <span style="color:#8C969E">■ context</span> quarter 0 polygon, outer rib quads

The three inner beams use the same rule. <span style="color:#2196EA">Seam beam 0</span> is `quad({inner_beams[0][0], inner_beams[1][0], inner_beams[0][1], outer_ribs[0][1]})`: it lies between the seam plane and its offset by `inner_beams` = 60 and runs from the outer rib's inner face to the tilted oculus plane. <span style="color:#2196EA">The oculus beam</span> is `quad({inner_beams[1][0], inner_beams[2][1], inner_beams[1][1], inner_beams[0][1]})`: it lies between the tilted bearing plane `oculus.tilted` and the vertical back face `oculus.back`, and it ends on the far faces of the two seam beams. The tilted plane was rotated about the oculus edge, so its datum trace is the oculus edge itself, and corner 1 of seam beam 0 is the oculus corner (0,-1000). <span style="color:#2196EA">Seam beam 2</span> is `quad({inner_beams[2][0], outer_ribs[1][1], inner_beams[2][1], inner_beams[1][0]})`. These quads read no wedge plane, so the second pass (frame 45) gives the same quads.

```cpp
result.inner_beams = quads({
    {cp.inner_beams[0][0], cp.inner_beams[1][0], cp.inner_beams[0][1], cp.outer_ribs[0][1]},
    {cp.inner_beams[1][0], cp.inner_beams[2][1], cp.inner_beams[1][1], cp.inner_beams[0][1]},
    {cp.inner_beams[2][0], cp.outer_ribs[1][1], cp.inner_beams[2][1], cp.inner_beams[1][0]},
});
```

| Variable | Value | Meaning |
|---|---|---|
| `quads.inner_beams[0]` | (0,-2900), (0,-1000), (-60,-940), (-60,-2900) | Seam beam 0. |
| `quads.inner_beams[1]` | (-60,-940), (-940,-60), (-1024.853,-60), (-60,-1024.853) | Oculus beam, from the oculus edge to the back face, 60 behind it along its normal (84.853 along x or y). |
| `quads.inner_beams[2]` | (-1000,0), (-2900,0), (-2900,-60), (-940,-60) | Seam beam 2, on seam 3 read into quarter 0. |

Code: `construction_quads`, [floor.cpp:226-230](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L226-L230).

## 37. Inner rib quads

![](floor/037_inner_rib_quads.webp)

<span style="color:#2196EA">■ built</span> `quads.inner_ribs[0]`, `quads.inner_ribs[1]`   <span style="color:#EB7721">■ variable</span> corner 2 = `p0` / `p1`, corner 3 = `head[2]` / `head[3]`   <span style="color:#455B6B">■ input</span> column head `columns[0].head`   <span style="color:#8C969E">■ context</span> quarter 0 polygon, outer rib and inner beam quads

<span style="color:#2196EA">Inner rib k</span> is `quad({inner_ribs[k][1], inner_beams[1][1], inner_ribs[k][0], wedges[1][0]})`. Its two faces are the central face `inner_ribs[k][1]` and the outer face `inner_ribs[k][0]`, and it runs from the tilted chamfer plane `wedges[1][0]` to the oculus back face `inner_beams[1][1]`. In `construction_planes` (floor.cpp:194-200) the outer face of rib 0 was built through <span style="color:#EB7721">`p2 = head[2]`</span> and <span style="color:#EB7721">`p0`</span> `= plane_plane_plane(xy, cp.inner_beams[0][1], cp.inner_beams[1][1])`, the datum point where the far face of seam beam 0 meets the oculus back face. So corner 2 (`inner_beams[1][1]` x `inner_ribs[0][0]`) is exactly <span style="color:#EB7721">`p0`</span>. Corner 3 (`inner_ribs[0][0]` x `wedges[1][0]`) is exactly <span style="color:#EB7721">`head[2]`</span>, because the tilted chamfer plane was rotated about <span style="color:#455B6B">head</span> edge 2 (from <span style="color:#EB7721">`head[2]`</span> to <span style="color:#EB7721">`head[3]`</span>) and keeps that edge as its datum trace. Rib 1 is the same with <span style="color:#EB7721">`p1`</span> and <span style="color:#EB7721">`head[3]`</span>. Corners 0 and 1 are where the central face, the outer face offset by `inner_ribs` = 60, meets the same two end planes.

```cpp
result.inner_ribs = quads({
    {cp.inner_ribs[0][1], cp.inner_beams[1][1], cp.inner_ribs[0][0], cp.wedges[1][0]},
    {cp.inner_ribs[1][1], cp.inner_beams[1][1], cp.inner_ribs[1][0], cp.wedges[1][0]},
});
```

| Variable | Value | Meaning |
|---|---|---|
| `quads.inner_ribs[0]` | (-2823.178,-2836.822), (-103.178,-981.675), (-60,-1024.853), (-2780,-2880) | Inner rib 0: corners 0, 1 on the central face, 2 = `p0`, 3 = `head[2]`. |
| `quads.inner_ribs[1]` | (-2836.822,-2823.178), (-981.675,-103.178), (-1024.853,-60), (-2880,-2780) | Inner rib 1, the mirror in x = y: corner 2 = `p1`, 3 = `head[3]`. |
| `p0` | (-60,-1024.853) | Far face of seam beam 0 x oculus back face at the datum. |
| `head[2]` | (-2780,-2880) | First chamfer vertex of the column head. |

Code: `construction_quads`, [floor.cpp:231-234](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L231-L234).

## 38. Wedge quads (provisional)

![](floor/038_wedge_quads.webp)

<span style="color:#2196EA">■ built</span> `quads.wedges[0]`, `[1]`, `[2]`, first pass   <span style="color:#455B6B">■ input</span> provisional far faces `wedges[i][1]`, dashed   <span style="color:#8C969E">■ context</span> outer and inner rib quads, column head

<span style="color:#2196EA">Block i</span> is `quad({wedges[i][0], side face, wedges[i][1], other side face})`. Block 0 lies between `outer_ribs[0][1]` and `inner_ribs[0][0]`, block 1 (the middle block) between `inner_ribs[0][1]` and `inner_ribs[1][1]`, and block 2 between `inner_ribs[1][0]` and `outer_ribs[1][1]`. Each runs from its fan plane `wedges[i][0]` to its far face <span style="color:#455B6B">`wedges[i][1]`</span>. In this first pass the far faces are still <span style="color:#455B6B">the provisional ones</span> `wedge_fan` made in chapter 2: `pair(wedge0, wedge)`, `pair(tilted, wedge * middle_wedge_factor)` and `pair(wedge2, wedge)`, which are 240, 300 and 240 along the fan normals. The fan planes lean, so in plan these are 242.6, 304.6 and 242.6 apart. No code reads these first-pass wedge quads: `run_ins` reads only `quads.outer_ribs`, and the second pass at floor.cpp:371 overwrites them.

```cpp
result.wedges = quads({
    {cp.wedges[0][0], cp.outer_ribs[0][1], cp.wedges[0][1], cp.inner_ribs[0][0]},
    {cp.wedges[1][0], cp.inner_ribs[0][1], cp.wedges[1][1], cp.inner_ribs[1][1]},
    {cp.wedges[2][0], cp.inner_ribs[1][0], cp.wedges[2][1], cp.outer_ribs[1][1]},
});
```

| Variable | Value | Meaning |
|---|---|---|
| `quads.wedges[0]` | (-2780,-2880), (-2780,-2900), (-2537.377,-2900), (-2537.377,-2714.522) | Side block 0, on its provisional far face. |
| `quads.wedges[1]` | (-2836.822,-2823.178), (-2823.178,-2836.822), (-2567.055,-2662.136), (-2662.136,-2567.055) | Middle block. |
| `quads.wedges[2]` | (-2900,-2780), (-2880,-2780), (-2714.522,-2537.377), (-2900,-2537.377) | Side block 2. |
| `wedge` | 240 | Provisional side block thickness along the normal. |
| `middle_wedge_factor` | 1.25 | Provisional middle block thickness in `wedge`: 300. |

Code: `construction_quads`, [floor.cpp:235-239](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L235-L239); provisional far faces `wedge_fan`, [floor.cpp:158](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L158).

## 39. T-section quads

![](floor/039_tsection_quads.webp)

<span style="color:#2196EA">■ built</span> `quads.tsections[0..5]`   <span style="color:#455B6B">■ input</span> block end, side 3-0 on the provisional `wedges[i][1]`, bold   <span style="color:#8C969E">■ context</span> rib, beam and first-pass wedge quads

<span style="color:#2196EA">T-section i</span> is `quad({tsections[i][0], beam far face, tsections[i][1], block far face})`. The `tsections` plane pairs are a rib face and its offset by `tsections` = 27 (chapter 2), so every flange quad is 27 wide. Flanges 0 and 1 run from `inner_beams[0][1]` to <span style="color:#455B6B">`wedges[0][1]`</span>, flanges 2 and 3 from `inner_beams[1][1]` to <span style="color:#455B6B">`wedges[1][1]`</span>, and flanges 4 and 5 from `inner_beams[2][1]` to <span style="color:#455B6B">`wedges[2][1]`</span>. <span style="color:#455B6B">Side 3-0</span> of each flange lies on its block's far face (bold in the picture). These quads therefore depend on the provisional far faces as well, and the second pass must rebuild them.

```cpp
result.tsections = quads({
    {cp.tsections[0][0], cp.inner_beams[0][1], cp.tsections[0][1], cp.wedges[0][1]},
    {cp.tsections[1][0], cp.inner_beams[0][1], cp.tsections[1][1], cp.wedges[0][1]},
    {cp.tsections[2][0], cp.inner_beams[1][1], cp.tsections[2][1], cp.wedges[1][1]},
    // 3 to 5: the same on inner rib 1 and outer rib 1
});
```

| Variable | Value | Meaning |
|---|---|---|
| `quads.tsections[0]` | (-2537.377,-2900), (-60,-2900), (-60,-2873), (-2537.377,-2873) | Flange beside outer rib 0. |
| `quads.tsections[1]` | (-2537.377,-2714.522), (-60,-1024.853), (-60,-1057.535), (-2537.377,-2747.204) | Flange on the outer side of inner rib 0. |
| `quads.tsections[2]` | (-2567.055,-2662.136), (-103.178,-981.675), (-122.608,-962.245), (-2586.485,-2642.706) | Flange on the central side of inner rib 0. |
| `quads.tsections[3..5]` | mirrors of [2], [1], [0] in x = y | Inner rib 1 central, inner rib 1 outer, outer rib 1. |
| `tsections` | 27 | Flange width. |

Code: `construction_quads`, [floor.cpp:240-247](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L240-L247).

## 40. Outer parabola at the trial run-in

![](floor/040_outer_parabola.webp)

<span style="color:#2196EA">■ built</span> soffit `outer_parabola(quads.outer_ribs[0], 240)` and its 7 points   <span style="color:#EB7721">■ variable</span> `run_in` = `wedge` = 240; the depths `height` and `static_h()`, dashed   <span style="color:#455B6B">■ input</span> rib axis `start` -> `end`, control polygon, dashed   <span style="color:#8C969E">■ context</span> fan plane and seam plane traces

`outer_parabola(quad, run_in, parameters)` makes an outer rib's soffit. It takes <span style="color:#455B6B">`start = quad.get_point(0)`</span> and <span style="color:#455B6B">`end = quad.get_point(1)`</span> (the rib axis at the datum), steps <span style="color:#EB7721">`run_in`</span> along the axis to `trimmed = start + unit(end - start) * run_in`, and takes `middle = trimmed + (end - trimmed) * 0.5`. It returns <span style="color:#2196EA">`Polyline::quadratic_points(trimmed - height z, middle - static_h() z, end - static_h() z)`</span>: the quadratic Bezier `B(t) = (1-t)^2 P0 + 2t(1-t) P1 + t^2 P2`, sampled at the default <span style="color:#2196EA">7 points</span>, t = k/6. The <span style="color:#455B6B">control point P1</span> is the plan midpoint of P0 and P2, so the 7 points are evenly spaced in plan. The control point and the end are at the same depth, so the tangent at the seam end is horizontal. The depth simplifies to z(t) = -static_h() - rise (1-t)^2. <span style="color:#EB7721">`static_h()`</span> is <span style="color:#EB7721">`height`</span> `- rise` (floor_plan.cpp:14-16). `run_ins` first evaluates this curve with <span style="color:#EB7721">`run_in = wedge`</span>, the trial value in this frame.

```cpp
const Point trimmed = start + (end - start).normalized() * run_in;
const Point middle = trimmed + (end - trimmed) * 0.5;
return Polyline::quadratic_points(trimmed + Vector(0.0, 0.0, -parameters.height),
    middle + Vector(0.0, 0.0, -parameters.static_h()), end + Vector(0.0, 0.0, -parameters.static_h()));
```

| Variable | Value | Meaning |
|---|---|---|
| `start`, `end` | (-2780,-3000,0), (0,-3000,0) | `quads.outer_ribs[0]` corners 0 and 1: fan trace and seam at the datum. |
| `run_in` | 240 (= `wedge`, the trial value) | Plan distance along the axis from `start` to the parabola's first station. |
| `trimmed` | (-2540,-3000,0) | Station of the first Bezier point. |
| `middle` | (-1270,-3000,0) | Station of the control point. |
| `height` | 650 | Depth of the first point. |
| `rise` | 453 | Rise from the first point to the seam. |
| `static_h()` | 197 | Depth of the control point and of the seam end. |
| soffit [x, z] | (-2540,-650), (-2116.667,-511.583), (-1693.333,-398.333), (-1270,-310.25), (-846.667,-247.333), (-423.333,-209.583), (0,-197) | `outer_parabola(quads.outer_ribs[0], 240)` in y = -3000. |

Code: `outer_parabola`, [floor.cpp:253-261](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L253-L261); `Polyline::quadratic_points`, [session_cpp polyline.cpp:264-281](https://github.com/petrasvestartas/session_cpp/blob/9919e7f7fd09acd90feea9dccf72aa63cf083fbf/src/polyline.cpp#L264-L281).

## 41. fan_end: first chord extended onto the fan plane

![](floor/041_fan_end.webp)

<span style="color:#2196EA">■ built</span> `trim(outer_parabola(quads.outer_ribs[0], 240), fan, seam)`   <span style="color:#EB7721">■ variable</span> first chord extended by `EXTENSION`, dashed; `fan_end` = -694.79   <span style="color:#455B6B">■ input</span> soffit of frame 40, fan plane `wedges[0][0]`   <span style="color:#8C969E">■ context</span> dropped Bezier start

The parabola starts `run_in` past the fan plane's datum trace, so it does not reach the column. `fan_end` finds where the rib's soffit does reach it. It calls <span style="color:#2196EA">`trim(outer_parabola(quad, run_in), fan, seam)`</span>. `trim` copies the points and replaces the first point by `pts[0] + unit(pts[0] - pts[1]) * EXTENSION` and the last by `pts[n-1] + unit(pts[n-1] - pts[n-2]) * EXTENSION`, with <span style="color:#EB7721">EXTENSION</span> = 1000. Both end chords are therefore extended in straight lines, and the <span style="color:#8C969E">Bezier start</span> (-2540,-650) is no longer a point of the polyline. The <span style="color:#EB7721">extended first chord</span> still passes through it. `cut` then calls `cut_by_plane(`<span style="color:#455B6B">`fan`</span>`)` and `cut_by_plane(seam)`. With no flip given, each cut keeps the side of the plane that holds the polyline's half-length point (polyline.cpp:874), inserts the crossing on the segment that crosses the plane and removes consecutive duplicates closer than 1e-6. The curve is never re-evaluated: below the Bezier start the soffit is the sloped extension of the first chord. Each cut crosses one extended end chord, so the result keeps 7 points: the crossing on the <span style="color:#455B6B">fan plane</span>, the five inner Bezier points and the crossing on the seam plane, which is the old end (0,-197) because the seam end lies on that plane. Last, <span style="color:#EB7721">`fan_end`</span> measures both ends' distances to the fan plane and returns the z of the end that lies on it. For outer rib 0 at run_in 240 that end is (-2676.996,-3000,-694.7934), 44.79 deeper than the Bezier start.

```cpp
const std::vector<Point> pts = trim(outer_parabola(quad, run_in, parameters), fan, seam).get_points();
const double d0 = std::abs((pts.front() - fan.origin()).dot(fan.z_axis()));
const double d1 = std::abs((pts.back() - fan.origin()).dot(fan.z_axis()));
return d0 > d1 ? pts.back()[2] : pts.front()[2];
```

| Variable | Value | Meaning |
|---|---|---|
| `EXTENSION` | 1000 | How far `trim` pushes each end out along its end chord (floor_geometry.cpp:9). |
| extended first point | (-3490.5,-3000,-960.8) | `pts[0]` after the push, beyond the fan plane. |
| extended last point | (999.6,-3000,-167.3) | `pts[n-1]` after the push, beyond the seam plane. |
| `fan`, `seam` | `cp.wedges[0][0]`, `cp.inner_beams[0][0]` | The planes outer rib 0 is cut by (rib 1: `cp.wedges[2][0]`, `cp.inner_beams[2][0]`). |
| `d0`, `d1` | 0 and the far end's distance, d0 < d1 | Distances of the trimmed ends from the fan plane; the nearer end is returned. |
| `fan_end(quads.outer_ribs[0], 240, ...)` | -694.7934 at (-2676.996,-3000) | z of the soffit on the fan plane. |
| `fan_end(quads.outer_ribs[1], 240, ...)` | -694.7934 | The same for outer rib 1. |

Code: `fan_end`, [floor.cpp:264-271](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L264-L271); `trim`, `cut`, [floor_geometry.cpp:69-82](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor_geometry.cpp#L69-L82); `Polyline::cut_by_plane`, [polyline.cpp:860-900](https://github.com/petrasvestartas/session_cpp/blob/9919e7f7fd09acd90feea9dccf72aa63cf083fbf/src/polyline.cpp#L860-L900).

## 42. run_ins: the corner's shared level

![](floor/042_shared_level.webp)

<span style="color:#2196EA">■ built</span> `level` = max of both ends = -694.7934   <span style="color:#EB7721">■ variable</span> `fan_end` of both outer ribs at `wedge`   <span style="color:#455B6B">■ input</span> end faces on the fan planes `cp.wedges[0][0]`, `cp.wedges[2][0]`   <span style="color:#8C969E">■ context</span> trimmed soffits

Both outer ribs of a corner end on the column, and the design wants their soffits to meet the column at one level. `run_ins` pairs each outer rib with its fan and seam planes, <span style="color:#455B6B">`fans = {cp.wedges[0][0], cp.wedges[2][0]}`</span> and `seams = {cp.inner_beams[0][0], cp.inner_beams[2][0]}`. It evaluates <span style="color:#EB7721">`fan_end`</span> of both ribs at the trial run-in `wedge` and takes <span style="color:#2196EA">`level = max(...)`</span>, the higher (shallower) of the two ends. Then it solves each rib's run-in onto that level with `run_in_to_level` (frame 43). The rib whose end is already at the level keeps `wedge`. On the square bay both ends are -694.7934, so <span style="color:#2196EA">`level`</span> is -694.7934 and both ribs keep 240.

```cpp
const double level = std::max(fan_end(quads.outer_ribs[0], parameters.wedge, fans[0], seams[0], parameters),
                              fan_end(quads.outer_ribs[1], parameters.wedge, fans[1], seams[1], parameters));
return {run_in_to_level(quads.outer_ribs[0], fans[0], seams[0], level, parameters),
        run_in_to_level(quads.outer_ribs[1], fans[1], seams[1], level, parameters)};
```

| Variable | Value | Meaning |
|---|---|---|
| `fans` | `{cp.wedges[0][0], cp.wedges[2][0]}` | Fan plane of outer rib 0 and of outer rib 1. |
| `seams` | `{cp.inner_beams[0][0], cp.inner_beams[2][0]}` | Seam plane each outer rib ends on. |
| `level` | -694.7934 (3000 x 2400: -689.979) | Target z of both rib ends: the shallower `fan_end` at `wedge`. |

Code: `run_ins`, [floor.cpp:305-312](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L305-L312) (called at 369).

## 43. run_in_to_level: secant (3000x2400)

![](floor/043_secant.webp)

<span style="color:#2196EA">■ built</span> soffit at the root x* = 187.667, its Bezier start and its end on the fan plane, x* on the graph   <span style="color:#EB7721">■ variable</span> f(x), the iterates `x0`, `x1` and their ends, the first secant line (dashed) and `x2`   <span style="color:#455B6B">■ input</span> fan plane `wedges[2][0]`, `level` (dashed)   <span style="color:#8C969E">■ context</span> soffits at `x0`, `x1` and their Bezier starts, graph axis

`run_in_to_level` solves <span style="color:#EB7721">f(x)</span> = `fan_end(quad, x, fan, seam) - level` = 0 for the run-in x. The Bezier start depth is fixed at -height. A longer run-in moves the start toward the seam, so the first chord gets steeper and its extension reaches the fan plane deeper; f falls as x grows. The function starts at <span style="color:#EB7721">`x0 = wedge`</span>. If `|f0| <= RUN_IN_TOLERANCE` it returns `x0`: that happens on the square, where f(240) = 0, and on the shallower rib of any corner. Otherwise it sets <span style="color:#EB7721">`x1 = x0 + 1`</span> and runs up to `RUN_IN_STEPS` secant steps <span style="color:#EB7721">`x2 = x1 - f1 (x1 - x0) / (f1 - f0)`</span>. It returns `x1` as soon as `|f1| <= RUN_IN_TOLERANCE`. It throws `std::runtime_error` when `x2` leaves (0, `axis`), with `axis` the length of `get_point(0)` to `get_point(1)`, or when the loop ends without convergence. The f1 computed in the last step is never tested before that throw. On the 3000 x 2400 bay the deeper end is outer rib 1, the shorter rib along x = -3000. Its run-in shrinks from 240 to <span style="color:#2196EA">187.667</span> until its end rises to the <span style="color:#455B6B">level</span> -689.979 set by outer rib 0, which keeps 240. The picture, in the plane of outer rib 1 (looking along -x), shows on the left the <span style="color:#8C969E">soffits at x0, x1</span> with their <span style="color:#8C969E">Bezier starts</span> and their <span style="color:#EB7721">ends on the fan plane</span>, and <span style="color:#2196EA">the root's soffit, its Bezier start and its end</span>, each cut on the <span style="color:#455B6B">fan plane</span>. On the right it plots <span style="color:#EB7721">f(x)</span> for x from 180 to 245, drawn 6 mm per mm of run-in and 4 mm per mm of f, its zero line on the level, with the <span style="color:#EB7721">first secant line</span> through <span style="color:#EB7721">(x0, f0)</span> and <span style="color:#EB7721">(x1, f1)</span> reaching zero at <span style="color:#EB7721">x2</span> = 188.99, just past <span style="color:#2196EA">the root 187.667</span>.

```cpp
const double x2 = x1 - f1 * (x1 - x0) / (f1 - f0);
if (x2 <= 0.0 || x2 >= axis)
    throw std::runtime_error(...);
x0 = x1; f0 = f1; x1 = x2;
f1 = fan_end(quad, x1, fan, seam, parameters) - level;
```

The secant on outer rib 1 of the 3000 x 2400 bay:

| Iterate | x | f(x) = fan_end - level |
|---|---|---|
| `x0 = wedge` | 240 | -22.217 |
| `x1 = x0 + 1` | 241 | -22.653 |
| first `x2` | 241 - f1 (1) / (f1 - f0) = 188.99 (189.04 from the rounded f0, f1 shown) | (secant line hits zero here) |
| root | 187.667 | within 1e-11 |

| Variable | Value | Meaning |
|---|---|---|
| `axis` | 2780 on the square | Upper bound of the run-in: the rib axis length. |
| `x0`, `f0` | 240, 0 (3000 x 2400 rib 1: -22.217) | Trial run-in and its miss. |
| `x1`, `f1` | 241, -22.653 (3000 x 2400 rib 1) | Second secant point. |
| `RUN_IN_TOLERANCE` | 1e-11 mm | How far off the level an end may stay. |
| `RUN_IN_STEPS` | 50 | Secant step cap. |
| `geometry.run_in` | {240, 240} (3000 x 2400: {240, 187.667}) | The solved run-ins. |

Code: `run_in_to_level`, [floor.cpp:274-302](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L274-L302) (called at 311); constants [floor.cpp:11-12](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L11-L12).

## 44. block_planes: final far faces

![](floor/044_block_planes.webp)

<span style="color:#2196EA">■ built</span> final `quads.wedges` and their far faces `cp.wedges[i][1]`   <span style="color:#EB7721">■ variable</span> `thickness[i]` along each fan normal   <span style="color:#8C969E">■ context</span> provisional far faces (dashed), rib quads, column head

With the run-ins known, `block_planes` gives the three column blocks their real thickness. It sets <span style="color:#EB7721">`thickness = {run_in[0], middle_wedge_factor * 0.5 * (run_in[0] + run_in[1]), run_in[1]}`</span>, and for i = 0..2 replaces <span style="color:#2196EA">`column.wedge_fan[i][1]`</span> by `column.wedge_fan[i][0].translate_by_normal(thickness[i])` and sets <span style="color:#2196EA">`cp.wedges[i][1]`</span> to the same plane. The fan planes `[i][0]` are not changed. A side block is therefore exactly as thick, along its fan normal, as its outer rib's run-in. On the square the result equals the provisional faces (240 / 300 / 240), so the picture uses the 3000 x 2400 bay. There the middle block's far face moves from 300 to 267.292 and side block 2's from 240 to 187.667; the <span style="color:#8C969E">provisional faces are dashed grey</span>.

```cpp
const std::array<double, 3> thickness = {run_in[0], parameters.middle_wedge_factor * (0.5 * (run_in[0] + run_in[1])), run_in[1]};
for (size_t i = 0; i < 3; i++) {
    column.wedge_fan[i][1] = column.wedge_fan[i][0].translate_by_normal(thickness[i]);
    cp.wedges[i][1] = column.wedge_fan[i][1];
}
```

| Variable | Value | Meaning |
|---|---|---|
| `thickness` | {240, 300, 240} (3000 x 2400: {240, 267.292, 187.667}) | Far-face offsets along the fan normals; in plan 242.6 / 304.6 / 242.6 on the square. |
| `middle_wedge_factor` | 1.25 | The middle block in mean run-ins. |
| `cp.wedges[i][1]`, `column.wedge_fan[i][1]` | fan plane moved by `thickness[i]` | Final block far faces. |

Code: `block_planes`, [floor.cpp:315-323](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L315-L323) (called at 370).

## 45. Construction quads, second pass

![](floor/045_second_pass.webp)

<span style="color:#2196EA">■ built</span> `quads.wedges`, rebuilt   <span style="color:#EBB121">■ result</span> `quads.tsections`, rebuilt   <span style="color:#8C969E">■ context</span> `outer_ribs`, `inner_beams`, `inner_ribs` quads, unchanged

`compute_quarter` calls `geometry.quads = construction_quads(geometry.planes)` a second time, because `block_planes` replaced `cp.wedges[i][1]`, which bounds <span style="color:#2196EA">`quads.wedges`</span> (side 2-3) and <span style="color:#EBB121">`quads.tsections`</span> (side 3-0). The <span style="color:#8C969E">outer rib, inner beam and inner rib quads</span> read only `cp.wedges[i][0]` or no wedge plane, so they come out identical. The outer rib quads the run-ins were solved on are therefore still the ones `boundary_parabolas` uses next. On the square every quad equals the first pass. So `construction_quads` runs twice per quarter, and `run_ins` evaluates `outer_parabola` at least four times (twice for `level`, then once per rib for f0, plus once per secant step on a rib that misses) before `boundary_parabolas` evaluates it for the stored soffits.

| Variable | Value | Meaning |
|---|---|---|
| `geometry.quads` (final) | 16 plan quads at z 0, equal to the first pass on the square | The footprints the members are built on. |
| `quads.wedges`, `quads.tsections` | rebuilt on the final far faces | The only families that change. |

Code: `compute_quarter`, [floor.cpp:371](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L371); `construction_quads`, [floor.cpp:218-250](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L218-L250).

## 46. boundary_parabolas at the solved run-in

![](floor/046_boundary_parabolas.webp)

<span style="color:#2196EA">■ built</span> `parabolas[0][0]`, `parabolas[1][0]` and their 7 points   <span style="color:#EB7721">■ variable</span> run-in station `run_in[k]` at -height, dashed   <span style="color:#455B6B">■ input</span> seam station at -static_h(), dashed   <span style="color:#8C969E">■ context</span> quarter 0 quads

`boundary_parabolas` makes, for k = 0, 1, <span style="color:#2196EA">`parabola = outer_parabola(quads.outer_ribs[k], run_in[k], parameters)`</span>. This is the construction of frame 40 with the solved run-in on the final quads. It stores `{parabola, offset_polyline(parabola, tsections), offset_polyline(parabola, 2 * tsections)}` as `parabolas[k]`. The <span style="color:#2196EA">stored soffit</span> is the untrimmed 7-point Bezier: it starts at the <span style="color:#EB7721">run-in station</span> at -height and ends at the <span style="color:#455B6B">seam</span> at -static_h(). The rib outline in chapter 5 trims it with the same `trim`, by the fan plane `cp.wedges[0][0]` (or `[2][0]`) and by the end `Quarter::rib_seam_ends` gives (floor_members.cpp:69-74): with `seam_through_ribs` true, the default, that is `cp.inner_beams[0][1]` (or `[2][1]`), the far face of the seam beam, and not the seam plane `fan_end` cut by; with it false, the seam plane `cp.inner_beams[0][0]` (or `[2][0]`).

```cpp
for (size_t k = 0; k < 2; k++) {
    const Polyline parabola = outer_parabola(quads.outer_ribs[k], run_in[k], parameters);
    parabolas.push_back({parabola, offset_polyline(parabola, parameters.tsections), offset_polyline(parabola, 2.0 * parameters.tsections)});
}
```

| Variable | Value | Meaning |
|---|---|---|
| `run_in` | {240, 240} | `geometry.run_in` from frame 43. |
| `parabolas[0][0]` | (-2540,-3000,-650) ... (0,-3000,-197) | Outer rib 0 soffit. |
| `parabolas[1][0]` | (-3000,-2540,-650) ... (-3000,0,-197) | Outer rib 1 soffit. |

Code: `boundary_parabolas`, [floor.cpp:326-333](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L326-L333) (called at 372).

## 47. Layers by offset_polyline

![](floor/047_layers.webp)

<span style="color:#2196EA">■ built</span> `parabolas[0][1]`, the +t layer, its start and mitre points   <span style="color:#EBB121">■ result</span> `parabolas[0][2]`, the +2t layer   <span style="color:#EB7721">■ variable</span> `tsections` = 27   <span style="color:#455B6B">■ input</span> soffit `parabolas[0][0]`, segment planes moved by t and the start cap, dashed

`offset_polyline(polyline, distance)` builds n + 1 planes. The first is a <span style="color:#455B6B">start cap</span> through `pts[0]` with normal `pts[1] - pts[0]`. Then comes <span style="color:#455B6B">one plane per segment</span> through the segment's centre with normal `x.cross(z.cross(x))`, which is square to the segment, in its vertical plane, and pointing up; each is moved by <span style="color:#EB7721">`translate_by_normal(distance)`</span>. The last is an end cap through `pts[n-1]` with normal `pts[n-2] - pts[n-1]`. A vertical `base` plane through `pts[0]` with normal `z.cross(pts[n-1] - pts[0])` keeps the result in the rib's plane. Point i of the result is `plane_plane_plane(planes[i], planes[i+1], base)`. The end points therefore sit on the end normals, square to the first and last segment, and the inner points are the <span style="color:#2196EA">mitred corners</span> of the offset segments. This offsets the polyline, not the Bezier curve. The <span style="color:#EBB121">+2t layer</span> is offset from the <span style="color:#455B6B">soffit</span> by 54 directly, not from the <span style="color:#2196EA">+t layer</span>. The members read <span style="color:#2196EA">+t</span> as the t-section top and the bed underside, and <span style="color:#EBB121">+2t</span> as the bed top.

```cpp
planes.push_back(Plane::from_point_normal(line.center(), x.cross(y)).translate_by_normal(distance));
...
result.push_back(plane_plane_plane(planes[i], planes[i + 1], base).value());
```

| Variable | Value | Meaning |
|---|---|---|
| `tsections` | 27 | Layer thickness t. |
| `planes` | n + 1 = 8 planes | Start cap, the 6 segment planes moved by `distance`, end cap. |
| `base` | vertical plane through the chord `pts[0]` to `pts[n-1]` | Keeps the offset in the rib's plane. |
| `parabolas[0][1]` [x, z] | (-2548.391,-624.337) ... (-0.802,-170.012) | Soffit + t, the t-section top. |
| `parabolas[0][2]` [x, z] | (-2556.782,-598.674) ... (-1.604,-143.024) | Soffit + 2t, the bed top. |

Code: `offset_polyline`, [floor_geometry.cpp:84-109](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor_geometry.cpp#L84-L109) (called at [floor.cpp:332](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L332)).

## 48. Shadow parabolas on the inner ribs

![](floor/048_shadows.webp)

<span style="color:#2196EA">■ built</span> shadows `parabolas[2]`, `parabolas[3]`, all three layers   <span style="color:#EB7721">■ variable</span> projection direction `outer_ribs[i][0].z_axis()`, dashed   <span style="color:#455B6B">■ input</span> soffits `parabolas[0][0]`, `parabolas[1][0]`   <span style="color:#8C969E">■ context</span> outer and inner rib quads

For i = 0, 1, `boundary_parabolas` builds `projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0],` <span style="color:#EB7721">`cp.outer_ribs[i][0].z_axis()`</span>`)`. This affine map moves a point p along <span style="color:#EB7721">d = the outer rib's normal</span> until it lands on the inner rib's outer face: p - d (n.p - n.o) / (n.d), with n and o the face's normal and origin. All three curves of <span style="color:#455B6B">`parabolas[i]`</span> are transformed and appended as <span style="color:#2196EA">`parabolas[2 + i]`</span>. The normal of outer rib 0 is (0,1,0) and that of outer rib 1 is (1,0,0), both horizontal, so z and the along-edge coordinate are kept and only the across-edge coordinate changes. The 7 points stay evenly spaced on a straight plan line, now on the slanted face. z at index k depends only on t = k/6, so both shadows share z per index whatever the run-in. At index 6 on the square they are (0,-983.930,-197) and (-983.930,0,-197), 1391.5 apart. The shadows are not trimmed: they run from the run-in station to the seam station x = 0 (or y = 0), past the inner rib quad's end. `central_panel` in chapter 4 reads <span style="color:#2196EA">`parabolas[2][0]`</span> and <span style="color:#2196EA">`parabolas[3][0]`</span> as its two shadows.

```cpp
for (size_t i = 0; i < 2; i++) {
    const Xform projection = Xform::project_to_plane_by_axis(cp.inner_ribs[i][0], cp.outer_ribs[i][0].z_axis());
    const std::array<Polyline, 3>& outer = parabolas[i];
    parabolas.push_back({outer[0].transformed(projection), outer[1].transformed(projection), outer[2].transformed(projection)});
}
```

| Variable | Value | Meaning |
|---|---|---|
| `projection` (i = 0) | along (0,1,0) onto `cp.inner_ribs[0][0]` | Slide of outer rib 0's layers onto inner rib 0's outer face. |
| `parabolas[2][0]` | (-2540,-2716.311,-650), (-1270,-1850.120,-310.250) at index 3, (0,-983.930,-197) | Shadow of outer rib 0's soffit. |
| `parabolas[3][0]` | (-2716.311,-2540,-650) ... (-983.930,0,-197) | Shadow of outer rib 1's soffit, the mirror in x = y. |
| `parabolas[2][1..2]`, `parabolas[3][1..2]` | the +t and +2t layers projected the same way | Shadow layers. |

Code: `boundary_parabolas`, [floor.cpp:335-339](https://github.com/petrasvestartas/wood/blob/44f9aa85952d32a9264125f4e9940e55b05a4512/src/templates/floor/floor.cpp#L335-L339); `Xform::project_to_plane_by_axis`, [session_cpp xform.cpp:612-638](https://github.com/petrasvestartas/session_cpp/blob/9919e7f7fd09acd90feea9dccf72aa63cf083fbf/src/xform.cpp#L612-L638).
