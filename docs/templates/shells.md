# Shells {#templates_shells}

[TOC]

Plates and boards on a surface: a translation shell swept from two polylines, and a two-layer lamella gridshell on a NURBS surface. Every picture is the example's `data/output/pb/live.pb` rendered by session_viewer; its code folds open under it.

## translation_shell

A cross section polyline swept along a profile polyline by accumulating the profile's displacement steps: a quad mesh, then `Mesh::miter_contours` gives every quad a bottom and a top outline at the plate thickness, and the corners sharper than the chamfer angle are chamfered. `TranslationShell` holds the mesh and the plates in `elements`.

![translation shell](templates/templates_shells_1_translation_shell.png)

<details><summary>Example code: templates_shells_1_translation_shell.cpp</summary>

\include{lineno} templates_shells_1_translation_shell.cpp

</details>

## lamella_gridshell

A two-directional, two-layer lamella gridshell on a NURBS surface, after the asymptotic gridshells of Eike Schling. `curves` picks the two lamella families: 0 the u and v iso-curves, on any surface; 1 the asymptotic curves, where the normal curvature II(d, d) is zero, on a surface of negative Gaussian curvature. Each family is traced in (u, v) with fixed RK4 steps of `step` mm, as Bowerbird does, from seeds at even arc lengths along a spine of the other family through the middle of the domain, until it reaches the surface edge; the crossings are found segment against segment in (u, v). The first family is the top layer, `spacing / 2` up the local surface normal, the second the bottom layer, as far down.

Every lamella is two upright boards, `height` along the local normal and `thickness` across, `gap` apart: each board is a `Beam` on the lamella centreline with a `profile_rectangle(thickness, height)` section moved up and across in the section frame, and the local normal as its up direction at every station, so every section is the rectangle the local normal and the normal cross the tangent span, and the board sides are ruled by the surface normals. The boards run straight for a gap either side of every crossing and past each end, and follow the curve between. At every crossing a `Column` stud runs along the normal through the gaps of both layers, `overrun` past each outer face. Its section is a hexagon of three flat pairs, all `gap` across: one pair against the top boards, one against the bottom boards and one across the long corners, so the stud touches all four boards and cuts none; a 60 degree crossing gives the regular hexagon. `spacing` must be at least `height`, or the layers overlap.

Along an asymptotic curve the normal curvature is zero, so a board standing on the normal bends only about its weak axis and unrolls to a straight strip: the point of the system. The example builds a 10 m saddle in asymptotic mode and a 6 m one in iso mode beside it, and prints for each the largest normal curvature along the lamellas, the largest distance of an unrolled lamella from a straight line and the largest tilt of a board section from the normal; both first numbers are about zero for the asymptotic shell and large for the iso one. It then finds the face contacts, fails unless every stud touches its four boards, and cuts every two convex pieces (a board segment, a stud) of different elements by each other, failing on any volume left.

![lamella gridshell](templates/templates_shells_2_gridshell.png)

![one crossing: two boards per layer and the stud in both gaps](templates/templates_shells_2_gridshell_joint.png)

<details><summary>Example code: templates_shells_2_gridshell.cpp</summary>

\include{lineno} templates_shells_2_gridshell.cpp

</details>
