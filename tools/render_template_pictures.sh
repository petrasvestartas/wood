#!/usr/bin/env bash
# Every picture of the shells, folding, chevron, cross, reciprocal and module template pages, rendered by the session_viewer through
# render_template_docs.py from the built examples: the recipe, so the pages can be regenerated whole after any change.
#
#   bash tools/render_template_pictures.sh            every family
#   bash tools/render_template_pictures.sh module     one family: shells, folding, chevron, cross, reciprocal or module
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
    "${RENDER[@]}" templates_folding_17_cross_fold_divided plates
    "${RENDER[@]}" templates_folding_18_cross_fold_sloping plates
    "${RENDER[@]}" templates_folding_19_fold_composition plates
    "${RENDER[@]}" templates_folding_20_triangle_square plates
    "${RENDER[@]}" templates_folding_21_triangle_triangle plates
    "${RENDER[@]}" templates_folding_22_triangle_pentagon plates
    "${RENDER[@]}" templates_folding_23_triangle_hexagon plates
    "${RENDER[@]}" templates_folding_24_fan_fold plates
    "${RENDER[@]}" templates_folding_25_one_fold plates
    "${RENDER[@]}" templates_folding_26_three_folds plates
    "${RENDER[@]}" templates_folding_27_counter_centre plates
    "${RENDER[@]}" templates_folding_28_ridge_to_valley plates
    "${RENDER[@]}" templates_folding_29_elevated_ridge_to_valley plates
    "${RENDER[@]}" templates_folding_30_alternate_fold plates
    "${RENDER[@]}" templates_folding_31_deep_centre plates
    "${RENDER[@]}" templates_folding_32_deep_centre_valley plates
    "${RENDER[@]}" templates_folding_33_frame_ridge_to_valley plates
    "${RENDER[@]}" templates_folding_34_a_frame plates
    "${RENDER[@]}" templates_folding_35_pointed_arch plates
    "${RENDER[@]}" templates_folding_36_folded_truncated_pyramid plates
    "${RENDER[@]}" templates_folding_37_curved_fold lamellas
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
