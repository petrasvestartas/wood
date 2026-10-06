# Folding {#templates_folding}

[TOC]

Folded plate structures: a reflex fold, the chevron strips of an Annen surface, and a diamond mesh. Every picture is the example's `data/output/pb/live.pb` rendered by session_viewer; its code folds open under it.

## reflex_fold

The profile folded along the cross section: each profile row is projected onto the perpendicular bisector plane at each cross-section point, so the strips alternate in a reflex fold. One plate per fold with its own bottom and top chamfer.

![reflex fold](templates/templates_folding_1_reflex_fold.png)

<details><summary>Example code: templates_folding_1_reflex_fold.cpp</summary>

\include{lineno} templates_folding_1_reflex_fold.cpp

</details>

## chevron

The Annen shell: one of the 23 NURBS surfaces in `data/annen_surfaces.json` divided into chevron strips, eight outlines per face folded into four plates, with the insertion vectors, joint types, three-valence groups and adjacency the solver reads. `wood_chevron::annen_surfaces` loads the surfaces, `Chevron` builds the mesh and the plates.

![chevron](templates/templates_folding_2_chevron.png)

<details><summary>Example code: templates_folding_2_chevron.cpp</summary>

\include{lineno} templates_folding_2_chevron.cpp

</details>

## diamond_mesh

A NURBS surface split into a rhombus pattern: triangle pairs that share alternating edge-midpoint vertices, six triangles per cell on the first row and four after, welded into one mesh. `Mesh::miter_contours` gives every triangle its bottom and top outline and the corners sharper than the chamfer angle are chamfered. The default surface is a bicubic arch, 3000 by 5000 with a rise of 1500.

![diamond mesh](templates/templates_folding_3_diamond_mesh.png)

<details><summary>Example code: templates_folding_3_diamond_mesh.cpp</summary>

\include{lineno} templates_folding_3_diamond_mesh.cpp

</details>
