# Wood

Wood is the joinery solver of the doctoral thesis [Design-to-Fabrication Workflow for Raw-Sawn-Timber using Joinery Solver](https://infoscience.epfl.ch/record/288650) (Petras Vestartas, EPFL thesis 8928, IBOIS, 2021, advisor Yves Weinand), carried on as a C++ library on the [session_cpp](https://github.com/petrasvestartas/session_cpp) kernel. The thesis asked how a timber structure can go from a design model to fabrication data without modelling its joints one by one: the members are given, the solver finds where they touch and cuts the joints from the contacts, so the joinery follows from the geometry and changes with it.

A model is a session of elements and interactions. The elements are the timber members and what acts on them: plates and curved plates, beams, curved and variable beams, columns, blocks, supports, cut planes, profiles and pins. An interaction is what one element does to another: a contact, a glued block, a cut, a joint. The joint library holds the plate joints of the thesis and of the IBOIS prototypes it grew from, through-tenons, finger joints, dovetails, keys, hooks and pins, and the beam joints, cross laps, scarfs, butts, wedges, connector plates and pin sets, cut into meshes and exact NURBS BReps. Templates build whole structures from a guide, the Annen chevron shells, folded plates, floors, grids, trusses and vaults, as sessions the solver joins, and every step of every template is an example that runs on its own.

<div class="wood-card-grid cols-2">
  <a class="wood-card" href="architecture.html">
    <img src="wood_architecture.webp" alt="Architecture">
    <span class="wood-card-title">Architecture</span>
  </a>
  <a class="wood-card" href="elements.html">
    <img src="wood_elements_sheet_1_elements.webp" alt="Elements">
    <span class="wood-card-title">Elements</span>
  </a>
  <a class="wood-card" href="templates.html">
    <img src="wood_templates_pipeline.webp" alt="Templates">
    <span class="wood-card-title">Templates</span>
  </a>
  <a class="wood-card" href="examples.html">
    <img src="wood_boolean_cutting.webp" alt="Examples">
    <span class="wood-card-title">Examples</span>
  </a>
</div>
