#!/usr/bin/env bash
# Every picture of the shells, folding, curved_folding, chevron, cross, reciprocal and module template pages, rendered by the session_viewer through
# render_template_docs.py from the built examples: the recipe, so the pages can be regenerated whole after any change.
#
#   bash tools/render_template_pictures.sh            every family
#   bash tools/render_template_pictures.sh module     one family: shells, folding, curved_folding, chevron, cross, reciprocal or module
#
# PYTHON names the interpreter that runs the renderer (python3 by default, python on Windows).
set -euo pipefail

cd "$(dirname "$0")/.."
FAMILY="${1:-all}"
RENDER=("${PYTHON:-python3}" tools/render_template_docs.py)

if [ "$FAMILY" = all ] || [ "$FAMILY" = shells ]; then
    "${RENDER[@]}" templates_shells_1_translation_shell curves sections mesh plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = folding ]; then
    "${RENDER[@]}" templates_folding_1_reflex_fold plates
    "${RENDER[@]}" templates_folding_2_diamond_mesh plates
    "${RENDER[@]}" templates_folding_3_prismatic_fold all
    "${RENDER[@]}" templates_folding_4_folded_barrel all
    "${RENDER[@]}" templates_folding_5_counter_fold plates
    "${RENDER[@]}" templates_folding_6_elevated_fold plates
    "${RENDER[@]}" templates_folding_7_conical_fold plates
    "${RENDER[@]}" templates_folding_8_two_hinged_frame plates
    "${RENDER[@]}" templates_folding_9_three_hinged_frame plates
    "${RENDER[@]}" templates_folding_10_three_hinged_arch plates
    "${RENDER[@]}" templates_folding_11_triangle_fold plates
    "${RENDER[@]}" templates_folding_12_square_fold plates
    "${RENDER[@]}" templates_folding_13_hexagon_fold plates
    "${RENDER[@]}" templates_folding_14_pyramid plates
    "${RENDER[@]}" templates_folding_15_truncated_pyramid plates
    "${RENDER[@]}" templates_folding_16_cross_fold plates
    "${RENDER[@]}" templates_folding_17_cross_fold_rising plates
    "${RENDER[@]}" templates_folding_18_cross_fold_sloping plates
    "${RENDER[@]}" templates_folding_19_fold_composition plates
    "${RENDER[@]}" templates_folding_20_triangle_square plates
    "${RENDER[@]}" templates_folding_21_triangle_triangle plates
    "${RENDER[@]}" templates_folding_22_triangle_pentagon plates
    "${RENDER[@]}" templates_folding_23_triangle_hexagon plates
    "${RENDER[@]}" templates_folding_25_one_fold plates
    "${RENDER[@]}" templates_folding_26_three_folds plates
    "${RENDER[@]}" templates_folding_27_counter_centre plates
    "${RENDER[@]}" templates_folding_28_ridge_to_valley plates
    "${RENDER[@]}" templates_folding_29_elevated_ridge_to_valley plates
    "${RENDER[@]}" templates_folding_30_alternate_fold plates
    "${RENDER[@]}" templates_folding_31_deep_centre plates
    "${RENDER[@]}" templates_folding_32_deep_centre_valley plates
    "${RENDER[@]}" templates_folding_34_a_frame plates
    "${RENDER[@]}" templates_folding_35_pointed_arch plates
    "${RENDER[@]}" templates_folding_36_folded_truncated_pyramid plates
    "${RENDER[@]}" templates_folding_38_truncated_pyramid_corner_folds plates
    "${RENDER[@]}" templates_folding_39_truncated_pyramid_corner_edge_folds plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = curved_folding ]; then
    "${RENDER[@]}" templates_curved_folding_1_curved_fold lamellas
    "${RENDER[@]}" --view front --name elevation templates_curved_folding_1_curved_fold lamellas
    "${RENDER[@]}" templates_curved_folding_2_reflection_fold lamellas all
    "${RENDER[@]}" --view right --name end templates_curved_folding_2_reflection_fold lamellas
    "${RENDER[@]}" templates_curved_folding_3_curved_folded_plate lamellas all
    "${RENDER[@]}" --view front --name section templates_curved_folding_3_curved_folded_plate lamellas
    "${RENDER[@]}" templates_curved_folding_4_groin_vault lamellas all
    "${RENDER[@]}" --view top --name plan templates_curved_folding_4_groin_vault lamellas
    "${RENDER[@]}" templates_curved_folding_5_groin_vault_sloping lamellas
    "${RENDER[@]}" templates_curved_folding_6_groin_vault_rising lamellas
    "${RENDER[@]}" --name lamellas templates_curved_folding_7_pavilion_vault lamellas+walls
    "${RENDER[@]}" --view top --name plan templates_curved_folding_7_pavilion_vault lamellas
    "${RENDER[@]}" templates_curved_folding_8_triangle_vault lamellas
    "${RENDER[@]}" templates_curved_folding_9_pentagon_vault lamellas
    "${RENDER[@]}" templates_curved_folding_10_octagon_vault lamellas
    "${RENDER[@]}" templates_curved_folding_11_hexagon_vault lamellas
    "${RENDER[@]}" templates_curved_folding_12_cross_vault lamellas
    "${RENDER[@]}" --view top --name plan templates_curved_folding_12_cross_vault lamellas
    "${RENDER[@]}" templates_curved_folding_13_two_hinged_frame lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_13_two_hinged_frame lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_13_two_hinged_frame lamellas
    "${RENDER[@]}" --view top --name plan templates_curved_folding_13_two_hinged_frame lamellas
    "${RENDER[@]}" templates_curved_folding_14_three_hinged_frame lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_14_three_hinged_frame lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_14_three_hinged_frame lamellas
    "${RENDER[@]}" --view top --name plan templates_curved_folding_14_three_hinged_frame lamellas
    "${RENDER[@]}" templates_curved_folding_15_two_hinged_arch lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_15_two_hinged_arch lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_15_two_hinged_arch lamellas
    "${RENDER[@]}" templates_curved_folding_16_arch_with_top_hinge lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_16_arch_with_top_hinge lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_16_arch_with_top_hinge lamellas
    "${RENDER[@]}" --view top --name plan templates_curved_folding_16_arch_with_top_hinge lamellas
    "${RENDER[@]}" templates_curved_folding_17_a_frame lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_17_a_frame lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_17_a_frame lamellas
    "${RENDER[@]}" templates_curved_folding_18_cantilever lamellas
    "${RENDER[@]}" --view front --name section templates_curved_folding_18_cantilever lamellas
    "${RENDER[@]}" --view right --name end templates_curved_folding_18_cantilever lamellas
    "${RENDER[@]}" templates_curved_folding_19_vault_field lamellas
    "${RENDER[@]}" --view front --name elevation templates_curved_folding_19_vault_field lamellas
    "${RENDER[@]}" templates_curved_folding_20_curved_crease_unfolding all
    "${RENDER[@]}" --view front --name elevation templates_curved_folding_20_curved_crease_unfolding plates
    "${RENDER[@]}" templates_curved_folding_21_catenary_unfolding all
    "${RENDER[@]}" templates_curved_folding_22_crease_quadrants all
    "${RENDER[@]}" templates_curved_folding_23_synclastic_unfolding plates
    "${RENDER[@]}" templates_curved_folding_24_anticlastic_unfolding plates
    "${RENDER[@]}" templates_curved_folding_25_apex_level_unfolding plates
    "${RENDER[@]}" templates_curved_folding_26_support_level_unfolding plates
    "${RENDER[@]}" templates_curved_folding_27_alternating_unfolding plates
    "${RENDER[@]}" templates_curved_folding_28_sideways_unfolding plates
    "${RENDER[@]}" templates_curved_folding_29_corrugated_dome plates
    "${RENDER[@]}" templates_curved_folding_30_corrugated_funnel plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = chevron ]; then
    "${RENDER[@]}" templates_chevron_1_chevron surface mesh strips edge_planes bisectors plates insertion
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = cross ]; then
    "${RENDER[@]}" templates_cross_1_vda_mesh all mesh face_planes edge_planes bisector_planes plates connector_frames connectors
    "${RENDER[@]}" --size 8000x5333 --crop 0.25,0.05,0.75,0.55 --name cross_laps templates_cross_1_vda_mesh plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = reciprocal ]; then
    "${RENDER[@]}" templates_reciprocal_1_rotation mesh axes beams frame
    "${RENDER[@]}" templates_reciprocal_2_move mesh axes beams frame
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = module ]; then
    "${RENDER[@]}" templates_module_1_wall all
    "${RENDER[@]}" --name frame templates_module_1_wall rails+studs+opening_0+opening_1
    "${RENDER[@]}" --name sheathing templates_module_1_wall sheathing_0+sheathing_1
    "${RENDER[@]}" --name contacts templates_module_1_wall contacts_frame+contacts_sheathing
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_1_wall all
    "${RENDER[@]}" templates_module_2_cassette_shell all cassette_4 contacts
    "${RENDER[@]}" --name two_cassettes templates_module_2_cassette_shell cassette_4+cassette_8
    "${RENDER[@]}" templates_module_3_cassette_house all contacts
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_3_cassette_house all
    "${RENDER[@]}" --name front templates_module_3_cassette_house cassette_0
    "${RENDER[@]}" --opacity 0.95 --name gable templates_module_3_cassette_house cassette_3
    "${RENDER[@]}" --view right --name section templates_module_3_cassette_house all
    "${RENDER[@]}" --view right --size 8000x5333 --crop 0.0,0.36,0.14,0.52 --name eave templates_module_3_cassette_house all
    "${RENDER[@]}" --view right --size 8000x5333 --crop 0.42,0.0,0.58,0.14 --name ridge templates_module_3_cassette_house all
    "${RENDER[@]}" templates_module_4_room_module all
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_4_room_module all
    "${RENDER[@]}" --name floor templates_module_4_room_module module_0_0_floor_beams
    "${RENDER[@]}" --name ceiling templates_module_4_room_module module_0_1_ceiling_beams+module_0_1_posts
    "${RENDER[@]}" --view right --name section templates_module_4_room_module all
    "${RENDER[@]}" --name single templates_module_4_room_module module_0_1
    "${RENDER[@]}" --name wall templates_module_4_room_module module_0_0_wall_start_rails+module_0_0_wall_start_studs+module_0_0_wall_start_opening_0
    "${RENDER[@]}" --name contacts templates_module_4_room_module contacts_frame+contacts_sheathing
    "${RENDER[@]}" templates_module_5_module_block all
    "${RENDER[@]}" templates_module_6_bank_pavilion all
    PAVILION=module_0_0_floor+module_0_0_posts+module_0_0_wall_front+module_0_0_wall_back+module_0_0_wall_start+module_0_0_wall_end
    PAVILION+=+module_0_1_floor+module_0_1_posts+module_0_1_wall_start+module_0_1_wall_end
    PAVILION+=+module_0_2_floor+module_0_2_posts+module_0_2_wall_start+module_0_2_wall_end
    PAVILION+=+module_0_3_floor+module_0_3_posts+module_0_3_wall_front+module_0_3_wall_back+module_0_3_wall_start+module_0_3_wall_end
    "${RENDER[@]}" --view top --name plan templates_module_6_bank_pavilion "$PAVILION"
    "${RENDER[@]}" --name open templates_module_6_bank_pavilion "$PAVILION+module_0_1_ceiling_beams+module_0_2_ceiling_beams"
fi
