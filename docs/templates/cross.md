# Cross {#templates_cross}

A plate per mesh face with connector plates across every interior edge. Every picture is the example's `data/output/pb/live.pb` rendered by session_viewer; its code folds open under it.

## vda_mesh

Any mesh into plates and connectors: every face gets a bottom and top outline per face position, its sides cut back by the bisector planes between it and its neighbours so the plates meet in mitres; every interior edge gets a row of connector rectangles across it, two per subdivision, on the planes perpendicular to the edge. `VdaMesh` keeps the outlines in `f_polylines` and `e_polylines` as bottom, top pairs; the example turns each pair into a `Plate`. The default mesh is a fifteen-face hexagonal dome.

![vda mesh](templates/templates_cross_1_vda_mesh.png)

<details><summary>Example code: templates_cross_1_vda_mesh.cpp</summary>

\include{lineno} templates_cross_1_vda_mesh.cpp

</details>
