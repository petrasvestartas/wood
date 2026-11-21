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
| ![](joint_library/beam_corner_lap.png) | **Corner lap joint**<br>Eckblatt, p. 262<br>`corner_lap` |
| ![](joint_library/beam_dovetailed_corner_lap.png) | **Dovetailed corner lap joint**<br>Eckblatt mit schräger Blattfläche, p. 266<br>`dovetailed_corner_lap` |
| ![](joint_library/beam_beveled_corner_lap.png) | **Beveled corner lap joint**<br>Eckblatt mit zweifach schräger Blattfläche, p. 268<br>`beveled_corner_lap` |
| ![](joint_library/beam_cogged_corner_lap.png) | **Cogged corner lap joint**<br>Hakeneckblatt, p. 272<br>`cogged_corner_lap` |
| ![](joint_library/beam_sloped_cogged_corner_lap.png) | **Sloped cogged corner lap joint**<br>Hakeneckblatt mit schrägem Haken, p. 274<br>`sloped_cogged_corner_lap` |
| ![](joint_library/beam_double_cogged_corner_lap.png) | **Double cogged corner lap joint**<br>Hakeneckblatt mit mittigem Haken, p. 276<br>`double_cogged_corner_lap` |
| ![](joint_library/beam_cogged_dovetailed_corner_lap.png) | **Cogged dovetailed corner lap joint**<br>Hakeneckblatt mit schwalbenschwanzförmigem Haken, p. 278<br>`cogged_dovetailed_corner_lap` |
| ![](joint_library/beam_blind_mortise_tenon.png) | **Blind mortise and tenon**<br>verdeckter Zapfen, p. 496<br>`blind_mortise_tenon` |
| ![](joint_library/beam_half_lap.png) | **Half lap joint**<br>Querblatt, p. 420<br>`half_lap` |
| ![](joint_library/beam_stopped_half_lap.png) | **Stopped half lap joint**<br>halbverdecktes Querblatt, p. 422<br>`stopped_half_lap` |
| ![](joint_library/beam_beveled_half_lap.png) | **Beveled half lap joint**<br>Querblatt mit schräger Blattfläche, p. 424<br>`beveled_half_lap` |
| ![](joint_library/beam_stopped_beveled_half_lap.png) | **Stopped beveled half lap joint**<br>halbverdecktes Querblatt mit schräger Blattfläche, p. 426<br>`stopped_beveled_half_lap` |
| ![](joint_library/beam_cogged_lap.png) | **Cogged lap joint**<br>Hakenquerblatt, p. 428<br>`cogged_lap` |
| ![](joint_library/beam_cogged_lap_stub_tenon.png) | **Cogged lap joint with stub tenon**<br>Hakenquerblatt mit Brüstungszapfen, p. 430<br>`cogged_lap_stub_tenon` |
| ![](joint_library/beam_sloped_cogged_lap.png) | **Sloped cogged lap joint**<br>Hakenquerblatt mit schrägem Haken, p. 432<br>`sloped_cogged_lap` |
| ![](joint_library/beam_double_cogged_lap.png) | **Double cogged lap joint**<br>Hakenquerblatt mit mittigem Haken, p. 434<br>`double_cogged_lap` |
| ![](joint_library/beam_stopped_mortise_tenon.png) | **Stopped mortise and tenon joint**<br>Querzapfen, p. 436<br>`stopped_mortise_tenon` |
| ![](joint_library/beam_stopped_dovetailed_mortise_tenon.png) | **Stopped dovetailed mortise and tenon joint**<br>Schwalbenschwanzquerzapfen, p. 438<br>`stopped_dovetailed_mortise_tenon` |
| ![](joint_library/beam_dovetail_half_lap.png) | **Dovetail half lap joint**<br>Schwalbenschwanzquerblatt, p. 440<br>`dovetail_half_lap` |
| ![](joint_library/beam_stopped_dovetail_half_lap.png) | **Stopped dovetail half lap joint**<br>halbverdecktes Schwalbenschwanzquerblatt, p. 442<br>`stopped_dovetail_half_lap` |
| ![](joint_library/beam_one_sided_dovetail_half_lap.png) | **One-sided dovetail half lap joint**<br>einseitiges Schwalbenschwanzquerblatt, p. 444<br>`one_sided_dovetail_half_lap` |
| ![](joint_library/beam_stopped_one_sided_dovetail_half_lap.png) | **Stopped one-sided dovetail half lap joint**<br>halbverdecktes einseitiges Schwalbenschwanzquerblatt, p. 446<br>`stopped_one_sided_dovetail_half_lap` |
| ![](joint_library/beam_housed_dovetail_half_lap.png) | **Housed dovetail half lap joint**<br>eingelassenes Schwalbenschwanzquerblatt, p. 448<br>`housed_dovetail_half_lap` |
| ![](joint_library/beam_stopped_housed_dovetail_half_lap.png) | **Stopped housed dovetail half lap joint**<br>halbverdecktes eingelassenes Schwalbenschwanzquerblatt, p. 450<br>`stopped_housed_dovetail_half_lap` |
| ![](joint_library/beam_stepped_lap.png) | **Stepped lap joint**<br>halbverdecktes Stufenquerblatt, p. 452<br>`stepped_lap` |
| ![](joint_library/beam_sloped_stopped_half_lap.png) | **Sloped stopped half lap**<br>verdecktes schräg angeschnittenes Querblatt, p. 456<br>`sloped_stopped_half_lap` |
| ![](joint_library/beam_mortise_tenon_sloped_shoulder.png) | **Mortise and tenon with sloped shoulder**<br>verdeckter Zapfen mit einseitig schräger Brüstung, p. 458<br>`mortise_tenon_sloped_shoulder` |
| ![](joint_library/beam_blind_housed_mortise_tenon_sloped_shoulder.png) | **Blind housed mortise and tenon with sloped shoulder**<br>verdeckter Zapfen mit schräger und gestufter Brüstung, p. 460<br>`blind_housed_mortise_tenon_sloped_shoulder` |
| ![](joint_library/beam_undersquinted_stopped_half_lap.png) | **Undersquinted stopped half lap**<br>halbverdecktes gerades Blatt mit schräger Stirn, p. 462<br>`undersquinted_stopped_half_lap` |
| ![](joint_library/beam_housed_stop_mitered_mortise_tenon.png) | **Housed stop-mitered mortise and tenon**<br>keilförmig eingelassener Zapfen, p. 466<br>`housed_stop_mitered_mortise_tenon` |
| ![](joint_library/beam_mortise_tenon_fan_shaped_housing.png) | **Mortise and tenon with fan-shaped housing**<br>schwalbenschwanzförmiger eingelassener Zapfen, p. 468<br>`mortise_tenon_fan_shaped_housing` |
| ![](joint_library/beam_housed_mortise_tenon_shifted_shoulders.png) | **Housed mortise and tenon with shifted shoulders**<br>Zapfen mit einseitig vorgeschobener Brüstung, p. 454<br>`housed_mortise_tenon_shifted_shoulders` |
| ![](joint_library/beam_blind_pegged.png) | **Blind pegged joint**<br>Verdollung, p. 470<br>`blind_pegged` |
| ![](joint_library/beam_notch.png) | **Notch joint**<br>Ankämmung mit einseitigem Sitz, p. 472<br>`notch` |
| ![](joint_library/beam_double_notch.png) | **Double notch joint**<br>Ankämmung, p. 474<br>`double_notch` |
| ![](joint_library/beam_cog.png) | **Cog joint**<br>Ankämmung mit Stufe, p. 476<br>`cog` |
| ![](joint_library/beam_stepped_cog.png) | **Stepped cog joint**<br>Ankämmung mit doppelter Stufe, p. 478<br>`stepped_cog` |
| ![](joint_library/beam_double_cog.png) | **Double cog joint**<br>Ankämmung mit mittigem Haken, p. 480<br>`double_cog` |
| ![](joint_library/beam_x_shaped_cog.png) | **X-shaped cog joint**<br>Ankämmung mit Kreuz, p. 482<br>`x_shaped_cog` |
| ![](joint_library/beam_v_shaped_cog.png) | **V-shaped cog joint**<br>Ankämmung mit halbem Kreuz, p. 484<br>`v_shaped_cog` |
| ![](joint_library/beam_dovetail_cog.png) | **Dovetail cog joint**<br>Ankämmung mit Schwalbenschwanz, p. 486<br>`dovetail_cog` |
| ![](joint_library/beam_half_dovetail_cog.png) | **Half-dovetail cog joint**<br>Ankämmung mit einseitigem Schwalbenschwanz, p. 488<br>`half_dovetail_cog` |
| ![](joint_library/beam_sloped_cog.png) | **Sloped cog joint**<br>Ankämmung mit schräger Stufe, p. 490<br>`sloped_cog` |
| ![](joint_library/beam_through_mortise_tenon.png) | **Through mortise and tenon**<br>durchgehender Zapfen, p. 492<br>`through_mortise_tenon` |
| ![](joint_library/beam_through_mortise_tenon_housing.png) | **Through mortise and tenon with housing**<br>eingelassener durchgehender Zapfen, p. 494<br>`through_mortise_tenon_housing` |
| ![](joint_library/beam_tenon_chase_mortise.png) | **Tenon and chase mortise**<br>Schleifzapfen, p. 498<br>`tenon_chase_mortise` |
| ![](joint_library/beam_mortise_tenon_beveled_shoulders.png) | **Mortise and tenon with beveled shoulders**<br>verdeckter Zapfen mit schräger Brust, p. 500<br>`mortise_tenon_beveled_shoulders` |
| ![](joint_library/beam_mortise_tenon_mitered_shoulders.png) | **Mortise and tenon with mitered shoulders**<br>verdeckter Zapfen mit Führungsstoß, p. 502<br>`mortise_tenon_mitered_shoulders` |
| ![](joint_library/beam_pegged_through_mortise_tenon.png) | **Pegged through mortise and tenon**<br>durchgehender Zapfen mit Dübelsicherung, p. 504<br>`pegged_through_mortise_tenon` |
| ![](joint_library/beam_pegged_blind_mortise_tenon.png) | **Pegged blind mortise and tenon**<br>verdeckter Zapfen mit Dübelsicherung, p. 506<br>`pegged_blind_mortise_tenon` |
| ![](joint_library/beam_triple_shouldered_through_mortise_tenon.png) | **Triple-shouldered through mortise and tenon**<br>durchgehender abgesetzter Zapfen, p. 508<br>`triple_shouldered_through_mortise_tenon` |
| ![](joint_library/beam_fully_shouldered_through_mortise_tenon.png) | **Fully shouldered through mortise and tenon joint**<br>durchgehender beidseitig abgesetzter Zapfen, p. 510<br>`fully_shouldered_through_mortise_tenon` |
| ![](joint_library/beam_triple_through_mortise_tenon.png) | **Triple through mortise and tenon**<br>durchgehender dreifacher Zapfen, p. 512<br>`triple_through_mortise_tenon` |
| ![](joint_library/beam_quadruple_through_mortise_tenon.png) | **Quadruple through mortise and tenon**<br>durchgehender vierfacher Zapfen, p. 514<br>`quadruple_through_mortise_tenon` |
| ![](joint_library/beam_haunched_through_mortise_tenon.png) | **Haunched through mortise and tenon**<br>durchgehender Stufenzapfen, p. 516<br>`haunched_through_mortise_tenon` |
| ![](joint_library/beam_blind_haunched_mortise_tenon.png) | **Blind haunched mortise and tenon**<br>verdeckter Stufenzapfen, p. 518<br>`blind_haunched_mortise_tenon` |
| ![](joint_library/beam_through_mortise_tenon_sloped_haunch.png) | **Through mortise and tenon with sloped haunch**<br>durchgehender Zapfen mit schräger Stufe, p. 520<br>`through_mortise_tenon_sloped_haunch` |
| ![](joint_library/beam_blind_housed_haunched_through_mortise_tenon.png) | **Blind housed haunched through mortise and tenon**<br>durchgehender Stufenzapfen mit gestufter Brüstung, p. 522<br>`blind_housed_haunched_through_mortise_tenon` |
| ![](joint_library/beam_haunched_collared_through_mortise_tenon.png) | **Haunched collared through mortise and tenon**<br>durchgehender Stufenzapfen mit Führungszapfen, p. 524<br>`haunched_collared_through_mortise_tenon` |
| ![](joint_library/beam_half_dovetail_tenon_folding_wedges.png) | **Half-dovetail tenon with folding wedges**<br>verkeilter verdeckter einseitiger Schwalbenschwanzzapfen, p. 526<br>`half_dovetail_tenon_folding_wedges` |
| ![](joint_library/beam_foxtail_wedged_mortise_tenon.png) | **Foxtail-wedged mortise and tenon**<br>verkeilter verdeckter Zapfen, p. 528<br>`foxtail_wedged_mortise_tenon` |
| ![](joint_library/beam_wedged_through_mortise_tenon.png) | **Wedged through mortise and tenon**<br>verkeilter durchgehender Zapfen, p. 530<br>`wedged_through_mortise_tenon` |
| ![](joint_library/beam_vertically_wedged_tusk_tenon.png) | **Vertically wedged tusk tenon**<br>durchgesteckter Zapfen mit Keilschloss, p. 532<br>`vertically_wedged_tusk_tenon` |
| ![](joint_library/beam_horizontally_wedged_tusk_tenon.png) | **Horizontally wedged tusk tenon**<br>durchgesteckter Zapfen mit doppeltem Keilschloss, p. 534<br>`horizontally_wedged_tusk_tenon` |
| ![](joint_library/beam_horizontally_wedged_double_tusk_tenon.png) | **Horizontally wedged double tusk tenon**<br>durchgesteckter doppelter Zapfen mit Keilschloss, p. 536<br>`horizontally_wedged_double_tusk_tenon` |
| ![](joint_library/beam_double_through_mortise_tenon.png) | **Double through mortise and tenon**<br>durchgehender doppelter Zapfen, p. 542<br>`double_through_mortise_tenon` |
| ![](joint_library/beam_housed_double_through_mortise_tenon.png) | **Housed double through mortise and tenon**<br>eingelassener durchgehender doppelter Zapfen, p. 544<br>`housed_double_through_mortise_tenon` |
| ![](joint_library/beam_double_mortise_tenon_mitered_shoulders.png) | **Double mortise and tenon with mitered shoulders**<br>durchgehender doppelter Zapfen mit Führungsstoß, p. 546<br>`double_mortise_tenon_mitered_shoulders` |
| ![](joint_library/beam_t_bridle.png) | **T-bridle joint**<br>Einhälsung, p. 550<br>`t_bridle` |
| ![](joint_library/beam_t_bridle_blind_mortise_tenon.png) | **T-bridle joint with blind mortise and tenon**<br>Einhälsung mit verdecktem Zapfen, p. 552<br>`t_bridle_blind_mortise_tenon` |
| ![](joint_library/beam_wedged_half_dovetailed_t_bridle.png) | **Wedged half-dovetailed T-bridle joint**<br>verkeilte einseitig trapezförmige Einhälsung, p. 548<br>`wedged_half_dovetailed_t_bridle` |
| ![](joint_library/beam_dovetail_t_bridle.png) | **Dovetail T-bridle joint**<br>schwalbenschwanzförmige Einhälsung, p. 564<br>`dovetail_t_bridle` |
| ![](joint_library/beam_wedged_dovetail_half_lap.png) | **Wedged dovetail half lap joint**<br>verkeiltes Schwalbenschwanzblatt, p. 570<br>`wedged_dovetail_half_lap` |
| ![](joint_library/beam_double_lap_stub_tenon.png) | **Double lap joint with stub tenon**<br>Gabelaufblattung mit verdecktem Zapfen, p. 554<br>`double_lap_stub_tenon` |
| ![](joint_library/beam_notched_lap_tenon.png) | **Notched lap joint with tenon**<br>einseitige Einhälsung mit durchgehendem Zapfen, p. 556<br>`notched_lap_tenon` |
| ![](joint_library/beam_notched_lap_stub_tenon.png) | **Notched lap joint with stub tenon**<br>einseitige Einhälsung mit verdecktem Zapfen, p. 558<br>`notched_lap_stub_tenon` |
| ![](joint_library/beam_lap_tenon.png) | **Lap joint with tenon**<br>einseitige Gabelaufblattung mit durchgehendem Zapfen, p. 560<br>`lap_tenon` |
| ![](joint_library/beam_lap_stub_tenon.png) | **Lap joint with stub tenon**<br>einseitige Gabelaufblattung mit verdecktem Zapfen, p. 562<br>`lap_stub_tenon` |
| ![](joint_library/beam_housed_dovetail.png) | **Housed dovetail joint**<br>eingeschobener abgesetzter Schwalbenschwanzzapfen, p. 572<br>`housed_dovetail` |
| ![](joint_library/beam_x_shaped_stub_tenon.png) | **X-shaped stub tenon joint**<br>Kreuzzapfen, p. 574<br>`x_shaped_stub_tenon` |
| ![](joint_library/beam_fully_shouldered_blind_mortise_tenon.png) | **Fully shouldered blind mortise and tenon**<br>verdeckter beidseitig abgesetzter Zapfen, p. 576<br>`fully_shouldered_blind_mortise_tenon` |
<!-- /joint-table -->

## Library
Every record built on pairs of 120 x 120 beams standing in the xz plane, a group and a row per design: on a square corner and on one leaning at 50 degrees where the record exists there, each drawn apart as the book draws it, the member 350 mm out along its own axis and every peg along its own; the ways each goes in are the record's insertions.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp
