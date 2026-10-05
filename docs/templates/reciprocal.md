# Reciprocal {#templates_reciprocal}

Reciprocal frames: one beam plate per mesh edge, moved or rotated past its neighbours. Every picture is the example's `data/output/pb/live.pb` rendered by session_viewer; its code folds open under it.

## reciprocal_move

A nexorade by translation: the edges of a quad or hexagonal mesh on a surface become beams, the even edges of every face moved sideways in the face plane by the shift, so each beam bears on the next; every end is cut flush against the beam it lands on. The boundary is a frame of straight beams with one tilt per boundary curve, mirrored sections across every mitre and butt corners (`reciprocal_boundary.h`). `SURFACE` and `GRID` pick the case, `reciprocal_surface.h` holds the test surfaces and the meshes on them. One plate per beam.

![reciprocal move](templates/templates_reciprocal_2_move.png)

<details><summary>Example code: templates_reciprocal_2_move.cpp</summary>

\include{lineno} templates_reciprocal_2_move.cpp

</details>

## reciprocal_rotation

A nexorade by rotation: every mesh edge is stretched about its midpoint and turned about the edge normal, so the beams round each vertex form a pinwheel; each end then stops at the first side face it would cross. The same boundary frame as the move template. At home on quads, since on hexagons the rotated beams can cross each other.

![reciprocal rotation](templates/templates_reciprocal_1_rotation.png)

<details><summary>Example code: templates_reciprocal_1_rotation.cpp</summary>

\include{lineno} templates_reciprocal_1_rotation.cpp

</details>
