# Beam Compendium Joints {#elements_joint_beam_compendium}

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
| assembly | joint | tile |
|---|---|---|
| **Half lap joint**, p. 42 | | |
|  | ![](joint_library/beam_half_lap_splice.png) | ![](joint_library/tile_half_lap_splice.png) |
| **Mitered half lap joint**, p. 44 | | |
|  | ![](joint_library/beam_mitered_half_lap_splice.png) | ![](joint_library/tile_mitered_half_lap_splice.png) |
| **Beveled lap joint**, p. 46 | | |
|  | ![](joint_library/beam_beveled_lap_splice.png) | ![](joint_library/tile_beveled_lap_splice.png) |
| **Beveled lap joint with stub tenons**, p. 48 | | |
|  | ![](joint_library/beam_beveled_lap_stub_tenons_splice.png) | ![](joint_library/tile_beveled_lap_stub_tenons_splice.png) |
| **Half lap joint with stub tenon**, p. 50 | | |
|  | ![](joint_library/beam_half_lap_stub_tenon_splice.png) | ![](joint_library/tile_half_lap_stub_tenon_splice.png) |
| **Half lap joint with haunched tenon**, p. 52 | | |
|  | ![](joint_library/beam_half_lap_haunched_tenon_splice.png) | ![](joint_library/tile_half_lap_haunched_tenon_splice.png) |
| **Half lap joint with stub tenons**, p. 54 | | |
|  | ![](joint_library/beam_half_lap_stub_tenons_splice.png) | ![](joint_library/tile_half_lap_stub_tenons_splice.png) |
| **Undersquinted half lap joint**, p. 56 | | |
|  | ![](joint_library/beam_undersquinted_half_lap_splice.png) | ![](joint_library/tile_undersquinted_half_lap_splice.png) |
| **Pegged undersquinted half lap joint**, p. 58 | | |
|  | ![](joint_library/beam_pegged_undersquinted_half_lap_splice.png) | ![](joint_library/tile_pegged_undersquinted_half_lap_splice.png) |
| **Pegged undersquinted half lap joint**, p. 60 | | |
|  | ![](joint_library/beam_twice_pegged_undersquinted_half_lap_splice.png) | ![](joint_library/tile_twice_pegged_undersquinted_half_lap_splice.png) |
| **Spear point half lap joint**, p. 62 | | |
|  | ![](joint_library/beam_spear_point_half_lap_splice.png) | ![](joint_library/tile_spear_point_half_lap_splice.png) |
| **Beveled spear point lap joint**, p. 64 | | |
|  | ![](joint_library/beam_beveled_spear_point_lap_splice.png) | ![](joint_library/tile_beveled_spear_point_lap_splice.png) |
| **Undersquinted spear point half lap joint**, p. 66 | | |
|  | ![](joint_library/beam_undersquinted_spear_point_half_lap_splice.png) | ![](joint_library/tile_undersquinted_spear_point_half_lap_splice.png) |
| **Pegged undersquinted spear point half lap joint**, p. 68 | | |
|  | ![](joint_library/beam_pegged_undersquinted_spear_point_half_lap_splice.png) | ![](joint_library/tile_pegged_undersquinted_spear_point_half_lap_splice.png) |
| **Pegged undersquinted spear point half lap joint**, p. 70 | | |
|  | ![](joint_library/beam_twice_pegged_undersquinted_spear_point_half_lap_splice.png) | ![](joint_library/tile_twice_pegged_undersquinted_spear_point_half_lap_splice.png) |
| **Cogged lap joint**, p. 72 | | |
|  | ![](joint_library/beam_cogged_lap_splice.png) | ![](joint_library/tile_cogged_lap_splice.png) |
| **Cogged lap joint with folding wedges**, p. 74 | | |
|  | ![](joint_library/beam_cogged_lap_folding_wedges_splice.png) | ![](joint_library/tile_cogged_lap_folding_wedges_splice.png) |
| **Undersquinted cogged lap joint with folding wedges**, p. 76 | | |
|  | ![](joint_library/beam_undersquinted_cogged_lap_folding_wedges_splice.png) | ![](joint_library/tile_undersquinted_cogged_lap_folding_wedges_splice.png) |
| **Spear point cogged lap joint**, p. 78 | | |
|  | ![](joint_library/beam_spear_point_cogged_lap_splice.png) | ![](joint_library/tile_spear_point_cogged_lap_splice.png) |
| **Spear point cogged lap joint with folding wedges**, p. 80 | | |
|  | ![](joint_library/beam_spear_point_cogged_lap_folding_wedges_splice.png) | ![](joint_library/tile_spear_point_cogged_lap_folding_wedges_splice.png) |
| **Undersquinted spear point cogged lap joint with folding wedges**, p. 82 | | |
|  | ![](joint_library/beam_undersquinted_spear_point_cogged_lap_folding_wedges_splice.png) | ![](joint_library/tile_undersquinted_spear_point_cogged_lap_folding_wedges_splice.png) |
| **Half lap joint with floating piece**, p. 84 | | |
|  | ![](joint_library/beam_half_lap_floating_piece_splice.png) | ![](joint_library/tile_half_lap_floating_piece_splice.png) |
| **Undersquinted half lap joint with floating piece**, p. 86 | | |
|  | ![](joint_library/beam_undersquinted_half_lap_floating_piece_splice.png) | ![](joint_library/tile_undersquinted_half_lap_floating_piece_splice.png) |
| **Cogged lap joint with floating tenoned piece**, p. 88 | | |
|  | ![](joint_library/beam_cogged_lap_floating_tenoned_piece_splice.png) | ![](joint_library/tile_cogged_lap_floating_tenoned_piece_splice.png) |
| **Cogged lap joint with floating piece**, p. 90 | | |
|  | ![](joint_library/beam_cogged_lap_floating_piece_splice.png) | ![](joint_library/tile_cogged_lap_floating_piece_splice.png) |
| **Cogged lap joint with floating piece and folding wedges**, p. 92 | | |
|  | ![](joint_library/beam_cogged_lap_floating_piece_folding_wedges_splice.png) | ![](joint_library/tile_cogged_lap_floating_piece_folding_wedges_splice.png) |
| **Undersquinted cogged lap joint with floating piece and folding wedges**, p. 94 | | |
|  | ![](joint_library/beam_undersquinted_cogged_lap_floating_piece_wedges_splice.png) | ![](joint_library/tile_undersquinted_cogged_lap_floating_piece_wedges_splice.png) |
| **Stepped cogged lap joint with floating piece**, p. 96 | | |
|  | ![](joint_library/beam_stepped_cogged_lap_floating_piece_splice.png) | ![](joint_library/tile_stepped_cogged_lap_floating_piece_splice.png) |
| **Stepped cogged lap joint with floating piece and folding wedges**, p. 98 | | |
|  | ![](joint_library/beam_stepped_cogged_lap_floating_piece_folding_wedges_splice.png) | ![](joint_library/tile_stepped_cogged_lap_floating_piece_folding_wedges_splice.png) |
| **Undersquinted stepped cogged lap joint with floating piece and folding wedges**, p. 100 | | |
|  | ![](joint_library/beam_undersquinted_stepped_cogged_lap_floating_piece_wedges_splice.png) | ![](joint_library/tile_undersquinted_stepped_cogged_lap_floating_piece_wedges_splice.png) |
| **Triple-cogged lap joint**, p. 102 | | |
|  | ![](joint_library/beam_triple_cogged_lap_splice.png) | ![](joint_library/tile_triple_cogged_lap_splice.png) |
| **Rabbeted half lap joint**, p. 104 | | |
|  | ![](joint_library/beam_rabbeted_half_lap_splice.png) | ![](joint_library/tile_rabbeted_half_lap_splice.png) |
| **Rabbeted cogged lap joint**, p. 106 | | |
|  | ![](joint_library/beam_rabbeted_cogged_lap_splice.png) | ![](joint_library/tile_rabbeted_cogged_lap_splice.png) |
| **Rabbeted cogged lap joint with folding wedges**, p. 108 | | |
|  | ![](joint_library/beam_rabbeted_cogged_lap_folding_wedges_splice.png) | ![](joint_library/tile_rabbeted_cogged_lap_folding_wedges_splice.png) |
| **Rabbeted cogged lap joint with stub tenons and folding wedges**, p. 110 | | |
| ![](joint_library/beam_rabbeted_cogged_lap_stub_tenons_folding_wedges_splice.gif) | ![](joint_library/beam_rabbeted_cogged_lap_stub_tenons_folding_wedges_splice.png) | ![](joint_library/tile_rabbeted_cogged_lap_stub_tenons_folding_wedges_splice.png) |
| **Half-blind rabbeted lap joint with tapered keys**, p. 112 | | |
|  | ![](joint_library/beam_half_blind_rabbeted_lap_tapered_keys_splice.png) | ![](joint_library/tile_half_blind_rabbeted_lap_tapered_keys_splice.png) |
| **Stop-splayed scarf joint**, p. 114 | | |
|  | ![](joint_library/beam_stop_splayed_scarf.png) | ![](joint_library/tile_stop_splayed_scarf.png) |
| **Undersquinted stop-splayed scarf joint**, p. 116 | | |
|  | ![](joint_library/beam_undersquinted_stop_splayed_scarf.png) | ![](joint_library/tile_undersquinted_stop_splayed_scarf.png) |
| **Stop-splayed scarf joint with stub tenons**, p. 118 | | |
|  | ![](joint_library/beam_stop_splayed_scarf_stub_tenons.png) | ![](joint_library/tile_stop_splayed_scarf_stub_tenons.png) |
| **Pegged stop-splayed scarf joint**, p. 120 | | |
|  | ![](joint_library/beam_pegged_stop_splayed_scarf.png) | ![](joint_library/tile_pegged_stop_splayed_scarf.png) |
| **Stop-splayed cogged scarf joint**, p. 122 | | |
|  | ![](joint_library/beam_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_stop_splayed_cogged_scarf.png) |
| **Undersquinted stop-splayed cogged scarf joint**, p. 124 | | |
|  | ![](joint_library/beam_undersquinted_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_undersquinted_stop_splayed_cogged_scarf.png) |
| **Stop-splayed cogged scarf joint with folding wedges**, p. 126 | | |
|  | ![](joint_library/beam_stop_splayed_cogged_scarf_folding_wedges.png) | ![](joint_library/tile_stop_splayed_cogged_scarf_folding_wedges.png) |
| **Undersquinted stop-splayed cogged scarf joint with folding wedges**, p. 128 | | |
|  | ![](joint_library/beam_undersquinted_stop_splayed_cogged_scarf_folding_wedges.png) | ![](joint_library/tile_undersquinted_stop_splayed_cogged_scarf_folding_wedges.png) |
| **Pegged stop-splayed cogged scarf joint**, p. 130 | | |
|  | ![](joint_library/beam_pegged_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_pegged_stop_splayed_cogged_scarf.png) |
| **Stop-splayed cogged scarf joint with stub tenons and blind pegs**, p. 132 | | |
|  | ![](joint_library/beam_stop_splayed_cogged_scarf_stub_tenons_blind_pegs.png) | ![](joint_library/tile_stop_splayed_cogged_scarf_stub_tenons_blind_pegs.png) |
| **Spear point stop-splayed cogged scarf joint with folding wedges**, p. 134 | | |
|  | ![](joint_library/beam_spear_point_stop_splayed_cogged_scarf_folding_wedges.png) | ![](joint_library/tile_spear_point_stop_splayed_cogged_scarf_folding_wedges.png) |
| **Stop-splayed scarf joint with hidden hook**, p. 136 | | |
|  | ![](joint_library/beam_stop_splayed_scarf_hidden_hook.png) | ![](joint_library/tile_stop_splayed_scarf_hidden_hook.png) |
| **Rabbeted stop-splayed cogged scarf joint**, p. 138 | | |
|  | ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_rabbeted_stop_splayed_cogged_scarf.png) |
| **Pegged rabbeted stop-splayed cogged scarf joint**, p. 140 | | |
|  | ![](joint_library/beam_pegged_rabbeted_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_pegged_rabbeted_stop_splayed_cogged_scarf.png) |
| **Rabbeted stop-splayed cogged scarf joint with folding wedges**, p. 142 | | |
|  | ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf_folding_wedges.png) | ![](joint_library/tile_rabbeted_stop_splayed_cogged_scarf_folding_wedges.png) |
| **Rabbeted stop-splayed cogged scarf joint with stub tenons and folding wedges**, p. 144 | | |
|  | ![](joint_library/beam_rabbeted_stop_splayed_cogged_scarf_stub_tenons_folding_wedges.png) | ![](joint_library/tile_rabbeted_stop_splayed_cogged_scarf_stub_tenons_folding_wedges.png) |
| **Pegged rabbeted stop-splayed sloped cogged scarf joint**, p. 146 | | |
|  | ![](joint_library/beam_pegged_rabbeted_stop_splayed_sloped_cogged_scarf.png) | ![](joint_library/tile_pegged_rabbeted_stop_splayed_sloped_cogged_scarf.png) |
| **Pegged rabbeted notched stop-splayed cogged scarf joint**, p. 148 | | |
|  | ![](joint_library/beam_pegged_rabbeted_notched_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_pegged_rabbeted_notched_stop_splayed_cogged_scarf.png) |
| **Pegged rabbeted stop-splayed cogged scarf joint with stub tenons**, p. 150 | | |
|  | ![](joint_library/beam_pegged_rabbeted_stop_splayed_cogged_scarf_stub_tenons.png) | ![](joint_library/tile_pegged_rabbeted_stop_splayed_cogged_scarf_stub_tenons.png) |
| **Pegged half blind rabbeted stop-splayed cogged scarf joint**, p. 152 | | |
|  | ![](joint_library/beam_pegged_half_blind_rabbeted_stop_splayed_cogged_scarf.png) | ![](joint_library/tile_pegged_half_blind_rabbeted_stop_splayed_cogged_scarf.png) |
| **Blind rabbeted diagonally cut half lap joint with a key**, p. 154 | | |
|  | ![](joint_library/beam_blind_rabbeted_diagonal_half_lap_key_splice.png) | ![](joint_library/tile_blind_rabbeted_diagonal_half_lap_key_splice.png) |
| **Splayed scarf joint**, p. 156 | | |
|  | ![](joint_library/beam_splayed_scarf.png) | ![](joint_library/tile_splayed_scarf.png) |
| **Half lap joint with mitered ends**, p. 158 | | |
|  | ![](joint_library/beam_half_lap_mitered_ends_splice.png) | ![](joint_library/tile_half_lap_mitered_ends_splice.png) |
| **Triple-splayed scarf joint**, p. 160 | | |
|  | ![](joint_library/beam_triple_splayed_scarf.png) | ![](joint_library/tile_triple_splayed_scarf.png) |
| **Halved splayed scarf joint**, p. 162 | | |
|  | ![](joint_library/beam_halved_splayed_scarf.png) | ![](joint_library/tile_halved_splayed_scarf.png) |
| **Halved splayed rabbeted scarf joint with tapered key**, p. 164 | | |
|  | ![](joint_library/beam_halved_splayed_rabbeted_scarf_key.png) | ![](joint_library/tile_halved_splayed_rabbeted_scarf_key.png) |
| **Halved splayed scarf joint with oblique mating surfaces and tapered key**, p. 166 | | |
|  | ![](joint_library/beam_halved_splayed_scarf_oblique_key.png) | ![](joint_library/tile_halved_splayed_scarf_oblique_key.png) |
| **Birdsmouth**, p. 168 | | |
|  | ![](joint_library/beam_birdsmouth_splice.png) | ![](joint_library/tile_birdsmouth_splice.png) |
| **Diagonally cut birdsmouth**, p. 170 | | |
|  | ![](joint_library/beam_diagonally_cut_birdsmouth_splice.png) | ![](joint_library/tile_diagonally_cut_birdsmouth_splice.png) |
| **Triple-faced splayed scarf joint**, p. 172 | | |
|  | ![](joint_library/beam_triple_faced_splayed_scarf.png) | ![](joint_library/tile_triple_faced_splayed_scarf.png) |
| **Four-faced splayed scarf joint**, p. 174 | | |
|  | ![](joint_library/beam_four_faced_splayed_scarf.png) | ![](joint_library/tile_four_faced_splayed_scarf.png) |
| **Stub tenon joint**, p. 176 | | |
|  | ![](joint_library/beam_stub_tenon_splice.png) | ![](joint_library/tile_stub_tenon_splice.png) |
| **Half-blind stub tenon joint**, p. 178 | | |
|  | ![](joint_library/beam_half_blind_stub_tenon_splice.png) | ![](joint_library/tile_half_blind_stub_tenon_splice.png) |
| **Floating tenon joint**, p. 180 | | |
|  | ![](joint_library/beam_floating_tenon_splice.png) | ![](joint_library/tile_floating_tenon_splice.png) |
| **Pegged bridle joint**, p. 182 | | |
|  | ![](joint_library/beam_pegged_bridle_splice.png) | ![](joint_library/tile_pegged_bridle_splice.png) |
| **Mortise and tenon with V-ends and undercut shoulders**, p. 184 | | |
|  | ![](joint_library/beam_v_end_undercut_tenon_splice.png) | ![](joint_library/tile_v_end_undercut_tenon_splice.png) |
| **Pegged mortise and tenon with undercut shoulders**, p. 186 | | |
|  | ![](joint_library/beam_pegged_undercut_tenon_splice.png) | ![](joint_library/tile_pegged_undercut_tenon_splice.png) |
| **Spear point mortise and tenon with beveled shoulders**, p. 188 | | |
|  | ![](joint_library/beam_spear_point_beveled_tenon_splice.png) | ![](joint_library/tile_spear_point_beveled_tenon_splice.png) |
| **Pegged spear point mortise and tenon with V-shaped shoulders**, p. 190 | | |
|  | ![](joint_library/beam_pegged_spear_point_v_shoulder_tenon_splice.png) | ![](joint_library/tile_pegged_spear_point_v_shoulder_tenon_splice.png) |
| **Pegged rabbeted mortise and tenon joint**, p. 192 | | |
|  | ![](joint_library/beam_pegged_rabbeted_mortise_tenon_splice.png) | ![](joint_library/tile_pegged_rabbeted_mortise_tenon_splice.png) |
| **Rabbeted rod tenon joint with tapered keys**, p. 194 | | |
|  | ![](joint_library/beam_rabbeted_rod_tenon_tapered_keys_splice.png) | ![](joint_library/tile_rabbeted_rod_tenon_tapered_keys_splice.png) |
| **Rod joint with two half rods and tapered keys**, p. 196 | | |
|  | ![](joint_library/beam_two_half_rods_tapered_keys_splice.png) | ![](joint_library/tile_two_half_rods_tapered_keys_splice.png) |
| **Double tenon joint**, p. 198 | | |
|  | ![](joint_library/beam_double_tenon_splice.png) | ![](joint_library/tile_double_tenon_splice.png) |
| **Half-blind double tenon joint**, p. 200 | | |
|  | ![](joint_library/beam_half_blind_double_tenon_splice.png) | ![](joint_library/tile_half_blind_double_tenon_splice.png) |
| **Blind square stub tenon joint**, p. 202 | | |
|  | ![](joint_library/beam_blind_square_stub_tenon_splice.png) | ![](joint_library/tile_blind_square_stub_tenon_splice.png) |
| **Blind fan-shaped stub tenon joint**, p. 204 | | |
|  | ![](joint_library/beam_blind_fan_stub_tenon_splice.png) | ![](joint_library/tile_blind_fan_stub_tenon_splice.png) |
| **Half-blind L-shaped stub tenon joint**, p. 206 | | |
|  | ![](joint_library/beam_half_blind_l_stub_tenon_splice.png) | ![](joint_library/tile_half_blind_l_stub_tenon_splice.png) |
| **Blind U-shaped stub tenon joint**, p. 208 | | |
|  | ![](joint_library/beam_blind_u_stub_tenon_splice.png) | ![](joint_library/tile_blind_u_stub_tenon_splice.png) |
| **Double halved lap joint**, p. 210 | | |
|  | ![](joint_library/beam_double_halved_lap_splice.png) | ![](joint_library/tile_double_halved_lap_splice.png) |
| **X-shaped tenon joint**, p. 212 | | |
|  | ![](joint_library/beam_x_shaped_tenon_splice.png) | ![](joint_library/tile_x_shaped_tenon_splice.png) |
| **Double mortise and stub tenon joint**, p. 214 | | |
|  | ![](joint_library/beam_double_mortise_stub_tenon_splice.png) | ![](joint_library/tile_double_mortise_stub_tenon_splice.png) |
| **Cross-shaped stub tenon joint**, p. 216 | | |
|  | ![](joint_library/beam_cross_shaped_stub_tenon_splice.png) | ![](joint_library/tile_cross_shaped_stub_tenon_splice.png) |
| **Spear point cross-shaped stub tenon joint with undercut shoulders**, p. 218 | | |
|  | ![](joint_library/beam_spear_point_cross_stub_tenon_splice.png) | ![](joint_library/tile_spear_point_cross_stub_tenon_splice.png) |
| **Dovetail tenon joint**, p. 220 | | |
|  | ![](joint_library/beam_dovetail_tenon_splice.png) | ![](joint_library/tile_dovetail_tenon_splice.png) |
| **Triple-shouldered dovetail tenon joint**, p. 222 | | |
|  | ![](joint_library/beam_triple_shouldered_dovetail_tenon_splice.png) | ![](joint_library/tile_triple_shouldered_dovetail_tenon_splice.png) |
| **Lapped dovetail joint**, p. 224 | | |
|  | ![](joint_library/beam_lapped_dovetail_splice.png) | ![](joint_library/tile_lapped_dovetail_splice.png) |
| **Lapped dovetail joint with stub tenon**, p. 226 | | |
|  | ![](joint_library/beam_lapped_dovetail_stub_tenon_splice.png) | ![](joint_library/tile_lapped_dovetail_stub_tenon_splice.png) |
| **Lapped double dovetail joint**, p. 228 | | |
|  | ![](joint_library/beam_lapped_double_dovetail_splice.png) | ![](joint_library/tile_lapped_double_dovetail_splice.png) |
| **Butt joint with floating butterfly tenon**, p. 230 | | |
|  | ![](joint_library/beam_butt_floating_butterfly_tenon_splice.png) | ![](joint_library/tile_butt_floating_butterfly_tenon_splice.png) |
| **Butt joint with butterfly key**, p. 232 | | |
|  | ![](joint_library/beam_butt_butterfly_key_splice.png) | ![](joint_library/tile_butt_butterfly_key_splice.png) |
| **Half lap joint with butterfly key**, p. 234 | | |
|  | ![](joint_library/beam_half_lap_butterfly_key_splice.png) | ![](joint_library/tile_half_lap_butterfly_key_splice.png) |
| **Double dovetail tenon joint**, p. 236 | | |
|  | ![](joint_library/beam_double_dovetail_tenon_splice.png) | ![](joint_library/tile_double_dovetail_tenon_splice.png) |
| **Four-faced dovetail joint**, p. 238 | | |
|  | ![](joint_library/beam_four_faced_dovetail_splice.png) | ![](joint_library/tile_four_faced_dovetail_splice.png) |
| **Tapered dovetail tenon joint with mitered shoulders**, p. 240 | | |
|  | ![](joint_library/beam_tapered_dovetail_mitered_shoulders_splice.png) | ![](joint_library/tile_tapered_dovetail_mitered_shoulders_splice.png) |
| **Kawai joint**, p. 242 | | |
|  | ![](joint_library/beam_kawai_splice.png) | ![](joint_library/tile_kawai_splice.png) |
| **Gooseneck tenon joint**, p. 244 | | |
|  | ![](joint_library/beam_gooseneck_tenon_splice.png) | ![](joint_library/tile_gooseneck_tenon_splice.png) |
| **Triple-shouldered gooseneck tenon joint**, p. 246 | | |
|  | ![](joint_library/beam_triple_shouldered_gooseneck_splice.png) | ![](joint_library/tile_triple_shouldered_gooseneck_splice.png) |
| **Lapped gooseneck joint**, p. 248 | | |
|  | ![](joint_library/beam_lapped_gooseneck_splice.png) | ![](joint_library/tile_lapped_gooseneck_splice.png) |
| **Lapped gooseneck joint with stub tenon**, p. 250 | | |
|  | ![](joint_library/beam_lapped_gooseneck_stub_tenon_splice.png) | ![](joint_library/tile_lapped_gooseneck_stub_tenon_splice.png) |
| **Haunched gooseneck tenon with stub tenons**, p. 252 | | |
|  | ![](joint_library/beam_haunched_gooseneck_stub_tenons_splice.png) | ![](joint_library/tile_haunched_gooseneck_stub_tenons_splice.png) |
| **Rabbeted gooseneck joint with barb**, p. 254 | | |
|  | ![](joint_library/beam_rabbeted_gooseneck_barb_splice.png) | ![](joint_library/tile_rabbeted_gooseneck_barb_splice.png) |
| **Butt joint with floating gooseneck tenon**, p. 256 | | |
|  | ![](joint_library/beam_butt_floating_gooseneck_tenon_splice.png) | ![](joint_library/tile_butt_floating_gooseneck_tenon_splice.png) |
| **Butt joint with a gooseneck key and stub tenons**, p. 258 | | |
|  | ![](joint_library/beam_butt_gooseneck_key_stub_tenons_splice.png) | ![](joint_library/tile_butt_gooseneck_key_stub_tenons_splice.png) |
| **Four-faced gooseneck joint**, p. 260 | | |
|  | ![](joint_library/beam_four_faced_gooseneck_splice.png) | ![](joint_library/tile_four_faced_gooseneck_splice.png) |
| **Corner lap joint**, p. 262 | | |
| ![](joint_library/beam_corner_lap.gif) | ![](joint_library/beam_corner_lap.png) | ![](joint_library/tile_corner_lap.png) |
| **Blind mitered corner lap joint**, p. 264 | | |
|  | ![](joint_library/beam_blind_mitered_corner_lap.png) | ![](joint_library/tile_blind_mitered_corner_lap.png) |
| **Dovetailed corner lap joint**, p. 266 | | |
| ![](joint_library/beam_dovetailed_corner_lap.gif) | ![](joint_library/beam_dovetailed_corner_lap.png) | ![](joint_library/tile_dovetailed_corner_lap.png) |
| **Beveled corner lap joint**, p. 268 | | |
| ![](joint_library/beam_beveled_corner_lap.gif) | ![](joint_library/beam_beveled_corner_lap.png) | ![](joint_library/tile_beveled_corner_lap.png) |
| **Blind mitered beveled corner lap joint**, p. 270 | | |
|  | ![](joint_library/beam_blind_mitered_beveled_corner_lap.png) | ![](joint_library/tile_blind_mitered_beveled_corner_lap.png) |
| **Cogged corner lap joint**, p. 272 | | |
| ![](joint_library/beam_cogged_corner_lap.gif) | ![](joint_library/beam_cogged_corner_lap.png) | ![](joint_library/tile_cogged_corner_lap.png) |
| **Sloped cogged corner lap joint**, p. 274 | | |
| ![](joint_library/beam_sloped_cogged_corner_lap.gif) | ![](joint_library/beam_sloped_cogged_corner_lap.png) | ![](joint_library/tile_sloped_cogged_corner_lap.png) |
| **Double cogged corner lap joint**, p. 276 | | |
| ![](joint_library/beam_double_cogged_corner_lap.gif) | ![](joint_library/beam_double_cogged_corner_lap.png) | ![](joint_library/tile_double_cogged_corner_lap.png) |
| **Cogged dovetailed corner lap joint**, p. 278 | | |
| ![](joint_library/beam_cogged_dovetailed_corner_lap.gif) | ![](joint_library/beam_cogged_dovetailed_corner_lap.png) | ![](joint_library/tile_cogged_dovetailed_corner_lap.png) |
| **Half-blind cogged corner lap joint**, p. 280 | | |
| ![](joint_library/beam_half_blind_cogged_corner_lap.gif) | ![](joint_library/beam_half_blind_cogged_corner_lap.png) | ![](joint_library/tile_half_blind_cogged_corner_lap.png) |
| **Half-blind mitered sloped cogged corner lap joint**, p. 282 | | |
|  | ![](joint_library/beam_half_blind_mitered_sloped_cogged_corner_lap.png) | ![](joint_library/tile_half_blind_mitered_sloped_cogged_corner_lap.png) |
| **Blind mitered cogged corner lap joint**, p. 284 | | |
|  | ![](joint_library/beam_blind_mitered_cogged_corner_lap.png) | ![](joint_library/tile_blind_mitered_cogged_corner_lap.png) |
| **Half-blind dovetailed corner lap joint**, p. 286 | | |
| ![](joint_library/beam_half_blind_dovetailed_corner_lap.gif) | ![](joint_library/beam_half_blind_dovetailed_corner_lap.png) | ![](joint_library/tile_half_blind_dovetailed_corner_lap.png) |
| **Rabbeted tongue and dado corner joint**, p. 288 | | |
| ![](joint_library/beam_rabbeted_tongue_dado_corner.gif) | ![](joint_library/beam_rabbeted_tongue_dado_corner.png) | ![](joint_library/tile_rabbeted_tongue_dado_corner.png) |
| **Rabbeted tenoned corner joint**, p. 290 | | |
| ![](joint_library/beam_rabbeted_tenoned_corner.gif) | ![](joint_library/beam_rabbeted_tenoned_corner.png) | ![](joint_library/tile_rabbeted_tenoned_corner.png) |
| **Rabbeted tenoned miter joint**, p. 292 | | |
|  | ![](joint_library/beam_rabbeted_tenoned_miter.png) | ![](joint_library/tile_rabbeted_tenoned_miter.png) |
| **Bridle joint**, p. 294 | | |
| ![](joint_library/beam_bridle.gif) | ![](joint_library/beam_bridle.png) | ![](joint_library/tile_bridle.png) |
| **Twin bridle joint**, p. 296 | | |
| ![](joint_library/beam_twin_bridle.gif) | ![](joint_library/beam_twin_bridle.png) | ![](joint_library/tile_twin_bridle.png) |
| **Through dovetail tenon joint**, p. 298 | | |
| ![](joint_library/beam_through_dovetail_tenon.gif) | ![](joint_library/beam_through_dovetail_tenon.png) | ![](joint_library/tile_through_dovetail_tenon.png) |
| **Pegged through dovetail tenon joint**, p. 300 | | |
| ![](joint_library/beam_pegged_through_dovetail_tenon.gif) | ![](joint_library/beam_pegged_through_dovetail_tenon.png) | ![](joint_library/tile_pegged_through_dovetail_tenon.png) |
| **Half-blind bridle joint**, p. 302 | | |
| ![](joint_library/beam_half_blind_bridle.gif) | ![](joint_library/beam_half_blind_bridle.png) | ![](joint_library/tile_half_blind_bridle.png) |
| **Half-blind haunched mortise and tenon joint**, p. 304 | | |
| ![](joint_library/beam_half_blind_haunched_mortise_tenon.gif) | ![](joint_library/beam_half_blind_haunched_mortise_tenon.png) | ![](joint_library/tile_half_blind_haunched_mortise_tenon.png) |
| **Wedged haunched through mortise and tenon joint**, p. 306 | | |
|  | ![](joint_library/beam_wedged_haunched_through_tenon_corner.png) | ![](joint_library/tile_wedged_haunched_through_tenon_corner.png) |
| **Triple-shouldered through mortise and tenon joint**, p. 308 | | |
| ![](joint_library/beam_triple_shouldered_through_mortise_tenon_corner.gif) | ![](joint_library/beam_triple_shouldered_through_mortise_tenon_corner.png) | ![](joint_library/tile_triple_shouldered_through_mortise_tenon_corner.png) |
| **Wedged triple-shouldered through mortise and tenon joint**, p. 310 | | |
|  | ![](joint_library/beam_wedged_triple_shouldered_through_tenon_corner.png) | ![](joint_library/tile_wedged_triple_shouldered_through_tenon_corner.png) |
| **Through mortise and tenon with sloped haunch**, p. 312 | | |
| ![](joint_library/beam_through_mortise_tenon_sloped_haunch_corner.gif) | ![](joint_library/beam_through_mortise_tenon_sloped_haunch_corner.png) | ![](joint_library/tile_through_mortise_tenon_sloped_haunch_corner.png) |
| **Beveled triple-shouldered mortise and tenon with housing**, p. 314 | | |
|  | ![](joint_library/beam_beveled_triple_shouldered_housed_tenon_corner.png) | ![](joint_library/tile_beveled_triple_shouldered_housed_tenon_corner.png) |
| **Triple-shouldered and sloped through mortise and tenon**, p. 316 | | |
| ![](joint_library/beam_triple_shouldered_sloped_through_mortise_tenon.gif) | ![](joint_library/beam_triple_shouldered_sloped_through_mortise_tenon.png) | ![](joint_library/tile_triple_shouldered_sloped_through_mortise_tenon.png) |
| **Fully shouldered through mortise and tenon**, p. 318 | | |
| ![](joint_library/beam_fully_shouldered_through_mortise_tenon_corner.gif) | ![](joint_library/beam_fully_shouldered_through_mortise_tenon_corner.png) | ![](joint_library/tile_fully_shouldered_through_mortise_tenon_corner.png) |
| **Blind bridle joint**, p. 320 | | |
|  | ![](joint_library/beam_blind_bridle_corner.png) | ![](joint_library/tile_blind_bridle_corner.png) |
| **Blind dovetail tenon**, p. 322 | | |
|  | ![](joint_library/beam_blind_dovetail_tenon_corner.png) | ![](joint_library/tile_blind_dovetail_tenon_corner.png) |
| **Blind triple-shouldered through mortise and tenon**, p. 324 | | |
|  | ![](joint_library/beam_blind_triple_shouldered_through_tenon_corner.png) | ![](joint_library/tile_blind_triple_shouldered_through_tenon_corner.png) |
| **Dado and rabbet joint**, p. 326 | | |
| ![](joint_library/beam_dado_rabbet_corner.gif) | ![](joint_library/beam_dado_rabbet_corner.png) | ![](joint_library/tile_dado_rabbet_corner.png) |
| **Blind dado and rabbet joint**, p. 328 | | |
|  | ![](joint_library/beam_blind_dado_rabbet_corner.png) | ![](joint_library/tile_blind_dado_rabbet_corner.png) |
| **Collared haunched mortise and tenon joint**, p. 330 | | |
|  | ![](joint_library/beam_collared_haunched_mortise_tenon_corner.png) | ![](joint_library/tile_collared_haunched_mortise_tenon_corner.png) |
| **Wedged collared blind mortise and tenon**, p. 332 | | |
|  | ![](joint_library/beam_wedged_collared_blind_mortise_tenon_corner.png) | ![](joint_library/tile_wedged_collared_blind_mortise_tenon_corner.png) |
| **Plain miter**, p. 334 | | |
| ![](joint_library/beam_plain_miter.gif) | ![](joint_library/beam_plain_miter.png) | ![](joint_library/tile_plain_miter.png) |
| **Plain miter with spline key**, p. 336 | | |
| ![](joint_library/beam_plain_miter_spline_key.gif) | ![](joint_library/beam_plain_miter_spline_key.png) | ![](joint_library/tile_plain_miter_spline_key.png) |
| **Plain miter with spline**, p. 338 | | |
|  | ![](joint_library/beam_plain_miter_spline.png) | ![](joint_library/tile_plain_miter_spline.png) |
| **Plain miter with dovetail key**, p. 340 | | |
|  | ![](joint_library/beam_plain_miter_dovetail_key.png) | ![](joint_library/tile_plain_miter_dovetail_key.png) |
| **Plain miter with butterfly key**, p. 342 | | |
| ![](joint_library/beam_plain_miter_butterfly_key.gif) | ![](joint_library/beam_plain_miter_butterfly_key.png) | ![](joint_library/tile_plain_miter_butterfly_key.png) |
| **Plain miter with perpendicular butterfly key**, p. 344 | | |
|  | ![](joint_library/beam_plain_miter_perpendicular_butterfly_key.png) | ![](joint_library/tile_plain_miter_perpendicular_butterfly_key.png) |
| **Mitered half lap**, p. 346 | | |
| ![](joint_library/beam_mitered_half_lap.gif) | ![](joint_library/beam_mitered_half_lap.png) | ![](joint_library/tile_mitered_half_lap.png) |
| **Blind mitered half lap**, p. 348 | | |
| ![](joint_library/beam_blind_mitered_half_lap.gif) | ![](joint_library/beam_blind_mitered_half_lap.png) | ![](joint_library/tile_blind_mitered_half_lap.png) |
| **Blind mitered mortise and tenon joint**, p. 350 | | |
|  | ![](joint_library/beam_blind_mitered_mortise_tenon.png) | ![](joint_library/tile_blind_mitered_mortise_tenon.png) |
| **Mitered bridle joint**, p. 352 | | |
| ![](joint_library/beam_mitered_bridle.gif) | ![](joint_library/beam_mitered_bridle.png) | ![](joint_library/tile_mitered_bridle.png) |
| **Blind stub tenon miter**, p. 354 | | |
| ![](joint_library/beam_blind_stub_tenon_miter.gif) | ![](joint_library/beam_blind_stub_tenon_miter.png) | ![](joint_library/tile_blind_stub_tenon_miter.png) |
| **Through single dovetail miter**, p. 356 | | |
| ![](joint_library/beam_through_single_dovetail_miter.gif) | ![](joint_library/beam_through_single_dovetail_miter.png) | ![](joint_library/tile_through_single_dovetail_miter.png) |
| **Blind dovetail miter**, p. 358 | | |
|  | ![](joint_library/beam_blind_dovetail_miter.png) | ![](joint_library/tile_blind_dovetail_miter.png) |
| **Stopped miter with blind dovetail tenon**, p. 360 | | |
|  | ![](joint_library/beam_stopped_miter_blind_dovetail_tenon.png) | ![](joint_library/tile_stopped_miter_blind_dovetail_tenon.png) |
| **Bridle joint with one-sided miter**, p. 362 | | |
| ![](joint_library/beam_bridle_one_sided_miter.gif) | ![](joint_library/beam_bridle_one_sided_miter.png) | ![](joint_library/tile_bridle_one_sided_miter.png) |
| **Canvas stretcher**, p. 364 | | |
|  | ![](joint_library/beam_canvas_stretcher_corner.png) | ![](joint_library/tile_canvas_stretcher_corner.png) |
| **Double mortise and tenon miter**, p. 366 | | |
| ![](joint_library/beam_double_mortise_tenon_miter.gif) | ![](joint_library/beam_double_mortise_tenon_miter.png) | ![](joint_library/tile_double_mortise_tenon_miter.png) |
| **Double mortise and tenon with one-sided lapped miter**, p. 368 | | |
|  | ![](joint_library/beam_double_mortise_tenon_one_sided_miter.png) | ![](joint_library/tile_double_mortise_tenon_one_sided_miter.png) |
| **Haunched through mortise and tenon with two-sided lapped miter**, p. 370 | | |
|  | ![](joint_library/beam_haunched_through_tenon_two_sided_miter.png) | ![](joint_library/tile_haunched_through_tenon_two_sided_miter.png) |
| **Haunched through mortise and tenon with two-sided stopped miter**, p. 372 | | |
|  | ![](joint_library/beam_haunched_through_tenon_two_sided_stopped_miter.png) | ![](joint_library/tile_haunched_through_tenon_two_sided_stopped_miter.png) |
| **Haunched through mortise and tenon with one-sided lapped miter**, p. 374 | | |
|  | ![](joint_library/beam_haunched_through_tenon_one_sided_miter.png) | ![](joint_library/tile_haunched_through_tenon_one_sided_miter.png) |
| **Haunched through mortise and tenon with one-sided stopped lapped miter**, p. 376 | | |
|  | ![](joint_library/beam_haunched_through_tenon_one_sided_stopped_miter.png) | ![](joint_library/tile_haunched_through_tenon_one_sided_stopped_miter.png) |
| **Blind haunched mortise and tenon miter**, p. 378 | | |
|  | ![](joint_library/beam_blind_haunched_tenon_miter.png) | ![](joint_library/tile_blind_haunched_tenon_miter.png) |
| **Haunched through double mortise and tenon with one-sided lapped miter**, p. 380 | | |
|  | ![](joint_library/beam_haunched_through_double_tenon_one_sided_miter.png) | ![](joint_library/tile_haunched_through_double_tenon_one_sided_miter.png) |
| **Triple-shouldered double tenon miter**, p. 382 | | |
|  | ![](joint_library/beam_triple_shouldered_double_tenon_miter.png) | ![](joint_library/tile_triple_shouldered_double_tenon_miter.png) |
| **Housed stub tenon**, p. 384 | | |
|  | ![](joint_library/beam_housed_stub_tenon_corner.png) | ![](joint_library/tile_housed_stub_tenon_corner.png) |
| **Housed dovetail tenon**, p. 386 | | |
|  | ![](joint_library/beam_housed_dovetail_tenon_corner.png) | ![](joint_library/tile_housed_dovetail_tenon_corner.png) |
| **Blind housed dovetail tenon**, p. 388 | | |
|  | ![](joint_library/beam_blind_housed_dovetail_tenon_corner.png) | ![](joint_library/tile_blind_housed_dovetail_tenon_corner.png) |
| **Half-blind gooseneck tenon**, p. 390 | | |
|  | ![](joint_library/beam_half_blind_gooseneck_tenon_corner.png) | ![](joint_library/tile_half_blind_gooseneck_tenon_corner.png) |
| **Half-blind dovetail bridle joint**, p. 392 | | |
|  | ![](joint_library/beam_half_blind_dovetail_bridle_corner.png) | ![](joint_library/tile_half_blind_dovetail_bridle_corner.png) |
| **Through dovetail bridle joint**, p. 394 | | |
|  | ![](joint_library/beam_through_dovetail_bridle_corner.png) | ![](joint_library/tile_through_dovetail_bridle_corner.png) |
| **Half-blind triple shouldered dovetail tenon**, p. 396 | | |
|  | ![](joint_library/beam_half_blind_triple_shouldered_dovetail_corner.png) | ![](joint_library/tile_half_blind_triple_shouldered_dovetail_corner.png) |
| **Triple-shouldered stub tenon**, p. 398 | | |
| ![](joint_library/beam_triple_shouldered_stub_tenon_corner.gif) | ![](joint_library/beam_triple_shouldered_stub_tenon_corner.png) | ![](joint_library/tile_triple_shouldered_stub_tenon_corner.png) |
| **Housed dovetail joint**, p. 400 | | |
| ![](joint_library/beam_housed_dovetail_corner.gif) | ![](joint_library/beam_housed_dovetail_corner.png) | ![](joint_library/tile_housed_dovetail_corner.png) |
| **Blind fan-shaped double mortise and tenon**, p. 402 | | |
|  | ![](joint_library/beam_blind_fan_double_tenon_corner.png) | ![](joint_library/tile_blind_fan_double_tenon_corner.png) |
| **Kawai joint**, p. 404 | | |
|  | ![](joint_library/beam_kawai_corner.png) | ![](joint_library/tile_kawai_corner.png) |
| **Notched lap joint with stub tenon**, p. 406 | | |
|  | ![](joint_library/beam_notched_lap_stub_tenon_corner.png) | ![](joint_library/tile_notched_lap_stub_tenon_corner.png) |
| **Blind pegged joint**, p. 408 | | |
| ![](joint_library/beam_blind_pegged_corner.gif) | ![](joint_library/beam_blind_pegged_corner.png) | ![](joint_library/tile_blind_pegged_corner.png) |
| **Cog joint**, p. 410 | | |
| ![](joint_library/beam_cog_corner.gif) | ![](joint_library/beam_cog_corner.png) | ![](joint_library/tile_cog_corner.png) |
| **Sloped cog joint**, p. 412 | | |
| ![](joint_library/beam_sloped_cog_corner.gif) | ![](joint_library/beam_sloped_cog_corner.png) | ![](joint_library/tile_sloped_cog_corner.png) |
| **Dovetail cog joint**, p. 414 | | |
| ![](joint_library/beam_dovetail_cog_corner.gif) | ![](joint_library/beam_dovetail_cog_corner.png) | ![](joint_library/tile_dovetail_cog_corner.png) |
| **V-shaped cog joint**, p. 416 | | |
| ![](joint_library/beam_v_shaped_cog_corner.gif) | ![](joint_library/beam_v_shaped_cog_corner.png) | ![](joint_library/tile_v_shaped_cog_corner.png) |
| **Diagonally cut cog joint**, p. 418 | | |
|  | ![](joint_library/beam_diagonally_cut_cog_corner.png) | ![](joint_library/tile_diagonally_cut_cog_corner.png) |
| **Half lap joint**, p. 420 | | |
|  | ![](joint_library/beam_half_lap.png) | ![](joint_library/tile_half_lap.png) |
| **Stopped half lap joint**, p. 422 | | |
|  | ![](joint_library/beam_stopped_half_lap.png) | ![](joint_library/tile_stopped_half_lap.png) |
| **Beveled half lap joint**, p. 424 | | |
|  | ![](joint_library/beam_beveled_half_lap.png) | ![](joint_library/tile_beveled_half_lap.png) |
| **Stopped beveled half lap joint**, p. 426 | | |
|  | ![](joint_library/beam_stopped_beveled_half_lap.png) | ![](joint_library/tile_stopped_beveled_half_lap.png) |
| **Cogged lap joint**, p. 428 | | |
|  | ![](joint_library/beam_cogged_lap.png) | ![](joint_library/tile_cogged_lap.png) |
| **Cogged lap joint with stub tenon**, p. 430 | | |
|  | ![](joint_library/beam_cogged_lap_stub_tenon.png) | ![](joint_library/tile_cogged_lap_stub_tenon.png) |
| **Sloped cogged lap joint**, p. 432 | | |
|  | ![](joint_library/beam_sloped_cogged_lap.png) | ![](joint_library/tile_sloped_cogged_lap.png) |
| **Double cogged lap joint**, p. 434 | | |
|  | ![](joint_library/beam_double_cogged_lap.png) | ![](joint_library/tile_double_cogged_lap.png) |
| **Stopped mortise and tenon joint**, p. 436 | | |
|  | ![](joint_library/beam_stopped_mortise_tenon.png) | ![](joint_library/tile_stopped_mortise_tenon.png) |
| **Stopped dovetailed mortise and tenon joint**, p. 438 | | |
|  | ![](joint_library/beam_stopped_dovetailed_mortise_tenon.png) | ![](joint_library/tile_stopped_dovetailed_mortise_tenon.png) |
| **Dovetail half lap joint**, p. 440 | | |
|  | ![](joint_library/beam_dovetail_half_lap.png) | ![](joint_library/tile_dovetail_half_lap.png) |
| **Stopped dovetail half lap joint**, p. 442 | | |
|  | ![](joint_library/beam_stopped_dovetail_half_lap.png) | ![](joint_library/tile_stopped_dovetail_half_lap.png) |
| **One-sided dovetail half lap joint**, p. 444 | | |
|  | ![](joint_library/beam_one_sided_dovetail_half_lap.png) | ![](joint_library/tile_one_sided_dovetail_half_lap.png) |
| **Stopped one-sided dovetail half lap joint**, p. 446 | | |
|  | ![](joint_library/beam_stopped_one_sided_dovetail_half_lap.png) | ![](joint_library/tile_stopped_one_sided_dovetail_half_lap.png) |
| **Housed dovetail half lap joint**, p. 448 | | |
|  | ![](joint_library/beam_housed_dovetail_half_lap.png) | ![](joint_library/tile_housed_dovetail_half_lap.png) |
| **Stopped housed dovetail half lap joint**, p. 450 | | |
|  | ![](joint_library/beam_stopped_housed_dovetail_half_lap.png) | ![](joint_library/tile_stopped_housed_dovetail_half_lap.png) |
| **Stepped lap joint**, p. 452 | | |
|  | ![](joint_library/beam_stepped_lap.png) | ![](joint_library/tile_stepped_lap.png) |
| **Housed mortise and tenon with shifted shoulders**, p. 454 | | |
|  | ![](joint_library/beam_housed_mortise_tenon_shifted_shoulders.png) | ![](joint_library/tile_housed_mortise_tenon_shifted_shoulders.png) |
| **Sloped stopped half lap**, p. 456 | | |
|  | ![](joint_library/beam_sloped_stopped_half_lap.png) | ![](joint_library/tile_sloped_stopped_half_lap.png) |
| **Mortise and tenon with sloped shoulder**, p. 458 | | |
|  | ![](joint_library/beam_mortise_tenon_sloped_shoulder.png) | ![](joint_library/tile_mortise_tenon_sloped_shoulder.png) |
| **Blind housed mortise and tenon with sloped shoulder**, p. 460 | | |
|  | ![](joint_library/beam_blind_housed_mortise_tenon_sloped_shoulder.png) | ![](joint_library/tile_blind_housed_mortise_tenon_sloped_shoulder.png) |
| **Undersquinted stopped half lap**, p. 462 | | |
|  | ![](joint_library/beam_undersquinted_stopped_half_lap.png) | ![](joint_library/tile_undersquinted_stopped_half_lap.png) |
| **Mortise and tenon with beveled shoulders**, p. 464 | | |
|  | ![](joint_library/beam_housed_mortise_tenon_beveled_shoulders.png) | ![](joint_library/tile_housed_mortise_tenon_beveled_shoulders.png) |
| **Housed stop-mitered mortise and tenon**, p. 466 | | |
|  | ![](joint_library/beam_housed_stop_mitered_mortise_tenon.png) | ![](joint_library/tile_housed_stop_mitered_mortise_tenon.png) |
| **Mortise and tenon with fan-shaped housing**, p. 468 | | |
|  | ![](joint_library/beam_mortise_tenon_fan_shaped_housing.png) | ![](joint_library/tile_mortise_tenon_fan_shaped_housing.png) |
| **Blind pegged joint**, p. 470 | | |
|  | ![](joint_library/beam_blind_pegged.png) | ![](joint_library/tile_blind_pegged.png) |
| **Notch joint**, p. 472 | | |
|  | ![](joint_library/beam_notch.png) | ![](joint_library/tile_notch.png) |
| **Double notch joint**, p. 474 | | |
|  | ![](joint_library/beam_double_notch.png) | ![](joint_library/tile_double_notch.png) |
| **Cog joint**, p. 476 | | |
|  | ![](joint_library/beam_cog.png) | ![](joint_library/tile_cog.png) |
| **Stepped cog joint**, p. 478 | | |
|  | ![](joint_library/beam_stepped_cog.png) | ![](joint_library/tile_stepped_cog.png) |
| **Double cog joint**, p. 480 | | |
|  | ![](joint_library/beam_double_cog.png) | ![](joint_library/tile_double_cog.png) |
| **X-shaped cog joint**, p. 482 | | |
|  | ![](joint_library/beam_x_shaped_cog.png) | ![](joint_library/tile_x_shaped_cog.png) |
| **V-shaped cog joint**, p. 484 | | |
|  | ![](joint_library/beam_v_shaped_cog.png) | ![](joint_library/tile_v_shaped_cog.png) |
| **Dovetail cog joint**, p. 486 | | |
|  | ![](joint_library/beam_dovetail_cog.png) | ![](joint_library/tile_dovetail_cog.png) |
| **Half-dovetail cog joint**, p. 488 | | |
|  | ![](joint_library/beam_half_dovetail_cog.png) | ![](joint_library/tile_half_dovetail_cog.png) |
| **Sloped cog joint**, p. 490 | | |
|  | ![](joint_library/beam_sloped_cog.png) | ![](joint_library/tile_sloped_cog.png) |
| **Through mortise and tenon**, p. 492 | | |
|  | ![](joint_library/beam_through_mortise_tenon.png) | ![](joint_library/tile_through_mortise_tenon.png) |
| **Through mortise and tenon with housing**, p. 494 | | |
|  | ![](joint_library/beam_through_mortise_tenon_housing.png) | ![](joint_library/tile_through_mortise_tenon_housing.png) |
| **Blind mortise and tenon**, p. 496 | | |
|  | ![](joint_library/beam_blind_mortise_tenon.png) | ![](joint_library/tile_blind_mortise_tenon.png) |
| **Tenon and chase mortise**, p. 498 | | |
|  | ![](joint_library/beam_tenon_chase_mortise.png) | ![](joint_library/tile_tenon_chase_mortise.png) |
| **Mortise and tenon with beveled shoulders**, p. 500 | | |
|  | ![](joint_library/beam_mortise_tenon_beveled_shoulders.png) | ![](joint_library/tile_mortise_tenon_beveled_shoulders.png) |
| **Mortise and tenon with mitered shoulders**, p. 502 | | |
|  | ![](joint_library/beam_mortise_tenon_mitered_shoulders.png) | ![](joint_library/tile_mortise_tenon_mitered_shoulders.png) |
| **Pegged through mortise and tenon**, p. 504 | | |
|  | ![](joint_library/beam_pegged_through_mortise_tenon.png) | ![](joint_library/tile_pegged_through_mortise_tenon.png) |
| **Pegged blind mortise and tenon**, p. 506 | | |
|  | ![](joint_library/beam_pegged_blind_mortise_tenon.png) | ![](joint_library/tile_pegged_blind_mortise_tenon.png) |
| **Triple-shouldered through mortise and tenon**, p. 508 | | |
|  | ![](joint_library/beam_triple_shouldered_through_mortise_tenon.png) | ![](joint_library/tile_triple_shouldered_through_mortise_tenon.png) |
| **Fully shouldered through mortise and tenon joint**, p. 510 | | |
|  | ![](joint_library/beam_fully_shouldered_through_mortise_tenon.png) | ![](joint_library/tile_fully_shouldered_through_mortise_tenon.png) |
| **Triple through mortise and tenon**, p. 512 | | |
|  | ![](joint_library/beam_triple_through_mortise_tenon.png) | ![](joint_library/tile_triple_through_mortise_tenon.png) |
| **Quadruple through mortise and tenon**, p. 514 | | |
|  | ![](joint_library/beam_quadruple_through_mortise_tenon.png) | ![](joint_library/tile_quadruple_through_mortise_tenon.png) |
| **Haunched through mortise and tenon**, p. 516 | | |
|  | ![](joint_library/beam_haunched_through_mortise_tenon.png) | ![](joint_library/tile_haunched_through_mortise_tenon.png) |
| **Blind haunched mortise and tenon**, p. 518 | | |
|  | ![](joint_library/beam_blind_haunched_mortise_tenon.png) | ![](joint_library/tile_blind_haunched_mortise_tenon.png) |
| **Through mortise and tenon with sloped haunch**, p. 520 | | |
|  | ![](joint_library/beam_through_mortise_tenon_sloped_haunch.png) | ![](joint_library/tile_through_mortise_tenon_sloped_haunch.png) |
| **Blind housed haunched through mortise and tenon**, p. 522 | | |
|  | ![](joint_library/beam_blind_housed_haunched_through_mortise_tenon.png) | ![](joint_library/tile_blind_housed_haunched_through_mortise_tenon.png) |
| **Haunched collared through mortise and tenon**, p. 524 | | |
|  | ![](joint_library/beam_haunched_collared_through_mortise_tenon.png) | ![](joint_library/tile_haunched_collared_through_mortise_tenon.png) |
| **Half-dovetail tenon with folding wedges**, p. 526 | | |
|  | ![](joint_library/beam_half_dovetail_tenon_folding_wedges.png) | ![](joint_library/tile_half_dovetail_tenon_folding_wedges.png) |
| **Foxtail-wedged mortise and tenon**, p. 528 | | |
|  | ![](joint_library/beam_foxtail_wedged_mortise_tenon.png) | ![](joint_library/tile_foxtail_wedged_mortise_tenon.png) |
| **Wedged through mortise and tenon**, p. 530 | | |
|  | ![](joint_library/beam_wedged_through_mortise_tenon.png) | ![](joint_library/tile_wedged_through_mortise_tenon.png) |
| **Vertically wedged tusk tenon**, p. 532 | | |
|  | ![](joint_library/beam_vertically_wedged_tusk_tenon.png) | ![](joint_library/tile_vertically_wedged_tusk_tenon.png) |
| **Horizontally wedged tusk tenon**, p. 534 | | |
|  | ![](joint_library/beam_horizontally_wedged_tusk_tenon.png) | ![](joint_library/tile_horizontally_wedged_tusk_tenon.png) |
| **Horizontally wedged double tusk tenon**, p. 536 | | |
|  | ![](joint_library/beam_horizontally_wedged_double_tusk_tenon.png) | ![](joint_library/tile_horizontally_wedged_double_tusk_tenon.png) |
| **Right-angle joint with dovetailed spline tenon and tapered key**, p. 538 | | |
|  | ![](joint_library/beam_right_angle_spline_tenon_tapered_key.png) | ![](joint_library/tile_right_angle_spline_tenon_tapered_key.png) |
| **Housed right-angle joint with spline tenon, tapered keys, and peg**, p. 540 | | |
|  | ![](joint_library/beam_housed_right_angle_spline_tenon_keys_peg.png) | ![](joint_library/tile_housed_right_angle_spline_tenon_keys_peg.png) |
| **Double through mortise and tenon**, p. 542 | | |
|  | ![](joint_library/beam_double_through_mortise_tenon.png) | ![](joint_library/tile_double_through_mortise_tenon.png) |
| **Housed double through mortise and tenon**, p. 544 | | |
|  | ![](joint_library/beam_housed_double_through_mortise_tenon.png) | ![](joint_library/tile_housed_double_through_mortise_tenon.png) |
| **Double mortise and tenon with mitered shoulders**, p. 546 | | |
|  | ![](joint_library/beam_double_mortise_tenon_mitered_shoulders.png) | ![](joint_library/tile_double_mortise_tenon_mitered_shoulders.png) |
| **Wedged half-dovetailed T-bridle joint**, p. 548 | | |
|  | ![](joint_library/beam_wedged_half_dovetailed_t_bridle.png) | ![](joint_library/tile_wedged_half_dovetailed_t_bridle.png) |
| **T-bridle joint**, p. 550 | | |
|  | ![](joint_library/beam_t_bridle.png) | ![](joint_library/tile_t_bridle.png) |
| **T-bridle joint with blind mortise and tenon**, p. 552 | | |
|  | ![](joint_library/beam_t_bridle_blind_mortise_tenon.png) | ![](joint_library/tile_t_bridle_blind_mortise_tenon.png) |
| **Double lap joint with stub tenon**, p. 554 | | |
|  | ![](joint_library/beam_double_lap_stub_tenon.png) | ![](joint_library/tile_double_lap_stub_tenon.png) |
| **Notched lap joint with tenon**, p. 556 | | |
|  | ![](joint_library/beam_notched_lap_tenon.png) | ![](joint_library/tile_notched_lap_tenon.png) |
| **Notched lap joint with stub tenon**, p. 558 | | |
|  | ![](joint_library/beam_notched_lap_stub_tenon.png) | ![](joint_library/tile_notched_lap_stub_tenon.png) |
| **Lap joint with tenon**, p. 560 | | |
|  | ![](joint_library/beam_lap_tenon.png) | ![](joint_library/tile_lap_tenon.png) |
| **Lap joint with stub tenon**, p. 562 | | |
|  | ![](joint_library/beam_lap_stub_tenon.png) | ![](joint_library/tile_lap_stub_tenon.png) |
| **Dovetail T-bridle joint**, p. 564 | | |
|  | ![](joint_library/beam_dovetail_t_bridle.png) | ![](joint_library/tile_dovetail_t_bridle.png) |
| **Diagonally cut T-bridle joint**, p. 566 | | |
|  | ![](joint_library/beam_diagonally_cut_t_bridle.png) | ![](joint_library/tile_diagonally_cut_t_bridle.png) |
| **Rising dovetail joint**, p. 568 | | |
|  | ![](joint_library/beam_rising_dovetail.png) | ![](joint_library/tile_rising_dovetail.png) |
| **Wedged dovetail half lap joint**, p. 570 | | |
|  | ![](joint_library/beam_wedged_dovetail_half_lap.png) | ![](joint_library/tile_wedged_dovetail_half_lap.png) |
| **Housed dovetail joint**, p. 572 | | |
|  | ![](joint_library/beam_housed_dovetail.png) | ![](joint_library/tile_housed_dovetail.png) |
| **X-shaped stub tenon joint**, p. 574 | | |
|  | ![](joint_library/beam_x_shaped_stub_tenon.png) | ![](joint_library/tile_x_shaped_stub_tenon.png) |
| **Fully shouldered blind mortise and tenon**, p. 576 | | |
|  | ![](joint_library/beam_fully_shouldered_blind_mortise_tenon.png) | ![](joint_library/tile_fully_shouldered_blind_mortise_tenon.png) |
| **Post with stub tenon and open slot mortise and tenon**, p. 578 | | |
|  | ![](joint_library/beam_post_stub_tenon_slot_mortise_splice.png) | ![](joint_library/tile_post_stub_tenon_slot_mortise_splice.png) |
| **Post with stub tenon and lapped mortise and tenon**, p. 580 | | |
|  | ![](joint_library/beam_post_stub_tenon_lapped_splice.png) | ![](joint_library/tile_post_stub_tenon_lapped_splice.png) |
| **Post with through tenon and mortised stop-splayed cogged scarf joint with stub tenons**, p. 582 | | |
|  | ![](joint_library/beam_post_through_tenon_stop_splayed_scarf_stub_tenons.png) | ![](joint_library/tile_post_through_tenon_stop_splayed_scarf_stub_tenons.png) |
| **Notched oblique thrust joint**, p. 584 | | |
| ![](joint_library/beam_notched_oblique_thrust.gif) | ![](joint_library/beam_notched_oblique_thrust.png) | ![](joint_library/tile_notched_oblique_thrust.png) |
| **Notched oblique thrust joint with right-angled end**, p. 586 | | |
| ![](joint_library/beam_notched_oblique_thrust_right_angled_end.gif) | ![](joint_library/beam_notched_oblique_thrust_right_angled_end.png) | ![](joint_library/tile_notched_oblique_thrust_right_angled_end.png) |
| **Notched oblique thrust joint with visible end**, p. 588 | | |
| ![](joint_library/beam_notched_oblique_thrust_visible_end.gif) | ![](joint_library/beam_notched_oblique_thrust_visible_end.png) | ![](joint_library/tile_notched_oblique_thrust_visible_end.png) |
| **Oblique thrust joint with notched heel**, p. 590 | | |
| ![](joint_library/beam_oblique_thrust_notched_heel.gif) | ![](joint_library/beam_oblique_thrust_notched_heel.png) | ![](joint_library/tile_oblique_thrust_notched_heel.png) |
| **Double notched oblique thrust joint**, p. 592 | | |
| ![](joint_library/beam_double_notched_oblique_thrust.gif) | ![](joint_library/beam_double_notched_oblique_thrust.png) | ![](joint_library/tile_double_notched_oblique_thrust.png) |
| **Oblique thrust joint with sloped tenon and notched heel**, p. 594 | | |
| ![](joint_library/beam_oblique_thrust_sloped_tenon_notched_heel.gif) | ![](joint_library/beam_oblique_thrust_sloped_tenon_notched_heel.png) | ![](joint_library/tile_oblique_thrust_sloped_tenon_notched_heel.png) |
| **Oblique thrust joint with mortise and tenon**, p. 596 | | |
| ![](joint_library/beam_oblique_thrust_mortise_tenon.gif) | ![](joint_library/beam_oblique_thrust_mortise_tenon.png) | ![](joint_library/tile_oblique_thrust_mortise_tenon.png) |
| **Oblique thrust joint with sloped mortise and tenon**, p. 598 | | |
| ![](joint_library/beam_oblique_thrust_sloped_mortise_tenon.gif) | ![](joint_library/beam_oblique_thrust_sloped_mortise_tenon.png) | ![](joint_library/tile_oblique_thrust_sloped_mortise_tenon.png) |
| **Notched oblique thrust joint with mortise and tenon**, p. 600 | | |
| ![](joint_library/beam_notched_oblique_thrust_mortise_tenon.gif) | ![](joint_library/beam_notched_oblique_thrust_mortise_tenon.png) | ![](joint_library/tile_notched_oblique_thrust_mortise_tenon.png) |
| **Notched oblique thrust joint with pegged mortise and tenon**, p. 602 | | |
| ![](joint_library/beam_notched_oblique_thrust_pegged_mortise_tenon.gif) | ![](joint_library/beam_notched_oblique_thrust_pegged_mortise_tenon.png) | ![](joint_library/tile_notched_oblique_thrust_pegged_mortise_tenon.png) |
| **Slotted notched oblique thrust joint**, p. 604 | | |
| ![](joint_library/beam_slotted_notched_oblique_thrust.gif) | ![](joint_library/beam_slotted_notched_oblique_thrust.png) | ![](joint_library/tile_slotted_notched_oblique_thrust.png) |
| **Slotted notched oblique thrust joint with visible end**, p. 606 | | |
| ![](joint_library/beam_slotted_notched_oblique_thrust_visible_end.gif) | ![](joint_library/beam_slotted_notched_oblique_thrust_visible_end.png) | ![](joint_library/tile_slotted_notched_oblique_thrust_visible_end.png) |
| **Pegged angled half lap joint**, p. 608 | | |
| ![](joint_library/beam_pegged_angled_half_lap.gif) | ![](joint_library/beam_pegged_angled_half_lap.png) | ![](joint_library/tile_pegged_angled_half_lap.png) |
| **Pegged angled stopped half lap joint**, p. 610 | | |
| ![](joint_library/beam_pegged_angled_stopped_half_lap.gif) | ![](joint_library/beam_pegged_angled_stopped_half_lap.png) | ![](joint_library/tile_pegged_angled_stopped_half_lap.png) |
| **Pegged angled lapped mortise and tenon joint**, p. 612 | | |
| ![](joint_library/beam_pegged_angled_lapped_mortise_tenon.gif) | ![](joint_library/beam_pegged_angled_lapped_mortise_tenon.png) | ![](joint_library/tile_pegged_angled_lapped_mortise_tenon.png) |
| **Pegged shouldered angled half lap joint**, p. 614 | | |
| ![](joint_library/beam_pegged_shouldered_angled_half_lap.gif) | ![](joint_library/beam_pegged_shouldered_angled_half_lap.png) | ![](joint_library/tile_pegged_shouldered_angled_half_lap.png) |
| **Pegged angled half-dovetail lap joint with sloped shoulder**, p. 616 | | |
| ![](joint_library/beam_pegged_angled_half_dovetail_lap_sloped_shoulder.gif) | ![](joint_library/beam_pegged_angled_half_dovetail_lap_sloped_shoulder.png) | ![](joint_library/tile_pegged_angled_half_dovetail_lap_sloped_shoulder.png) |
| **Pegged angled half-dovetail half lap joint**, p. 618 | | |
| ![](joint_library/beam_pegged_angled_half_dovetail_half_lap.gif) | ![](joint_library/beam_pegged_angled_half_dovetail_half_lap.png) | ![](joint_library/tile_pegged_angled_half_dovetail_half_lap.png) |
| **Pegged stopped angled half-dovetail half lap joint**, p. 620 | | |
| ![](joint_library/beam_pegged_stopped_angled_half_dovetail_half_lap.gif) | ![](joint_library/beam_pegged_stopped_angled_half_dovetail_half_lap.png) | ![](joint_library/tile_pegged_stopped_angled_half_dovetail_half_lap.png) |
| **Pegged angled stopped dovetail half lap joint**, p. 622 | | |
| ![](joint_library/beam_pegged_angled_stopped_dovetail_half_lap.gif) | ![](joint_library/beam_pegged_angled_stopped_dovetail_half_lap.png) | ![](joint_library/tile_pegged_angled_stopped_dovetail_half_lap.png) |
| **Pegged rafter foot**, p. 624 | | |
| ![](joint_library/beam_pegged_rafter_foot.gif) | ![](joint_library/beam_pegged_rafter_foot.png) | ![](joint_library/tile_pegged_rafter_foot.png) |
| **Pegged mitered rafter foot**, p. 626 | | |
| ![](joint_library/beam_pegged_mitered_rafter_foot.gif) | ![](joint_library/beam_pegged_mitered_rafter_foot.png) | ![](joint_library/tile_pegged_mitered_rafter_foot.png) |
| **Housed rafter foot**, p. 628 | | |
| ![](joint_library/beam_housed_rafter_foot.gif) | ![](joint_library/beam_housed_rafter_foot.png) | ![](joint_library/tile_housed_rafter_foot.png) |
| **V-grooved rafter foot**, p. 630 | | |
| ![](joint_library/beam_v_grooved_rafter_foot.gif) | ![](joint_library/beam_v_grooved_rafter_foot.png) | ![](joint_library/tile_v_grooved_rafter_foot.png) |
| **Birdsmouth rafter foot**, p. 632 | | |
| ![](joint_library/beam_birdsmouth_rafter_foot.gif) | ![](joint_library/beam_birdsmouth_rafter_foot.png) | ![](joint_library/tile_birdsmouth_rafter_foot.png) |
| **Rafter foot with tenon and undersquinted shoulders**, p. 634 | | |
| ![](joint_library/beam_rafter_foot_tenon_undersquinted_shoulders.gif) | ![](joint_library/beam_rafter_foot_tenon_undersquinted_shoulders.png) | ![](joint_library/tile_rafter_foot_tenon_undersquinted_shoulders.png) |
| **Birdsmouth rafter joint**, p. 636 | | |
| ![](joint_library/beam_birdsmouth_rafter_joint.gif) | ![](joint_library/beam_birdsmouth_rafter_joint.png) | ![](joint_library/tile_birdsmouth_rafter_joint.png) |
| **Notched rafter joint**, p. 638 | | |
| ![](joint_library/beam_notched_rafter_joint.gif) | ![](joint_library/beam_notched_rafter_joint.png) | ![](joint_library/tile_notched_rafter_joint.png) |
| **V-grooved rafter joint**, p. 640 | | |
| ![](joint_library/beam_v_grooved_rafter_joint.gif) | ![](joint_library/beam_v_grooved_rafter_joint.png) | ![](joint_library/tile_v_grooved_rafter_joint.png) |
| **Step lap rafter joint**, p. 642 | | |
| ![](joint_library/beam_step_lap_rafter_joint.gif) | ![](joint_library/beam_step_lap_rafter_joint.png) | ![](joint_library/tile_step_lap_rafter_joint.png) |
| **Stopped birdsmouth rafter foot**, p. 644 | | |
| ![](joint_library/beam_stopped_birdsmouth_rafter_foot.gif) | ![](joint_library/beam_stopped_birdsmouth_rafter_foot.png) | ![](joint_library/tile_stopped_birdsmouth_rafter_foot.png) |
| **Cross lap joint**, p. 646 | | |
|  | ![](joint_library/beam_cross_lap.png) | ![](joint_library/tile_cross_lap.png) |
| **Pegged cross lap joint**, p. 648 | | |
|  | ![](joint_library/beam_pegged_cross_lap.png) | ![](joint_library/tile_pegged_cross_lap.png) |
| **Four-part cogged cross lap joint**, p. 650 | | |
|  | ![](joint_library/beam_four_part_cogged_cross_lap.png) | ![](joint_library/tile_four_part_cogged_cross_lap.png) |
| **V-mitered cross lap joint**, p. 652 | | |
|  | ![](joint_library/beam_v_mitered_cross_lap.png) | ![](joint_library/tile_v_mitered_cross_lap.png) |
| **Cross lap joint with mitered abutments**, p. 654 | | |
|  | ![](joint_library/beam_mitered_abutments_cross_lap.png) | ![](joint_library/tile_mitered_abutments_cross_lap.png) |
| **Stepped cross lap joint**, p. 656 | | |
|  | ![](joint_library/beam_stepped_cross_lap.png) | ![](joint_library/tile_stepped_cross_lap.png) |
| **Cogged cross lap joint**, p. 658 | | |
|  | ![](joint_library/beam_cogged_cross_lap.png) | ![](joint_library/tile_cogged_cross_lap.png) |
| **Cogged cross lap joint with stub tenons**, p. 660 | | |
|  | ![](joint_library/beam_cogged_cross_lap_stub_tenons.png) | ![](joint_library/tile_cogged_cross_lap_stub_tenons.png) |
| **Cross lap joint with stub tenons**, p. 662 | | |
|  | ![](joint_library/beam_cross_lap_stub_tenons.png) | ![](joint_library/tile_cross_lap_stub_tenons.png) |
| **Offset cross lap joint**, p. 664 | | |
|  | ![](joint_library/beam_offset_cross_lap.png) | ![](joint_library/tile_offset_cross_lap.png) |
| **Double offset cross lap joint**, p. 666 | | |
|  | ![](joint_library/beam_double_offset_cross_lap.png) | ![](joint_library/tile_double_offset_cross_lap.png) |
| **Triple offset cross lap joint**, p. 668 | | |
|  | ![](joint_library/beam_triple_offset_cross_lap.png) | ![](joint_library/tile_triple_offset_cross_lap.png) |
| **Dovetailed cross lap joint**, p. 670 | | |
|  | ![](joint_library/beam_dovetailed_cross_lap.png) | ![](joint_library/tile_dovetailed_cross_lap.png) |
| **Oblique notched cross lap joint**, p. 672 | | |
|  | ![](joint_library/beam_oblique_notched_cross_lap.png) | ![](joint_library/tile_oblique_notched_cross_lap.png) |
| **Oblique notched heel cross lap joint**, p. 674 | | |
|  | ![](joint_library/beam_oblique_notched_heel_cross_lap.png) | ![](joint_library/tile_oblique_notched_heel_cross_lap.png) |
| **Double notched oblique cross lap joint**, p. 676 | | |
|  | ![](joint_library/beam_double_notched_oblique_cross_lap.png) | ![](joint_library/tile_double_notched_oblique_cross_lap.png) |
| **Three-way cross lap joint**, p. 678 | | |
|  | ![](joint_library/beam_three_way_cross_lap.png) | ![](joint_library/tile_three_way_cross_lap.png) |
| **Joint with blind peg**, p. 680 | | |
|  | ![](joint_library/beam_blind_pegged_cross.png) | ![](joint_library/tile_blind_pegged_cross.png) |
| **Notch joint**, p. 682 | | |
|  | ![](joint_library/beam_notch_cross.png) | ![](joint_library/tile_notch_cross.png) |
| **Double notch joint**, p. 684 | | |
|  | ![](joint_library/beam_double_notch_cross.png) | ![](joint_library/tile_double_notch_cross.png) |
| **Double notch joint with blind peg**, p. 686 | | |
|  | ![](joint_library/beam_double_notch_blind_peg_cross.png) | ![](joint_library/tile_double_notch_blind_peg_cross.png) |
| **Double notch joint with stub tenons**, p. 688 | | |
|  | ![](joint_library/beam_double_notch_stub_tenons_cross.png) | ![](joint_library/tile_double_notch_stub_tenons_cross.png) |
| **Oblique double notch joint**, p. 690 | | |
|  | ![](joint_library/beam_oblique_double_notch_cross.png) | ![](joint_library/tile_oblique_double_notch_cross.png) |
| **Cog joint**, p. 692 | | |
|  | ![](joint_library/beam_cog_cross.png) | ![](joint_library/tile_cog_cross.png) |
| **Double cog joint**, p. 694 | | |
|  | ![](joint_library/beam_double_cog_cross.png) | ![](joint_library/tile_double_cog_cross.png) |
| **Sloped cog joint**, p. 696 | | |
|  | ![](joint_library/beam_sloped_cog_cross.png) | ![](joint_library/tile_sloped_cog_cross.png) |
| **Sloped double cog joint**, p. 698 | | |
|  | ![](joint_library/beam_sloped_double_cog_cross.png) | ![](joint_library/tile_sloped_double_cog_cross.png) |
| **V-shaped cog joint**, p. 700 | | |
|  | ![](joint_library/beam_v_shaped_cog_cross.png) | ![](joint_library/tile_v_shaped_cog_cross.png) |
| **X-shaped cog joint**, p. 702 | | |
|  | ![](joint_library/beam_x_shaped_cog_cross.png) | ![](joint_library/tile_x_shaped_cog_cross.png) |
| **Dovetail cog joint**, p. 704 | | |
|  | ![](joint_library/beam_dovetail_cog_cross.png) | ![](joint_library/tile_dovetail_cog_cross.png) |
| **Half-dovetail cog joint**, p. 706 | | |
|  | ![](joint_library/beam_half_dovetail_cog_cross.png) | ![](joint_library/tile_half_dovetail_cog_cross.png) |
| **Post with stub tenon and mortised double cogged joint**, p. 708 | | |
|  | ![](joint_library/beam_post_stub_tenon_double_cogged.png) | ![](joint_library/tile_post_stub_tenon_double_cogged.png) |
| **T-bridle joint with stub tenons and mortised beam**, p. 710 | | |
|  | ![](joint_library/beam_t_bridle_stub_tenons_mortised_beam.png) | ![](joint_library/tile_t_bridle_stub_tenons_mortised_beam.png) |
| **Post with through tenon and mortised crossed joint**, p. 712 | | |
|  | ![](joint_library/beam_post_through_tenon_crossed.png) | ![](joint_library/tile_post_through_tenon_crossed.png) |
| **Stacked tenon and mortised cross lap joint**, p. 714 | | |
|  | ![](joint_library/beam_stacked_tenon_mortised_cross_lap.png) | ![](joint_library/tile_stacked_tenon_mortised_cross_lap.png) |
| **Castle joint**, p. 716 | | |
|  | ![](joint_library/beam_castle_joint.png) | ![](joint_library/tile_castle_joint.png) |
| **Notched corner-post joint with beveled-shoulder tenons**, p. 718 | | |
|  | ![](joint_library/beam_notched_corner_post_beveled_tenons.png) | ![](joint_library/tile_notched_corner_post_beveled_tenons.png) |
| **Corner-post joint with open slot mortise and tenon**, p. 720 | | |
|  | ![](joint_library/beam_corner_post_open_slot_mortise_tenon.png) | ![](joint_library/tile_corner_post_open_slot_mortise_tenon.png) |
| **Corner-post joint with fan-shaped stub tenon and blind dado and rabbet joint**, p. 722 | | |
|  | ![](joint_library/beam_corner_post_fan_stub_tenon_blind_dado.png) | ![](joint_library/tile_corner_post_fan_stub_tenon_blind_dado.png) |
| **Housed and stopped dovetail lap and fully shouldered stub tenon**, p. 724 | | |
|  | ![](joint_library/beam_housed_stopped_dovetail_lap_post_stub_tenon.png) | ![](joint_library/tile_housed_stopped_dovetail_lap_post_stub_tenon.png) |
| **Lapped and haunched mortise and tenon joint**, p. 726 | | |
|  | ![](joint_library/beam_lapped_haunched_mortise_tenon_post.png) | ![](joint_library/tile_lapped_haunched_mortise_tenon_post.png) |
| **Mitered and haunched mortise and tenon joint**, p. 728 | | |
|  | ![](joint_library/beam_mitered_haunched_mortise_tenon_post.png) | ![](joint_library/tile_mitered_haunched_mortise_tenon_post.png) |
| **Chidori puzzle joint**, p. 730 | | |
| ![](joint_library/beam_chidori_puzzle.gif) | ![](joint_library/beam_chidori_puzzle.png) | ![](joint_library/tile_chidori_puzzle.png) |
| **Cross puzzle**, p. 732 | | |
| ![](joint_library/beam_cross_puzzle.gif) | ![](joint_library/beam_cross_puzzle.png) | ![](joint_library/tile_cross_puzzle.png) |
| **Diagonal cross puzzle**, p. 734 | | |
|  | ![](joint_library/beam_diagonal_cross_puzzle.png) | ![](joint_library/tile_diagonal_cross_puzzle.png) |
| **Wedged two-way cogged lap joint**, p. 736 | | |
|  | ![](joint_library/beam_wedged_two_way_cogged_lap.png) | ![](joint_library/tile_wedged_two_way_cogged_lap.png) |
| **Pegged two-way splayed scarf joint**, p. 738 | | |
|  | ![](joint_library/beam_pegged_two_way_splayed_scarf.png) | ![](joint_library/tile_pegged_two_way_splayed_scarf.png) |
| **Pegged two-way gooseneck joint**, p. 740 | | |
|  | ![](joint_library/beam_pegged_two_way_gooseneck.png) | ![](joint_library/tile_pegged_two_way_gooseneck.png) |
| **Wedged two-way oblique cogged joint**, p. 742 | | |
|  | ![](joint_library/beam_wedged_two_way_oblique_cogged.png) | ![](joint_library/tile_wedged_two_way_oblique_cogged.png) |
| **Wedged four-way splayed cogged joint**, p. 744 | | |
| ![](joint_library/beam_wedged_four_way_splayed_cogged.gif) | ![](joint_library/beam_wedged_four_way_splayed_cogged.png) | ![](joint_library/tile_wedged_four_way_splayed_cogged.png) |
| **Two-way rod tenon joint with tapered keys**, p. 746 | | |
|  | ![](joint_library/beam_two_way_rod_tenon_tapered_keys.png) | ![](joint_library/tile_two_way_rod_tenon_tapered_keys.png) |
| **Three-way rod tenon joint with tapered keys and wedged haunched tenon**, p. 748 | | |
|  | ![](joint_library/beam_three_way_rod_tenon_keys_wedged_haunch.png) | ![](joint_library/tile_three_way_rod_tenon_keys_wedged_haunch.png) |
| **Four-way rod tenon joint with tapered keys and pegs**, p. 750 | | |
|  | ![](joint_library/beam_four_way_rod_tenon_keys_pegs.png) | ![](joint_library/tile_four_way_rod_tenon_keys_pegs.png) |
| **Two-way blind housed rod tenon joint with tapered keys**, p. 752 | | |
|  | ![](joint_library/beam_two_way_blind_housed_rod_tenon.png) | ![](joint_library/tile_two_way_blind_housed_rod_tenon.png) |
| **Two-way collared rod tenon joint with tapered keys**, p. 754 | | |
|  | ![](joint_library/beam_two_way_collared_rod_tenon.png) | ![](joint_library/tile_two_way_collared_rod_tenon.png) |
| **Two-way rabbeted rod tenon joint with tapered keys**, p. 756 | | |
|  | ![](joint_library/beam_two_way_rabbeted_rod_tenon.png) | ![](joint_library/tile_two_way_rabbeted_rod_tenon.png) |
| **Two-way partially housed rod tenon joint with tapered keys**, p. 758 | | |
|  | ![](joint_library/beam_two_way_partially_housed_rod_tenon.png) | ![](joint_library/tile_two_way_partially_housed_rod_tenon.png) |
| **Pegged two-way tenon joint**, p. 760 | | |
|  | ![](joint_library/beam_pegged_two_way_tenon.png) | ![](joint_library/tile_pegged_two_way_tenon.png) |
| **Two-way splined rod joint with tapered keys and a peg**, p. 762 | | |
|  | ![](joint_library/beam_two_way_splined_rod_keys_peg.png) | ![](joint_library/tile_two_way_splined_rod_keys_peg.png) |
| **Pegged two-way lapped rod tenon joint with housing**, p. 764 | | |
|  | ![](joint_library/beam_pegged_two_way_lapped_rod_tenon_housing.png) | ![](joint_library/tile_pegged_two_way_lapped_rod_tenon_housing.png) |
| **Shiplap joint**, p. 768 | | |
| ![](joint_library/beam_shiplap_edge.gif) | ![](joint_library/beam_shiplap_edge.png) | ![](joint_library/tile_shiplap_edge.png) |
| **V-grooved joint**, p. 770 | | |
| ![](joint_library/beam_v_grooved_edge.gif) | ![](joint_library/beam_v_grooved_edge.png) | ![](joint_library/tile_v_grooved_edge.png) |
| **Tongue and groove joint**, p. 772 | | |
| ![](joint_library/beam_tongue_groove_edge.gif) | ![](joint_library/beam_tongue_groove_edge.png) | ![](joint_library/tile_tongue_groove_edge.png) |
| **Tongued and rabbeted joint**, p. 774 | | |
| ![](joint_library/beam_tongued_rabbeted_edge.gif) | ![](joint_library/beam_tongued_rabbeted_edge.png) | ![](joint_library/tile_tongued_rabbeted_edge.png) |
| **Floating tenon joint**, p. 776 | | |
| ![](joint_library/beam_floating_tenon_edge.gif) | ![](joint_library/beam_floating_tenon_edge.png) | ![](joint_library/tile_floating_tenon_edge.png) |
| **Foxtail-wedged floating tenon joint**, p. 778 | | |
| ![](joint_library/beam_foxtail_wedged_floating_tenon_edge.gif) | ![](joint_library/beam_foxtail_wedged_floating_tenon_edge.png) | ![](joint_library/tile_foxtail_wedged_floating_tenon_edge.png) |
| **Spline and groove joint**, p. 780 | | |
| ![](joint_library/beam_spline_groove_edge.gif) | ![](joint_library/beam_spline_groove_edge.png) | ![](joint_library/tile_spline_groove_edge.png) |
| **Floating tenon and spline joint**, p. 782 | | |
| ![](joint_library/beam_floating_tenon_spline_edge.gif) | ![](joint_library/beam_floating_tenon_spline_edge.png) | ![](joint_library/tile_floating_tenon_spline_edge.png) |
| **Blind dovetail joint**, p. 784 | | |
| ![](joint_library/beam_blind_dovetail_edge.gif) | ![](joint_library/beam_blind_dovetail_edge.png) | ![](joint_library/tile_blind_dovetail_edge.png) |
| **Butterfly key joint**, p. 786 | | |
| ![](joint_library/beam_butterfly_key_edge.gif) | ![](joint_library/beam_butterfly_key_edge.png) | ![](joint_library/tile_butterfly_key_edge.png) |
| **Sliding dovetail batten**, p. 788 | | |
| ![](joint_library/beam_sliding_dovetail_batten.gif) | ![](joint_library/beam_sliding_dovetail_batten.png) | ![](joint_library/tile_sliding_dovetail_batten.png) |
| **Sliding dovetail batten with keys**, p. 790 | | |
| ![](joint_library/beam_sliding_dovetail_batten_keys.gif) | ![](joint_library/beam_sliding_dovetail_batten_keys.png) | ![](joint_library/tile_sliding_dovetail_batten_keys.png) |
| **Blind dovetailed keyhole batten**, p. 792 | | |
| ![](joint_library/beam_blind_dovetailed_keyhole_batten.gif) | ![](joint_library/beam_blind_dovetailed_keyhole_batten.png) | ![](joint_library/tile_blind_dovetailed_keyhole_batten.png) |
| **Halved splayed joint**, p. 794 | | |
| ![](joint_library/beam_halved_splayed_end.gif) | ![](joint_library/beam_halved_splayed_end.png) | ![](joint_library/tile_halved_splayed_end.png) |
| **End-to-end finger joint**, p. 796 | | |
| ![](joint_library/beam_finger_end.gif) | ![](joint_library/beam_finger_end.png) | ![](joint_library/tile_finger_end.png) |
| **End-to-end dovetail joint**, p. 798 | | |
| ![](joint_library/beam_dovetail_end.gif) | ![](joint_library/beam_dovetail_end.png) | ![](joint_library/tile_dovetail_end.png) |
| **V-shaped dado with spline**, p. 800 | | |
| ![](joint_library/beam_v_dado_spline_end.gif) | ![](joint_library/beam_v_dado_spline_end.png) | ![](joint_library/tile_v_dado_spline_end.png) |
| **Dado with spline**, p. 802 | | |
| ![](joint_library/beam_dado_spline_end.gif) | ![](joint_library/beam_dado_spline_end.png) | ![](joint_library/tile_dado_spline_end.png) |
| **Splined breadboard end**, p. 804 | | |
| ![](joint_library/beam_splined_breadboard.gif) | ![](joint_library/beam_splined_breadboard.png) | ![](joint_library/tile_splined_breadboard.png) |
| **Tongue and groove breadboard end**, p. 806 | | |
| ![](joint_library/beam_tongue_groove_breadboard.gif) | ![](joint_library/beam_tongue_groove_breadboard.png) | ![](joint_library/tile_tongue_groove_breadboard.png) |
| **Dado and tongue breadboard end**, p. 808 | | |
| ![](joint_library/beam_dado_tongue_breadboard.gif) | ![](joint_library/beam_dado_tongue_breadboard.png) | ![](joint_library/tile_dado_tongue_breadboard.png) |
| **Haunched mortise and tenon breadboard end**, p. 810 | | |
| ![](joint_library/beam_haunched_mortise_tenon_breadboard.gif) | ![](joint_library/beam_haunched_mortise_tenon_breadboard.png) | ![](joint_library/tile_haunched_mortise_tenon_breadboard.png) |
| **Wedged and haunched tenon breadboard end**, p. 812 | | |
| ![](joint_library/beam_wedged_haunched_tenon_breadboard.gif) | ![](joint_library/beam_wedged_haunched_tenon_breadboard.png) | ![](joint_library/tile_wedged_haunched_tenon_breadboard.png) |
| **Rabbet joint**, p. 814 | | |
| ![](joint_library/beam_rabbet_case_corner.gif) | ![](joint_library/beam_rabbet_case_corner.png) | ![](joint_library/tile_rabbet_case_corner.png) |
| **Mitered rabbet joint**, p. 816 | | |
| ![](joint_library/beam_mitered_rabbet_case_corner.gif) | ![](joint_library/beam_mitered_rabbet_case_corner.png) | ![](joint_library/tile_mitered_rabbet_case_corner.png) |
| **Dado and rabbet joint**, p. 818 | | |
| ![](joint_library/beam_dado_rabbet_case_corner.gif) | ![](joint_library/beam_dado_rabbet_case_corner.png) | ![](joint_library/tile_dado_rabbet_case_corner.png) |
| **Lock joint**, p. 820 | | |
| ![](joint_library/beam_lock_case_corner.gif) | ![](joint_library/beam_lock_case_corner.png) | ![](joint_library/tile_lock_case_corner.png) |
| **Lock miter joint**, p. 822 | | |
| ![](joint_library/beam_lock_miter_case_corner.gif) | ![](joint_library/beam_lock_miter_case_corner.png) | ![](joint_library/tile_lock_miter_case_corner.png) |
| **Splined miter joint**, p. 824 | | |
| ![](joint_library/beam_splined_miter_case_corner.gif) | ![](joint_library/beam_splined_miter_case_corner.png) | ![](joint_library/tile_splined_miter_case_corner.png) |
| **Miter joint with spline keys**, p. 826 | | |
| ![](joint_library/beam_miter_spline_keys_case_corner.gif) | ![](joint_library/beam_miter_spline_keys_case_corner.png) | ![](joint_library/tile_miter_spline_keys_case_corner.png) |
| **Miter joint with dovetail keys**, p. 828 | | |
| ![](joint_library/beam_miter_dovetail_keys_case_corner.gif) | ![](joint_library/beam_miter_dovetail_keys_case_corner.png) | ![](joint_library/tile_miter_dovetail_keys_case_corner.png) |
| **Finger joint**, p. 830 | | |
| ![](joint_library/beam_finger_case_corner.gif) | ![](joint_library/beam_finger_case_corner.png) | ![](joint_library/tile_finger_case_corner.png) |
| **Through dovetail joint**, p. 832 | | |
| ![](joint_library/beam_through_dovetail_case_corner.gif) | ![](joint_library/beam_through_dovetail_case_corner.png) | ![](joint_library/tile_through_dovetail_case_corner.png) |
| **Half-blind dovetail joint**, p. 834 | | |
| ![](joint_library/beam_half_blind_dovetail_case_corner.gif) | ![](joint_library/beam_half_blind_dovetail_case_corner.png) | ![](joint_library/tile_half_blind_dovetail_case_corner.png) |
| **Blind dovetail joint**, p. 836 | | |
| ![](joint_library/beam_blind_dovetail_case_corner.gif) | ![](joint_library/beam_blind_dovetail_case_corner.png) | ![](joint_library/tile_blind_dovetail_case_corner.png) |
| **Cogged dovetail joint**, p. 838 | | |
| ![](joint_library/beam_cogged_dovetail_case_corner.gif) | ![](joint_library/beam_cogged_dovetail_case_corner.png) | ![](joint_library/tile_cogged_dovetail_case_corner.png) |
| **Angled finger joint**, p. 840 | | |
| ![](joint_library/beam_angled_finger_case_corner.gif) | ![](joint_library/beam_angled_finger_case_corner.png) | ![](joint_library/tile_angled_finger_case_corner.png) |
| **Radial dovetail joint**, p. 842 | | |
| ![](joint_library/beam_radial_dovetail_case_corner.gif) | ![](joint_library/beam_radial_dovetail_case_corner.png) | ![](joint_library/tile_radial_dovetail_case_corner.png) |
| **Housing joint**, p. 844 | | |
| ![](joint_library/beam_housing_case.gif) | ![](joint_library/beam_housing_case.png) | ![](joint_library/tile_housing_case.png) |
| **Stopped housing joint**, p. 846 | | |
| ![](joint_library/beam_stopped_housing_case.gif) | ![](joint_library/beam_stopped_housing_case.png) | ![](joint_library/tile_stopped_housing_case.png) |
| **Beveled housing joint**, p. 848 | | |
| ![](joint_library/beam_beveled_housing_case.gif) | ![](joint_library/beam_beveled_housing_case.png) | ![](joint_library/tile_beveled_housing_case.png) |
| **Tongue and dado joint**, p. 850 | | |
| ![](joint_library/beam_tongue_dado_case.gif) | ![](joint_library/beam_tongue_dado_case.png) | ![](joint_library/tile_tongue_dado_case.png) |
| **Rabbet and dado joint**, p. 852 | | |
| ![](joint_library/beam_rabbet_dado_case.gif) | ![](joint_library/beam_rabbet_dado_case.png) | ![](joint_library/tile_rabbet_dado_case.png) |
| **Sliding dovetail joint**, p. 854 | | |
| ![](joint_library/beam_sliding_dovetail_case.gif) | ![](joint_library/beam_sliding_dovetail_case.png) | ![](joint_library/tile_sliding_dovetail_case.png) |
| **Single-shouldered sliding dovetail joint**, p. 856 | | |
| ![](joint_library/beam_single_shouldered_sliding_dovetail_case.gif) | ![](joint_library/beam_single_shouldered_sliding_dovetail_case.png) | ![](joint_library/tile_single_shouldered_sliding_dovetail_case.png) |
<!-- /joint-table -->

## Library
Every record built on pairs of 120 x 120 beams standing in the xz plane, a group and a row per design: on a square corner and on one leaning at 50 degrees where the record exists there, each drawn apart as the book draws it, the member 350 mm out along its own axis and every peg along its own; the ways each goes in are the record's insertions.

![](elements/element_joint_beam_library.png)

\include{lineno} elements/element_joint_beam_library.cpp

## Tiles
Every record as its joinery tile: the wood each cut takes from each member within the joint's zone as pairs of outlines in the unit box, the faces of a prism the solver lofts, the male a blue and the female b red, labelled fixed or repeating by divisions.

![](joint_library/tile_wedged_four_way_splayed_cogged.png)

\include{lineno} elements/element_joint_beam_library_tiles.cpp
