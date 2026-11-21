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

The joints of the book not in the library yet are listed on @ref elements_joint_library_todo.

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
| ![](joint_library/beam_rabbet_case_corner.png) | **Rabbet joint**<br>gefälzte Eckverbindung, p. 814<br>`rabbet_case_corner` |
| ![](joint_library/beam_dado_rabbet_case_corner.png) | **Dado and rabbet joint**<br>gespundete Eckverbindung, p. 818<br>`dado_rabbet_case_corner` |
| ![](joint_library/beam_splined_miter_case_corner.png) | **Splined miter joint**<br>Eckverbindung auf Gehrung mit eingesetzter Feder, p. 824<br>`splined_miter_case_corner` |
| ![](joint_library/beam_miter_spline_keys_case_corner.png) | **Miter joint with spline keys**<br>Eckverbindung auf Gehrung mit eingesetzten Federn, p. 826<br>`miter_spline_keys_case_corner` |
| ![](joint_library/beam_finger_case_corner.png) | **Finger joint**<br>Eckverbindung mit Fingerzinken, p. 830<br>`finger_case_corner` |
| ![](joint_library/beam_through_dovetail_case_corner.png) | **Through dovetail joint**<br>Schwalbenschwanzzinken, p. 832<br>`through_dovetail_case_corner` |
| ![](joint_library/beam_half_blind_dovetail_case_corner.png) | **Half-blind dovetail joint**<br>halbverdeckte Schwalbenschwanzzinken, p. 834<br>`half_blind_dovetail_case_corner` |
| ![](joint_library/beam_mitered_rabbet_case_corner.png) | **Mitered rabbet joint**<br>gefälzte Eckverbindung mit Gehrung, p. 816<br>`mitered_rabbet_case_corner` |
| ![](joint_library/beam_lock_case_corner.png) | **Lock joint**<br>gespundete Eckverbindung mit Verdeck, p. 820<br>`lock_case_corner` |
| ![](joint_library/beam_angled_finger_case_corner.png) | **Angled finger joint**<br>Eckverbindung mit schrägen Fingerzinken, p. 840<br>`angled_finger_case_corner` |
| ![](joint_library/beam_lock_miter_case_corner.png) | **Lock miter joint**<br>gespundete Eckverbindung mit Verdeck auf Gehrung, p. 822<br>`lock_miter_case_corner` |
| ![](joint_library/beam_miter_dovetail_keys_case_corner.png) | **Miter joint with dovetail keys**<br>Eckverbindung auf Gehrung mit eingesetzten Schwalbenschwanzfedern, p. 828<br>`miter_dovetail_keys_case_corner` |
| ![](joint_library/beam_blind_dovetail_case_corner.png) | **Blind dovetail joint**<br>Eckverbindung mit verdeckten Schwalbenschwanzzinken, p. 836<br>`blind_dovetail_case_corner` |
| ![](joint_library/beam_cogged_dovetail_case_corner.png) | **Cogged dovetail joint**<br>Eckverbindung mit ganzen und abgesetzten Schwalbenschwanzzinken, p. 838<br>`cogged_dovetail_case_corner` |
| ![](joint_library/beam_radial_dovetail_case_corner.png) | **Radial dovetail joint**<br>Eckverbindung mit Teufelszinken, p. 842<br>`radial_dovetail_case_corner` |
| ![](joint_library/beam_shiplap_edge.png) | **Shiplap joint**<br>Überfälzung, p. 768<br>`shiplap_edge` |
| ![](joint_library/beam_v_grooved_edge.png) | **V-grooved joint**<br>v-förmige Nut, p. 770<br>`v_grooved_edge` |
| ![](joint_library/beam_tongue_groove_edge.png) | **Tongue and groove joint**<br>Nut und Feder, p. 772<br>`tongue_groove_edge` |
| ![](joint_library/beam_floating_tenon_edge.png) | **Floating tenon joint**<br>Stoß mit eingesetztem Zapfen, p. 776<br>`floating_tenon_edge` |
| ![](joint_library/beam_spline_groove_edge.png) | **Spline and groove joint**<br>Nut mit eingesetzter Feder, p. 780<br>`spline_groove_edge` |
| ![](joint_library/beam_butterfly_key_edge.png) | **Butterfly key joint**<br>Schwalbenschwanzeinlage, p. 786<br>`butterfly_key_edge` |
| ![](joint_library/beam_tongued_rabbeted_edge.png) | **Tongued and rabbeted joint**<br>Nut und Feder mit versetzter Brüstung, p. 774<br>`tongued_rabbeted_edge` |
| ![](joint_library/beam_blind_dovetail_edge.png) | **Blind dovetail joint**<br>abgesetzte Fuge mit Trapezprofil, p. 784<br>`blind_dovetail_edge` |
| ![](joint_library/beam_sliding_dovetail_batten.png) | **Sliding dovetail batten**<br>Gratleiste, p. 788<br>`sliding_dovetail_batten` |
| ![](joint_library/beam_floating_tenon_spline_edge.png) | **Floating tenon and spline joint**<br>Nut mit eingesetzter Feder und Zapfen, p. 782<br>`floating_tenon_spline_edge` |
| ![](joint_library/beam_sliding_dovetail_batten_keys.png) | **Sliding dovetail batten with keys**<br>Gratleiste mit Dübelschloss, p. 790<br>`sliding_dovetail_batten_keys` |
| ![](joint_library/beam_finger_end.png) | **End-to-end finger joint**<br>Fingerzinkenstoß, p. 796<br>`finger_end` |
| ![](joint_library/beam_tongue_groove_breadboard.png) | **Tongue and groove breadboard end**<br>Hirnleiste mit Nut und Feder, p. 806<br>`tongue_groove_breadboard` |
| ![](joint_library/beam_dado_tongue_breadboard.png) | **Dado and tongue breadboard end**<br>Hirnleiste mit Falz und Feder, p. 808<br>`dado_tongue_breadboard` |
| ![](joint_library/beam_splined_breadboard.png) | **Splined breadboard end**<br>Hirnleiste mit Nut und Fremdfeder, p. 804<br>`splined_breadboard` |
| ![](joint_library/beam_haunched_mortise_tenon_breadboard.png) | **Haunched mortise and tenon breadboard end**<br>Hirnleiste mit Nut, Feder und Zapfenlöchern, p. 810<br>`haunched_mortise_tenon_breadboard` |
| ![](joint_library/beam_dovetail_end.png) | **End-to-end dovetail joint**<br>Zinkenstoß, p. 798<br>`dovetail_end` |
| ![](joint_library/beam_dado_spline_end.png) | **Dado with spline**<br>eingelassene Hirnleiste, p. 802<br>`dado_spline_end` |
| ![](joint_library/beam_v_dado_spline_end.png) | **V-shaped dado with spline**<br>eingelassene keilförmige Hirnleiste, p. 800<br>`v_dado_spline_end` |
| ![](joint_library/beam_halved_splayed_end.png) | **Halved splayed joint**<br>schräg angeschnittene wechselseitige Überblattung, p. 794<br>`halved_splayed_end` |
| ![](joint_library/beam_wedged_haunched_tenon_breadboard.png) | **Wedged and haunched tenon breadboard end**<br>verkeilte Hirnleiste mit Nut und Zapfenloch, p. 812<br>`wedged_haunched_tenon_breadboard` |
| ![](joint_library/beam_housing_case.png) | **Housing joint**<br>Nutverbindung, p. 844<br>`housing_case` |
| ![](joint_library/beam_stopped_housing_case.png) | **Stopped housing joint**<br>abgesetzte Nutverbindung, p. 846<br>`stopped_housing_case` |
| ![](joint_library/beam_tongue_dado_case.png) | **Tongue and dado joint**<br>Nutverbindung mit mittiger Feder, p. 850<br>`tongue_dado_case` |
| ![](joint_library/beam_rabbet_dado_case.png) | **Rabbet and dado joint**<br>Nutverbindung mit außermittiger Feder, p. 852<br>`rabbet_dado_case` |
| ![](joint_library/beam_sliding_dovetail_case.png) | **Sliding dovetail joint**<br>Gratnutverbindung, p. 854<br>`sliding_dovetail_case` |
| ![](joint_library/beam_single_shouldered_sliding_dovetail_case.png) | **Single-shouldered sliding dovetail joint**<br>einseitige Gratnutverbindung, p. 856<br>`single_shouldered_sliding_dovetail_case` |
| ![](joint_library/beam_beveled_housing_case.png) | **Beveled housing joint**<br>schräg eingelassene Winkelverbindung, p. 848<br>`beveled_housing_case` |
| ![](joint_library/beam_corner_lap.png) | **Corner lap joint**<br>Eckblatt, p. 262<br>`corner_lap` |
| ![](joint_library/beam_dovetailed_corner_lap.png) | **Dovetailed corner lap joint**<br>Eckblatt mit schräger Blattfläche, p. 266<br>`dovetailed_corner_lap` |
| ![](joint_library/beam_beveled_corner_lap.png) | **Beveled corner lap joint**<br>Eckblatt mit zweifach schräger Blattfläche, p. 268<br>`beveled_corner_lap` |
| ![](joint_library/beam_cogged_corner_lap.png) | **Cogged corner lap joint**<br>Hakeneckblatt, p. 272<br>`cogged_corner_lap` |
| ![](joint_library/beam_sloped_cogged_corner_lap.png) | **Sloped cogged corner lap joint**<br>Hakeneckblatt mit schrägem Haken, p. 274<br>`sloped_cogged_corner_lap` |
| ![](joint_library/beam_double_cogged_corner_lap.png) | **Double cogged corner lap joint**<br>Hakeneckblatt mit mittigem Haken, p. 276<br>`double_cogged_corner_lap` |
| ![](joint_library/beam_cogged_dovetailed_corner_lap.png) | **Cogged dovetailed corner lap joint**<br>Hakeneckblatt mit schwalbenschwanzförmigem Haken, p. 278<br>`cogged_dovetailed_corner_lap` |
| ![](joint_library/beam_half_blind_cogged_corner_lap.png) | **Half-blind cogged corner lap joint**<br>halbverdecktes Hakeneckblatt mit Wange, p. 280<br>`half_blind_cogged_corner_lap` |
| ![](joint_library/beam_half_blind_dovetailed_corner_lap.png) | **Half-blind dovetailed corner lap joint**<br>halbes Schwalbenschwanzeckblatt, p. 286<br>`half_blind_dovetailed_corner_lap` |
| ![](joint_library/beam_rabbeted_tenoned_corner.png) | **Rabbeted tenoned corner joint**<br>abgesetzter Eckstoß mit Führungszapfen, p. 290<br>`rabbeted_tenoned_corner` |
| ![](joint_library/beam_rabbeted_tongue_dado_corner.png) | **Rabbeted tongue and dado corner joint**<br>Eckstoß mit doppeltem Führungszapfen, p. 288<br>`rabbeted_tongue_dado_corner` |
| ![](joint_library/beam_bridle.png) | **Bridle joint**<br>Scherzapfen, p. 294<br>`bridle` |
| ![](joint_library/beam_twin_bridle.png) | **Twin bridle joint**<br>doppelter Scherzapfen, p. 296<br>`twin_bridle` |
| ![](joint_library/beam_through_dovetail_tenon.png) | **Through dovetail tenon joint**<br>durchgehender Schwalbenschwanzeckzapfen, p. 298<br>`through_dovetail_tenon` |
| ![](joint_library/beam_pegged_through_dovetail_tenon.png) | **Pegged through dovetail tenon joint**<br>durchgehender Schwalbenschwanzeckzapfen mit Dübelsicherung, p. 300<br>`pegged_through_dovetail_tenon` |
| ![](joint_library/beam_half_blind_bridle.png) | **Half-blind bridle joint**<br>halbverdeckter Scherzapfen, p. 302<br>`half_blind_bridle` |
| ![](joint_library/beam_triple_shouldered_through_mortise_tenon_corner.png) | **Triple-shouldered through mortise and tenon joint**<br>durchgehender abgesetzter Eckzapfen, p. 308<br>`triple_shouldered_through_mortise_tenon_corner` |
| ![](joint_library/beam_half_blind_haunched_mortise_tenon.png) | **Half-blind haunched mortise and tenon joint**<br>halbverdeckter Stufeneckzapfen, p. 304<br>`half_blind_haunched_mortise_tenon` |
| ![](joint_library/beam_plain_miter.png) | **Plain miter**<br>stumpfer Gehrungstoß, p. 334<br>`plain_miter` |
| ![](joint_library/beam_plain_miter_butterfly_key.png) | **Plain miter with butterfly key**<br>Gehrungsstoß mit Schwalbenschwanzeinlage, p. 342<br>`plain_miter_butterfly_key` |
| ![](joint_library/beam_mitered_half_lap.png) | **Mitered half lap**<br>Eckblatt auf Gehrung, p. 346<br>`mitered_half_lap` |
| ![](joint_library/beam_blind_mitered_half_lap.png) | **Blind mitered half lap**<br>verdecktes Eckblatt auf Gehrung, p. 348<br>`blind_mitered_half_lap` |
| ![](joint_library/beam_mitered_bridle.png) | **Mitered bridle joint**<br>durchgehender Zapfen mit Gehrungsstoß, p. 352<br>`mitered_bridle` |
| ![](joint_library/beam_blind_stub_tenon_miter.png) | **Blind stub tenon miter**<br>verdeckter Zapfen mit Gehrungsstoß, p. 354<br>`blind_stub_tenon_miter` |
| ![](joint_library/beam_plain_miter_spline_key.png) | **Plain miter with spline key**<br>Gehrungsstoß mit Feder, p. 336<br>`plain_miter_spline_key` |
| ![](joint_library/beam_bridle_one_sided_miter.png) | **Bridle joint with one-sided miter**<br>Zapfen mit einseitiger Gehrung, p. 362<br>`bridle_one_sided_miter` |
| ![](joint_library/beam_double_mortise_tenon_miter.png) | **Double mortise and tenon miter**<br>Zapfen mit doppeltem Gehrungsblatt, p. 366<br>`double_mortise_tenon_miter` |
| ![](joint_library/beam_blind_pegged_corner.png) | **Blind pegged joint**<br>Verdollung, p. 408<br>`blind_pegged_corner` |
| ![](joint_library/beam_cog_corner.png) | **Cog joint**<br>Eckkamm, p. 410<br>`cog_corner` |
| ![](joint_library/beam_sloped_cog_corner.png) | **Sloped cog joint**<br>schräger Eckkamm, p. 412<br>`sloped_cog_corner` |
| ![](joint_library/beam_dovetail_cog_corner.png) | **Dovetail cog joint**<br>Eckkamm mit Schwalbenschwanz, p. 414<br>`dovetail_cog_corner` |
| ![](joint_library/beam_v_shaped_cog_corner.png) | **V-shaped cog joint**<br>Eckkamm mit halbem Kreuz, p. 416<br>`v_shaped_cog_corner` |
| ![](joint_library/beam_fully_shouldered_through_mortise_tenon_corner.png) | **Fully shouldered through mortise and tenon**<br>beidseitig abgesetzter durchgehender Eckzapfen, p. 318<br>`fully_shouldered_through_mortise_tenon_corner` |
| ![](joint_library/beam_through_mortise_tenon_sloped_haunch_corner.png) | **Through mortise and tenon with sloped haunch**<br>durchgehender abgesetzter Eckzapfen mit schräger Stufe, p. 312<br>`through_mortise_tenon_sloped_haunch_corner` |
| ![](joint_library/beam_triple_shouldered_sloped_through_mortise_tenon.png) | **Triple-shouldered and sloped through mortise and tenon**<br>abgesetzter durchgehender schräger Eckzapfen, p. 316<br>`triple_shouldered_sloped_through_mortise_tenon` |
| ![](joint_library/beam_dado_rabbet_corner.png) | **Dado and rabbet joint**<br>Eckstoß mit Führungszapfen, p. 326<br>`dado_rabbet_corner` |
| ![](joint_library/beam_triple_shouldered_stub_tenon_corner.png) | **Triple-shouldered stub tenon**<br>verdeckter abgesetzter Eckzapfen, p. 398<br>`triple_shouldered_stub_tenon_corner` |
| ![](joint_library/beam_through_single_dovetail_miter.png) | **Through single dovetail miter**<br>durchgehender Schwalbenschwanzzapfen mit Gehrungsstoß, p. 356<br>`through_single_dovetail_miter` |
| ![](joint_library/beam_plain_miter_spline.png) | **Plain miter with spline**<br>Gehrungsstoß mit Federstäbchen, p. 338<br>`plain_miter_spline` |
| ![](joint_library/beam_blind_dado_rabbet_corner.png) | **Blind dado and rabbet joint**<br>Eckstoß mit Führungszapfen und Wange auf Gehrung, p. 328<br>`blind_dado_rabbet_corner` |
| ![](joint_library/beam_housed_stub_tenon_corner.png) | **Housed stub tenon**<br>halbverdeckter eingelassener Eckzapfen, p. 384<br>`housed_stub_tenon_corner` |
| ![](joint_library/beam_housed_dovetail_tenon_corner.png) | **Housed dovetail tenon**<br>halbverdeckter eingelassener Schwalbenschwanzeckzapfen, p. 386<br>`housed_dovetail_tenon_corner` |
| ![](joint_library/beam_half_blind_dovetail_bridle_corner.png) | **Half-blind dovetail bridle joint**<br>halbverdeckter Schwalbenschwanzeckzapfen, p. 392<br>`half_blind_dovetail_bridle_corner` |
| ![](joint_library/beam_through_dovetail_bridle_corner.png) | **Through dovetail bridle joint**<br>durchgehender Schwalbenschwanzeckzapfen, p. 394<br>`through_dovetail_bridle_corner` |
| ![](joint_library/beam_half_blind_triple_shouldered_dovetail_corner.png) | **Half-blind triple shouldered dovetail tenon**<br>halbverdeckter abgesetzter Schwalbenschwanzeckzapfen, p. 396<br>`half_blind_triple_shouldered_dovetail_corner` |
| ![](joint_library/beam_blind_housed_dovetail_tenon_corner.png) | **Blind housed dovetail tenon**<br>Schwalbenschwanzeckzapfen mit gestufter Brüstung, p. 388<br>`blind_housed_dovetail_tenon_corner` |
| ![](joint_library/beam_half_blind_gooseneck_tenon_corner.png) | **Half-blind gooseneck tenon**<br>halbverdeckter Sicheleckzapfen, p. 390<br>`half_blind_gooseneck_tenon_corner` |
| ![](joint_library/beam_plain_miter_perpendicular_butterfly_key.png) | **Plain miter with perpendicular butterfly key**<br>Gehrungsstoß mit Schwalbenschwanzdübel, p. 344<br>`plain_miter_perpendicular_butterfly_key` |
| ![](joint_library/beam_double_mortise_tenon_one_sided_miter.png) | **Double mortise and tenon with one-sided lapped miter**<br>doppelter Zapfen mit Gehrungsblatt, p. 368<br>`double_mortise_tenon_one_sided_miter` |
| ![](joint_library/beam_haunched_through_tenon_two_sided_miter.png) | **Haunched through mortise and tenon with two-sided lapped miter**<br>durchgehender abgesetzter Stufenzapfen mit zweiseitigem Gehrungsblatt, p. 370<br>`haunched_through_tenon_two_sided_miter` |
| ![](joint_library/beam_haunched_through_tenon_one_sided_miter.png) | **Haunched through mortise and tenon with one-sided lapped miter**<br>durchgehender abgesetzter Stufenzapfen mit Gehrungsblatt, p. 374<br>`haunched_through_tenon_one_sided_miter` |
| ![](joint_library/beam_haunched_through_tenon_two_sided_stopped_miter.png) | **Haunched through mortise and tenon with two-sided stopped miter**<br>durchgehender abgesetzter Stufenzapfen mit zweiseitigem halben Gehrungsblatt, p. 372<br>`haunched_through_tenon_two_sided_stopped_miter` |
| ![](joint_library/beam_haunched_through_tenon_one_sided_stopped_miter.png) | **Haunched through mortise and tenon with one-sided stopped lapped miter**<br>durchgehender abgesetzter Stufenzapfen mit halbem Gehrungsblatt, p. 376<br>`haunched_through_tenon_one_sided_stopped_miter` |
| ![](joint_library/beam_blind_haunched_tenon_miter.png) | **Blind haunched mortise and tenon miter**<br>verdeckter abgesetzter Stufenzapfen mit Gehrungsblatt und Wange, p. 378<br>`blind_haunched_tenon_miter` |
| ![](joint_library/beam_haunched_through_double_tenon_one_sided_miter.png) | **Haunched through double mortise and tenon with one-sided lapped miter**<br>durchgehender abgesetzter doppelter Stufenzapfen mit Gehrungsblatt, p. 380<br>`haunched_through_double_tenon_one_sided_miter` |
| ![](joint_library/beam_blind_mitered_corner_lap.png) | **Blind mitered corner lap joint**<br>verdecktes Eckblatt mit Wange auf Gehrung, p. 264<br>`blind_mitered_corner_lap` |
| ![](joint_library/beam_blind_mitered_cogged_corner_lap.png) | **Blind mitered cogged corner lap joint**<br>verdecktes Hakeneckblatt mit Wange auf Gehrung, p. 284<br>`blind_mitered_cogged_corner_lap` |
| ![](joint_library/beam_blind_mitered_beveled_corner_lap.png) | **Blind mitered beveled corner lap joint**<br>verdecktes Eckblatt mit zweifach schräger Blattfläche und Wange auf Gehrung, p. 270<br>`blind_mitered_beveled_corner_lap` |
| ![](joint_library/beam_half_blind_mitered_sloped_cogged_corner_lap.png) | **Half-blind mitered sloped cogged corner lap joint**<br>halbverdecktes schräges Hakeneckblatt mit Wange auf Gehrung, p. 282<br>`half_blind_mitered_sloped_cogged_corner_lap` |
| ![](joint_library/beam_rabbeted_tenoned_miter.png) | **Rabbeted tenoned miter joint**<br>abgesetzter Eckstoß auf Gehrung mit Führungszapfen, p. 292<br>`rabbeted_tenoned_miter` |
| ![](joint_library/beam_wedged_haunched_through_tenon_corner.png) | **Wedged haunched through mortise and tenon joint**<br>verkeilter durchgehender Stufeneckzapfen, p. 306<br>`wedged_haunched_through_tenon_corner` |
| ![](joint_library/beam_wedged_triple_shouldered_through_tenon_corner.png) | **Wedged triple-shouldered through mortise and tenon joint**<br>durchgehender abgesetzter Eckzapfen mit Keilschloss, p. 310<br>`wedged_triple_shouldered_through_tenon_corner` |
| ![](joint_library/beam_collared_haunched_mortise_tenon_corner.png) | **Collared haunched mortise and tenon joint**<br>durchgehender abgesetzter Stufen-eckzapfen mit Führungszapfen, p. 330<br>`collared_haunched_mortise_tenon_corner` |
| ![](joint_library/beam_beveled_triple_shouldered_housed_tenon_corner.png) | **Beveled triple-shouldered mortise and tenon with housing**<br>verdeckter abgesetzter Eckzapfen mit schräger Brust, p. 314<br>`beveled_triple_shouldered_housed_tenon_corner` |
| ![](joint_library/beam_blind_bridle_corner.png) | **Blind bridle joint**<br>verdeckter Scherzapfen mit Wange auf Gehrung, p. 320<br>`blind_bridle_corner` |
| ![](joint_library/beam_blind_dovetail_miter.png) | **Blind dovetail miter**<br>verdeckter Schwalbenschwanzzapfen mit Gehrungsstoß, p. 358<br>`blind_dovetail_miter` |
| ![](joint_library/beam_blind_mitered_mortise_tenon.png) | **Blind mitered mortise and tenon joint**<br>verdeckter beidseitiger Zapfen mit Gehrungsstoß, p. 350<br>`blind_mitered_mortise_tenon` |
| ![](joint_library/beam_housed_dovetail_corner.png) | **Housed dovetail joint**<br>verdeckter beidseitig abgesetzter Schwalbenschwanzeckzapfen, p. 400<br>`housed_dovetail_corner` |
| ![](joint_library/beam_plain_miter_dovetail_key.png) | **Plain miter with dovetail key**<br>Gehrungsstoß mit Schwalbenschwanzfeder, p. 340<br>`plain_miter_dovetail_key` |
| ![](joint_library/beam_diagonally_cut_cog_corner.png) | **Diagonally cut cog joint**<br>Eckkamm mit Haken, p. 418<br>`diagonally_cut_cog_corner` |
| ![](joint_library/beam_notched_lap_stub_tenon_corner.png) | **Notched lap joint with stub tenon**<br>Meisterwitz, p. 406<br>`notched_lap_stub_tenon_corner` |
| ![](joint_library/beam_triple_shouldered_double_tenon_miter.png) | **Triple-shouldered double tenon miter**<br>durchgehender abgesetzter doppelter Zapfen mit Gehrungsblatt und Wange, p. 382<br>`triple_shouldered_double_tenon_miter` |
| ![](joint_library/beam_blind_triple_shouldered_through_tenon_corner.png) | **Blind triple-shouldered through mortise and tenon**<br>durchgehender abgesetzer Eckzapfen mit Wange auf Gehrung, p. 324<br>`blind_triple_shouldered_through_tenon_corner` |
| ![](joint_library/beam_blind_fan_double_tenon_corner.png) | **Blind fan-shaped double mortise and tenon**<br>verdeckter doppelter trapezförmiger Eckzapfen mit Wange auf Gehrung, p. 402<br>`blind_fan_double_tenon_corner` |
| ![](joint_library/beam_wedged_collared_blind_mortise_tenon_corner.png) | **Wedged collared blind mortise and tenon**<br>verkeilter durchgehender abgesetzter Eckzapfen mit Führungszapfen und Wange auf Gehrung, p. 332<br>`wedged_collared_blind_mortise_tenon_corner` |
| ![](joint_library/beam_cross_lap.png) | **Cross lap joint**<br>Kreuzblatt, p. 646<br>`cross_lap` |
| ![](joint_library/beam_pegged_cross_lap.png) | **Pegged cross lap joint**<br>Kreuzblatt mit Dübel, p. 648<br>`pegged_cross_lap` |
| ![](joint_library/beam_blind_pegged_cross.png) | **Joint with blind peg**<br>Überkreuzung mit Dolle, p. 680<br>`blind_pegged_cross` |
| ![](joint_library/beam_notch_cross.png) | **Notch joint**<br>Kreuzkamm mit einseitigem Sitz, p. 682<br>`notch_cross` |
| ![](joint_library/beam_double_notch_cross.png) | **Double notch joint**<br>Kreuzkamm, p. 684<br>`double_notch_cross` |
| ![](joint_library/beam_cog_cross.png) | **Cog joint**<br>Kreuzkamm mit Stufe, p. 692<br>`cog_cross` |
| ![](joint_library/beam_double_cog_cross.png) | **Double cog joint**<br>Kreuzkamm mit mittiger Stufe, p. 694<br>`double_cog_cross` |
| ![](joint_library/beam_sloped_cog_cross.png) | **Sloped cog joint**<br>Kreuzkamm mit einseitig schräger Stufe, p. 696<br>`sloped_cog_cross` |
| ![](joint_library/beam_v_shaped_cog_cross.png) | **V-shaped cog joint**<br>Kreuzkamm mit halbem Kreuz, p. 700<br>`v_shaped_cog_cross` |
| ![](joint_library/beam_x_shaped_cog_cross.png) | **X-shaped cog joint**<br>Kreuzkamm mit Kreuz, p. 702<br>`x_shaped_cog_cross` |
| ![](joint_library/beam_dovetail_cog_cross.png) | **Dovetail cog joint**<br>Kreuzkamm mit Schwalbenschwanz, p. 704<br>`dovetail_cog_cross` |
| ![](joint_library/beam_half_dovetail_cog_cross.png) | **Half-dovetail cog joint**<br>Kreuzkamm mit einseitigem Schwalbenschwanz, p. 706<br>`half_dovetail_cog_cross` |
| ![](joint_library/beam_double_notch_blind_peg_cross.png) | **Double notch joint with blind peg**<br>Kreuzkamm mit Dolle, p. 686<br>`double_notch_blind_peg_cross` |
| ![](joint_library/beam_stepped_cross_lap.png) | **Stepped cross lap joint**<br>Kreuzblatt mit Haken, p. 656<br>`stepped_cross_lap` |
| ![](joint_library/beam_cogged_cross_lap.png) | **Cogged cross lap joint**<br>Kreuzblatt mit mittigem Haken, p. 658<br>`cogged_cross_lap` |
| ![](joint_library/beam_offset_cross_lap.png) | **Offset cross lap joint**<br>Kreuzblatt mit Führung, p. 664<br>`offset_cross_lap` |
| ![](joint_library/beam_double_offset_cross_lap.png) | **Double offset cross lap joint**<br>zweiseitig abgesetztes Kreuzblatt, p. 666<br>`double_offset_cross_lap` |
| ![](joint_library/beam_triple_offset_cross_lap.png) | **Triple offset cross lap joint**<br>zweiseitig abgesetztes Kreuzblatt mit Führung, p. 668<br>`triple_offset_cross_lap` |
| ![](joint_library/beam_sloped_double_cog_cross.png) | **Sloped double cog joint**<br>Kreuzkamm mit mittiger beidseitig schräger Stufe, p. 698<br>`sloped_double_cog_cross` |
| ![](joint_library/beam_oblique_double_notch_cross.png) | **Oblique double notch joint**<br>Kreuzkamm als Versatz, p. 690<br>`oblique_double_notch_cross` |
| ![](joint_library/beam_oblique_notched_cross_lap.png) | **Oblique notched cross lap joint**<br>schräges Kreuzblatt mit Stirnversatz, p. 672<br>`oblique_notched_cross_lap` |
| ![](joint_library/beam_oblique_notched_heel_cross_lap.png) | **Oblique notched heel cross lap joint**<br>schräges Kreuzblatt mit Fersenversatz, p. 674<br>`oblique_notched_heel_cross_lap` |
| ![](joint_library/beam_double_notched_oblique_cross_lap.png) | **Double notched oblique cross lap joint**<br>schräges Kreuzblatt mit Stirn- und Fersenversatz, p. 676<br>`double_notched_oblique_cross_lap` |
| ![](joint_library/beam_three_way_cross_lap.png) | **Three-way cross lap joint**<br>doppelte Überblattung, p. 678<br>`three_way_cross_lap` |
| ![](joint_library/beam_cross_lap_stub_tenons.png) | **Cross lap joint with stub tenons**<br>Kreuzblatt mit beidseitigen Brüstungszapfen, p. 662<br>`cross_lap_stub_tenons` |
| ![](joint_library/beam_cogged_cross_lap_stub_tenons.png) | **Cogged cross lap joint with stub tenons**<br>Kreuzblatt mit mittigem Haken und beidseitigen Brüstungszapfen, p. 660<br>`cogged_cross_lap_stub_tenons` |
| ![](joint_library/beam_double_notch_stub_tenons_cross.png) | **Double notch joint with stub tenons**<br>Kreuzkamm mit abgesetzter Falz, p. 688<br>`double_notch_stub_tenons_cross` |
| ![](joint_library/beam_v_mitered_cross_lap.png) | **V-mitered cross lap joint**<br>Kreuzblatt mit Grat, p. 652<br>`v_mitered_cross_lap` |
| ![](joint_library/beam_mitered_abutments_cross_lap.png) | **Cross lap joint with mitered abutments**<br>schräg abgesetztes Kreuzblatt, p. 654<br>`mitered_abutments_cross_lap` |
| ![](joint_library/beam_post_through_tenon_crossed.png) | **Post with through tenon and mortised crossed joint**<br>durchgesteckter Zapfen mit aufgesetzter Überkreuzung, p. 712<br>`post_through_tenon_crossed` |
| ![](joint_library/beam_post_stub_tenon_double_cogged.png) | **Post with stub tenon and mortised double cogged joint**<br>verdeckter Zapfen mit Kreuzkamm und mittiger Stufe, p. 708<br>`post_stub_tenon_double_cogged` |
| ![](joint_library/beam_stacked_tenon_mortised_cross_lap.png) | **Stacked tenon and mortised cross lap joint**<br>durchgesteckter beidseitig abgesetzter Stufenzapfen mit Kreuzblatt, p. 714<br>`stacked_tenon_mortised_cross_lap` |
| ![](joint_library/beam_castle_joint.png) | **Castle joint**<br>doppelte Einhälsung, p. 716<br>`castle_joint` |
| ![](joint_library/beam_t_bridle_stub_tenons_mortised_beam.png) | **T-bridle joint with stub tenons and mortised beam**<br>Einhälsung mit Brüstungszapfen und aufgesetzter Überkreuzung, p. 710<br>`t_bridle_stub_tenons_mortised_beam` |
| ![](joint_library/beam_pegged_two_way_tenon.png) | **Pegged two-way tenon joint**<br>durchgesteckter Zapfen mit Dübelsicherung, p. 760<br>`pegged_two_way_tenon` |
| ![](joint_library/beam_two_way_rod_tenon_tapered_keys.png) | **Two-way rod tenon joint with tapered keys**<br>durchgesteckter Stabzapfen mit Spannkeilen, p. 746<br>`two_way_rod_tenon_tapered_keys` |
| ![](joint_library/beam_two_way_blind_housed_rod_tenon.png) | **Two-way blind housed rod tenon joint with tapered keys**<br>eingelassener durchgesteckter Stabzapfen mit gestufter Brust und Spannkeilen, p. 752<br>`two_way_blind_housed_rod_tenon` |
| ![](joint_library/beam_two_way_collared_rod_tenon.png) | **Two-way collared rod tenon joint with tapered keys**<br>eingelassener durchgesteckter Stabzapfen mit gestufter abgesetzter Brust und Spannkeilen, p. 754<br>`two_way_collared_rod_tenon` |
| ![](joint_library/beam_corner_post_open_slot_mortise_tenon.png) | **Corner-post joint with open slot mortise and tenon**<br>Scherzapfen mit Zapfenloch und durchgehendem Zapfen, p. 720<br>`corner_post_open_slot_mortise_tenon` |
| ![](joint_library/beam_lapped_haunched_mortise_tenon_post.png) | **Lapped and haunched mortise and tenon joint**<br>wechselseitig abgesetzte verdeckte Stufenzapfen, p. 726<br>`lapped_haunched_mortise_tenon_post` |
| ![](joint_library/beam_mitered_haunched_mortise_tenon_post.png) | **Mitered and haunched mortise and tenon joint**<br>verdeckte Stufenzapfen auf Gehrung, p. 728<br>`mitered_haunched_mortise_tenon_post` |
| ![](joint_library/beam_corner_post_fan_stub_tenon_blind_dado.png) | **Corner-post joint with fan-shaped stub tenon and blind dado and rabbet joint**<br>Eckstoß mit Führungszapfen und Wange auf Gehrung sowie durchgehendem trapezförmigem Zapfen, p. 722<br>`corner_post_fan_stub_tenon_blind_dado` |
| ![](joint_library/beam_housed_stopped_dovetail_lap_post_stub_tenon.png) | **Housed and stopped dovetail lap and fully shouldered stub tenon**<br>verdeckter eingelassener Schwalbenschwanzzapfen mit abgesetzter Brust und Pfosten mit verdecktem Zapfen, p. 724<br>`housed_stopped_dovetail_lap_post_stub_tenon` |
| ![](joint_library/beam_pegged_two_way_splayed_scarf.png) | **Pegged two-way splayed scarf joint**<br>eingeschobene Schäftung mit Dübelsicherung, p. 738<br>`pegged_two_way_splayed_scarf` |
| ![](joint_library/beam_two_way_rabbeted_rod_tenon.png) | **Two-way rabbeted rod tenon joint with tapered keys**<br>eingelassener durchgesteckter Stabzapfen mit Steckfalz und Spannkeilen, p. 756<br>`two_way_rabbeted_rod_tenon` |
| ![](joint_library/beam_two_way_partially_housed_rod_tenon.png) | **Two-way partially housed rod tenon joint with tapered keys**<br>eingelassener durchgesteckter Stabzapfen mit abgesetzter Nut und Spannkeilen, p. 758<br>`two_way_partially_housed_rod_tenon` |
| ![](joint_library/beam_pegged_two_way_lapped_rod_tenon_housing.png) | **Pegged two-way lapped rod tenon joint with housing**<br>durchgesteckte Stabzapfen mit Blatt und Zapfenloch sowie Dübelsicherung, p. 764<br>`pegged_two_way_lapped_rod_tenon_housing` |
| ![](joint_library/beam_two_way_splined_rod_keys_peg.png) | **Two-way splined rod joint with tapered keys and a peg**<br>durchgesteckter Fremdzapfen mit Spannkeilen und Dübelsicherung, p. 762<br>`two_way_splined_rod_keys_peg` |
| ![](joint_library/beam_three_way_rod_tenon_keys_wedged_haunch.png) | **Three-way rod tenon joint with tapered keys and wedged haunched tenon**<br>durchgesteckter Stabzapfen mit Spannkeilen und verkeiltem Stufenzapfen, p. 748<br>`three_way_rod_tenon_keys_wedged_haunch` |
| ![](joint_library/beam_four_way_rod_tenon_keys_pegs.png) | **Four-way rod tenon joint with tapered keys and pegs**<br>durchgesteckte Stufenstabzapfen mit Spannkeilen und Dübelsicherung, p. 750<br>`four_way_rod_tenon_keys_pegs` |
| ![](joint_library/beam_wedged_two_way_cogged_lap.png) | **Wedged two-way cogged lap joint**<br>verkeiltes gerades Hakendruckschloss, p. 736<br>`wedged_two_way_cogged_lap` |
| ![](joint_library/beam_wedged_two_way_oblique_cogged.png) | **Wedged two-way oblique cogged joint**<br>verkeiltes zweiseitiges Hakendruckschloss mit schrägen Haken, p. 742<br>`wedged_two_way_oblique_cogged` |
| ![](joint_library/beam_pegged_two_way_gooseneck.png) | **Pegged two-way gooseneck joint**<br>Sichelzapfenverbindung mit Dübelsicherung, p. 740<br>`pegged_two_way_gooseneck` |
| ![](joint_library/beam_notched_corner_post_beveled_tenons.png) | **Notched corner-post joint with beveled-shoulder tenons**<br>Zapfen mit schräger einseitig auf Gehrung angeschnittener Brust, p. 718<br>`notched_corner_post_beveled_tenons` |
| ![](joint_library/beam_half_lap_splice.png) | **Half lap joint**<br>gerades Blatt, p. 42<br>`half_lap_splice` |
| ![](joint_library/beam_beveled_lap_splice.png) | **Beveled lap joint**<br>Zugblatt, p. 46<br>`beveled_lap_splice` |
| ![](joint_library/beam_half_lap_stub_tenon_splice.png) | **Half lap joint with stub tenon**<br>gerades Blatt mit einseitigem Zapfen, p. 50<br>`half_lap_stub_tenon_splice` |
| ![](joint_library/beam_undersquinted_half_lap_splice.png) | **Undersquinted half lap joint**<br>gerades Blatt mit hinterschnittener Brüstung, p. 56<br>`undersquinted_half_lap_splice` |
| ![](joint_library/beam_pegged_undersquinted_half_lap_splice.png) | **Pegged undersquinted half lap joint**<br>gerades Blatt mit hinterschnittener Brüstung und Dübelsicherung, p. 58<br>`pegged_undersquinted_half_lap_splice` |
| ![](joint_library/beam_twice_pegged_undersquinted_half_lap_splice.png) | **Pegged undersquinted half lap joint**<br>gerades Blatt mit hinterschnittener Brüstung und zweifacher Dübelsicherung, p. 60<br>`twice_pegged_undersquinted_half_lap_splice` |
| ![](joint_library/beam_spear_point_half_lap_splice.png) | **Spear point half lap joint**<br>gerades Blatt mit Gratschnitt, p. 62<br>`spear_point_half_lap_splice` |
| ![](joint_library/beam_beveled_spear_point_lap_splice.png) | **Beveled spear point lap joint**<br>Zugblatt mit Gratschnitt, p. 64<br>`beveled_spear_point_lap_splice` |
| ![](joint_library/beam_undersquinted_spear_point_half_lap_splice.png) | **Undersquinted spear point half lap joint**<br>gerades Blatt mit schrägem Gratschnitt, p. 66<br>`undersquinted_spear_point_half_lap_splice` |
| ![](joint_library/beam_cogged_lap_splice.png) | **Cogged lap joint**<br>gerades Hakenblatt, p. 72<br>`cogged_lap_splice` |
| ![](joint_library/beam_pegged_undersquinted_spear_point_half_lap_splice.png) | **Pegged undersquinted spear point half lap joint**<br>gerades Blatt mit schrägem Gratschnitt und Dübelsicherung, p. 68<br>`pegged_undersquinted_spear_point_half_lap_splice` |
| ![](joint_library/beam_twice_pegged_undersquinted_spear_point_half_lap_splice.png) | **Pegged undersquinted spear point half lap joint**<br>gerades Blatt mit schrägem Gratschnitt und zweifacher Dübelsicherung, p. 70<br>`twice_pegged_undersquinted_spear_point_half_lap_splice` |
| ![](joint_library/beam_stop_splayed_scarf.png) | **Stop-splayed scarf joint**<br>schräges Blatt, p. 114<br>`stop_splayed_scarf` |
| ![](joint_library/beam_undersquinted_stop_splayed_scarf.png) | **Undersquinted stop-splayed scarf joint**<br>schräges Blatt mit schräger Brüstung, p. 116<br>`undersquinted_stop_splayed_scarf` |
| ![](joint_library/beam_pegged_stop_splayed_scarf.png) | **Pegged stop-splayed scarf joint**<br>schräges Blatt mit Dübelsicherung, p. 120<br>`pegged_stop_splayed_scarf` |
| ![](joint_library/beam_stop_splayed_cogged_scarf.png) | **Stop-splayed cogged scarf joint**<br>schräges Hakenblatt, p. 122<br>`stop_splayed_cogged_scarf` |
| ![](joint_library/beam_undersquinted_stop_splayed_cogged_scarf.png) | **Undersquinted stop-splayed cogged scarf joint**<br>schräges Hakenblatt mit schräger Brüstung, p. 124<br>`undersquinted_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_stop_splayed_scarf_stub_tenons.png) | **Stop-splayed scarf joint with stub tenons**<br>schräges Blatt mit Brüstungszapfen, p. 118<br>`stop_splayed_scarf_stub_tenons` |
| ![](joint_library/beam_pegged_stop_splayed_cogged_scarf.png) | **Pegged stop-splayed cogged scarf joint**<br>schräges Hakenblatt mit Dübelsicherung, p. 130<br>`pegged_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_splayed_scarf.png) | **Splayed scarf joint**<br>Schäftung, p. 156<br>`splayed_scarf` |
| ![](joint_library/beam_triple_cogged_lap_splice.png) | **Triple-cogged lap joint**<br>gerades Hakenblatt mit dreifachem Haken, p. 102<br>`triple_cogged_lap_splice` |
| ![](joint_library/beam_rabbeted_half_lap_splice.png) | **Rabbeted half lap joint**<br>gerades Blatt mit Steckfalz, p. 104<br>`rabbeted_half_lap_splice` |
| ![](joint_library/beam_rabbeted_cogged_lap_splice.png) | **Rabbeted cogged lap joint**<br>gerades Hakenblatt mit Steckfalz, p. 106<br>`rabbeted_cogged_lap_splice` |
| ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf.png) | **Rabbeted stop-splayed cogged scarf joint**<br>schräges Hakenblatt mit Steckfalz, p. 138<br>`rabbeted_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_pegged_rabbeted_stop_splayed_cogged_scarf.png) | **Pegged rabbeted stop-splayed cogged scarf joint**<br>schräges Hakenblatt mit Steckfalz und Dübelsicherung, p. 140<br>`pegged_rabbeted_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_mitered_half_lap_splice.png) | **Mitered half lap joint**<br>gerades Blatt mit einseitig schrägem Stoß, p. 44<br>`mitered_half_lap_splice` |
| ![](joint_library/beam_beveled_lap_stub_tenons_splice.png) | **Beveled lap joint with stub tenons**<br>Zugblatt mit Brüstungszapfen, p. 48<br>`beveled_lap_stub_tenons_splice` |
| ![](joint_library/beam_half_lap_stub_tenons_splice.png) | **Half lap joint with stub tenons**<br>gerades Blatt mit beidseitigem Zapfen, p. 54<br>`half_lap_stub_tenons_splice` |
| ![](joint_library/beam_stub_tenon_splice.png) | **Stub tenon joint**<br>Zapfen, p. 176<br>`stub_tenon_splice` |
| ![](joint_library/beam_half_blind_stub_tenon_splice.png) | **Half-blind stub tenon joint**<br>einseitig abgesetzter Zapfen, p. 178<br>`half_blind_stub_tenon_splice` |
| ![](joint_library/beam_floating_tenon_splice.png) | **Floating tenon joint**<br>eingesetzter Zapfen, p. 180<br>`floating_tenon_splice` |
| ![](joint_library/beam_pegged_bridle_splice.png) | **Pegged bridle joint**<br>Zapfen mit Dübelsicherung, p. 182<br>`pegged_bridle_splice` |
| ![](joint_library/beam_double_tenon_splice.png) | **Double tenon joint**<br>doppelter Zapfen, p. 198<br>`double_tenon_splice` |
| ![](joint_library/beam_half_blind_double_tenon_splice.png) | **Half-blind double tenon joint**<br>einseitig abgesetzter doppelter Zapfen, p. 200<br>`half_blind_double_tenon_splice` |
| ![](joint_library/beam_blind_square_stub_tenon_splice.png) | **Blind square stub tenon joint**<br>verdeckter beidseitig abgesetzter Zapfen, p. 202<br>`blind_square_stub_tenon_splice` |
| ![](joint_library/beam_x_shaped_tenon_splice.png) | **X-shaped tenon joint**<br>Kreuzzapfen, p. 212<br>`x_shaped_tenon_splice` |
| ![](joint_library/beam_cross_shaped_stub_tenon_splice.png) | **Cross-shaped stub tenon joint**<br>kreuzförmiger Zapfen, p. 216<br>`cross_shaped_stub_tenon_splice` |
| ![](joint_library/beam_dovetail_tenon_splice.png) | **Dovetail tenon joint**<br>Schwalbenschwanzzapfen, p. 220<br>`dovetail_tenon_splice` |
| ![](joint_library/beam_butt_floating_butterfly_tenon_splice.png) | **Butt joint with floating butterfly tenon**<br>eingesetzter Schwalbenschwanzzapfen, p. 230<br>`butt_floating_butterfly_tenon_splice` |
| ![](joint_library/beam_butt_butterfly_key_splice.png) | **Butt joint with butterfly key**<br>Schwalbenschwanzeinlage, p. 232<br>`butt_butterfly_key_splice` |
| ![](joint_library/beam_lapped_dovetail_splice.png) | **Lapped dovetail joint**<br>Schwalbenschwanzzapfen mit geradem Blatt, p. 224<br>`lapped_dovetail_splice` |
| ![](joint_library/beam_gooseneck_tenon_splice.png) | **Gooseneck tenon joint**<br>Sichelzapfen, p. 244<br>`gooseneck_tenon_splice` |
| ![](joint_library/beam_birdsmouth_splice.png) | **Birdsmouth**<br>Gratstoß, p. 168<br>`birdsmouth_splice` |
| ![](joint_library/beam_double_halved_lap_splice.png) | **Double halved lap joint**<br>diagonal versetztes abgesetztes Blatt, p. 210<br>`double_halved_lap_splice` |
| ![](joint_library/beam_double_dovetail_tenon_splice.png) | **Double dovetail tenon joint**<br>doppelter Schwalbenschwanzzapfen, p. 236<br>`double_dovetail_tenon_splice` |
| ![](joint_library/beam_half_lap_butterfly_key_splice.png) | **Half lap joint with butterfly key**<br>gerades Blatt mit Schwalbenschwanzeinlage, p. 234<br>`half_lap_butterfly_key_splice` |
| ![](joint_library/beam_cogged_lap_folding_wedges_splice.png) | **Cogged lap joint with folding wedges**<br>verkeiltes gerades Hakenblatt, p. 74<br>`cogged_lap_folding_wedges_splice` |
| ![](joint_library/beam_rabbeted_cogged_lap_folding_wedges_splice.png) | **Rabbeted cogged lap joint with folding wedges**<br>verkeiltes gerades Hakenblatt mit Steckfalz, p. 108<br>`rabbeted_cogged_lap_folding_wedges_splice` |
| ![](joint_library/beam_stop_splayed_cogged_scarf_folding_wedges.png) | **Stop-splayed cogged scarf joint with folding wedges**<br>verkeiltes schräges Hakenblatt, p. 126<br>`stop_splayed_cogged_scarf_folding_wedges` |
| ![](joint_library/beam_undersquinted_stop_splayed_cogged_scarf_folding_wedges.png) | **Undersquinted stop-splayed cogged scarf joint with folding wedges**<br>verkeiltes schräges Hakenblatt mit schräger Brüstung, p. 128<br>`undersquinted_stop_splayed_cogged_scarf_folding_wedges` |
| ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf_folding_wedges.png) | **Rabbeted stop-splayed cogged scarf joint with folding wedges**<br>verkeiltes schräges Hakenblatt mit Steckfalz, p. 142<br>`rabbeted_stop_splayed_cogged_scarf_folding_wedges` |
| ![](joint_library/beam_spear_point_cogged_lap_splice.png) | **Spear point cogged lap joint**<br>gerades Hakenblatt mit Gratschnitt, p. 78<br>`spear_point_cogged_lap_splice` |
| ![](joint_library/beam_spear_point_cogged_lap_folding_wedges_splice.png) | **Spear point cogged lap joint with folding wedges**<br>verkeiltes gerades Hakenblatt mit Gratschnitt, p. 80<br>`spear_point_cogged_lap_folding_wedges_splice` |
| ![](joint_library/beam_undersquinted_cogged_lap_folding_wedges_splice.png) | **Undersquinted cogged lap joint with folding wedges**<br>verkeiltes gerades Hakenblatt mit hinterschnittener Brüstung, p. 76<br>`undersquinted_cogged_lap_folding_wedges_splice` |
| ![](joint_library/beam_half_lap_floating_piece_splice.png) | **Half lap joint with floating piece**<br>gerades Blatt mit eingesetztem Mittelstück, p. 84<br>`half_lap_floating_piece_splice` |
| ![](joint_library/beam_triple_shouldered_gooseneck_splice.png) | **Triple-shouldered gooseneck tenon joint**<br>abgesetzter Sichelzapfen, p. 246<br>`triple_shouldered_gooseneck_splice` |
| ![](joint_library/beam_lapped_gooseneck_splice.png) | **Lapped gooseneck joint**<br>Sichelzapfen mit geradem Blatt, p. 248<br>`lapped_gooseneck_splice` |
| ![](joint_library/beam_lapped_gooseneck_stub_tenon_splice.png) | **Lapped gooseneck joint with stub tenon**<br>Sichelzapfen mit geradem Blatt und Brüstungszapfen, p. 250<br>`lapped_gooseneck_stub_tenon_splice` |
| ![](joint_library/beam_triple_shouldered_dovetail_tenon_splice.png) | **Triple-shouldered dovetail tenon joint**<br>abgesetzter Schwalbenschwanzzapfen, p. 222<br>`triple_shouldered_dovetail_tenon_splice` |
| ![](joint_library/beam_lapped_dovetail_stub_tenon_splice.png) | **Lapped dovetail joint with stub tenon**<br>Schwalbenschwanzzapfen mit geradem Blatt und Brüstungszapfen, p. 226<br>`lapped_dovetail_stub_tenon_splice` |
| ![](joint_library/beam_lapped_double_dovetail_splice.png) | **Lapped double dovetail joint**<br>beidseitiger Schwalbenschwanzzapfen mit geradem Blatt, p. 228<br>`lapped_double_dovetail_splice` |
| ![](joint_library/beam_half_lap_haunched_tenon_splice.png) | **Half lap joint with haunched tenon**<br>gerades Blatt mit Stufenzapfen, p. 52<br>`half_lap_haunched_tenon_splice` |
| ![](joint_library/beam_half_lap_mitered_ends_splice.png) | **Half lap joint with mitered ends**<br>stehendes gerades Blatt mit beidseitig schrägem Stoß, p. 158<br>`half_lap_mitered_ends_splice` |
| ![](joint_library/beam_blind_fan_stub_tenon_splice.png) | **Blind fan-shaped stub tenon joint**<br>verdeckter trapezförmiger Zapfen, p. 204<br>`blind_fan_stub_tenon_splice` |
| ![](joint_library/beam_blind_u_stub_tenon_splice.png) | **Blind U-shaped stub tenon joint**<br>verdeckter u-förmiger Zapfen, p. 208<br>`blind_u_stub_tenon_splice` |
| ![](joint_library/beam_undersquinted_half_lap_floating_piece_splice.png) | **Undersquinted half lap joint with floating piece**<br>gerades Blatt mit hinterschnittener Brüstung und eingesetztem Mittelstück, p. 86<br>`undersquinted_half_lap_floating_piece_splice` |
| ![](joint_library/beam_halved_splayed_scarf.png) | **Halved splayed scarf joint**<br>doppelte Schäftung, p. 162<br>`halved_splayed_scarf` |
| ![](joint_library/beam_four_faced_splayed_scarf.png) | **Four-faced splayed scarf joint**<br>vierseitige Schäftung, p. 174<br>`four_faced_splayed_scarf` |
| ![](joint_library/beam_diagonally_cut_birdsmouth_splice.png) | **Diagonally cut birdsmouth**<br>diagonale doppelte Schäftung, p. 170<br>`diagonally_cut_birdsmouth_splice` |
| ![](joint_library/beam_triple_splayed_scarf.png) | **Triple-splayed scarf joint**<br>dreifache Schäftung, p. 160<br>`triple_splayed_scarf` |
| ![](joint_library/beam_v_end_undercut_tenon_splice.png) | **Mortise and tenon with V-ends and undercut shoulders**<br>Zapfen mit hinterschnittener Brüstung und v-förmig eingeschnittener Stirn, p. 184<br>`v_end_undercut_tenon_splice` |
| ![](joint_library/beam_pegged_undercut_tenon_splice.png) | **Pegged mortise and tenon with undercut shoulders**<br>Zapfen mit hinterschnittener Brüstung und Dübelsicherung, p. 186<br>`pegged_undercut_tenon_splice` |
| ![](joint_library/beam_spear_point_beveled_tenon_splice.png) | **Spear point mortise and tenon with beveled shoulders**<br>Zapfen mit Gratschnitt und schräger Brüstung, p. 188<br>`spear_point_beveled_tenon_splice` |
| ![](joint_library/beam_pegged_spear_point_v_shoulder_tenon_splice.png) | **Pegged spear point mortise and tenon with V-shaped shoulders**<br>Zapfen mit Gratschnitt, v-förmig eingeschnittener Brüstung und Dübelsicherung, p. 190<br>`pegged_spear_point_v_shoulder_tenon_splice` |
| ![](joint_library/beam_double_mortise_stub_tenon_splice.png) | **Double mortise and stub tenon joint**<br>geschlitzter Zapfen mit beidseitigem Zapfenloch, p. 214<br>`double_mortise_stub_tenon_splice` |
| ![](joint_library/beam_two_half_rods_tapered_keys_splice.png) | **Rod joint with two half rods and tapered keys**<br>beidseitiger Stabzapfen mit Spannkeilen, p. 196<br>`two_half_rods_tapered_keys_splice` |
| ![](joint_library/beam_cogged_lap_floating_piece_splice.png) | **Cogged lap joint with floating piece**<br>gerades Hakenblatt mit eingesetztem Hakenstück, p. 90<br>`cogged_lap_floating_piece_splice` |
| ![](joint_library/beam_stepped_cogged_lap_floating_piece_splice.png) | **Stepped cogged lap joint with floating piece**<br>gestuftes Hakenblatt mit eingesetztem Hakenstück, p. 96<br>`stepped_cogged_lap_floating_piece_splice` |
| ![](joint_library/beam_butt_floating_gooseneck_tenon_splice.png) | **Butt joint with floating gooseneck tenon**<br>Stoß mit eingesetztem Sichelzapfen, p. 256<br>`butt_floating_gooseneck_tenon_splice` |
| ![](joint_library/beam_cogged_lap_floating_piece_folding_wedges_splice.png) | **Cogged lap joint with floating piece and folding wedges**<br>verkeiltes gerades Hakenblatt mit eingesetztem Hakenstück, p. 92<br>`cogged_lap_floating_piece_folding_wedges_splice` |
| ![](joint_library/beam_stepped_cogged_lap_floating_piece_folding_wedges_splice.png) | **Stepped cogged lap joint with floating piece and folding wedges**<br>verkeiltes gestuftes Hakenblatt mit eingesetztem Hakenstück, p. 98<br>`stepped_cogged_lap_floating_piece_folding_wedges_splice` |
| ![](joint_library/beam_butt_gooseneck_key_stub_tenons_splice.png) | **Butt joint with a gooseneck key and stub tenons**<br>Stoß mit Sichelzapfeneinlage und doppelten Brüstungszapfen, p. 258<br>`butt_gooseneck_key_stub_tenons_splice` |
| ![](joint_library/beam_halved_splayed_scarf_oblique_key.png) | **Halved splayed scarf joint with oblique mating surfaces and tapered key**<br>schräg angeschnittene doppelte Schäftung mit Spannkeil, p. 166<br>`halved_splayed_scarf_oblique_key` |
| ![](joint_library/beam_halved_splayed_rabbeted_scarf_key.png) | **Halved splayed rabbeted scarf joint with tapered key**<br>doppelte Schäftung mit Brüstungszapfen und Spannkeil, p. 164<br>`halved_splayed_rabbeted_scarf_key` |
| ![](joint_library/beam_pegged_rabbeted_mortise_tenon_splice.png) | **Pegged rabbeted mortise and tenon joint**<br>Zapfen mit Steckfalz und Dübelsicherung, p. 192<br>`pegged_rabbeted_mortise_tenon_splice` |
| ![](joint_library/beam_rabbeted_rod_tenon_tapered_keys_splice.png) | **Rabbeted rod tenon joint with tapered keys**<br>Stabzapfen mit Steckfalz und Spannkeilen, p. 194<br>`rabbeted_rod_tenon_tapered_keys_splice` |
| ![](joint_library/beam_undersquinted_cogged_lap_floating_piece_wedges_splice.png) | **Undersquinted cogged lap joint with floating piece and folding wedges**<br>verkeiltes gerades Hakenblatt mit hinterschnittener Brüstung und eingesetztem Hakenstück, p. 94<br>`undersquinted_cogged_lap_floating_piece_wedges_splice` |
| ![](joint_library/beam_undersquinted_stepped_cogged_lap_floating_piece_wedges_splice.png) | **Undersquinted stepped cogged lap joint with floating piece and folding wedges**<br>verkeiltes gestuftes Hakenblatt mit hinterschnittener Brüstung und eingesetztem Hakenstück, p. 100<br>`undersquinted_stepped_cogged_lap_floating_piece_wedges_splice` |
| ![](joint_library/beam_undersquinted_spear_point_cogged_lap_folding_wedges_splice.png) | **Undersquinted spear point cogged lap joint with folding wedges**<br>verkeiltes gerades Hakenblatt mit schrägem Gratschnitt, p. 82<br>`undersquinted_spear_point_cogged_lap_folding_wedges_splice` |
| ![](joint_library/beam_rabbeted_cogged_lap_stub_tenons_folding_wedges_splice.png) | **Rabbeted cogged lap joint with stub tenons and folding wedges**<br>verkeiltes gerades Hakenblatt mit Steckfalz und Brüstungszapfen, p. 110<br>`rabbeted_cogged_lap_stub_tenons_folding_wedges_splice` |
| ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf_stub_tenons_folding_wedges.png) | **Rabbeted stop-splayed cogged scarf joint with stub tenons and folding wedges**<br>verkeiltes schräges Hakenblatt mit Steckfalz und Brüstungszapfen, p. 144<br>`rabbeted_stop_splayed_cogged_scarf_stub_tenons_folding_wedges` |
| ![](joint_library/beam_pegged_rabbeted_stop_splayed_cogged_scarf_stub_tenons.png) | **Pegged rabbeted stop-splayed cogged scarf joint with stub tenons**<br>stehendes schräges Hakenblatt mit Steckfalz, Brüstungszapfen und Dübelsicherung, p. 150<br>`pegged_rabbeted_stop_splayed_cogged_scarf_stub_tenons` |
| ![](joint_library/beam_haunched_gooseneck_stub_tenons_splice.png) | **Haunched gooseneck tenon with stub tenons**<br>Sichelstufenzapfen mit doppeltem Brüstungszapfen, p. 252<br>`haunched_gooseneck_stub_tenons_splice` |
| ![](joint_library/beam_half_blind_l_stub_tenon_splice.png) | **Half-blind L-shaped stub tenon joint**<br>Winkelzapfen, p. 206<br>`half_blind_l_stub_tenon_splice` |
| ![](joint_library/beam_stop_splayed_scarf_hidden_hook.png) | **Stop-splayed scarf joint with hidden hook**<br>schräges Blatt mit verdecktem Haken, p. 136<br>`stop_splayed_scarf_hidden_hook` |
| ![](joint_library/beam_spear_point_stop_splayed_cogged_scarf_folding_wedges.png) | **Spear point stop-splayed cogged scarf joint with folding wedges**<br>verkeiltes schräges Hakenblatt mit Gratschnitt, p. 134<br>`spear_point_stop_splayed_cogged_scarf_folding_wedges` |
| ![](joint_library/beam_pegged_rabbeted_stop_splayed_sloped_cogged_scarf.png) | **Pegged rabbeted stop-splayed sloped cogged scarf joint**<br>stehendes schräges Hakenblatt mit Steckfalz und doppelter Dübelsicherung, p. 146<br>`pegged_rabbeted_stop_splayed_sloped_cogged_scarf` |
| ![](joint_library/beam_four_faced_dovetail_splice.png) | **Four-faced dovetail joint**<br>diagonaler doppelter Schwalbenschwanzzapfen, p. 238<br>`four_faced_dovetail_splice` |
| ![](joint_library/beam_four_faced_gooseneck_splice.png) | **Four-faced gooseneck joint**<br>diagonaler doppelter Sichelzapfen mit Steckfalz, p. 260<br>`four_faced_gooseneck_splice` |
| ![](joint_library/beam_tapered_dovetail_mitered_shoulders_splice.png) | **Tapered dovetail tenon joint with mitered shoulders**<br>Stoß mit Gratschnitt und keilförmigen Schwalbenschwanzzapfen, p. 240<br>`tapered_dovetail_mitered_shoulders_splice` |
| ![](joint_library/beam_spear_point_cross_stub_tenon_splice.png) | **Spear point cross-shaped stub tenon joint with undercut shoulders**<br>kreuzförmiger Zapfen mit Gratschnitt und hinterschnittener Brüstung, p. 218<br>`spear_point_cross_stub_tenon_splice` |
| ![](joint_library/beam_blind_rabbeted_diagonal_half_lap_key_splice.png) | **Blind rabbeted diagonally cut half lap joint with a key**<br>diagonales Blatt mit verdecktem Steckfalz und Spannkeil, p. 154<br>`blind_rabbeted_diagonal_half_lap_key_splice` |
| ![](joint_library/beam_post_through_tenon_stop_splayed_scarf_stub_tenons.png) | **Post with through tenon and mortised stop-splayed cogged scarf joint with stub tenons**<br>schräges Hakenblatt mit Brüstungszapfen auf durchgehendem Pfostenzapfen, p. 582<br>`post_through_tenon_stop_splayed_scarf_stub_tenons` |
| ![](joint_library/beam_half_blind_rabbeted_lap_tapered_keys_splice.png) | **Half-blind rabbeted lap joint with tapered keys**<br>halbverdecktes gerades Blatt mit Steckfalz und Spannkeilen, p. 112<br>`half_blind_rabbeted_lap_tapered_keys_splice` |
| ![](joint_library/beam_pegged_half_blind_rabbeted_stop_splayed_cogged_scarf.png) | **Pegged half blind rabbeted stop-splayed cogged scarf joint**<br>halbverdecktes schräges Hakenblatt mit Steckfalz und Dübelsicherung, p. 152<br>`pegged_half_blind_rabbeted_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_pegged_rabbeted_notched_stop_splayed_cogged_scarf.png) | **Pegged rabbeted notched stop-splayed cogged scarf joint**<br>stehendes schräges Hakenblatt mit unterbrochener Steckfalz und Dübelsicherung, p. 148<br>`pegged_rabbeted_notched_stop_splayed_cogged_scarf` |
| ![](joint_library/beam_cogged_lap_floating_tenoned_piece_splice.png) | **Cogged lap joint with floating tenoned piece**<br>gerades Hakenblatt mit eingesetztem Hakenstück mit Zapfen, p. 88<br>`cogged_lap_floating_tenoned_piece_splice` |
| ![](joint_library/beam_triple_faced_splayed_scarf.png) | **Triple-faced splayed scarf joint**<br>einseitig schräg angeschnittene doppelte Schäftung, p. 172<br>`triple_faced_splayed_scarf` |
| ![](joint_library/beam_stop_splayed_cogged_scarf_stub_tenons_blind_pegs.png) | **Stop-splayed cogged scarf joint with stub tenons and blind pegs**<br>schräges Hakenblatt mit Brüstungszapfen und Dollen, p. 132<br>`stop_splayed_cogged_scarf_stub_tenons_blind_pegs` |
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
| ![](joint_library/beam_housed_mortise_tenon_beveled_shoulders.png) | **Mortise and tenon with beveled shoulders**<br>verdeckter Zapfen mit eingelassener schräger Brüstung, p. 464<br>`housed_mortise_tenon_beveled_shoulders` |
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
| ![](joint_library/beam_diagonally_cut_t_bridle.png) | **Diagonally cut T-bridle joint**<br>diagonale Einhälsung, p. 566<br>`diagonally_cut_t_bridle` |
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
| ![](joint_library/beam_post_stub_tenon_slot_mortise_splice.png) | **Post with stub tenon and open slot mortise and tenon**<br>abgesetzter Zapfenstoß auf verdecktem Zapfen, p. 578<br>`post_stub_tenon_slot_mortise_splice` |
| ![](joint_library/beam_post_stub_tenon_lapped_splice.png) | **Post with stub tenon and lapped mortise and tenon**<br>gerades Blatt mit Zapfen auf verdecktem Zapfen, p. 580<br>`post_stub_tenon_lapped_splice` |
| ![](joint_library/beam_right_angle_spline_tenon_tapered_key.png) | **Right-angle joint with dovetailed spline tenon and tapered key**<br>Stoß mit Führungszapfen und eingesetztem Schwalbenschwanzzapfen mit Spannkeil, p. 538<br>`right_angle_spline_tenon_tapered_key` |
| ![](joint_library/beam_housed_right_angle_spline_tenon_keys_peg.png) | **Housed right-angle joint with spline tenon, tapered keys, and peg**<br>eingelassener stumpfer Stoß mit eingesetztem Zapfen und Dübelsicherung sowie Spannkeilen, p. 540<br>`housed_right_angle_spline_tenon_keys_peg` |
<!-- /joint-table -->

## Library
Every record built on pairs of 120 x 120 beams standing in the xz plane, a group and a row per design: on a square corner and on one leaning at 50 degrees where the record exists there, each drawn apart as the book draws it, the member 350 mm out along its own axis and every peg along its own; the ways each goes in are the record's insertions.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp
