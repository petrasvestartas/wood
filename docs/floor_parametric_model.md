# The floor bay as a parametric model

Design document, 2026-10-02. Nothing in any repository was changed for it. It is written from the
compas_tf sources (consumed, never edited), the wood tree at `d15538c` (HEAD: the quarter-turn
port) and `1b42862` (the reverted corner-frame generalisation), and the probe scripts under the
session scratchpad `floor_model/` (listed in appendix A). Every claim about source cites
`file:line`; every number names the probe output it comes from. Revised the same day after a
skeptic pass of 22 objections, all upheld and re-checked against the sources and the probes
(`skeptic_probe.py`, `ring_bearing_probe.py` re-run under the guard).

Conventions. Units mm and degrees. The floor datum z = 0 is the top of every member and the
column top; depths are negative z; the scene lifts everything by `bay_height`. `floor_guide.py:N`
means `compas_tf/src/compas_tf/floor_guide.py`, `example_model_N` the compas_tf examples, wood
paths are relative to `wood/`. "Square" is compas_tf's example 1 configuration (half spans
3000 / 3000, head 220 / 120, ribs 100 / 60, beams 60, wedge 240, t 27, height 650, rise 453,
oculus 1000); "rectangle" is the same with half spans 3000 / 2400.

Fixed rule from the user: member thicknesses, t-section offsets and column dimensions stay fixed
when the plan changes; only plan geometry varies.

---------------------------------------------------------------------------------------------------

## 0. Summary

What the original shape is. compas_tf's floor is one quarter of a square bay plus a four-fold
rotation, nothing more. The quarter is a pentagon: column corner, the midpoint of the bay edge
after it, two oculus corners, the midpoint of the bay edge before it (`floor_guide.py:186-194`).
Every member is a plane offset of that pentagon or of the column-head pentagon at the corner
(`:198-211`): outer ribs along the two bay edges (`:233-242`), inner beams along the two seams and
the oculus edge (`:244-257`), inner ribs from the two chamfer vertices to the inner corners of the
seam and oculus beams (`:259-278`), a fan of three tilted planes on the column head (`:280-319`),
flange planes beside every rib face (`:321-332`). Depth comes from one parabola rule: each outer
rib's axis, trimmed by `size_wedge` at the column, carries a 7-point quadratic Bezier from
`-height` at the column to `-static_h` at the seam with a horizontal tangent there (`:668-696`);
each inner rib carries the shadow of its outer rib (`:698-715`); beds and flanges are the +t / +2t
offsets swept between rib faces (`:752-864`, `:924-1023`). The bay is the quarter turned four
times (`example_model_3_columns_model.py:27-29`, `example_model_4_quarters.py:62-71`) and the
oculus ring is quarter 0's oculus plane turned four times with a pinwheel end rule
(`:1328-1345`).

Two things are square-only and the port never separated them from the rest:

1. The tiling. The neighbour across a seam is the mirror of the quarter in that seam plane; it
   coincides with the quarter turn only when the two half spans are equal. On 3000 x 2400 the
   turned bay is a 12-vertex pinwheel with 0 rib seams (section 4.2); the quarter built in place
   at every corner tiles the 6000 x 4800 rectangle with every seam matched (section 4.3).
2. One plan direction does six jobs. In the square the chamfer direction
   c = (-1, 1, 0) / sqrt(2) is also the oculus edge direction (`:174-179` with gx = gy), the sweep
   of the inner ribs (`:1127`, `:1131`), the sweep of the soffit edges of flanges 2a / 3b on the
   inner ribs' outer faces (`:970`, `:1005`), the sweep of the central bed row and of flanges
   2b / 3a (`:839`, `:843`, `:981`, `:992`), and the one horizontal direction that lies in both
   inner-rib end planes. That is the quarter's mirror symmetry about its own diagonal. On a
   rectangle the direction splits three ways (4.6 deg and 17.0 deg apart), the inner-rib end faces
   leave their planes by 4.9 / 18.0 mm and the central beds miss the second rib's flange by up to
   37 mm (section 4.1).

The model. One `Floor` = `FloorPlan` (four corners, the oculus) + `FloorSizes` (everything fixed).
The Floor computes every shared entity once (centre, four bay edges with their rib bands, four
seams, four oculus corners and edges, four column corners with their carved fans, the levels) and
every quarter's private geometry once (planes, parabolas, shadows, central panel), and both parties
that meet on a shared plane read the same normal and offset. A quarter is a view (index plus
floor) built in place for any corner, so the square is numerically the turned quarter and a
rectangle is the mirror tiling (for the central panel this needs the `section` layers of 4.4). The
ring is built from the four quarters' own oculus planes with compas_tf's pinwheel. Every rib end
face is cut in its end plane on each rib face (R4). The central panel follows rule A: its ruling u
is solved so that the first inner rib's central trace projects onto the second rib's with the
right span, and one rib sweep r is solved so the start points match; this reduces exactly to c on
the square and closes to 1e-13 on every rectangle probed, keeps every face planar and needs no
kernel change. Connectors are generated from the relationships the Floor states (76 rows: 48
connectors, 4 supports, 24 cutters), with the contact search kept as a verification pass and a
`FloorReport` of the relations compas_tf relied on silently. The square reproduces every existing
parity record except the central row (bed row 1, the tops of flanges 2b / 3a, bed plane 1, block
1), which the fixed rule moves by up to 2.9 / 5.8 mm (4.4): square parity of that row and a layer
thickness that does not change with the plan are mutually exclusive. The rectangle becomes the
consistent assembly compas_tf never had.

---------------------------------------------------------------------------------------------------

## 1. Inputs with physical meaning

### 1.1 FloorSizes (fixed; never a function of the plan)

Values are the example-1 set that the wood port and every reference dump use
(`example_model_1_floorguide.py:16-28`); compas_tf's class defaults are in parentheses where they
differ (`floor_guide.py:30-46`). The class defaults (250 / 100 / 100) are not a working
configuration: two of the three wedge-block lofts come out open (section 8 of
`guide_3000x3000_rotation_classdefaults.txt`, `:211`) and the rib bottom sits 43 mm above the
cutter level (its section 4, `:145`), so the model's defaults are the example set.

| field | value | compas_tf | physical meaning | where it enters |
|---|---|---|---|---|
| `column_head` | 220 (250) | `size_column_head` `:34,52` | side of the square column shaft and of the head polygon at the corner; the outer ribs start `column_head` from the corner | head polygon `:205-208`; shaft `example_model_2:33-34`; column axis inset `corner_point_column` `:160-164`; outer rib axis start `:678-681` |
| `column_head_chamfer` | 120 (100) | `size_column_head_chamfer` `:35,53` | where the chamfer vertices sit on the shaft faces (the chamfer leg is `column_head - chamfer` = 100, the chamfer edge 141.4 long); also the capitel width, so the carved head footprint is 340 x 340 | head polygon `:206-207`; capitel `example_model_2:37`, `column.py:296-302`; side wedge seat = `chamfer - outer_ribs` = 20 mm |
| `outer_ribs` | 100 | `size_outer_ribs` `:36,59` | outer rib thickness: the band of the two planes on and inside each bay edge | `:239-241` |
| `inner_ribs` | 60 | `size_inner_ribs` `:37,60` | inner rib thickness, perpendicular to the rib | `:275-277`; central wedge seat = 141.4 - 2 x 61.06 = 19.3 mm |
| `inner_beams` | 60 | `size_inner_beams` `:38,61` | seam beam and oculus beam thickness, and the ring beam width at z 0: the ring's inner plane is the oculus back face (`:255`, this far INTO the quarter) offset back by twice this (`:1330`), i.e. `inner_beams` inside the oculus edge toward the centre (647.1 from the origin against the edge's 707.1 on the square) | `:253-256`, `:1330`; dormant strips `:316-318` |
| `wedge` | 240 (100) | `size_wedge` `:39,62` | side wedge block thickness, perpendicular to the tilted fan plane; the middle block is `middle_wedge_factor` times it; also the trim of the parabola axis at the column end | `:313-315`, `:679-680`, `:740` |
| `tsections` | 27 | `size_tsections` `:40,63` | flange plane offset from the rib faces, and the +t / +2t layer thickness of beds and flanges; the oculus plate levels | `:326-331`, `:693-694`, `:1324-1326` |
| `height` | 650 | `height` `:41,76` | rib depth at the parabola start, one wedge thickness past the column face | `:686` |
| `rise` | 453 | `rise` `:42,77` | drop of the rib soffit from the seam to the parabola start; `static_h = height - rise` = 197 is the depth at every seam, of the inner beams and of the ring | `:78`, `:687-688`, `:1259`, `:1325` |
| `wedge_plane_angle` | -10 | `wedge_plane_angle` `:44,65-68` | tilt of the chamfer fan plane about its top edge, leaning toward the bay going down; the two side planes are not rotated by it (`:291` is commented out) but lean so that each is parallel to the chamfer plane's crease with the inner rib's central face `[k][1]` (`:293-300`); their own crease with the chamfer plane then runs through the chamfer vertex along the inner rib's OUTER face `[k][0]` (`skeptic_probe.out` A: 0.000 on `[0][0]`, 42.71 mm off `[0][1]`); 8.433 deg on the square | `:290-300` |
| `oculus_plane_angle` | 5 | default argument of the `construction_planes` property `:220`, used at `:248`; unreachable, hence a constant in compas_tf | tilt of the oculus bearing plane about the oculus edge: the oculus narrows going down (60 -> 42.76 ring width) | `:246-248`, `:1328` |
| `column_head_depth` | 730 | `column_head_lowest_height = -730` `:79`, `:387`; `capitel_height` `example_model_2:38` | depth of the carved head and of the capitel | cutter level 2 |
| `bay_height` | 3500 | `bay_height` `:45,70-73` | storey: the quarters and the ring are lifted by it; the column is `bay_height - 150` on a 150 support, so the column top is the datum | `example_model_4:56`, `_5:31`, `_2:35-36`; `support.py:133` |
| `middle_wedge_factor` | 1.25 | literal `:314` | thickness of the middle wedge block in wedge thicknesses (300); a stand-in for the computed-and-discarded `plane1_offset` `:302-306` | `:314` |

Not a size, hence not in the table: compas_tf's `1.65` (`:386`) fixes the middle cutter level
`-(height + 1.65 t)` = -694.55 as a tuning of a relationship, the carved band's bottom = the outer
rib's bottom at the fan plane (-694.793 on the square; `outer_ribs_bottom` `:743-750` exists for
exactly this and is unused). It is the `CutterLevel` rule of 6.2 R8 and open question 4, not a
`FloorSizes` field. The side wedge seat of 20 mm (`chamfer - outer_ribs`) and the parabola span
`L = half span - column_head - wedge` hold at right corners only (1.2, R8).

Support and connector dimensions stay where they are: the Sherpa support
(`src/joinery_solver/wood_elements/wood_element_support.h:13,16,60-61`; `support.py:128-133`) and
the `JointBeam` factory defaults (`wood_element_joint_beam.h:39-100`, from
`connectors.py:351-358, 801-807, 1076-1118`). The assembly dowels (radius 4, length 30, inset 50)
and the cross-lap share 0.5 are wood-only (`floor.h:194,197`).

### 1.2 FloorPlan (the only thing that varies)

| field | meaning | compas_tf |
|---|---|---|
| `corners[4]` | the bay corners counter-clockwise at z 0; `FloorPlan::rectangle(hx, hy)` gives (-hx,-hy), (hx,-hy), (hx,hy), (-hx,hy) | `size_grid_x`, `size_grid_y` are half spans: the quarter runs from `-size_grid` to 0 (`:49-50`, `:188-192`); corner 0 is compas_tf's quarter 0 |
| `oculus` | the oculus half-diagonal: the distance of an oculus corner from the centre along its seam | `size_oculus` `:43,55` |
| `rule` | how the four corners sit on the seams (section 6.2, R3): `square_diamond` (default, the same distance on every seam), `compas` (compas_tf's `size_oculus * gx/gy`, `* gy/gx` `:56-57`, generalised by the reverted port's geometric-mean formula, kept only for the compas_tf gate), `explicit_distances` | `:56-57`, `:171-180` |

Derived, never input: centre, midpoints, seams, oculus corners, column frames and axis points,
the chamfer direction, every construction plane, the side fan tilts, the parabola spans L from
the fan side plane's crossing of the bay edge, plus `wedge`, to the edge midpoint (that crossing
is `column_head` from the corner at a right corner, `column_head / cos((corner angle - 90) / 2)`
otherwise: `L = half span - column_head - wedge` = 2540 on the square, 2539.6 / 1958.3 at the
82.9 deg corner of the 6000/4800 x 4800 trapezoid, `central_panel_probe.out`, `:101-104` of its
script), the central panel's rulings, the cutter levels, `static_h`.

### 1.3 Named constants (compas_tf literals, `constexpr` in wood, not inputs)

7 parabola points (`:683`); 1000 mm end extension before plane cuts (`:773`, `:886`, `:943-946`,
`:1044`, `:1100`; `floor_geometry.cpp:9`); 100 mm cutter in-plane overshoot and thickness
(`:437-470`; `floor_members.cpp:10`); the 0.25 narrowing of the lowest chamfer cutter (`:425`;
`floor_members.cpp:312`); `output_wedges = False` (`:96`, `:1211`, `:1348`: wedge strips 3-5 and
the ring's inner wedges never exist); `wedge_block` returns `[]` (`:1173`).

---------------------------------------------------------------------------------------------------

## 2. Dependency graph (input -> derived -> consumers)

Indices: quarter q = 0..3 counter-clockwise from corner (-gx, -gy). Quarter q owns corner K_q, the
half of edge E_q (K_q -> K_{q+1}) up to its midpoint M_q, the half of edge E_{q-1} back to
M_{q-1}, seam S_q on its "beam 0" side and seam S_{q-1} on its "beam 2" side, and the oculus edge
Q_q from O_q to O_{q-1}. compas_tf's quarter 0: polygon (-g,-g), (0,-g), O_0 = (0,-oy),
O_3 = (-ox,0), (-g,0) (`:186-194`).

```
   K3 -------------------------- M2 -------------------------- K2
   |                              |                              |
   |          quarter 3           |          quarter 2           |
   |                              O2                             |
   |                            /    \                           |
   M3 - - - - - - - - - O3 -   C   - O1 - - - - - - - - - - - - M1       seams S_q = M_q -> C
   |                            \    /                           |       oculus corner O_q on S_q
   |                              O0                             |       quarter q = K_q M_q O_q O_{q-1} M_{q-1}
   |          quarter 0           |          quarter 1           |
   |                              |                              |
   K0 -------------------------- M0 -------------------------- K1
```

```
PLAN
  corners K_q, FloorSizes
    -> centre C = vertex centroid, where the two bimedians cross ....... compas_tf: the origin (:175-178, :188-192)
    -> edges E_q, midpoints M_q, half spans
    -> seams S_q = M_q -> C ............................................ the axes x = 0, y = 0 (:188-192)
    -> oculus corners O_q on S_q by the rule ........................... (:171-180; scaling :56-57)
    -> quarter polygon q = [K_q, M_q, O_q, O_{q-1}, M_{q-1}] ........... (:186-194)
    -> column head polygon q in the corner frame ....................... (:198-211)
    -> corner_point_column = K + (x + y) column_head / 2 ............... (:160-164)

PLANES (compas_tf construction_planes, one quarter)                       shared with
  outer_ribs[k]  = edge plane, inward normal, + outer_ribs ............ (:233-242)   the neighbour on that edge
  inner_beams[0] = seam plane into the quarter, + inner_beams ......... (:245, :253)  the neighbour across S_q
  inner_beams[2] = the same on S_{q-1} ................................ (:249, :256)  the neighbour across S_{q-1}
  inner_beams[1] = [oculus edge plane tilted -oculus_plane_angle, ..... (:246-248)    ring beam q
                    edge plane + inner_beams] ......................... (:255)
  p0, p1 = XY  x  beam offsets ........................................ (:264-265)
  inner_ribs[k]  = plane through chamfer vertex and p, + inner_ribs ... (:266-278)
  wedges[1]      = chamfer plane tilted wedge_plane_angle, + 1.25 w ... (:292, :314)  column cutters, blocks
  wedges[0], [2] = side planes through the head edges 1 and 3, each ... (:293-300)    column cutters, rib ends
                    PARALLEL to the tilted chamfer's crease with inner_ribs[k][1]
                    (the plane's own crease with the chamfer runs through the
                    chamfer vertex along inner_ribs[k][0]), + w ............ (:313, :315)
  wedges[3..5]   = beam offsets + inner_beams (dormant) ................ (:308-318)
  t_sections     = rib faces +- tsections .............................. (:325-332)
  quads at z 0 ......................................................... (:480-635)

DEPTH
  axis k = outer rib quad edge 0 from the fan plane to the seam, ...... (:673-681)
           trimmed by wedge: L = half span - column_head - wedge (right corner, 1.2)
  parabola k = 7-point Bezier from -height at the start to -static_h .. (:683-689)
               at the seam, control at the midpoint (horizontal tangent at the seam)
  +t, +2t = PolylineOffset in the rib's vertical plane ................ (:693-694; geometry.py:39-86)
  shadows [2], [3] = parabola k projected along the outer rib normal .. (:698-715)
                     onto inner_ribs[k][0]
  bed_top_planes[k] = best fit of the deepest +2t quad of panel k ..... (:866-922)
  block levels (dead: only wedge_block reads them and returns []) ..... (:721-741, :1173)

MEMBERS
  outer rib k: top = parabola cut by (wedges[2k][0], seam plane), ..... (:1025-1079)
               bottom = top translated along the rib normal
  inner rib k: top = shadow cut by (wedges[1][0], inner_beams[1][1]), . (:1081-1136)
               bottom = top translated along n0 - n1                      <- square only
  inner beams 0, 1, 2: plane fans ...................................... (:1235-1294)
  blocks 0, 1, 2: plane fans between rib pairs, bed plane, z 0 ........ (:1176-1233)
  flanges: project onto the face, then cut; soffit edge along the rib . (:924-1023)
           sweep (2a / 3b: n0 - n1 at :970 -> :972, :1005 -> :1007),
           top edge along the panel sweep (outer rib normal for 1, 2a,
           3b, 4 at :973, :1008; n0 - n1 for 2b, 3a at :981-984, :992-996)
  beds: cut the layer curves, then project along the panel sweep ...... (:752-864)
        panel 0 along +Y, panel 2 along +X, panel 1 along n0 - n1       <- square only
  ring: quarter 0's planes rotated 4x, pinwheel, bottom wedges, plate . (:1296-1380)   <- square only

COLUMN
  cutters: fan x levels {0, -(height + 1.65 t), -column_head_depth}, .. (:362-478)
           100 mm margins
  column model: shaft 220, capitel 120 x 730, support 150, cutters .... (:338-360; example_model_2:30-56)
                recentred by corner_point_column(220)

ASSEMBLY
  quarters and columns turned by i 90 deg ............................. (example_model_3:27-29, _4:62-71)   <- square only
  wedges: pairwise top/bottom contacts of the 16 ring plates .......... (example_model_6:45-68; connectors.py:395-457)
  column plates: columns x outer ribs ................................. (example_model_8:44-47; connectors.py:829-878)
  ties: outer ribs x outer ribs ....................................... (example_model_8:107-112; connectors.py:1151-1223)
  rib / block contacts, no connector in compas_tf ..................... (example_model_8_contacts_quarter.py:51-58)
```

Rib elevation along one bay edge (quarter 0, edge y = -gy):

```
 z = 0  K------head------|---wedge---|---------------- parabola axis, L = half span - head - wedge -------------M  seam
        |    column      |           .
        | carved fan     |            .        z(s) = -static_h - rise (1 - s/L)^2        (vertex at M: -197)
        |  (tilted       |              .
 -650   |   8.4 deg)     | start        ` .
 -694.5 |  cutter level  |                  ` - . _
 -730   |  head bottom   |                          ` - - . _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  -197
```

### 2.1 The one direction that does six jobs (why the square works)

In the square, c = (-1, 1, 0) / sqrt(2) is at once (a) the chamfer edge direction (`:206-207`,
fixed at 45 deg because the head is a square), (b) the oculus edge direction (`:174-179`, only
for gx = gy), (c) the inner-rib sweep `n0 - n1` (`:1127`, `:1131`), which also sweeps the soffit
edges of flanges 2a / 3b on the ribs' outer faces (`dir1` at `:970` -> `projection10` `:972`,
`:1005` -> `:1007`; their +t edges follow the outer rib normal, `:973`, `:1008`: the rib-sweep role
of R5, not the panel's), (d) the central-panel sweep (`:839`, `:843`, `:911-912`, `:981-984`,
`:992-996`), and (e) the one horizontal direction contained in both inner-rib end planes (the
chamfer plane is tilted about the chamfer; the oculus back face is vertical through the oculus
edge). Because of (e) the inner ribs' end
faces are planar and flush (`plan_probe.out`: 0.0000); because the two inner ribs are mirror
images about the diagonal, the central trough swept from rib 0's shadow also contains rib 1's
(`judge_check.py`: the c-projections of shadow 2 and shadow 3 onto the diagonal plane coincide
to 3.2e-13 mm). None of this is enforced; it is a coincidence of the square.

---------------------------------------------------------------------------------------------------

## 3. Neighbours: what must be identical

| shared entity | quarter q reads | its neighbour reads | identical by | compas_tf enforces it by | under quarter turns on 3000 x 2400 |
|---|---|---|---|---|---|
| bay edge E_q: the band `[edge plane, + outer_ribs]` | outer rib 0 (`outer_ribs[0]`, `:234`) | quarter q+1's outer rib 1 (`outer_ribs[1]`, `:235`) | both half ribs lie in one band, so their seam end faces coincide: 100 x 197 = 19700 mm2 (the tie) | the turn only (`example_model_4:65`) | the turned rib lies in the band of the other axis: 0 rib seams (`assembly_3000x2400_rotation.txt`) |
| seam plane S_q | inner beam 0 on `inner_beams[0][0]` (`:245`), outer rib 0's end cut (`:1076`) | quarter q+1's inner beam 2 on `inner_beams[2][0]` (`:249`), outer rib 1's end cut (`:1077`) | one plane, opposite normals; the two beam faces on it from the outer rib's inner face to the oculus plane, z in [-197, 0]: 376700.873 mm2 on the square (`reference_wedges.txt`) | the turn | coplanar but offset 522 mm: 209999.359 of 297515.590 / 328199.359 mm2 (`guide_3000x2400_rotation.txt:156-163`) |
| parabola vertex at M_q | its half of the edge, `-static_h` with a horizontal tangent (`:687-688`) | the other half | both halves of ONE edge have the same span, so the soffit is one parabola across the seam (C2); a horizontal tangent at `-static_h` on both sides gives C1 for any spans | the turn | the turned neighbour brings the other edge's span (2540 vs 1940) and does not meet (`guide_3000x2400_rotation.txt` section 3) |
| oculus corner O_q | polygon vertex 2 (`:190`) | quarter q+1's vertex 3 (`:191`) | the same point | the axes | 8 distinct corners instead of 4 (`guide_3000x2400_rotation.txt:94`) |
| oculus edge Q_q: tilted bearing plane, back face | inner beam 1 (`:1272-1281`), the inner ribs' end cut (`:1133-1134`), p0 / p1 (`:264-265`) | ring beam q's outer face (`:1341`), ring inner plane = back - 2 inner_beams (`:1330`) | the same tilted plane: 242696.248 mm2 contact on the square | the ring rotates quarter 0's plane (`:1328`) | the rotated ring misses quarters 1 and 3 by 73.5 mm (`guide_3000x2400_mirror.txt:166,168`); its eight outer corners are 242.6 / 249.9 mm (near the short-axis oculus points) and 379.0 / 439.0 mm (near the long-axis ones) from the quarters' oculus corners (`guide_3000x2400_mirror.txt:187-189`) |
| column corner K_q: head polygon, fan, levels, axis point | rib and block end cuts, blocks, beds, flanges (`:1076-1077`, `:1133-1134`, `:831`, `:846`, `:861`, `:963-1020`) | the column cutters (`:391-395`, `:400-401`) and the column placed at `corner_point_column` (`example_model_2:55`) | the carved faces ARE the rib end planes: 70214.105 mm2 per outer rib (`reference_rectangle.txt`) | the same plane objects, the same turn (`example_model_3:29`) | columns 1 and 3 land at (2290, -2890) / (-2290, 2890): 848.528 mm from where the in-place columns sit, (2890, -2290) / (-2890, 2290), and 862.670 mm from the bay corners (`guide_3000x2400_rotation.txt:95-99`; `skeptic_probe.out` B) |
| levels | z 0, `-static_h` (`:1259`), `-static_h + t`, `+ 2t` (`:1324-1326`), cutter levels (`:385-387`), lift (`example_model_4:56`) | every quarter and the ring | one storey | one guide | - |
| inner rib k and outer rib k | shadow = parabola k projected along the outer rib normal (`:699-707`) | - (inside the quarter) | the inner rib has the outer rib's profile at the same x (resp. y) | the projection | holds, but the two inner ribs then carry different profiles (L 2540 / 1940) |
| central panel and both inner ribs | bed row 1 from shadow 2 swept along `n0 - n1` (`:835-847`); flange 3a from shadow 3 (`:989-999`) | - | only by the diagonal mirror | nothing | 20.6 mm end mismatch, 411 mm between the curves (`probe_relations2.py`), see 4.1 |

The wood HEAD port reproduces every "enforced by the turn" entry literally
(`examples/templates_floor_{3,4,6,8}*.cpp`: `Xform::rotation_z(i * 90.0, true)`), so the table is
also the list of what the model has to make explicit.

---------------------------------------------------------------------------------------------------

## 4. What compas_tf does for a rectangle, with the probe's numbers

compas_tf can compute `FloorGuide(size_grid_x=3000, size_grid_y=2400)`; no example ever tiles it.
All numbers are for the example sizes with only `size_grid_y` changed.

### 4.1 Inside one quarter (`FloorGuide(3000, 2400)` as compas_tf computes it)

| item | square | rectangle | source |
|---|---|---|---|
| oculus points | 1000 / 1000 | 1250 / 800, aspect (gx/gy)^2; edge 1484.08 long at 147.38 deg, 12.381 deg off the 45 deg chamfer | `:56-57`; `judge_check.py` |
| inner rib axes | 34.296 / -124.296 deg to x, mirror images | 28.015 / -127.256 deg | `angles.txt` |
| sweep `d = n0 - n1` | = c (0.000 deg) | 4.621 deg off the chamfer, 17.001 deg off the oculus edge | `judge_check.py` |
| side fan tilts | 8.433 / 8.433 deg | 9.245 / 8.062 deg | `angles.txt` |
| parabola spans L, first chord slope | 2540 / 2540, 0.3270 | 2540 / 1940, 0.3270 / 0.4281 | `angles.txt` |
| rib bottom at the fan plane vs cutter level -694.55 | -694.793 (0.243 below) | x rib -691.663 (2.887 above), y rib -709.710 (15.160 below) | `angles.txt`; `guide_3000x2400_mirror.txt:145,149` |
| column contact of the outer rib end | 70214.105 = 99.965 % of 70238.715 | x rib 70076.481 = 100 %; y rib 70148.209 = 97.864 % of 71679.331 | `assembly_3000x2400_mirror.txt:46-48` |
| inner rib column-end faces vs the chamfer plane | 0.000 mm, 43080.376 mm2 each | two vertices 4.873 mm off, 43168.661 / 44247.407 mm2 | `quarter_3000x2400.txt` |
| inner rib oculus-end faces vs the back face | 0.000 | 17.960 mm off | `plan_probe.out:167-168` |
| inner_ribs - inner_beams contacts | 2 x 12029.331 mm2 | 0 | `quarter_3000x3000.txt:52,69`; `quarter_3000x2400.txt` |
| contacts inside the quarter | 100 | 90 (tsections-beds 36 -> 30, inner_ribs-beds 24 -> 23, inner_beams-beds 3 -> 2) | `quarter_*.txt` |
| rulings between same-index shadow vertices | all 135.00 deg | 126.56 .. 146.76 deg, fan 20.194 deg; a strip twists 23.554 mm | `design_probe.out` B |
| bed row 1 on rib 1 vs rib 1's own soffit | 0.00 | -15.4 .. -31.3 mm (same station), +5.2 .. -37.1 mm (station interpolated) | `design_probe.out`; `judge_check.py` |
| central bed quads along `n0 - n1`, twist from the column end | 0 | 26.32, 9.87, 5.02, 2.68, 1.28, 0.36 mm (29.58 along the chamfer) | `central_panel_probe.out` |
| bed row 1 edge vs flange 3a top on rib 1 | 0 | 411 mm apart (different curves); 20.6 mm end mismatch on rib 0 | `probe_relations2.py` (members map) |
| wedge seats on the head | 20 / 19.3 / 20 mm | 20 / 18.13 / 20 mm | `probe_relations2.py` |
| bed_top_planes normals | (-0.311, 0, 0.950), (-0.187, -0.187, 0.964), (0, -0.311, 0.950) | (-0.311, 0, 0.950), (-0.216, -0.184, 0.959), (0, -0.394, 0.919) | `angles.txt` |
| block level bottom (dead) | -614.921 | -604.366 | `plan_probe.out:175` |

Everything above the inner ribs row generalises; everything from it down is the diagonal
mirror failing. Rib and block side faces stay planar (`quarter_3000x2400.txt`: worst 0.000 mm),
the fan construction holds for any plan (each side plane's crease with the chamfer plane lies in
the inner rib's OUTER face `[k][0]` through the chamfer vertex, to 1e-11, `probe_relations.py:54-58`;
`skeptic_probe.out` A), the 6 rib / block contacts per quarter survive (165252.835, 186373.105,
183989.261, 184372.086, 145889.258, 146748.597 mm2).

### 4.2 Tiling by quarter turns (what the examples do)

`guide_3000x2400_rotation.txt`: the union of the four quarter polygons has 12 exterior vertices
and an 8-vertex hole, bounding box 6000 x 6000 (36.0e6 mm2 for 26.8e6 mm2 of floor); the outer
edges step 600 mm at every seam; seams share only 1150 of 1600 / 1750 mm; 8 distinct oculus
corners; columns 1 and 3 at (2290, -2890) / (-2290, 2890), 848.528 mm from the in-place column
centres (+-2890, +-2290) and 862.670 mm from the bay corners;
0 outer-rib seam contacts (the rib end lands on the neighbour's inner beam 2 instead); inner-beam
seams coplanar but 522.122 mm apart, overlap 209999.359 of 297515.590 / 328199.359 mm2; the
quarter's oculus beam meets the rotated ring over 191485.503 of 253629.409 mm2 plus 10160.992 mm2
on the previous ring beam.

### 4.3 Tiling by in-place construction (the mirror; what the model does)

`guide_3000x2400_mirror.txt`, `assembly_3000x2400_mirror.txt` (compas_tf's own plates rebuilt
from mirrored outlines, compas_tf's own detectors): union = the 6000 x 4800 rectangle with the
rhombus hole, 8 vertices, area 26.8e6 = 4 gx gy - 2 ox oy, no quarter overlap; columns at
(+-2890, +-2290); 4 outer-rib seams of 19700.000 mm2, 0.000 mm apart; 4 inner-beam seams fully
overlapping, 297515.590 (x = 0) and 328199.359 (y = 0) mm2; 8 column contacts (4.1); compas_tf's
rotated ring then meets only quarters 0 and 2 (6 of 8 ring contacts, nearest face 73.499 mm
away), while a ring built from the four quarters' own planes closes on itself
(`ring_bearing_probe.py`: beam overlap 0.000, ring area not covered by the beams 0.000 on five
bays) and covers every quarter's oculus beam face, 253629.409 mm2: measured with the probe's
mitred D2 ring (`guide_3000x2400_mirror.txt:204-209`). For the pinwheel from the four planes the
coverage is a condition, not a construction: ring beam q's outer face ends exactly at the corner
O_q at one end (`tilted_{q+1}` passes through O_q at z 0, `:246-248`, `:1340`) and
`inner_beams / sin(theta)` short of the corner O_{q-1} at the other (theta = the oculus corner
angle there, where ring beam q-1's inner plane crosses `tilted_q`), while quarter q's beam face
begins `inner_beams / sin(beta)` from that corner (beta = the angle between the seam and the
oculus edge there, the seam offsets `:253`, `:256`). Coverage needs `sin(theta) >= sin(beta)` at
every corner; with a rhombus on the seams (beta = theta / 2) that is theta <= 120 deg.
`square_diamond` gives 90 deg corners on every convex plan and always covers; compas_tf's rule
covers 3000 x 2400 with 5.1 mm to spare on quarters 1 / 3 (corners 65.2 / 114.8 deg) and fails
once gx / gy > 1.316: 15.2 mm of the quarter beam face uncovered on 3000 x 2000, 30.4 mm on
3000 x 1800 (`skeptic_probe.out` D). On the square the pinwheel gives the full 242696.248 mm2
against a 266094.970 mm2 ring face (`guide_3000x3000_rotation.txt:167-170`).

On the square the two tilings coincide: the pinwheel ring from the four quarters' planes equals
compas_tf's rotated ring to 1.990e-13 (`design_probe_a.out`), and the assembly reproduces
`reference_rectangle.txt` / `reference_tie.txt` / `reference_wedges.txt` exactly
(`assembly_3000x3000_mirror.txt`).

### 4.4 The central panel on a rectangle: the conflict and rule A

The three bed panels are parabolic cylinders: panel 0 swept square to the long outer rib, panel 2
square to the short one, panel 1 (central) between the two inner ribs' central faces. An inner
rib is the crease between two cylinders. With the central ruling locked to c and the ribs as
right prisms along c, the central cylinder through rib 0's trace cannot contain rib 1's once the
spans differ: 29.4 mm at the chamfer vertex on 3000 x 2400, 12.6 mm on 3000 x 2700; skewing the
outer panels' boards does not fix it (`trough_probe.out` S2-S7, which only ever vary the outer
rulings). That is where the two earlier designs stopped (a refusal, or a twisted rib soffit).

Rule A (`central_panel_probe.py`, verified independently by `judge_check.py`,
`judge/kappa_fixed_oculus.py`): free the central ruling u and the ribs' sweep r.

* u from the scale condition: projecting rib 0's central soffit trace along u onto rib 1's central
  face must span rib 1's own trace. Both traces are the same parabola family (vertex at the seam,
  `-static_h`, `rise`), and a projection between two lines along a fixed direction is affine, so
  matching the span and the start point matches the whole curve.
* r (one direction for both ribs) from the start-point condition: rib k's central trace is its
  shadow translated by `inner_ribs / (n_k . r)` along r.

Results (`judge_check.py`, `central_panel_probe.out`):

| bay, oculus | u off the chamfer | r off the chamfer | r vs rib 0 / rib 1 normal | shear across the 60 mm web | closure residual |
|---|---|---|---|---|---|
| square, 1000 | 0.000 | 0.000 (= c) | 10.7 / 10.7 deg | 11.3 / 11.3 mm (compas_tf's own) | 2e-13 mm |
| 3000 x 2400, compas 1250 / 800 | +16.059 | -33.942 | 17.0 / 41.7 deg | 18.3 / 53.4 mm | 4.6e-13 mm |
| 3000 x 2400, square diamond 1000 | +2.647 | -33.869 | 13.6 / 37.7 deg | 14.5 / 46.4 mm | 2.3e-13 mm |
| trapezoid 6000/4800 x 4800 (compas rule), quarters 0..3 | +23.738, -23.738, -6.936, +6.936 | -31.858, +31.858, +2.251, -2.251 | 17.3 / 41.5 deg (q0; swapped for q1), 18.4 / 10.7 deg (q2; swapped for q3) | - | <= 4e-13 |

The solution is a one-parameter family when each rib gets its own sweep (d_0, d_1): u is fixed by
the span ratio, d_0 is free and d_1 follows from the start-point condition. In the square every
member of that family is symmetric, from right prisms (each rib swept along its own normal,
0 / 0 deg) to compas_tf's c (10.7 / 10.7 deg), so neither "fairest" nor "equal obliqueness"
singles c out; the shared sweep does (`central_panel_probe.out`, "rule A family"). On 3000 x 2400
the equal-obliqueness member is d_0 -48.75 / d_1 -24.23 deg off the chamfer (31.8 / 32.0 deg to the
normals, closure 4.5e-13); either rib alone can be a right prism (d_0 = n_0 gives d_1 -41.219 deg,
rib 1 49.0 deg oblique with 68.9 mm shear; d_1 = n_1 gives d_0 -70.580 deg, rib 0 53.6 deg with
81.4 mm), only both at once is impossible (`skeptic_probe.out` C, `skeptic_family_check.py`). The
"quarter 1 / 3" family lines of `central_panel_probe.out` for the rectangle (d_0 -60.00 /
d_1 -73.02, 52.3 / 90.0 deg) are the probe's scan bound with d_1 lying in rib 1's face, not a
member: by mirror symmetry those quarters' member is +24.23 / +48.75. The probe script aborts in
its family scan after the first trapezoid (`TypeError` at `central_panel_probe.py:286`), so rule A
is verified on the square, two rectangles and one trapezoid, not on the general quadrilateral.

Layers. Rule A closes the soffit. compas_tf's +t / +2t layers are offsets made in the OUTER rib's
vertical plane and then projected and swept (`:693-694`, `:699-707`, `:835-847`), so in the
central panel their spacing is not t: on the square the central bed is 27.54 mm thick at the
column end tapering to 27.00 at the seam (cross-section, `synth_layer_probe.out`; 27.60 -> 27.01
measured along the face, `judge_check2.py`). On the rectangle rib 0's offsets swept along u miss
rib 1's own offsets by 5.365 .. 0.549 mm (+t) and 10.729 .. 1.099 mm (+2t) vertex by vertex
(`judge_check2.py`), so the two ribs cannot both keep their own offsets. Two consistent
definitions exist, both with every plate planar:

* `compas`: the panel's layers are rib 0's shadow offsets swept along u; flange 3a's top and bed
  row 1's rib-1 edge read the panel. Exactly compas_tf on the square. It fails the fixed rule and
  the tiling: the central bed is 27.54 -> 27.00 mm thick on the square and 26.86 -> 27.00 on
  3000 x 2400, a thickness that moves with the plan; and because the definition reads rib 0, a
  quarter whose rib 0 is the short rib (quarters 1 and 3 of a rectangle, `FloorGuide(2400, 3000)`
  in their frame) gets 28.12 -> 27.01 (+t) and 56.23 -> 54.01 (+2t) where quarters 0 and 2 get
  26.86 -> 27.00 and 53.72 -> 54.00 (`skeptic_probe.out` E): adjacent quarters differ by 1.26 /
  2.51 mm at the column end, so bed row 1, the tops of flanges 2b / 3a, `bed_top_planes[1]` and
  block 1 are not mirror images across the seam.
* `section`: the panel's layers are offsets of its soffit polyline in the panel's own
  cross-section, exactly t everywhere on every plan; one rule for the three panels (for panels 0
  and 2 it IS compas_tf's construction, because their cross-section is the outer rib's vertical
  plane), symmetric in the two ribs, so the mirror tiling holds for every member. On the square it
  moves the central row's +t vertices by 2.897 .. 0.285 mm and the +2t vertices by 5.794 .. 0.569
  mm along the rib faces from the column end to the seam, thickness 27.54 -> 27.00 and 55.07 ->
  54.00 become 27.00 and 54.00 (`synth_layer_probe.out`): bed row 1 (6 plates), flanges 2b / 3a
  (top edges), `bed_top_planes[1]` and the middle block move; nothing else does.

The fixed rule (a layer thickness is a member thickness) admits only `section`, and `section` is
not parity-preserving for that row: square parity of the central row and the rule are mutually
exclusive. The model's definition is therefore `section`; `compas` exists only as the parity mode
of the square gates until step 7 re-baselines the central row (open question 1 is whether the user
accepts that re-baseline, 2.9 / 5.8 mm at the vertices).

---------------------------------------------------------------------------------------------------

## 5. Critique of the wood port

### 5.1 HEAD `d15538c` (the quarter-turn port, after the revert)

A faithful transcription of compas_tf including every square-only assumption:

* `FloorGuide` carries `size_grid_x`, `size_grid_y` (`floor.h:40-41`), `oculus_points` with the
  (gx/gy)^2 scaling (`floor.cpp:94-100`), `corner_point_column(200)` with compas_tf's vestigial
  default (`floor.h:65`), and the dead `block_level_bottom/top` (`floor.cpp:224-234`), dumped by
  example 1 (`examples/templates_floor_1_floorguide.cpp:111-112`).
* Quarters, columns and the ring are placed by `Xform::rotation_z(i * 90.0, true)`
  (`examples/templates_floor_3_columns_model.cpp:30`, `_4_quarters.cpp:44`,
  `_6_contacts_floor.cpp:58`, `_8_contacts_cantilevers.cpp` via `add_models`); `add_column_model`
  and `add_quarter_model` take a placement (`floor_models.cpp:96-140`); the ring rotates quarter 0's
  planes (`floor_members.cpp:237-248`).
* The sweep `across = n0 - n1` is hard-wired for the inner ribs (`floor_members.cpp:59`), the
  soffit edges of flanges 2a / 3b (`:136` -> `:149`, `:167`), both edges of flanges 2b / 3a
  (`:154-162`), bed row 1 (`:202`) and bed plane 1 (`floor.cpp:240`): one vector where rule A has
  two directions (r for the rib sweeps, u for the panel).
* Connectors are found by search where the construction knows the pairs: 120 ring pairs for 8
  wedges (`floor_models.cpp:163-179`), 32 column x rib pairs for 8 plates (`181-196`), 28 rib pairs
  for 4 ties (`198-213`), 12 rib x block pairs per quarter on uncut clones for 6 dowel sets
  (`215-234`, `uncut()` at `69-82`). A pair that stops touching is silently skipped
  (`if (!contact ...) continue` at `171-172`, `189-190`, `206-207`), so only a count assertion can
  notice a lost connector.
* The literals 1.25 (`floor.cpp:74`), 1.65 (`floor_members.cpp:293`) and 0.25
  (`floor_members.cpp:312`) are copied without a name or a derivation; `CUTTER_MARGIN` 100
  (`floor_members.cpp:10`) and `EXTENSION` 1000 (`floor_geometry.cpp:9`) are named file constants
  with a one-line explanation each.
* The support frame and the column section are world-aligned squares
  (`floor_elements.cpp:65-85`), correct for the square only because the turn is applied afterwards.

Reproduced of compas_tf, verified: the 193-record example-1 dump at 0 deviation
(`compare_dumps.py reference_floorguide.txt`), examples 4 / 5 / 6 / 8 against
`reference_models.txt`, `reference_wedges.txt`, `reference_rectangle.txt`, `reference_tie.txt`
(the example-8 dump at HEAD carries the support joint, the 12 mm foot recess and the assembly
dowels, so its column and rib volumes, 172978229.06 and 98810031.25, are compared with the wood
baseline, while the test pins compas_tf's head cut 211196000.0 - 176418621.638340 (stock less the
carved column), 99598198.606 and 98812970.260 ribs and 1570456.693 tie on its own scenes,
`tests/floor_elements.cpp:15-19`; `reference_rectangle.txt`'s 172338552.51 is compas_tf's column
after the two plate pockets and the dowels and is pinned by no test).

### 5.2 The reverted generalisation `1b42862`

What the user rejected, point by point (lines are of that commit):

1. No floor object. `FloorGuide::rectangle(gx, gy, quadrant, sizes)` and
   `trapezoid(corners, quadrant, sizes)` return four independent quarters (`floor.cpp:97-117`); the
   seam planes, edge midpoints, oculus corners, levels, `size_inner_beams` and `bay_height` are
   stored four times and agree only because the caller passed the same arguments four times. The
   oculus took its planes from every quarter but its levels and lift from `quarters.front()`
   (`floor_members.cpp:229-243`, `floor_models.cpp:145`).
2. The sizes bag is a `FloorGuide` (`floor.cpp:105` copies a whole guide, points included, then
   overwrites five of them). After construction `size_oculus` is dead while `oculus_x/oculus_y` can
   be set to anything; the defaults (`floor.h:40-44`) silently encode quadrant 0 of the 6000 square.
3. An ad hoc oculus. `oculus_corner` (`floor.cpp:87-95`) puts corner k at
   `size_oculus |M_k| / sqrt(|M_{k-1}| |M_{k+1}|)` from the ORIGIN (`floor.cpp:89`). On a rectangle
   that reproduces compas_tf's (gx/gy)^2 scaling, itself never exercised; on a trapezoid shifted
   800 mm the corners land 751 .. 1383 mm from an origin that is not the bay's centre
   (`check_port.py` CHECK 2).
4. Quadrant indices instead of a bay: `static_cast<size_t>(quadrant) % 4` (`floor.cpp:99`) picks a
   corner from a list; nothing records that four guides belong together.
5. Members still swept square to their own planes (`floor.h:64` admits "a corner off the right
   angle leaves the rib ends off the column's carved faces"): 12.2-12.3 mm on an 82.9 / 97.1 deg
   trapezoid (`check_port.py` CHECK 2); the column became a rhombic prism and the support frame
   was built from non-orthogonal axes (`floor_elements.cpp:16-23, 72, 79-85`), which breaks the
   fixed-column rule. The trapezoid run shows the consequence: 18 faceted members, 32 "a face is
   not planar", 244 dowel stretches against 228 exact bores (`trap.out`).
6. The rectangle broke the hidden diagonal symmetry and nothing noticed: bed row 1's bottom misses
   flange 3a's top on inner rib 1 by 10.8 .. 31.3 mm (`check_port.py` CHECK 1) while the test
   checked counts, closedness and that the long rib is longer (`tests/floor_elements.cpp@1b42862:
   543-580`).
7. Connectors still searched (`floor_models.cpp:165-251`), so the lost contacts of 5 and 6 vanish
   silently.
8. Two kernel changes bundled to make the counts pass: the rectangle plate pocket opened through
   the column (`wood_element_joint_beam.cpp@1b42862:318`) and `unbridged()` loop surgery in
   `wood_brep_drill.cpp@1b42862:394`.
9. Naming and magic: `trapezoid` accepts any quadrilateral, `x_axis` / `y_axis` need not be
   orthogonal, `gx` is "half the bay" while the commit message says "a 3000 x 2400 bay".
10. What was not understood: compas_tf's floor is one quarter plus C4 symmetry. The port kept the
    quarter as the unit and removed the symmetry as a placement detail, leaving every formula that
    only works under that symmetry in place.

---------------------------------------------------------------------------------------------------

## 6. The model and the C++ API

### 6.1 Objects

```
FloorPlan (corners, oculus, rule)    FloorSizes (fixed)
            \                          /
             v                        v
                      Floor                      computed once, in the constructor:
   shared: centre C; edges[4] (bands); seams[4]; oculus_corners[4]; oculus_edges[4] (tilted, back,
   ring_inner); columns[4] (frame, head, chamfer_direction, wedge_fan, sides, levels, axis_point,
   support_plane, column_offset, wedge_seat)
   per quarter, geometry[4]: construction planes (re-origined, R1), quads, parabolas and shadows,
   central panel (u, r, section layers), bed_top_planes
            |                 |                      |
            v                 v                      v
      quarter(q)         oculus()               relationships()  ->  add_connectors / verify_contacts
      (a view)           (ring from the            (76 rows)
            |             four edges)
            v
   const references into geometry[q] -> members (outlines) -> elements -> scene
```

A quarter view stores nothing and derives nothing twice: it reads `floor.edges[q]`, `edges[q-1]`,
`seams[q]`, `seams[q-1]`, `oculus_edges[q]`, `columns[q]` and `geometry[q]`, and only builds
members (outlines) from them. The Floor constructor computes everything in dependency order per
quarter, because the chain is circular across the old view boundary (the wedge fan needs the inner
ribs' central faces, `:293-294`, `floor.cpp:67-68`; rule A needs the shadows, i.e. the parabolas
projected onto the inner rib faces, `floor.cpp:215-219`; the quarter's planes need the finished
fan): polygon -> outer rib bands -> seam and oculus planes -> inner rib planes -> wedge fan ->
parabolas and shadows -> central panel (rule A) -> bed planes, each stored in `geometry[q]` once.
HEAD rebuilds `construction_planes()` by value in every member function (`floor_members.cpp:44,
57, 75, 88, 132, 198, 229, 289`; `floor.cpp:164, 202, 226, 238`); the model stores it. Members are
built in world coordinates from world planes; no rotation or mirror transform is ever applied,
which is also why the kernel's refusal of mirrors (`wood_element_plate.cpp:225`, `is_mirror` at
`wood_element_geometry.cpp:507-514`) never triggers.

### 6.2 Construction rules

R1 Shared planes once. Every plane two members meet on is one object owned by the Floor and read
by both: the bay edge bands, the seam planes with a per-side sign, the oculus edges' tilted and
back planes, the column fans. A plane is its normal and offset; the origin is representation. The
Floor stores each shared plane with one origin, and the view's planes are copies re-origined at
compas_tf's points, the quarter polygon's half-edge midpoints (`floor_guide.py:233-249`,
`floor.cpp:128-136` today), because the dumps record origin + normal
(`templates_floor_1_floorguide.cpp:38-50`, `dump_floorguide.py:51`) and `compare_dumps.py` compares
every coordinate: quarter 0's two band records need (K_0 + M_0) / 2 and (M_3 + K_0) / 2
(`reference_floorguide.txt:4,6`: (-1500, -3000) and (-3000, -1500)), and quarter 1's record on the
same E_0 band needs (M_0 + K_1) / 2, so one stored origin satisfies none of the gates. The
re-origin changes no geometry and is done once, in the constructor (6.1); identity of neighbours
is then by construction, not by symmetry, and the tests (G6) check it as normal and offset.

R2 In-place quarters. Quarter q is built at its own corner in the right-handed frame (x along
E_q, y along E_{q-1} reversed; at a right corner the edge directions, otherwise symmetric about the
corner bisector), with compas_tf's construction verbatim. On the square this equals the turned
quarter (every step commutes with a rotation about Z); on any other plan it is the mirror tiling of
section 4.3 for every construction that is symmetric in the two ribs, which compas_tf's is except
where it reads rib 0 alone (bed row 1 from `boundary_parabolas[2]`, `:835-847`): with the
`section` layers of R5 the tiling holds for every member, with `compas` layers quarters 1 / 3 of a
rectangle differ from 0 / 2 in the central row (4.4).

R3 Oculus corners on the seams. `square_diamond`: O_q = C + oculus * unit(M_q - C), the same
distance on every seam (compas_tf for gx = gy). `compas`: compas_tf's `s gx/gy`, `s gy/gx`
(`:56-57`), equal on rectangles to the reverted port's formula; kept only for the per-view gate
against compas_tf's own `FloorGuide(3000, 2400)`. `explicit_distances`: four numbers. The oculus
shape follows: the bimedians bisect each other at C, so `square_diamond` gives equal diagonals
that bisect each other, a RECTANGLE with 90 deg corners on every convex plan (a square diamond on
a rectangular bay), `compas` a rhombus on a rectangular bay and a parallelogram otherwise,
`explicit_distances` a general quadrilateral. The ring coverage condition of R7 is part of
`FloorPlan::valid()`.

R4 End faces in the end planes, per rib face. A rib is the polyline cylinder of its soffit trace
swept along its sweep, bounded by its two end planes. Today the outline is cut on the first face
and translated to the second (`floor_members.cpp:17-40`, `floor_elements.cpp:24-47`: the far-face
corners are `high + across`), which puts the end face in the end plane only when the sweep lies in
that plane: true of every end on the square and of the outer ribs' fan ends everywhere (the side
plane contains the head edge, the rib's normal), false of the inner ribs' chamfer and back-face
ends (4.873 / 17.960 mm off on 3000 x 2400, 4.1) and of the seam ends on a trapezoid. The model
cuts the trace on EACH face by the end planes (the soffit extended by the 1000 mm first, as
compas_tf does, `:943-946`): the first and last sections of the loft take their far-face top
corner from the end plane's trace on that face at z 0 and their far-face soffit corner from where
the end plane crosses the far face's soffit (the first facet or its backward extension, since the
station shift across a 60 mm web is tens of mm against facets of about 420 mm). Every face stays
planar: the end quad lies in the end plane, the top at z 0, each facet strip in its own plane (its
far-face edge is that strip's own line). It is the same two-corners-per-station loft as today with
no `BeamVariable::cuts`, so `uncut()` (`floor_models.cpp:69-82`, which clears `cuts` and
`solid_cuts` wholesale) keeps every end face for the verification and the dowel search, the BRep
path is unchanged and on the square the sections are bit-identical to today's (the far corners
coincide with the translated ones). Bed and flange outlines are projected onto their face planes
first and cut there (compas_tf already does this for flanges, `:943-946`; beds cut first,
`:773-779`): identical whenever the sweep lies in the cut planes, i.e. everywhere on the square.
The dumped rib outline for the parity gate stays the cut outline, identical on the square. (The
alternative, end planes in `BeamVariable::cuts`, would need `uncut()` to tell them from the
connector cuts and would route every pocketed rib through `cut_mesh` + `solid_cuts_brep`,
`wood_element_beam_variable.cpp:79`, with a new topology; not chosen.)

R5 Panels as cylinders. Panel 0 is parabola 0 swept along +Y between the outer rib's inner face
and inner rib 0's outer face, panel 2 the same along +X, panel 1 the central cylinder of rule A
(R6). Flange soffit edges continue the rib soffit along the rib's sweep (1 / 4: the outer rib
normal; 2a / 3b on the inner ribs' outer faces and 2b / 3a on their central faces: r), flange tops
and bed layers follow the panel they sit in (1, 2a: +Y; 3b, 4: +X; 2b, 3a: u), compas_tf's own
split (`:928-952`: `projection10` for the soffit edge, `projection11` for the +t edge; `:970-973`,
`:1005-1008`). The layers are cross-section offsets (`section`, the model's definition) or
compas_tf's (`compas`, the parity mode), section 4.4.

R6 Rule A for the central panel. `CentralPanel { u, r }` per quarter: u by bisection on the span
ratio, r on the start point (both exact-reducing on the square: u = r = c). Rib k's central-face
trace = its shadow translated by `inner_ribs / (n_k . r)` along r; the central soffit = rib 0's
central trace swept along u, which contains rib 1's by construction (residual 1e-13). Both inner
ribs, bed row 1, flanges 2b / 3a and `bed_top_planes[1]` read that one cylinder. Tie-break: one
shared r (open question 2).

R7 Ring from the four edges. Ring beam q lofted between `oculus_edges[q].tilted` and
`ring_inner` (the back face offset back by 2 `inner_beams`, `:1330`: `inner_beams` inside the
edge toward the centre), from ring beam q-1's inner plane to ring beam q+1's tilted plane
(compas_tf's pinwheel `:1340`, `floor_members.cpp:248` today), four bottom wedges and the centre
plate unchanged (`:1364-1379`). The ring closes on itself for any convex quadrilateral
(`ring_bearing_probe.py`: beam overlap 0.000, ring area left uncovered 0.000 on five bays). It
covers the quarter beams' faces only when `sin(theta_j) >= sin(beta_j)` at every oculus corner j
(4.3: theta the corner angle, beta the angle between the seam and the next quarter's oculus edge
there), which holds for every `square_diamond` plan (90 deg corners) and for compas_tf's rule up
to gx / gy = 1.316; `FloorPlan::valid()` refuses a plan that fails it and
`FloorReport::ring_uncovered_mm2` measures it (quarter beam face area outside its ring beam's
face, 0 required).

R8 Fixed column. The column is the square shaft `column_head` with the 120 capitel and the 730
head at every corner; only its placement (frame, axis point), the carved fan (tilts, chamfer line)
and the middle cutter level are plan-dependent, the fan as it already is in compas_tf
(`:293-300`). At a non-right corner the bisector-symmetric square turns its faces by
(theta - 90) / 2 against both bay edges, with an effect of the same size at both kinds of corner
(`ring_bearing_probe.py`, 220 sin 3.565 deg = 13.7 mm on the 6000/4800 x 4800 trapezoid): at the
obtuse corner (97.13 deg) the outer rib band overhangs the column's outer face by 13.7 mm and the
side-wedge seat on the head grows from 20 to 33.5 mm (the band on the head face runs from -13.7 to
86.5 of 0..120); at the acute corner (82.87 deg) the column's outer face and the capitel stand
13.7 mm outside the bay edge and the seat shrinks to 6.1 mm (the band from 13.7 to 113.9). The
probe's "0 at the acute corners" is its `max(0.0, -lo)` (`ring_bearing_probe.py:37`), a
definition, not a result. `ColumnCorner::column_offset` is signed (+ band overhang, - column
outside the bay; 6.9 at 93.58, 15.6 at 98.11, 2.6 at 91.34 deg: about 1.93 mm per degree) and
`wedge_seat` reports the three seats. The fan side plane crosses the bay edge
`column_head / cos((theta - 90) / 2)` from the corner (220.43 at 82.87 deg), which is why the
parabola spans there are 2539.6 / 1958.3 / 1939.6 and not 2540 / 1958.7 / 1940 (1.2). Cutter
level: `CutterLevel::compas_factor` keeps -(height + 1.65 t) = -694.55 (`:386`),
`CutterLevel::rib_bottom` sets each corner's middle level to the deeper of its two outer ribs'
bottoms at the fan plane, compas_tf's unused `outer_ribs_bottom` intent (`:743-750`); one level
per corner, because the three top cutter quads and the three bottom ones share their corner
points (`:408-417`, `floor_members.cpp:304-320`). Open question 4.

R9 Connectors from relationships (section 8). The contact polygon of a named pair is read off
the outlines that share the plane; the kernel's `compute_face_contact` (`wood_session.h:102`) runs
only in `verify_contacts`, and `require_contact` throws naming the relation instead of skipping.

R10 Report. `Floor::check()` measures what compas_tf relied on silently and prints it on every
build; `ok()` asserts the structural relations (closure residual, end-face planarity, bed-flange
coincidence, seam and oculus identities) and warns on clearances (rib bottom vs cutter level, wedge
seat widths, column overhang, dowel counts).

### 6.3 API (namespace `wood_floor`, `src/templates/floor/`)

```cpp
/// The sizes that do not change with the plan: thicknesses, offsets, depths, angles, column and storey dimensions and compas_tf's two tuned factors.
struct FloorSizes {
    double column_head = 220.0;          // Side of the square shaft and of the head polygon at the corner (compas_tf size_column_head).
    double column_head_chamfer = 120.0;  // Where the chamfer vertices sit on the shaft faces; also the capitel width.
    double outer_ribs = 100.0;           // Outer rib thickness.
    double inner_ribs = 60.0;            // Inner rib thickness.
    double inner_beams = 60.0;           // Seam and oculus beam thickness; also the ring beam width at z 0.
    double wedge = 240.0;                // Side wedge block thickness; the middle block is middle_wedge_factor times it.
    double tsections = 27.0;             // Flange plane offset and bed layer thickness.
    double height = 650.0;               // Rib depth where the parabola starts, a wedge thickness past the column face.
    double rise = 453.0;                 // Parabola rise from there to the seam.
    double wedge_plane_angle = -10.0;    // Degrees the chamfer fan plane leans about its top edge.
    double oculus_plane_angle = 5.0;     // Degrees the oculus bearing plane leans about its top edge.
    double column_head_depth = 730.0;    // Depth of the carved head and of the capitel.
    double bay_height = 3500.0;          // Storey: the floor top above the slab, the column top.
    double middle_wedge_factor = 1.25;   // floor_guide.py:314: the middle block in wedge thicknesses.
    double static_h() const;             // height - rise: the depth at every seam and at the oculus.
};

/// How the oculus corners sit on the seams.
enum class OculusRule { square_diamond, compas, explicit_distances };

/// How the central panel's +t and +2t layers are made: offsets in the panel's own cross-section (the model: exactly tsections on every plan), or compas_tf's offsets in the outer rib's plane swept along the panel (the parity mode of the square gates).
enum class CentralLayers { section, compas };

/// The middle column cutter level: compas_tf's -(height + 1.65 tsections), or the deeper of the corner's two outer rib bottoms at the fan plane (R8).
enum class CutterLevel { compas_factor, rib_bottom };

/// The plan: four bay corners counter-clockwise at the datum z 0 and the oculus; everything else is derived.
struct FloorPlan {
    std::array<session_cpp::Point, 4> corners;
    double oculus = 1000.0;                                   // Distance of an oculus corner from the centre along its seam.
    OculusRule rule = OculusRule::square_diamond;
    std::array<double, 4> oculus_distances = {};              // explicit_distances only.
    static FloorPlan rectangle(double half_x, double half_y, double oculus = 1000.0, OculusRule rule = OculusRule::square_diamond);
    static FloorPlan quadrilateral(const std::array<session_cpp::Point, 4>& corners, double oculus = 1000.0, OculusRule rule = OculusRule::square_diamond);
    session_cpp::Point centre() const;                        // The vertex centroid, where the bimedians cross and bisect each other.
    session_cpp::Point midpoint(size_t k) const;              // Of edge k, corner k to corner k + 1.
    double corner_angle(size_t k) const;                      // Degrees.
    bool valid(std::string& why) const;                       // Counter-clockwise, convex, at z 0, oculus corners inside the quarters, rib starts beyond the chamfer vertices, ring coverage sin(theta_j) >= sin(beta_j) at every oculus corner (R7).
};

/// A bay edge: its line, midpoint and the two planes of the outer rib band on it, shared by the quarters on either side of the midpoint; stored with one origin, re-origined per quarter in geometry[q] (R1).
struct BayEdge { session_cpp::Line line; session_cpp::Point midpoint; std::array<session_cpp::Plane, 2> band; };

/// A seam: the line from an edge midpoint to the centre and its vertical plane; plane_into carries the normal into that quarter, faces_into adds the offset by inner_beams.
struct Seam {
    session_cpp::Line line; session_cpp::Plane plane;
    session_cpp::Plane plane_into(size_t quarter) const;
    std::array<session_cpp::Plane, 2> faces_into(size_t quarter) const;
};

/// The oculus edge of one quarter: its line, the tilted bearing plane the quarter beam and the ring beam share, the vertical back face, and the ring's inner plane.
struct OculusEdge { session_cpp::Line line; session_cpp::Plane tilted; session_cpp::Plane back; session_cpp::Plane ring_inner; };

/// A column corner: the fixed square column's frame, the head polygon, the carved fan, the cutter levels, the support plane and the column axis.
struct ColumnCorner {
    session_cpp::Point corner; session_cpp::Vector x_axis; session_cpp::Vector y_axis;   // Edge directions at a right corner, symmetric about the bisector otherwise.
    std::vector<session_cpp::Point> head;                                                // compas_tf quarter_column_polygon in the frame.
    session_cpp::Vector chamfer_direction;                                               // unit(head[3] - head[2]).
    std::array<std::array<session_cpp::Plane, 2>, 3> wedge_fan;                          // Side 0, chamfer, side 1 with their far faces (floor.cpp:59-80 today).
    std::array<session_cpp::Plane, 2> sides;                                             // The head edges on the bay boundary, normal into the bay (floor_guide.py:388-389).
    std::array<double, 3> levels;                                                        // 0, the middle level by CutterLevel (-694.55, or the deeper rib bottom: -694.793 on the square), -column_head_depth.
    session_cpp::Point axis_point; session_cpp::Plane support_plane; session_cpp::Line axis;
    std::array<double, 2> column_offset;                                                 // Per bay edge, mm, signed: + the outer rib band overhangs the column's outer face (obtuse corner), - the column stands outside the bay edge (acute); 0 at 90 deg (R8).
    std::array<double, 3> wedge_seat;                                                    // Side 0, chamfer, side 1 seats on the head, mm (20 / 19.3 / 20 on the square).
};

/// The central panel of one quarter by rule A: its ruling, the shared sweep of the two inner ribs, and the soffit, +t and +2t polylines in its cross-section.
struct CentralPanel {
    session_cpp::Vector ruling;                       // u.
    session_cpp::Vector rib_sweep;                    // r, one direction for both inner ribs.
    std::array<session_cpp::Polyline, 3> section;     // Soffit, +t, +2t in the plane perpendicular to the ruling.
    std::array<double, 2> obliqueness;                // Degrees between r and each rib's normal.
    double residual;                                  // Closure of rib 1's trace on the cylinder, mm.
};

/// The private geometry of one quarter, computed once by the Floor constructor (6.1): compas_tf's layout with outer_ribs from the bands, inner_beams from the seams and the oculus edge, wedges[0..2] from the column fan, every plane re-origined at the quarter's half-edge midpoints (R1).
struct QuarterGeometry {
    std::vector<session_cpp::Point> polygon;                               // Corner, midpoint, oculus corner, oculus corner, midpoint.
    ConstructionPlanes planes; ConstructionQuads quads;
    std::vector<std::array<session_cpp::Polyline, 3>> parabolas;           // Outer 0, outer 1, shadow 0, shadow 1, each with its +t / +2t.
    CentralPanel central_panel;
    std::vector<session_cpp::Plane> bed_top_planes;
    double block_level_bottom, block_level_top;                            // Dump only, kept for the 193-record gate.
};

/// One quarter as a view of the floor: the shared entities and its geometry read by reference, only members (outlines) built.
struct Quarter {
    const Floor& floor; size_t index;
    const QuarterGeometry& geometry() const;
    const ColumnCorner& column() const; const OculusEdge& oculus_edge() const;
    const Seam& seam(size_t side) const; const BayEdge& edge(size_t side) const;
    std::vector<Outline> outer_ribs() const, inner_ribs() const, inner_beams() const, wedges_inner_beams() const, tsections() const;
    std::vector<std::vector<Outline>> beds() const;
    std::vector<Outline> column_cutters() const;
};

/// The relations compas_tf relies on silently, measured per quarter; ok() when the structural ones hold.
struct FloorReport {
    std::array<double, 4> seam_plane_gap, oculus_corner_gap;              // mm, 0 by construction, checked anyway.
    std::array<double, 4> ruling_off_chamfer_deg, ruling_off_oculus_edge_deg;
    std::array<std::array<double, 2>, 4> rib_sweep_obliqueness_deg, rib_shear_mm;
    std::array<double, 4> closure_residual_mm, end_face_planarity_mm, bed_flange_coincidence_mm;
    std::array<std::array<double, 2>, 4> central_layer_shift_vs_compas_mm;  // +t, +2t vertex shift when CentralLayers::section is on; informational.
    std::array<std::array<double, 2>, 4> rib_bottom_clearance_mm;         // Outer rib bottom above the middle cutter level; negative = below the carved face.
    std::array<std::array<double, 3>, 4> wedge_seat_mm;
    std::array<std::array<double, 2>, 4> column_offset_mm;                // Signed, per corner and bay edge (R8).
    double ring_overlap_mm2;                                               // The ring beams' mutual overlap in plan, 0 required.
    double ring_uncovered_mm2;                                             // Quarter beam face area outside its ring beam's face, 0 required (R7).
    bool ok(double tolerance = 1e-6) const;
    std::string str() const;
};

/// The floor: a plan and sizes, every shared entity and every quarter's geometry computed once, quarter views on demand, the ring and the relationship table. A plain value, copyable and movable, since no view is stored.
struct Floor {
    FloorPlan plan; FloorSizes sizes; CentralLayers layers = CentralLayers::section; CutterLevel cutter_level = CutterLevel::compas_factor;
    session_cpp::Point centre;
    std::array<BayEdge, 4> edges; std::array<Seam, 4> seams;
    std::array<session_cpp::Point, 4> oculus_corners; std::array<OculusEdge, 4> oculus_edges;
    std::array<ColumnCorner, 4> columns; std::array<QuarterGeometry, 4> geometry;
    Floor(const FloorPlan& plan, const FloorSizes& sizes, CentralLayers layers = CentralLayers::section, CutterLevel level = CutterLevel::compas_factor);   // Throws when the plan is invalid. The one construction path: Floor floor(FloorPlan::rectangle(3000, 3000), FloorSizes{}).
    Quarter quarter(size_t q) const;                           // A view holding a reference; transient, never stored by the Floor.
    std::vector<Outline> oculus() const;                       // Four ring beams, four bottom wedges, the centre plate.
    std::vector<Relationship> relationships() const;           // Section 8.
    FloorReport check() const;
};

/// What two members share and the connector that belongs to it.
enum class Relation { support, cutter, column_plate, cross_lap, seam_tie, seam_wedge, oculus_wedge, block_dowels };
/// A member of the floor by quarter (-1 for the ring and the columns), family and index.
struct MemberRef { int quarter; Family family; size_t index; int row = -1; };
/// One relationship: the two members, the shared plane, the contact polygon read from their outlines, and the seam or corner it belongs to.
struct Relationship { Relation kind; MemberRef a; MemberRef b; session_cpp::Plane plane; session_cpp::Polyline contact; size_t seam_or_corner; };

/// The placed members of the whole floor by quarter and family, the ring beams, the columns and the supports.
struct FloorMembers {
    std::array<QuarterMembers, 4> quarters; std::vector<Member> ring;
    std::vector<std::shared_ptr<wood_session::Column>> columns; std::vector<std::shared_ptr<wood_session::Support>> supports;
    std::shared_ptr<session_cpp::Element> get(const MemberRef& ref) const;
};

FloorMembers add_floor(wood_session::WoodSession& session, const Floor& floor, const std::shared_ptr<session_cpp::TreeNode>& root);      // compas_tf's tree and names, lifted by bay_height, built in place.
std::shared_ptr<wood_session::Column> add_column_model(wood_session::WoodSession&, const Floor&, size_t corner, group);                 // Support, column, support joint, six cutters at corner k.
QuarterMembers add_quarter_model(wood_session::WoodSession&, const Quarter&, group);                                                     // The six families, suffix "_k".
std::vector<Member> add_oculus_model(wood_session::WoodSession&, const Floor&, group);
std::vector<std::shared_ptr<wood_session::JointBeam>> add_connectors(wood_session::WoodSession&, const Floor&, const FloorMembers&, group);   // One JointBeam per relationship through the existing factories.
std::vector<ContactMismatch> verify_contacts(wood_session::WoodSession&, const Floor&, const FloorMembers&, double tolerance = 1e-6);        // compute_face_contact on uncut copies against every constructed contact: plane, top edge, area.
std::shared_ptr<wood_session::InteractionContactFace> require_contact(wood_session::WoodSession&, const Member& a, const Member& b, wood_session::ContactType expected, const std::string& relation);   // Throws naming the relation.
```

The Floor stores no view, so it is an ordinary value (examples, tests and `add_floor` construct and
pass it freely); a `Quarter` holds a reference and lives as long as its call. `Family` moves from
`floor_models.cpp:9-16` into the header.

Unchanged: `ConstructionPlanes`, `ConstructionQuads`, `Outline`, `Member`, `to_rib` (except that the
end sections read their far-face corners from the second outline, R4), `to_beam`, `to_plate`, `outline_thickness`, `add_group`, `add_cross_laps`, `JointBeam`
factories, `floor_geometry.h`. Renamed: the scene struct `Quarter` (`floor.h:163-170`) becomes
`QuarterMembers`. Removed: `FloorGuide` (the view replaces it), `size_grid_x/y`,
`corner_point_column(200)`, `oculus_points`, `add_wedges`, `add_rectangle_plates`, `add_ties`,
`add_quarter_dowels` (their loops survive inside `verify_contacts`), the placement `Xform`
parameters.

### 6.4 What changes in the tree, file by file (HEAD lines)

* `floor.h:39-55` the inputs -> `FloorSizes` + `FloorPlan` + `Floor` + `QuarterGeometry` +
  `Quarter`; `62-74` plan functions move into `Floor` / `ColumnCorner`; `81-96` become the fields of
  `QuarterGeometry`, `103-124` keep their names on `Quarter`; `157-170` `Quarter` ->
  `QuarterMembers`; `173-197` the model functions as above.
* `floor.cpp:59-80` `wedge_planes` -> `ColumnCorner::wedge_fan` (same maths: the side planes take
  the DIRECTION of the chamfer plane's crease with the inner rib central faces, `67-70`, and pass
  through the head edges); `86-116` plan -> `Floor` constructor; `122-160` `construction_planes`
  reads the bands, `seams[q].faces_into(q)`, `seams[q-1].faces_into(q)`,
  `oculus_edges[q].{tilted, back}`, `columns[q].wedge_fan`, re-origined at the half-edge midpoints
  (R1), and is stored in `geometry[q]`; `200-222` parabolas unchanged, stored; `240` `rib_bisector`
  -> `geometry[q].central_panel.ruling`; `224-234` block levels kept for the dump.
* `floor_members.cpp:17-40` `rib()` cuts the trace on each face and fills the end sections' far
  corners from the end planes (R4); `44, 57, 75, 88, 132, 198, 229, 289` read `geometry()` instead
  of rebuilding the planes; `59` `across` -> `rib_sweep`; `136` `across` -> `rib_sweep` for the
  soffit edges of 2a / 2b / 3a / 3b (`149, 155, 161, 167`) and `ruling` for the +t edges of 2b / 3a
  (`156, 162`), with 2b / 3a tops and `180-221` bed row 1 reading `central_panel.section` (R5);
  `202` `across` -> `ruling`; beds project then cut; `227-258` oculus -> `Floor::oculus()` on
  `oculus_edges`; `287-332` cutters on `columns[q].levels`, `sides`, `wedge_fan` (`293` the level
  by `CutterLevel`).
* `floor_elements.cpp:24-47` `to_rib` reads the far-face corners of the first and last sections
  from the second outline (`bottom[1]`, `bottom[2]` and `bottom[0]`, `bottom[n-2]`) instead of
  `high + across`, nothing else; `65-85` `to_support` / `to_column` take a `ColumnCorner` (frame
  and square section from `x_axis`, `y_axis`; HEAD builds world-aligned squares at `67` and
  `74-82`).
* `floor_models.cpp:96-157` lose the placement `Xform` (members are built in place, lifted only);
  `163-249` become `add_connectors` over `relationships()` plus `verify_contacts`; `uncut()`
  (`69-82`) stays as is for the verification, since no rib carries plane cuts.
* Examples 1-8 construct `Floor floor(FloorPlan::rectangle(3000, 3000), FloorSizes{})` and read
  `floor.quarter(q)`; example 9 returns as the rectangle, example 10 as the trapezoid;
  `tests/floor_elements.cpp` keeps its constants (`15-19`) and gains the gates of section 9.
  Kernel: nothing.

---------------------------------------------------------------------------------------------------

## 7. Square, rectangle, trapezoid

### 7.1 Square (gx = gy): exact compas_tf

The four views are compas_tf's quarter 0 turned by q 90 deg; quarter 0 is bit-identical (the same
operations in the same order), quarters 1-3 equal the turned coordinates to roundoff (gate at
1e-6). The seam planes are x = 0 and y = 0; the ring from the four edges equals the rotated ring
(1.990e-13); the columns sit at `corner_point_column` turned by the frame, exactly
`example_model_2:55` + `_3:29`. Rule A gives u = r = c (`central_panel_probe.out`: 0.000 / 0.000,
residual 0), the per-face end corners coincide with the translated ones (every sweep lies in its
end planes), project-then-cut equals cut-then-project, and `CentralLayers::compas` reproduces the
central row while `section` moves it by the 2.897 / 5.794 mm of 4.4 (G1b). `check()` reports every
structural entry at 0. Names stay compas_tf's (`outer_ribs_<i>_<q>` ...).

### 7.2 Rectangle (gx != gy)

Plan: corners (+-hx, +-hy), seams on the axes, oculus a square diamond of half-diagonal `oculus`
by default. What holds by construction and was measured with compas_tf's own plates (section 4.3,
`OculusRule::compas` in the probes): the bay tiles without overlap; 4 rib seams of 19700 mm2;
4 full inner-beam seams; 8 column contacts; columns on the corners; the ring closes and meets every
quarter's oculus beam. What rule A adds (section 4.4): the inner ribs are oblique prisms along r
(13.6 / 37.7 deg to their normals with the default oculus, 14.5 / 46.4 mm shear across the web),
the central cylinder along u (+2.647 deg off the chamfer), both ribs' end faces in the chamfer plane
and the oculus back face (per-face end outlines, R4), bed row 1 and flanges 2b / 3a planar and
coincident on both ribs. What differs per edge: L = 2540 on the long edges, 1940 on the short ones,
so the short ribs are steeper (first chord 0.4281), deeper at the column (-709.71) and, with
`CutterLevel::compas_factor`, run 15.16 mm below the carved band (97.864 % of the end face bears,
70148.209 of 71679.331 mm2) while the long ribs' band is notched 2.887 mm below them
(`guide_3000x2400_mirror.txt:145,149`); `rib_bottom` carries both end faces fully (open question
4); the seam wedges get `int((edge - 3 thickness) / 320)`
dowels (3 x 67.08 = 201 on the square): seam beams of 1300 and 1900 with the default oculus give 3
on the short seams and 5 on the long ones, of 1500 and 1650 with the compas oculus 800 / 1250 give
4 and 4. The connector rules themselves read only the
contact (top edge, horizontal normal, world up; `connectors.py:395-457, 829-878, 1151-1189`;
`wood_element_joint_beam.cpp:193-253, 287-336, 340-365, 433-465`) and survive unchanged.

There is no compas_tf reference assembly for a rectangle; parity is defined per view against
compas_tf's own `FloorGuide(span_x(q), span_y(q))` for the members compas_tf gets right (gate R1),
and against the probes for the tiling.

### 7.3 Trapezoid and convex quadrilateral (`FloorPlan::quadrilateral`)

Same code path with these consequences: C is the vertex centroid, the seams are the half
bimedians (straight through C, so a seam beam of quarter q and of quarter q+1 share one plane),
the oculus is a rectangle (`square_diamond`) or a parallelogram (`compas`), R3; the two half ribs
of an edge share the band and meet on the oblique seam plane in coincident parallelogram end faces
(100 / sin(seam angle) wide, 197 deep, per-face cut by R4 since the rib normal is no longer in the
seam plane); the tie sits square to the seam plane as compas_tf places it
(`connectors.py:1151-1189`), so it is oblique to the ribs; the two seam beams' oculus-end cuts
differ slightly where the two oculus edges meet the seam at unequal angles (the seam contact is the
overlap, reported); the fixed square column is set symmetric about the corner bisector, so its
faces turn by (theta - 90) / 2 against both edges: the outer rib band meets its face obliquely,
ending on the fan plane by R4 (the reverted port's 12.2 mm offset disappears), the fan side plane
crosses the bay edge 220 / cos((theta - 90) / 2) from the corner (so L = 2539.6 / 1958.3 /
1939.6 on the probe trapezoid, 1.2), and 13.7 mm per 7.13 deg appear at both kinds of corner: the
band overhangs the column's outer face at the obtuse corners, the column's outer face and capitel
stand outside the bay edge at the acute ones, with the side-wedge seat 33.5 / 6.1 mm instead of
20 (R8). Verified: ring closure on three non-rectangular bays, the offset and seat numbers, rule A
on all four quarters of the 6000/4800 x 4800 trapezoid (u +23.738, -23.738, -6.936, +6.936 deg and
r -31.858, +31.858, +2.251, -2.251 deg off the chamfer for quarters 0..3, residuals <= 4e-13).
Not verified: rule A on the other two probe bays (the probe aborts), the oblique ties and the
column booleans at non-symmetric fans. Recommended order: rectangle first (open question 6).

---------------------------------------------------------------------------------------------------

## 8. Connectors by relationship

The `JointBeam` factories read only the contact polygon and the members' centroids
(`wood_element_joint_beam.cpp:195, 238-241` wedge; `289-296` rectangle plate; `342, 379` tie;
`435-439` dowels), `InteractionContactFace` has a public constructor from faces, type and polygon
(`src/joinery_solver/wood_interaction/wood_interaction_contact/wood_interaction_contact_face.h:31-37`),
and `add_connector` needs only the joint's targets (`wood_session.cpp:930-951, 1069-1091`). So a
relationship is (a, b, polygon, type) built from outlines. Outline index facts used: `loft_planes`
corner k lies on planes k and k+1 (`floor_geometry.cpp:163-186`); a rib's top outline is
`[p1, p0, soffit..., p1]` (`floor_members.cpp:34-36`).

| relation | pair (a, b), per q = 0..3 | shared plane | contact polygon | type | connector | count | compas_tf source of the pairing |
|---|---|---|---|---|---|---|---|
| seam_wedge q | inner beam 0 of q, inner beam 2 of q+1 | `seams[q].plane` | beam 0's loop on `plane_into(q)`: corners (band[1] x z0), (z0 x tilted), (tilted x -static_h), (-static_h x band[1]); 376700.873 mm2 on the square, top edge 1900 | side_side | `JointBeam::wedge`, margin 1.5 max(thickness), pocket 2/3 | 4 | `example_model_6:60-68` found it by search; the model knows it from `:245`, `:249` |
| oculus_wedge q | inner beam 1 of q, ring beam q | `oculus_edges[q].tilted` | beam 1's loop on the tilted plane, inside ring beam q's outer face when R7's coverage condition holds (always for `square_diamond`; `valid()` refuses the rest); 242696.248 mm2 | side_side | `wedge` (dowels flattened horizontal as `connectors.py:497-508`) | 4 | `:1272-1281` and `:1341` share the plane object |
| column_plate (q, 0) | column q, outer rib 0 of q | `columns[q].wedge_fan[0][0]` | the rib's column end face clipped at `levels[1]` (the carved face ends there, `:391-397`): 70214.105 of 70238.715 mm2 on the square | side | `JointBeam::rectangle_plate(column, rib, contact, rib.thickness)`, origin at the top-edge midpoint, e.g. (-2780, -2950, 3500) | 4 | `example_model_8:44-47`; the rib is cut by the plane the cutter is built on (`:1076`, `:393`) |
| column_plate (q, 1) | column q, outer rib 1 of q | `wedge_fan[2][0]` | idem | side | `rectangle_plate` | 4 | `:1077`, `:395` |
| cross_lap q | the two plates of corner q | - (their boxes cross in the 30 x 30 x 250 prism inside the column) | - | - | `JointBeam::cross_lap` | 4 | wood only; compas_tf leaves the boxes interpenetrating (`connectors.py:801-804`) |
| seam_tie q | outer rib 0 of q, outer rib 1 of q+1 | `seams[q].plane` | a's seam end face `{top[0], top[n-2], bottom[n-2], bottom[0]}`: 100 x 197 = 19700 mm2, wound toward q+1 | end_end | `JointBeam::tie` | 4 | `example_model_8:107-112`; compas_tf's male / female OBJ cutters followed the search order there (`connectors.py:1192-1223`); wood's tie has no male side |
| block_dowels (q, k) | block k with its two ribs: (outer 0, inner 0) for k = 0, (inner 0, inner 1) for k = 1, (inner 1, outer 1) for k = 2 | the rib pair planes `floor_members.cpp:91-95` = `:1204-1208` | the block's face on that plane: `{bottom[3], bottom[0], top[0], top[3]}` on planes[0], `{bottom[1], bottom[2], top[2], top[1]}` on planes[2]; 146247.3 / 186221.6 / 177024.3 mm2 on the square | side | `JointBeam::dowels` on uncut members, r 4, l 30, inset 50 | 24 | `example_model_8_contacts_quarter.py:51-58` (no connector in compas_tf) |
| support q | support q, column q | the column axis | - | - | `Joint::support` | 4 | `example_model_2:48-50`; `support.py:177` |
| cutter (q, 0..5) | column q, cutter plate | `wedge_fan`, `sides`, `levels` | - | difference | `Joint` solid difference | 24 | `:338-360`; `example_model_2:46` |

Totals: 44 contact relationships, 48 connectors (with the 4 cross laps), 76 rows with the supports
and cutters. `add_connectors` iterates `relationships()` in this order and keeps today's names
(`connector_wedge_k`, `connector_k`, `outer_rib_connector_k`, `connector_dowels_k`,
`connector_cross_lap_k`). Pockets of wedges, dowels and ties are assigned by the side of the contact
normal (`wood_element_joint_beam.cpp:239-243`, `379`, `439`), so their geometry is
order-independent: the tie is four mirror-symmetric pieces (`360`) with two mirror-image pocket
sets (`369-376`), each member taking the set on its own side (`379`), which is why every tied rib
has one volume (`tests/floor_elements.cpp:446-447`); the (a, b) order only fixes the `targets` /
`cutters` order in the pb (open question 5). The wedge margin keeps compas_tf's
`computed_thickness` (`plate.py:309-320`; 67.081863 / 68.578843 on the square).

`verify_contacts` runs `compute_face_contact` on uncut copies for every contact relationship and
reports plane angle, top-edge distance and area ratio. The square must give 44 / 44 within 1e-6
(the searched contacts are these faces); the rectangle 44 / 44 as well (the probes found the same
counts with mirrored plates). The search is kept only as this check: when geometry stops touching,
the constructed contact still exists and the mismatch is reported instead of a connector silently
vanishing.

---------------------------------------------------------------------------------------------------

## 9. Implementation plan and parity gates

Rules for every step (superproject `CLAUDE.md`): builds through `buildslot cmake --build build
--parallel 6` with `~/.local/bin/cmake`, every example through `tools/run_guarded.sh -t 10 -m 4`,
one heavy run at a time, `MINITEST_JOBS=6`, `CI=1 bash/minitest.sh --py --no-web` before a push,
`bash/ci_status.sh --wait` after it. One commit per step inside wood (the user commits), then the
superproject pointer. Every step ends green on every gate that existed before it.

References: `reference_floorguide.txt` (193 records), `reference_models.txt` (145),
`reference_wedges.txt` (65), `reference_rectangle.txt` (21), `reference_tie.txt` (17), generated
from compas_tf by `dump_floorguide.py` and `reference_*.py`; today in the session scratchpad, to
be moved into `data/reference/floor/` (open question 8). Comparison with
`scratchpad/compare_dumps.py <reference> <candidate> <tolerance>`.

| step | change | gates |
|---|---|---|
| 0 | Baseline: run examples 1, 4, 5, 6, 8 and the tests at `d15538c`, keep the dumps beside the references | G0 recorded |
| 1 | `FloorSizes` split out of `FloorGuide` (mechanical, nine files) | G1-G6 unchanged |
| 2 | `Floor`, the shared entities, `geometry[q]` and the `Quarter` view for the square; examples construct `Floor floor(FloorPlan::rectangle(3000, 3000), FloorSizes{})` with `CentralLayers::compas`; quarters in place, no `rotation_z` | G1 (dump bit-identical), G2, G3 |
| 3 | The ring from the four oculus edges | G2 (oculus records), G4 |
| 4 | Columns per `ColumnCorner` (frame, square section), cutters per corner | G3, G5 |
| 5 | `relationships()`, `add_connectors`, `verify_contacts`, `require_contact`; examples 6 and 8 and the tests call them | G4, G5, G6, G7 |
| 6 | The general rules, each square-neutral: (a) `chamfer_direction` / rule A (u, r) with `CentralLayers::compas` instead of `n0 - n1`; (b) per-face rib end outlines and project-then-cut beds; (c) `CutterLevel` with `compas_factor`; (d) `Floor::check()` printed by every example | G1 at 1e-6 (same vertices to roundoff), G2-G7 |
| 7 | `CentralLayers::section` as the default (the fixed rule; open question 1 is the re-baseline): the central-row records re-baselined with the measured shifts; `CutterLevel::rib_bottom` if adopted (open question 4), the head-cut constant re-pinned | G1 with the split tolerance of G1b; G3 re-pinned if question 4 is adopted |
| 8 | `examples/templates_floor_9_rectangle.cpp`: `Floor floor(FloorPlan::rectangle(3000, 2400), FloorSizes{})`, the whole bay, connectors, BReps, the report; `CMakeLists.txt` target | G8 (R1-R5) |
| 9 | `templates_floor_10_trapezoid.cpp` behind `FloorPlan::quadrilateral`, only after the rectangle is accepted | G9 |
| 10 | `docs/templates.md` and `docs/examples.md` updated, example descriptions, `pushmono`, CI green | - |

Gates:

* G1 Example 1: `compare_dumps.py reference_floorguide.txt data/output/pb/floor_1_floorguide.txt
  1e-6` prints `193 records, 0 failing` (planes as origin + normal, the re-origin of R1); after
  step 2 additionally byte-identical to the step-0 dump (`cmp`); after steps 3 and 5 byte-identical
  with the `oculus/*` records filtered out of both files (`grep -v '^oculus/'`), since the ring
  from the four quarters' planes equals the rotated ring only to roundoff (planes 1.570e-16,
  plates 1.990e-13, `design_probe_a.out`; cos(pi / 2) is 6.1e-17 in IEEE), those 18 records at
  1e-9. G1b after step 7: every record at 1e-6 except bed row 1,
  flanges 2b / 3a, `bed_top_planes/1` and `wedges_inner_beams/1`, which are compared with the
  re-baselined file and must show the shifts of section 4.4 (<= 2.897 mm +t, <= 5.794 mm +2t).
* G2 Examples 4 + 5 against `reference_models.txt`: 145 records matched by name, volume within 1e-9
  relative, box centre within 1e-6 (e.g. `outer_ribs_0_0 101714591.705756`, `oculus_0
  13625437.608763`, `oculus_8 45224696.115822`).
* G3 Example 2 and the test: support 500671.261678, head cut 211196000.0 - 176418621.638340
  (`tests/floor_elements.cpp:15-19, 140-147`).
* G4 Example 6 against `reference_wedges.txt`: 8 wedges, thickness 67.081863 / 68.578843, length
  1698.754410 / 1038.771407, 5 / 3 dowels, areas 376700.873272 / 242696.248126, carved beams
  17872870.438210 / 14355972.395447 / 10565126.977428 within 1e-9 relative, listed in seam then
  oculus order with the pair names of section 8.
* G5 Example 8: the contact records against `reference_rectangle.txt` (8 x 70214.105060, origins
  (-2780, -2950, 3500) ..., x axes) and `reference_tie.txt` (4 x 19700 at (0, -2950, 3500) ...);
  the volume records against the step-0 baseline (columns 172978229.06, ribs 98810031.25, ties
  1570456.692913, which include the support joint, the foot recess and the dowels); the console
  line `... 8 wedges, 24 dowel sets of 96 dowels, 8 rectangle plates with 4 cross laps, 4 ties`
  (`examples/templates_floor_8_contacts_cantilevers.cpp:228`), `0 faceted`, every dowel stretch an
  exact bore; R4 keeps every rib a plain loft with empty `cuts`, so on the square the elements and
  these records are the step-0 ones, not re-measured.
* G6 `tests/floor_elements` green under `MINITEST_JOBS=6`: unchanged constants and new checks:
  `seams[q].plane` equals quarter q's `inner_beams[0][0]` and quarter q+1's `inner_beams[2][0]`
  as normal and offset (opposite normals, origins differ by the re-origin of R1) to 1e-9;
  `oculus_edges[q]` equals quarter q's `inner_beams[1]` pair the same way;
  `Floor::oculus()` equals HEAD's `FloorGuide::oculus()` to 1e-9 on the square.
* G7 `verify_contacts` on the square: 44 / 44 within 1e-6 of the searched plane, top edge and area;
  `check().ok()` true with every structural entry 0.
* G8 Rectangle `Floor floor(FloorPlan::rectangle(3000, 2400), FloorSizes{})`:
  R1 per view, with `OculusRule::compas` and `CentralLayers::compas`, the guide records in the
  view's frame against compas_tf's `dump_floorguide.py` run with `size_grid_x = span_x(q)`,
  `size_grid_y = span_y(q)` (3000 / 2400 for q = 0, 2; 2400 / 3000 for q = 1, 3), coordinates
  mapped by the corner frame, 1e-6, for polygons, all planes (origin + normal, the view's
  re-origin of R1), quads, parabolas and shadows, outer ribs, inner beams, blocks 0 / 2, flanges
  1 / 4, bed rows 0 / 2, `bed_top_planes` 0 / 2 and the cutters with `CutterLevel::compas_factor`
  (the central-panel members and the inner ribs are excluded: compas_tf is inconsistent there).
  R2 tiling (compas oculus, the probes' numbers): 4 rib seams 19700.000, inner-beam seams
  297515.590 / 328199.359 fully overlapping, 8 column contacts 70076.481 / 70148.209 (of 71679.331
  with `compas_factor`; 71679.331 of 71679.331 with `rib_bottom`), ring-to-beam-1 contacts
  253629.409 x 4 (the full quarter beam faces, measured with the probe's mitred ring, section
  4.3), bounding box 6000 x 4800, columns
  at (+-2890, +-2290), 6 rib / block contacts per quarter (section 4.1); with the default oculus the
  same relations with the areas recorded at the first run.
  R3 central panel: inner rib end faces within 1e-9 of the chamfer plane and of the oculus back
  face; bed row 1 undersides equal to the flange tops on both ribs within 1e-9; every face planar
  within 1e-9; rule A u / r as in section 4.4.
  R4 ring: beam overlap and quarter beam face area outside the ring < 1e-6 (the compas oculus
  passes with 5.1 mm to spare on quarters 1 / 3, 4.3).
  R5 assembly: 4 / 4 / 8 / 4 / 24 connectors and 4 laps, `0 faceted`, every bore exact,
  `verify_contacts` 44 / 44.
* G9 Trapezoid 6000/4800 x 4800: R2 (a)-(c), R3-R5, square columns (section side 220 to 1e-9),
  orthonormal support frames, every outer rib end face within 1e-9 of its fan plane,
  `column_offset` -13.7 mm at the 82.87 deg corners (the column outside the bay edge) and +13.7 mm
  at the 97.13 deg corners (the band overhanging), wedge seats 6.1 / 33.5 mm, spans 2539.6 /
  1958.3 / 1939.6 (R8).

---------------------------------------------------------------------------------------------------

## 10. Open questions only the user can decide (with a recommended answer)

1. Central-panel layers (section 4.4). The fixed rule decides the definition: `section` (one rule
   for the three panels, exactly 27 on every plan, symmetric in the two ribs so the mirror tiling
   holds). The question is accepting its cost: the square's central row moves <= 2.90 / 5.79 mm at
   the vertices, 0.54 / 1.07 mm in thickness, and those records are re-baselined (G1b); `compas`
   (exact compas_tf on the square) keeps a bed thickness that varies with the plan and differs
   between adjacent quarters of a rectangle by 1.26 / 2.51 mm, so it stays only as the parity mode
   of the square gates before step 7. Recommended: `section`, as the last gated commit before the
   rectangle example, so every other step is proved against compas_tf first.
2. Rule A tie-break: one shared rib sweep r (reduces to compas_tf's direction in the square;
   13.6 / 37.7 deg obliqueness, 14.5 / 46.4 mm shear on 3000 x 2400 with the default oculus) or the
   equal-obliqueness member (31.8 / 32.0 deg, 37 mm each; does not single out c in the square where
   the whole family is equally oblique). Recommended: shared r, with the shear printed.
3. Oculus rule: `square_diamond` default (the oculus edge parallel to the chamfer, the carved
   chamfer exactly compas_tf's) with `compas` kept only for gate R1, or compas_tf's (gx/gy)^2
   scaling as the default (never exercised, 12.4 deg off the chamfer; a bay-similar rhombus would
   be ox/oy = gx/gy). Recommended: `square_diamond`.
4. Cutter middle level (`CutterLevel`, R8). The 1.65 is not a size but compas_tf's tuning of the
   relation "the carved band reaches the outer rib's bottom" (-694.55 against the square's
   -694.793), so keeping it fixed leaves that relation unenforced on every other plan: on
   3000 x 2400 the short rib's end face runs 15.16 mm below the band (97.864 % bears) and the long
   rib's band is notched 2.887 mm below the rib. `rib_bottom` derives each corner's level from the
   deeper of its two outer ribs (the shallower rib then sits over a band 18.05 mm deeper than
   itself on 3000 x 2400; on the square the cutters move 0.243 mm and the head-cut constant of G3
   is re-pinned). Recommended: `rib_bottom` as the model's definition in its own gated commit after
   the square gates, `compas_factor` kept as the parity mode; the report prints the clearance
   either way.
5. Tie pair order: wood's tie has no male side (section 8: mirror-symmetric pieces, each member
   pocketed on its own side of the contact normal, `wood_element_joint_beam.cpp:360-379`), so the
   rule "(outer rib 0 of q, outer rib 1 of q+1)" changes only the `targets` / `cutters` order of
   seam 3 against HEAD's list-order accident, no geometry (volumes identical,
   `tests/floor_elements.cpp:446-447`). The rule, or the accident for byte identity of `live.pb`?
   Recommended: the rule.
6. Trapezoids: wanted at all before the rectangle is accepted? And the column frame at a non-right
   corner: bisector-symmetric (both edges alike, about 1.93 mm per degree: the band overhangs the
   column at an obtuse corner, the column face stands outside the bay edge at an acute one, R8) or
   edge-aligned. Recommended: rectangle first; bisector-symmetric when the time comes.
7. Wedge sizing thickness: compas_tf's centroid distance (67.08 / 68.58, `plate.py:309-320`) or the
   plane offset 60 (changes lengths and dowel counts). Recommended: keep compas_tf's.
8. Move the five compas_tf reference dumps into `data/reference/floor/` so the gates run from the
   repository. Recommended: yes.
9. Keep `block_level_bottom/top` in the example-1 dump (the 193-record gate) or drop them with a
   regenerated reference. Recommended: keep.
10. The two kernel changes of `1b42862` (plate pocket through the column, `unbridged()`): not part
    of this model; evaluate only if G8 shows faceted members, as their own volume-gated commit.
    Recommended: not reintroduced by default.
11. `FloorSizes` defaults = the example set (220 / 120 / 240) instead of compas_tf's class defaults
    (250 / 100 / 100, not a working configuration). Recommended: the example set.
12. The port's documented deviations stay: ribs and beams as variable beams, the parametric tie,
    the column foot sunk 12 mm into the head plate (stock 211,776,800 vs 211,196,000 mm3). Recommended:
    keep.

---------------------------------------------------------------------------------------------------

## Appendix A. Probe scripts and outputs (session scratchpad `floor_model/`)

Run as `PYTHONPATH=compas_tf/src timeout 10m systemd-run --user --scope -q -p MemoryMax=6G
../tfenv/bin/python <script>`; each takes 2-3 s and under 100 MB.

| file | what it measures |
|---|---|
| `plan_probe.py` -> `plan_probe.out` | every construction plane, p0..p3, sweep vs cut planes, rib end offsets, block levels for 3000x3000, 3000x2400, 2400x3000 |
| `probe_relations.py`, `probe_relations2.py` | crease construction, seats, plane1_offset, cutter level vs rib bottom |
| `probe_guide.py` -> `guide_<gx>x<gy>_{rotation,mirror}.txt` | tiling, seam faces, parabolas, rib ends vs cutters, inner beams, ring |
| `probe_assembly.py` -> `assembly_*.txt` | compas_tf's own detectors on rebuilt plates: ring, column, seam contacts |
| `probe_quarter.py` -> `quarter_*.txt` | face planarity, inner rib end faces, block faces, internal contacts |
| `probe_angles.py` -> `angles.txt` | fan tilts, ring widths, chord slopes, bed planes |
| `design_probe*.py` -> `design_probe*.out`, `design_probe_a.out` | ring from mirrored planes, rulings fan, cylinder condition |
| `trough_probe.py` -> `trough_probe.out` | the three-trough conflict with the central ruling locked to c |
| `twist_probe.py` -> `twist_probe.out` | the ruled-rib alternative's twist (9.81 / 11.22 mm on 3000x2400) |
| `central_panel_probe.py` -> `central_panel_probe.out` | rule A, its family, rule B twists, five bays (aborts after the first trapezoid) |
| `ring_bearing_probe.py` | pinwheel closure on five bays, column overhang |
| `judge_check.py`, `judge_check2.py`, `judge/*.py` | independent re-derivations of rule A, the layers, kappa with a fixed oculus, the bisector-plane check |
| `synth_layer_probe.py` -> `synth_layer_probe.out` | `compas` vs `section` layers on the square and the rectangle |
| `skeptic_probe.py` -> `skeptic_probe.out`, `skeptic_family_check.py` | the fan crease's face, the turned columns' distances, rule A and its right-prism members, pinwheel coverage on eight plans, `compas` layers on the mirror tiling |

## Appendix B. Reference numbers of the square

Planes: outer ribs y = -3000 / -2900 and x = -3000 / -2900; seam beams x = 0 / -60, y = 0 / -60;
tilted oculus normal (-0.7044, -0.7044, -0.0872), back face offset 60; inner rib normals
(-0.5635, 0.8261, 0) / (0.8261, -0.5635, 0), p0 (-60, -1024.85), p2 (-2780, -2880); fan planes
(0.9892, 0, 0.1466) at (-2780, -2940), (0.6964, 0.6964, 0.1736) at (-2830, -2830), far faces 240 /
300 / 240. Parabola 0: (-2540, -3000, -650) .. (0, -3000, -197), offsets ending at -170.012 /
-143.024. Block levels -614.921 / -374.921. Bed planes z -550.203 with normals (-0.311, 0, 0.950),
(-0.187, -0.187, 0.964), (0, -0.311, 0.950). Outer rib corner on the fan plane
(-2677.03, -3000, -694.79). Ring beams between the tilted planes and the vertical planes 647.1 from
the origin, ring -197 .. -170, plate -170 .. -143. Contacts: seam wedges 376700.873272, oculus
wedges 242696.248126, column plates 70214.105060, ties 19700, rib / block 146247.3 / 186221.6 /
177024.3 mm2. Wedge connectors 1698.754410 (5 dowels) / 1038.771407 (3), thickness 67.081863 /
68.578843. Tie key 1570456.693007. Head cut 211196000.0 - 176418621.638340.

---------------------------------------------------------------------------------------------------

## Implemented

Branch `floor-model`, one commit per step, every gate of section 9 run under `tools/run_guarded.sh`
against the step-0 baseline (examples 1, 2, 4, 5, 6, 8 and the four tests built at `0d123ce`) and the
compas_tf references in `data/reference/floor/`.

* Step 0, `0d123ce`: the references and `tools/compare_dumps.py` in the repository (open question 8).
* Step 1, `3a61a0f`: `FloorSizes` (220 / 120 / 240, question 11); every dump byte-identical.
* Step 2, `a4b0f86`: `Floor`, `FloorPlan`, the shared entities, `geometry[q]`, the `Quarter` view,
  quarters in place; G1 byte-identical, G2, G3.
* Step 3, `1df0b60`: the ring from the four oculus edges; G1 byte-identical without `oculus/*`, those
  at 1e-9; G2, G4.
* Step 4, `5a3bfda`: columns per `ColumnCorner`; G3, G5.
* Step 5, `e448bd7`: `relationships()` (76 rows), `add_connectors`, `verify_contacts` (44 / 44),
  `require_contact`; G4-G7. Deviation: the wedge dowels are numbered along the contact as compas_tf
  numbers them, so `connector_wedge_k_cylinder_j` is the step-0 `_cylinder_{n-1-j}` (the same set).
* Step 6, `249591d`: rule A (`CentralPanel`: the shared rib sweep r found by a scan and bisection of
  the closure nearest n0 - n1, the ruling u from the start chord; on the square r = u = c, 10.704 deg
  oblique, 11.342 mm shear, closure 3.1e-12), per-face rib end outlines (R4: the far face's end
  corners on the end planes along the first and last facet, `to_rib` reading them), beds and bed
  planes projected first and cut on each face, `CutterLevel` with `compas_factor`, `Floor::check()` /
  `FloorReport` printed by every example; G1 0 failing and equal to step 0 at every printed digit,
  G2-G7, report ok. Deviations: `CentralPanel` stores the six central traces (soffit, +t, +2t on both
  central faces) and the informational `layer_shift` instead of the cross-section polylines; a bed row
  whose layers are cut on different facets on its two faces throws instead of being built (never on
  the square or the rectangle); `bed_flange_coincidence_mm` measures every bed underside corner
  against the top loop of the flange beside it; `central_layer_shift_vs_compas_mm` is the same-index
  vertex shift the design names, which on a rectangle is dominated by the stations sliding along the
  layer (40-73 mm on 3000 x 2400), not a distance between the layers.
* Step 7a, `a9092e6`: `CentralLayers::section` the default (question 1), `Floor::compas_parity` and
  `--compas` on every example for the parity gates; G1b: against compas_tf only bed row 1, flanges
  2b / 3a, `bed_top_planes/1` and `wedges_inner_beams/1` move, at most 2.666 mm (+t) and 5.332 mm
  (+2t) at the trimmed vertices; central beds 27.000000000 thick (compas_tf's 27.003 .. 27.390);
  re-baselined in `model_floorguide.txt` and `model_models.txt`.
* Step 7b, `cc760d6`: `CutterLevel::rib_bottom` the default (question 4), -694.793 on the square;
  the cutters move 0.243 mm at the level, the column contacts become the full 70238.714887 mm2, the
  model's head cut 34771221.351479 pinned beside compas_tf's 34777378.362; `model_contacts_cantilevers.txt`.
* Step 8, `c3084b4`: `templates_floor_9_rectangle`; G8 R1 141 records at 0 deviation in all four views
  against `FloorGuide(3000, 2400)` / `(2400, 3000)` (`oculus_points` left out too: compas_tf lists the
  four corners from its own quarter 0); R2 the probes' areas; R3 end faces 5.1e-13 mm, beds on flanges 0,
  every member face planar within 4.5e-11 mm, rule A 2.647 deg / 13.640 / 37.681 deg; R4 ring overlap
  1.1e-11 mm2, uncovered 0; R5 48 of 48 connectors, 44 of 44 contacts, 0 faceted, 384 of 384 bores
  exact. No kernel change (question 10 not needed). The bounding box and the column positions hold by
  construction (corner frames and axis points) and are not measured separately.
* Step 9 (trapezoid) skipped by decision (question 6).
* Step 10: the example descriptions, the floor section of `docs/templates.md`, this note;
  `docs/floor_model_report.md` lists the numbers per step.
