# Wood

![](diagrams/wood_architecture.webp)

Wood is a timber joinery solver and parametric structure library built on [`session_cpp`](https://github.com/petrasvestartas/session_cpp). Elements represent plates, curved plates, beams, columns, and connectors; joints detect where elements meet and cut mortises, tenons, finger joints, cross laps, and pin bores directly into their meshes and exact NURBS BReps. Parametric templates generate complete shells, folded plates, reciprocal frames, modular walls, trusses, grids, and vaulted floors ready for viewing and fabrication.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="architecture.html">
    <img src="wood_architecture.webp" alt="Architecture">
    <span class="wood-card-title">Architecture</span>
    <span class="wood-card-sub">WoodSession scene graph, contact detection &amp; boolean/BRep cutting</span>
  </a>
  <a class="wood-card" href="elements.html">
    <img src="wood_elements_taxonomy.webp" alt="Elements">
    <span class="wood-card-title">Elements</span>
    <span class="wood-card-sub">11 Shape Elements and 16 JointPlate / JointBeam families</span>
  </a>
  <a class="wood-card" href="templates.html">
    <img src="wood_templates_pipeline.webp" alt="Templates">
    <span class="wood-card-title">Templates</span>
    <span class="wood-card-sub">12 parametric structural templates from shells to Gothic vaults</span>
  </a>
  <a class="wood-card" href="examples.html">
    <img src="wood_boolean_cutting.webp" alt="Examples &amp; Cutting Gallery">
    <span class="wood-card-title">Examples</span>
    <span class="wood-card-sub">23 self-contained C++ targets, cutting gallery &amp; butt joinery</span>
  </a>
</div>
<div style="display:none">
![](diagrams/wood_elements_taxonomy.webp)
![](diagrams/wood_templates_pipeline.webp)
![](diagrams/wood_boolean_cutting.webp)
</div>
