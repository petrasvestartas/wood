#!/usr/bin/env bash
# Every picture of the template pages, a screenshot of the session_viewer cropped through render_template_docs.py from the built examples: the
# recipe, so the pages can be regenerated whole after any change. Each line names the example and the groups it shoots; tools/check_element_pages.py
# reads these lines to prove every picture on a page is made here.
#
#   bash tools/render_template_pictures.sh            every family
#   bash tools/render_template_pictures.sh module     one family: shells, folding, cylindrical_folding, developable_folding, chevron, cross, reciprocal, module, grid, vault, truss or bow
#
# PYTHON names the interpreter that runs the renderer (python3 by default, python on Windows). SHOT_PORT_BASE moves the Chrome ports off 9333.
set -euo pipefail

cd "$(dirname "$0")/.."
FAMILY="${1:-all}"
SHOTS="${SHOTS:-3}"
SLOT=0

# one line of the recipe in the background, each slot's Chrome on its own port, SHOTS at a time
render_job() {
    SHOT_PORT=$((${SHOT_PORT_BASE:-9333} + SLOT % SHOTS)) "${PYTHON:-python3}" tools/render_template_docs.py "$@" || echo "FAILED $*" &
    SLOT=$((SLOT + 1))
    if [ $((SLOT % SHOTS)) -eq 0 ]; then wait; fi
}
RENDER=(render_job)

# one scene file shot whole into the picture given (render_scene_picture.py), the same slots
scene_job() {
    SHOT_PORT=$((${SHOT_PORT_BASE:-9333} + SLOT % SHOTS)) "${PYTHON:-python3}" tools/render_scene_picture.py "$1" "$2" || echo "FAILED $2" &
    SLOT=$((SLOT + 1))
    if [ $((SLOT % SHOTS)) -eq 0 ]; then wait; fi
}

# the built example, wherever the generator put it
example_exe() {
    if [ -x "build/Release/$1.exe" ]; then echo "build/Release/$1"; else echo "build/$1"; fi
}

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
    "${RENDER[@]}" templates_folding_10_three_hinged_arch plates
    "${RENDER[@]}" templates_folding_12_square_fold plates
    "${RENDER[@]}" templates_folding_14_pyramid plates
    "${RENDER[@]}" templates_folding_15_truncated_pyramid plates
    "${RENDER[@]}" templates_folding_16_cross_fold plates
    "${RENDER[@]}" templates_folding_19_fold_composition plates
    "${RENDER[@]}" templates_folding_20_triangle_square plates
    "${RENDER[@]}" templates_folding_34_a_frame plates
    "${RENDER[@]}" templates_folding_36_folded_truncated_pyramid plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = cylindrical_folding ]; then
    "${RENDER[@]}" templates_cylindrical_folding_1_reflection lamellas
    "${RENDER[@]}" templates_cylindrical_folding_2_curved_folded_plate lamellas
    "${RENDER[@]}" templates_cylindrical_folding_3_mendrisio_pavilion lamellas
    "${RENDER[@]}" templates_cylindrical_folding_5_two_hinged_arch lamellas
    "${RENDER[@]}" templates_cylindrical_folding_7_a_frame lamellas
    "${RENDER[@]}" templates_cylindrical_folding_8_groin_vault lamellas
    "${RENDER[@]}" templates_cylindrical_folding_9_pavilion_vault lamellas
    "${RENDER[@]}" templates_cylindrical_folding_12_octagon_vault lamellas
    "${RENDER[@]}" --opacity 0.95 templates_cylindrical_folding_13_curved_folded_beam all
    "${RENDER[@]}" templates_cylindrical_folding_14_corner_fan strips
    "${RENDER[@]}" --view top --name plan templates_cylindrical_folding_14_corner_fan strips
    "${RENDER[@]}" --aspect 1.5 templates_cylindrical_folding_16_cylinder_carving stage_1 stage_2 stage_3
    "${RENDER[@]}" --aspect 1.5 templates_cylindrical_folding_17_cylinder_carving_rings ring_4 ring_6 ring_8
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = developable_folding ]; then
    "${RENDER[@]}" templates_developable_folding_1_curved_fold lamellas
    "${RENDER[@]}" templates_developable_folding_2_curved_crease_unfolding plates
    "${RENDER[@]}" --aspect 1.5 templates_developable_folding_4_crease_quadrants quadrant_1 quadrant_2 quadrant_3 quadrant_4
    "${RENDER[@]}" templates_developable_folding_5_synclastic_unfolding plates
    "${RENDER[@]}" templates_developable_folding_9_alternating_unfolding plates
    "${RENDER[@]}" templates_developable_folding_11_corrugated_dome plates
    "${RENDER[@]}" templates_developable_folding_12_corrugated_funnel plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = chevron ]; then
    SHOT_ONE_OBJECT=1 "${RENDER[@]}" templates_chevron_1_chevron surface
    "${RENDER[@]}" templates_chevron_1_chevron mesh strips
    "${RENDER[@]}" --opacity 1.0 templates_chevron_1_chevron plates
    VIEWER_PLANE_SIZE=550 "${RENDER[@]}" --opacity 0.55 --name edge_planes templates_chevron_1_chevron mesh+edge_planes
    VIEWER_PLANE_SIZE=650 "${RENDER[@]}" --opacity 0.55 --name bisectors templates_chevron_1_chevron mesh+bisectors
    "${RENDER[@]}" --opacity 1.0 --name joints templates_chevron_1_chevron box_56+box_57+box_58+box_59+box_79+box_80+box_81+box_82+box_102+box_103+box_104+box_105+box_125+box_126+box_127+box_128
    "${RENDER[@]}" templates_chevron_2_shifted mesh
    "${RENDER[@]}" --opacity 1.0 templates_chevron_2_shifted plates
    VIEWER_PLANE_SIZE=550 "${RENDER[@]}" --opacity 0.55 --name edge_planes templates_chevron_2_shifted mesh+edge_planes
    "${RENDER[@]}" --opacity 1.0 --name joints templates_chevron_2_shifted box_65+box_66+box_67+box_68+box_96+box_95+box_94+box_93+box_119+box_120+box_121+box_122+box_150+box_149+box_148+box_147
    "${RENDER[@]}" --opacity 1.0 templates_chevron_3_max2_arch plates
    "${RENDER[@]}" --opacity 1.0 --name joints templates_chevron_3_max2_arch box_61+box_62+box_63+box_64+box_86+box_87+box_88+box_89+box_111+box_112+box_113+box_114+box_136+box_137+box_138+box_139
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = cross ]; then
    "${RENDER[@]}" templates_cross_1_vda_mesh all plates connectors
    # a scene of planes alone stays on a white page: each with the mesh it stands on
    VIEWER_PLANE_SIZE=550 "${RENDER[@]}" --opacity 0.55 --name edge_planes templates_cross_1_vda_mesh mesh+edge_planes
    VIEWER_PLANE_SIZE=550 "${RENDER[@]}" --opacity 0.55 --name connector_frames templates_cross_1_vda_mesh mesh+connector_frames
    "${RENDER[@]}" --wheel 6,0.5,0.3 --name cross_laps templates_cross_1_vda_mesh plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = reciprocal ]; then
    "${RENDER[@]}" templates_reciprocal_1_rotation mesh axes beams frame
    "${RENDER[@]}" --name assembly templates_reciprocal_1_rotation beams+frame
    "${RENDER[@]}" templates_reciprocal_2_move mesh axes beams frame
    "${RENDER[@]}" --name assembly templates_reciprocal_2_move beams+frame
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = module ]; then
    "${RENDER[@]}" templates_module_1_wall all
    "${RENDER[@]}" --name frame templates_module_1_wall rails+studs+opening_0+opening_1
    "${RENDER[@]}" --name sheathing templates_module_1_wall sheathing_0+sheathing_1
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_1_wall all
    "${RENDER[@]}" templates_module_2_cassette_shell all cassette_4
    "${RENDER[@]}" --name two_cassettes templates_module_2_cassette_shell cassette_4+cassette_8
    "${RENDER[@]}" templates_module_3_cassette_house all
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_3_cassette_house all
    "${RENDER[@]}" --name front templates_module_3_cassette_house cassette_0
    "${RENDER[@]}" --opacity 0.95 --name gable templates_module_3_cassette_house cassette_3
    "${RENDER[@]}" --view right --name section templates_module_3_cassette_house all
    "${RENDER[@]}" --view right --wheel 12,0.355,0.445 --name eave templates_module_3_cassette_house all
    "${RENDER[@]}" --view right --wheel 12,0.5,0.225 --name ridge templates_module_3_cassette_house all
    "${RENDER[@]}" templates_module_4_room_module all
    "${RENDER[@]}" --opacity 0.95 --name inside templates_module_4_room_module all
    "${RENDER[@]}" --name floor templates_module_4_room_module module_0_0_floor_beams
    "${RENDER[@]}" --name ceiling templates_module_4_room_module module_0_1_ceiling_beams+module_0_1_posts
    "${RENDER[@]}" --view right --name section templates_module_4_room_module all
    "${RENDER[@]}" --name single templates_module_4_room_module module_0_1
    "${RENDER[@]}" --name wall templates_module_4_room_module module_0_0_wall_start_rails+module_0_0_wall_start_studs+module_0_0_wall_start_opening_0
    "${RENDER[@]}" templates_module_5_module_block all
    "${RENDER[@]}" templates_module_6_bank_pavilion all
    PAVILION=module_0_0_floor+module_0_0_posts+module_0_0_wall_front+module_0_0_wall_back+module_0_0_wall_start+module_0_0_wall_end
    PAVILION+=+module_0_1_floor+module_0_1_posts+module_0_1_wall_start+module_0_1_wall_end
    PAVILION+=+module_0_2_floor+module_0_2_posts+module_0_2_wall_start+module_0_2_wall_end
    PAVILION+=+module_0_3_floor+module_0_3_posts+module_0_3_wall_front+module_0_3_wall_back+module_0_3_wall_start+module_0_3_wall_end
    "${RENDER[@]}" --view top --name plan templates_module_6_bank_pavilion "$PAVILION"
    "${RENDER[@]}" --name open templates_module_6_bank_pavilion "$PAVILION+module_0_1_ceiling_beams+module_0_2_ceiling_beams"
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = grid ]; then
    # one picture per building, every group of each example's scene, cut to the card tiles; each example runs once
    GRID_SCENES="$(mktemp -d)"
    "${RENDER[@]}" --scene "$GRID_SCENES/footprint.pb" --aspect 1.5 templates_grid_1_footprint orthogonal radial hexagonal irregular courtyard pentagon cores
    # the isometric view looks along these two grids: turned, so they do not read as a flat sheet
    SHOT_ORBIT=300 "${RENDER[@]}" --scene "$GRID_SCENES/footprint.pb" --aspect 1.5 templates_grid_1_footprint skewed triangular
    "${RENDER[@]}" --aspect 1.5 templates_grid_2_solid box prism taper setback atrium curved
    "${RENDER[@]}" --aspect 1.5 templates_grid_3_lines crea_4x4 braced
    "${RENDER[@]}" --aspect 1.5 templates_grid_4_reference branch_plate branch_post_and_beam branch_purlin_on_girder branch_residential timber_bay_v1 timber_bay_v2 timber_bay_v3 timber_bay_v4
    "${RENDER[@]}" --aspect 1.5 templates_grid_5_framing head_section head_conical head_stepped node_flush node_through purlin_flush purlin_hung purlin_stacked profile_rectangle profile_round profile_w profile_hss profile_slab_band profile_t
    "${RENDER[@]}" --scene "$GRID_SCENES/footprint.pb" templates_grid_1_footprint walls_1 purlins_1
    "${RENDER[@]}" templates_grid_6_five_bay all plan_2 columns_1 heads_1 beams_1 decks_1
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = vault ]; then
    "${RENDER[@]}" templates_vault_1_arch all
    "${RENDER[@]}" templates_vault_2_barrel all
    "${RENDER[@]}" templates_vault_4_dome all
    "${RENDER[@]}" templates_vault_5_wall all
    "${RENDER[@]}" templates_vault_6_cross_vault all
    "${RENDER[@]}" templates_vault_7_cloister_vault all
    # one run of the example, every picture shot from its scene
    VAULT_SCENE="$(mktemp -d)/vault.pb"
    for bay in quadripartite_wells:quadripartite tierceron_exeter:tierceron star_stellar_burgos:star lierne_star_ely:lierne net_reticulated_parler:net fan_conoid_gloucester:fan; do
        "${RENDER[@]}" --scene "$VAULT_SCENE" --name "${bay#*:}" templates_vault_9_gothic_ribbed_vault "${bay%:*}"
        "${RENDER[@]}" --scene "$VAULT_SCENE" --view bottom --name "${bay#*:}_bottom" templates_vault_9_gothic_ribbed_vault "${bay%:*}"
    done
fi
# the library's trusses and the Pratt truss: every scene the examples write as truss_*.pb, one picture each
if [ "$FAMILY" = all ] || [ "$FAMILY" = truss ]; then
    rm -f data/output/pb/truss_*.pb
    "$(example_exe templates_truss_1_pratt)" > /dev/null
    cp data/output/pb/live.pb data/output/pb/truss_templates_truss_1_pratt.pb
    "$(example_exe templates_truss_2_library)" > /dev/null
    for pb in data/output/pb/truss_*.pb; do
        name=$(basename "$pb" .pb)
        name=${name#truss_templates_}
        case "$name" in truss_1_pratt) name=templates_truss_1_pratt ;; esac
        scene_job "$pb" "docs/images/templates/$name.png"
    done
fi

# Bow's 325 trusses (data/templates/truss_bow_1873.json), one webp each
if [ "$FAMILY" = all ] || [ "$FAMILY" = bow ]; then
    rm -f data/output/pb/truss_bow_*.pb
    "$(example_exe templates_truss_3_bow)" > /dev/null
    for pb in data/output/pb/truss_bow_*.pb; do
        scene_job "$pb" "docs/images/templates/$(basename "$pb" .pb).webp"
    done
fi
wait
