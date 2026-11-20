# Beam Compendium Joints {#elements_joint_beam_compendium}

[TOC]

Beam joints rebuilt from Bauer and Pauli's *Joinery Compendium* as its drawings make them, one record each in the YAML library `data/joints/*.yaml` (the format is written at the top of every file) built by `JointBeamElement::from_library`: each keeps its cutting procedure step by step, A1, A2 on its first member and B1, B2 on its second, and the directions each member goes in along.

## Constructors

```cpp
static std::shared_ptr<JointBeamElement> JointBeamElement::from_library(const std::string& design, const EdgeStraightElement& a, const EdgeStraightElement& b, const std::map<std::string, double>& parameters = {}, double overshoot = 2.0);
JointClassification classify_joint(const EdgeStraightElement& a, const EdgeStraightElement& b, double tolerance = 1.0);
AssemblySequence(const WoodSession& session, const std::string& base, double step = 10.0, double clearance = 10.0);
```

## Parameters

- Every record names its parameters with their defaults, shares of the members' sizes: `depth` a share of the carrier's depth, `thickness` of the member's width, `inset` of its depth; `from_library` takes others by name.
- `overshoot`: how far every removal runs past the faces it opens, so the one boolean that takes them away leaves no skin.

Every step keeps its saw or chisel planes and the wood it takes away (`JointElement::steps`, `target_steps(i)`); a member loses all of them in one boolean (`removal(i)`), and the steps and the insertion directions (`target_insertions`) are written with the scene. `add_interaction` also adds the directions to the member's own `insertion_vectors`.

## Assembly

`classify_joint` reads what two beams need from their axes: an end where the closest point lies within the other's half section of an axis end, then `end_end_inplane` (splicing), `end_end_outplane` (corner), `side_end` (t when square, else angled), `cross` (cross lap) or `side_side`. `AssemblySequence` takes the members of a session apart: again and again one slides out along a direction one of its joints gives, swept its own diagonal against every member still in without entering one, the base last; reversed, that is the assembly. A member no direction frees is reported locked, and a joint whose every direction another joint's member blocks is reported as a conflict.

## Families

The records of the library so far, each on a square and an inclined corner where it exists there, drawn apart as the book draws it. Pictures by `python3 tools/render_joint_library.py`.

<!-- joint-table -->
| joint | compendium |
|---|---|
| ![](joint_library/beam_notched_oblique_thrust.png) | **Notched oblique thrust joint**<br>Stirnversatz, p. 584<br>`notched_oblique_thrust` |
| ![](joint_library/beam_notched_oblique_thrust_right_angled_end.png) | **Notched oblique thrust joint with right-angled end**<br>Stirnversatz mit rechtwinkliger Stirn, p. 586<br>`notched_oblique_thrust_right_angled_end` |
| ![](joint_library/beam_notched_oblique_thrust_visible_end.png) | **Notched oblique thrust joint with visible end**<br>Brustversatz, p. 588<br>`notched_oblique_thrust_visible_end` |
| ![](joint_library/beam_oblique_thrust_notched_heel.png) | **Oblique thrust joint with notched heel**<br>Fersenversatz, p. 590<br>`oblique_thrust_notched_heel` |
| ![](joint_library/beam_double_notched_oblique_thrust.png) | **Double notched oblique thrust joint**<br>Stirn- und Fersenversatz, p. 592<br>`double_notched_oblique_thrust` |
| ![](joint_library/beam_oblique_thrust_sloped_tenon_notched_heel.png) | **Oblique thrust joint with sloped tenon and notched heel**<br>Fersenversatz mit schrägem Zapfen, p. 594<br>`oblique_thrust_sloped_tenon_notched_heel` |
| ![](joint_library/beam_oblique_thrust_mortise_tenon.png) | **Oblique thrust joint with mortise and tenon**<br>Schräger Stoß mit Zapfen, p. 596<br>`oblique_thrust_mortise_tenon` |
| ![](joint_library/beam_oblique_thrust_sloped_mortise_tenon.png) | **Oblique thrust joint with sloped mortise and tenon**<br>schräger Stoß mit schrägem Zapfen, p. 598<br>`oblique_thrust_sloped_mortise_tenon` |
| ![](joint_library/beam_notched_oblique_thrust_mortise_tenon.png) | **Notched oblique thrust joint with mortise and tenon**<br>Stirnversatz mit Zapfen, p. 600<br>`notched_oblique_thrust_mortise_tenon` |
| ![](joint_library/beam_notched_oblique_thrust_pegged_mortise_tenon.png) | **Notched oblique thrust joint with pegged mortise and tenon**<br>Stirnversatz mit Zapfen und Dübelsicherung, p. 602<br>`notched_oblique_thrust_pegged_mortise_tenon` |
| ![](joint_library/beam_slotted_notched_oblique_thrust.png) | **Slotted notched oblique thrust joint**<br>geschlitzter Stirnversatz, p. 604<br>`slotted_notched_oblique_thrust` |
| ![](joint_library/beam_slotted_notched_oblique_thrust_visible_end.png) | **Slotted notched oblique thrust joint with visible end**<br>geschlitzter Brustversatz, p. 606<br>`slotted_notched_oblique_thrust_visible_end` |
| ![](joint_library/beam_pegged_angled_half_lap.png) | **Pegged angled half lap joint**<br>schräges Blatt mit Dübelsicherung, p. 608<br>`pegged_angled_half_lap` |
| ![](joint_library/beam_pegged_angled_stopped_half_lap.png) | **Pegged angled stopped half lap joint**<br>halbverdecktes schräges Blatt mit Dübelsicherung, p. 610<br>`pegged_angled_stopped_half_lap` |
| ![](joint_library/beam_pegged_angled_lapped_mortise_tenon.png) | **Pegged angled lapped mortise and tenon joint**<br>schräges Blatt mit Zapfen und Dübelsicherung, p. 612<br>`pegged_angled_lapped_mortise_tenon` |
| ![](joint_library/beam_pegged_shouldered_angled_half_lap.png) | **Pegged shouldered angled half lap joint**<br>beidseitig abgesetztes schräges Blatt mit Dübelsicherung, p. 614<br>`pegged_shouldered_angled_half_lap` |
| ![](joint_library/beam_pegged_angled_half_dovetail_lap_sloped_shoulder.png) | **Pegged angled half-dovetail lap joint with sloped shoulder**<br>schräges einseitiges Schwalbenschwanzblatt mit verdecktem Versatz und Dübelsicherung, p. 616<br>`pegged_angled_half_dovetail_lap_sloped_shoulder` |
| ![](joint_library/beam_pegged_angled_half_dovetail_half_lap.png) | **Pegged angled half-dovetail half lap joint**<br>schräges einseitiges Schwalbenschwanzblatt mit Dübelsicherung, p. 618<br>`pegged_angled_half_dovetail_half_lap` |
| ![](joint_library/beam_pegged_stopped_angled_half_dovetail_half_lap.png) | **Pegged stopped angled half-dovetail half lap joint**<br>halbverdecktes schräges einseitiges Schwalbenschwanzblatt und Dübelsicherung, p. 620<br>`pegged_stopped_angled_half_dovetail_half_lap` |
| ![](joint_library/beam_pegged_angled_stopped_dovetail_half_lap.png) | **Pegged angled stopped dovetail half lap joint**<br>halbverdecktes schräges Schwalbenschwanzblatt mit Dübelsicherung, p. 622<br>`pegged_angled_stopped_dovetail_half_lap` |
| ![](joint_library/beam_pegged_rafter_foot.png) | **Pegged rafter foot**<br>Stütznagelung, p. 624<br>`pegged_rafter_foot` |
| ![](joint_library/beam_pegged_mitered_rafter_foot.png) | **Pegged mitered rafter foot**<br>Schiftungsstoß, p. 626<br>`pegged_mitered_rafter_foot` |
| ![](joint_library/beam_housed_rafter_foot.png) | **Housed rafter foot**<br>Einklauung, p. 628<br>`housed_rafter_foot` |
| ![](joint_library/beam_v_grooved_rafter_foot.png) | **V-grooved rafter foot**<br>Schiftereinblattung, p. 630<br>`v_grooved_rafter_foot` |
| ![](joint_library/beam_birdsmouth_rafter_foot.png) | **Birdsmouth rafter foot**<br>Aufklauung, p. 632<br>`birdsmouth_rafter_foot` |
| ![](joint_library/beam_rafter_foot_tenon_undersquinted_shoulders.png) | **Rafter foot with tenon and undersquinted shoulders**<br>Zapfung mit Klaue, p. 634<br>`rafter_foot_tenon_undersquinted_shoulders` |
| ![](joint_library/beam_birdsmouth_rafter_joint.png) | **Birdsmouth rafter joint**<br>Aufkervung, p. 636<br>`birdsmouth_rafter_joint` |
| ![](joint_library/beam_notched_rafter_joint.png) | **Notched rafter joint**<br>Aufkervung mit Ausklinkung, p. 638<br>`notched_rafter_joint` |
| ![](joint_library/beam_v_grooved_rafter_joint.png) | **V-grooved rafter joint**<br>Aufkervung mit Rückversatz, p. 640<br>`v_grooved_rafter_joint` |
| ![](joint_library/beam_step_lap_rafter_joint.png) | **Step lap rafter joint**<br>Aufkervung mit gestuftem Rückversatz, p. 642<br>`step_lap_rafter_joint` |
| ![](joint_library/beam_stopped_birdsmouth_rafter_foot.png) | **Stopped birdsmouth rafter foot**<br>Aufklauung mit Anschnitt, p. 644<br>`stopped_birdsmouth_rafter_foot` |
| ![](joint_library/beam_blind_mortise_tenon.png) | **Blind mortise and tenon**<br>verdeckter Zapfen, p. 496<br>`blind_mortise_tenon` |
| ![](joint_library/beam_half_lap.png) | **Half lap joint**<br>Querblatt, p. 420<br>`half_lap` |
| ![](joint_library/beam_stopped_half_lap.png) | **Stopped half lap joint**<br>halbverdecktes Querblatt, p. 422<br>`stopped_half_lap` |
| ![](joint_library/beam_beveled_half_lap.png) | **Beveled half lap joint**<br>Querblatt mit schräger Blattfläche, p. 424<br>`beveled_half_lap` |
| ![](joint_library/beam_stopped_beveled_half_lap.png) | **Stopped beveled half lap joint**<br>halbverdecktes Querblatt mit schräger Blattfläche, p. 426<br>`stopped_beveled_half_lap` |
| ![](joint_library/beam_cogged_lap.png) | **Cogged lap joint**<br>Hakenquerblatt, p. 428<br>`cogged_lap` |
| ![](joint_library/beam_sloped_cogged_lap.png) | **Sloped cogged lap joint**<br>Hakenquerblatt mit schrägem Haken, p. 432<br>`sloped_cogged_lap` |
| ![](joint_library/beam_double_cogged_lap.png) | **Double cogged lap joint**<br>Hakenquerblatt mit mittigem Haken, p. 434<br>`double_cogged_lap` |
<!-- /joint-table -->

## Library
Every record built on pairs of 120 x 120 beams standing in the xz plane, a group and a row per design: on a square corner and on one leaning at 50 degrees where the record exists there, each drawn apart as the book draws it, the member 350 mm out along its own axis and every peg along its own; the ways each goes in are the record's insertions.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp
