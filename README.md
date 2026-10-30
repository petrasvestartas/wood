# Wood

Timber joinery solver and parametric structure library built on [`session_cpp`](https://github.com/petrasvestartas/session_cpp).

Documentation: [Overview](https://petrasvestartas.github.io/wood/) · [Architecture](https://petrasvestartas.github.io/wood/architecture.html) · [Elements](https://petrasvestartas.github.io/wood/elements.html) · [Templates](https://petrasvestartas.github.io/wood/templates.html) · [Examples](https://petrasvestartas.github.io/wood/examples.html) · [Setup](SETUP.md)

![](docs/images/diagrams/wood_architecture.webp)

## Quick Start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target run_tests --parallel
```

Every example writes a `.pb` scene to `data/output/pb/live.pb` for `session_viewer`:

```bash
cmake --build build --target element_joint_plate_side_side_outplane_fingers --parallel
./build/element_joint_plate_side_side_outplane_fingers
```

## Architecture

| Layer | Headers | Purpose |
| --- | --- | --- |
| Session | `src/joinery_solver/wood_session.h` | `WoodSession`: element storage, tree groups, contact queries, interaction graph, `.pb` export |
| Elements | `src/joinery_solver/wood_elements/*.h` | `Plate`, `PlateCurved`, `Beam`, `BeamCurved`, `BeamVariable`, `Column`, `Block`, `Pin`, `Support`, `CutPlane`, `ConnectorPart`, `Joint`, `JointPlate`, `JointBeam` |
| Interactions | `src/joinery_solver/wood_interaction/**/*.h` | Contacts (`InteractionContactFace`, `InteractionContactCross`, `InteractionContactAxis`) and cutting features (`InteractionFeaturePlate`, `InteractionFeatureBeam`, `InteractionFeaturePlateBeam`, `InteractionFeaturePlane`, `InteractionFeatureSolid`) |
| Templates | `src/templates/*` | `Shells`, `Folding`, `CylindricalFolding`, `DevelopableFolding`, `Chevron`, `Cross`, `Module`, `Reciprocal`, `Grid`, `Floor`, `Truss`, `Vault` |

![](docs/images/diagrams/wood_elements_taxonomy.webp)

![](docs/images/diagrams/wood_joint_elements_taxonomy.webp)

![](docs/images/diagrams/wood_templates_pipeline.webp)

![](docs/images/diagrams/wood_boolean_cutting.webp)

## Types

| Class | Stock Input | Primary Cutters |
| --- | --- | --- |
| `Plate` | `bottom`, `top` planar polylines | Outline slots and tenons (`JointPlate`), `CutPlane`, `Joint::profile`, `Joint::solid`, `Joint::drill` |
| `PlateCurved` | `outer_a`, `outer_b`, `inner_a`, `inner_b` (`NurbsCurve`) | Ruled side miters, planar caps, `Development` flat cutting outline |
| `Beam` | `axis` (`Polyline`), `radii`, `up_vectors`, `sides` | Rectangular or round timber; `CutPlane`, `Joint::profile`, `Joint::solid`, `JointBeam`, `Joint::drill` |
| `BeamCurved` | `centreline` (`NurbsCurve`), `width`, `height`, `lamellae` | Curved glulam with RMF frames and 6-patch NURBS BRep; `CutPlane`, `Joint::solid`, `JointBeam` |
| `BeamVariable` | `axis`, `profiles` at `stations` | Parabolic soffit ribs, `CutPlane`, `Joint::solid`, `JointBeam` |
| `Column` | `Column::square(axis, frame, width)` | `head_blocks` (`SolidOperation::add`), `column_cutters` (`subtract`), `Joint::support` |
| `Support` | `Support(plane)` | Steel base plate, head plate, 3 anchor pins |
| `Block` / `Pin` / `ConnectorPart` | `Mesh` / `(head, direction, radius, length)` / connector solid | Child solids and dowels nested under `Column` or `JointBeam` connectors |
| `CutPlane` | `Plane` | Half-space clip (`InteractionFeaturePlane`) |
| `Joint` | `Joint::solid`, `Joint::profile`, `Joint::drill`, `Joint::support` | Closed solid / extruded profile cutters, analytical NURBS cylinder bores (`BRep`), and 16-gon mesh drills |
| `JointPlate` | 38 presets across 7 families in unit box `[-0.5, 0.5]^3` | `orient(contact, elements)` → `interaction(target)` |
| `JointBeam` | `from_contact`, `butt`, `wedge`, `rectangle_plate`, `butterfly_key`, `centred_pins`, `headed_pins` | Axis contact joinery, automatic butt planes, and face-contact connectors |

## Datasets

`wood_datasets.h` (`src/joinery_solver/wood_datasets.h`) runs the reference YAML/OBJ datasets in `data/`:

| Group | Files | Content |
| --- | --- | --- |
| Plate & Shell Datasets | `data/*.yml`, `data/*.obj` | Vidy Chapel, Annen Plus, HexBox, Rossiniere, in-plane / out-of-plane / top-to-side / top-to-top benchmarks |
| Beam Datasets | `data/phanomema_node.*` | Multi-beam axis contact node |
| Floor Serialization | `data/floor_model.pb` | Full vaulted timber floor round-trip benchmark |
| Template Inputs | `data/templates/*`, `data/annen_surfaces.json` | Bow's 1873 truss catalogue (`truss_bow_1873.json`), Annen NURBS shells, and VDA mesh inputs |

## Targets

| Group | CMake Targets |
| --- | --- |
| Tests & Docs | `run_tests`, `docs` (`cmake --build build --target docs`) |
| Workflow Examples | `1_session_contacts`, `2_contacts`, `3_features`, `4_traversal`, `5_settings`, `6_custom_joint`, `7_assignment`, `8_beams`, `9_cross_joints`, `10_profile_cuts`, `11_drill_solids`, `12_solid_features` |
| Elements | `element_plate`, `element_plate_holes`, `element_plate_session`, `element_plate_assign_joints`, `element_plate_curved`, `element_beam`, `element_beam_curved`, `element_beam_variable`, `element_beam_variable_cut`, `element_column`, `element_column_session`, `element_block`, `element_support`, `element_profile`, `element_pin`, `element_joint_drill`, `element_joint_cutter` |
| Plate Joints | `element_joint_plate_top_side_*` (6), `element_joint_plate_vidy_node`, `element_joint_plate_side_side_inplane_*` (7), `element_joint_plate_side_side_outplane_*` (10), `element_joint_plate_side_side_rotated_*` (5), `element_joint_plate_cross_*` (2), `element_joint_plate_top_top_*` (7), `element_joint_plate_border`, `element_joint_plate_parameters` |
| Beam Joints | `element_joint_beam_cross_*` (6), `element_joint_beam_top_side_milled_wedge`, `element_joint_beam_side_side_inplane_splayed_scarf`, `element_joint_beam_side_side_inplane_cross_splayed_scarf`, `element_joint_beam_jupiter_scarf`, `element_joint_beam_side_side_rotated_miter`, `element_joint_beam_butt`, `element_joint_beam_butt_plates`, `element_joint_beam_wedge`, `element_joint_beam_rectangle_plate`, `element_joint_beam_butterfly_key`, `element_joint_beam_centred_pins`, `element_joint_beam_headed_pins` |
| Templates | `templates_shells_1_translation_shell`, `templates_folding_{1..39}_*` (36), `templates_cylindrical_folding_{1..17}_*`, `templates_developable_folding_{1..12}_*`, `templates_chevron_1_chevron`, `templates_cross_1_vda_mesh`, `templates_module_{1..6}_*`, `templates_reciprocal_{1..2}_*`, `templates_grid_{1..5}_*`, `templates_floor_{1,2,3,7,8,9}_*`, `templates_truss_{1..3}_*`, `templates_vault_{1..9}_*` |
