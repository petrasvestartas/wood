#!/usr/bin/env bash
# Every picture of the template pages, a screenshot of the session_viewer cropped through render_template_docs.py from the built examples: the
# recipe, so the pages can be regenerated whole after any change. Each line names the example and the groups it shoots; tools/check_element_pages.py
# reads these lines to prove every picture on a page is made here.
#
#   bash tools/render_template_pictures.sh            every family
#   bash tools/render_template_pictures.sh module     one family: shells, folding, cylindrical_folding, developable_folding, chevron, cross, reciprocal, module, timber_floor, grid, vault, plaited, vidy, truss or bow
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
    "${RENDER[@]}" --name cross_laps templates_cross_1_vda_mesh plates+connectors
    "${RENDER[@]}" templates_cross_3_cardboard_shell all
    # the shell as the photograph of the paper shows it, from eye level
    "${RENDER[@]}" --view eye --turn 106 --size 6400x4000 --name eye_level templates_cross_3_cardboard_shell all
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
    "${RENDER[@]}" templates_module_7_module_types all
    "${RENDER[@]}" --aspect 1.5 templates_module_7_module_types mixed
    "${RENDER[@]}" --aspect 1.5 --name framed_bones templates_module_7_module_types framed_floor_rails+framed_floor_studs+framed_wall_back_rails+framed_wall_back_studs+framed_wall_front_rails+framed_wall_front_studs+framed_wall_front_opening_0+framed_wall_front_opening_1+framed_wall_start_rails+framed_wall_start_studs+framed_wall_start_opening_0+framed_wall_end_rails+framed_wall_end_studs+framed_wall_end_opening_0+framed_ceiling_rails+framed_ceiling_studs
    "${RENDER[@]}" --aspect 1.5 --name solid_inside templates_module_7_module_types solid_floor_panel+solid_wall_back_panel+solid_wall_end_panel+solid_wall_start_panel
    "${RENDER[@]}" templates_module_8_open_module all
    "${RENDER[@]}" templates_module_9_stacked_modules all
    "${RENDER[@]}" --view right --turn 180 --name front templates_module_9_stacked_modules all
    "${RENDER[@]}" --name bearings templates_module_9_stacked_modules bearings_1+module_0_0_0_ceiling+module_0_0_1_ceiling+module_0_0_2_ceiling
    "${RENDER[@]}" templates_module_10_module_rack all
    "${RENDER[@]}" templates_module_10_module_rack skeleton
    "${RENDER[@]}" templates_module_11_module_frame all
    "${RENDER[@]}" templates_module_11_module_frame skeleton
    "${RENDER[@]}" templates_module_12_module_levels all
    "${RENDER[@]}" templates_module_12_module_levels skeleton
    "${RENDER[@]}" templates_module_13_module_corridor all
    CORRIDOR=corridor_0
    CORRIDOR+=+module_0_0_0_floor+module_0_0_0_wall_front+module_0_0_0_wall_back+module_0_0_0_wall_start+module_0_0_0_wall_end+module_0_0_1_floor
    CORRIDOR+=+module_0_0_1_wall_front+module_0_0_1_wall_start+module_0_0_1_wall_end+module_0_0_1_frame_1+module_0_0_2_floor+module_0_0_2_wall_back
    CORRIDOR+=+module_0_0_2_wall_start+module_0_0_2_wall_end+module_0_0_2_frame_0+module_0_0_3_floor+module_0_0_3_wall_front+module_0_0_3_wall_back
    CORRIDOR+=+module_0_0_3_wall_start+module_0_0_3_wall_end+module_1_0_0_floor+module_1_0_0_wall_front+module_1_0_0_wall_back+module_1_0_0_wall_start
    CORRIDOR+=+module_1_0_0_wall_end+module_1_0_1_floor+module_1_0_1_wall_front+module_1_0_1_wall_start+module_1_0_1_wall_end+module_1_0_1_frame_1
    CORRIDOR+=+module_1_0_2_floor+module_1_0_2_wall_back+module_1_0_2_wall_start+module_1_0_2_wall_end+module_1_0_2_frame_0+module_1_0_3_floor
    CORRIDOR+=+module_1_0_3_wall_front+module_1_0_3_wall_back+module_1_0_3_wall_start+module_1_0_3_wall_end
    "${RENDER[@]}" --name plan templates_module_13_module_corridor "$CORRIDOR"
    WET=bearings_0
    WET+=+module_0_0_0_floor+module_0_0_0_wall_back+module_0_0_0_wall_front+module_0_0_0_wall_start+module_0_0_0_wall_end+module_0_0_0_pod
    WET+=+module_0_0_1_floor+module_0_0_1_wall_back+module_0_0_1_wall_front+module_0_0_1_wall_start+module_0_0_1_wall_end+module_0_0_1_pod
    "${RENDER[@]}" --name all templates_module_14_wet_room "$WET"
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = timber_floor ]; then
    "${RENDER[@]}" --aspect 1.5 templates_timber_floor_1_systems box dowelled cross_laminated composite
    "${RENDER[@]}" --view front --name section_ribs templates_timber_floor_1_systems joists+ribbed+box+hollow
    "${RENDER[@]}" --view front --name section_solid templates_timber_floor_1_systems planks+dowelled+cross_laminated+composite
    "${RENDER[@]}" --view right --aspect 2.5 --name resting_section templates_timber_floor_2_bearings resting
    "${RENDER[@]}" --view right --aspect 2.5 --name rebated_section templates_timber_floor_2_bearings rebated
    "${RENDER[@]}" --view right --aspect 2.5 --name battens_section templates_timber_floor_2_bearings battens
    "${RENDER[@]}" --view front --aspect 3 --name tongue_section templates_timber_floor_3_joints tongue
    "${RENDER[@]}" --view front --aspect 3 --name strip_section templates_timber_floor_3_joints strip
    "${RENDER[@]}" --view front --aspect 3 --name board_section templates_timber_floor_3_joints board
    "${RENDER[@]}" --aspect 1.5 --name open templates_timber_floor_4_earth arch_bearings+arch_beams+arch_earth
    "${RENDER[@]}" --view front --aspect 4 --name arch_section templates_timber_floor_4_earth arch_beams+arch_earth+arch_top
    "${RENDER[@]}" --view front --aspect 4 --name concealed_section templates_timber_floor_4_earth concealed_beams+concealed_earth+concealed_top
    "${RENDER[@]}" --aspect 1.5 templates_timber_floor_5_systems joists ribbed_above box beams_comb boards_dowelled cross_laminated_5
    "${RENDER[@]}" --aspect 1.5 templates_timber_floor_6_producers clt_box_closed lignatur_surface lignotrend_rib novatop_element
    "${RENDER[@]}" --aspect 1.5 templates_timber_floor_9_ribs clt_rib_box lvl_rib_strips
    "${RENDER[@]}" --aspect 1.5 --name concrete_ribs_open templates_timber_floor_9_ribs concrete_ribs_timber+concrete_ribs_pins
    "${RENDER[@]}" templates_timber_floor_10_spans all
    "${RENDER[@]}" --view front --name section templates_timber_floor_10_spans baubuche_7+kerto_rib_7+kerto_box_7+clt_box_7
    "${RENDER[@]}" --view front --name section_2 templates_timber_floor_10_spans lignatur_7+vmg_beam_7+vmg_joist_7
    "${RENDER[@]}" --aspect 1.5 templates_timber_floor_7_shear joint_cover joint_keys
    "${RENDER[@]}" --aspect 1.5 --name concrete_notched_open templates_timber_floor_7_shear concrete_notched_timber+concrete_notched_pins
    "${RENDER[@]}" --aspect 1.5 --name concrete_crossed_open templates_timber_floor_7_shear concrete_crossed_timber+concrete_crossed_pins
    "${RENDER[@]}" templates_timber_floor_8_build_ups all
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = grid ]; then
    # one picture per case, cut to the card tiles; each example runs once
    GRID_SCENES="$(mktemp -d)"
    "${RENDER[@]}" --scene "$GRID_SCENES/bay.pb" --aspect 1.5 templates_grid_1_bay_framing floor_l_plan isolated_bay slab_head beam_head node_flush node_through purlin_stacked round_columns
    "${RENDER[@]}" --scene "$GRID_SCENES/footprints.pb" --aspect 1.5 templates_grid_2_footprints orthogonal radial hexagonal irregular courtyard
    SHOT_ORBIT=300 "${RENDER[@]}" --scene "$GRID_SCENES/footprints.pb" --aspect 1.5 templates_grid_2_footprints skewed triangular
    "${RENDER[@]}" --scene "$GRID_SCENES/facades.pb" --aspect 1.5 templates_grid_3_facades curtain_mullion punched_openings braced_facade gable_facade
    "${RENDER[@]}" --scene "$GRID_SCENES/stammhaus.pb" --aspect 1.5 templates_grid_4_developed_facade stage_1_colonnade stage_2_spandrels stage_3_fin_screen stammhaus_headquarters
    "${RENDER[@]}" --scene "$GRID_SCENES/stammhaus.pb" --view front --aspect 1.5 --name stammhaus_elevation templates_grid_4_developed_facade stammhaus_headquarters
    "${RENDER[@]}" --scene "$GRID_SCENES/stammhaus.pb" --wheel 8,0.26,0.58 --aspect 1.5 --name stammhaus_detail templates_grid_4_developed_facade stammhaus_headquarters
    "${RENDER[@]}" --scene "$GRID_SCENES/multistorey.pb" --aspect 1.5 templates_grid_5_multistorey five_bay shifted_floors tapered_tower twisted_tower setback_tower cylindrical_rotunda oblique_flatiron core_and_facade
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

if [ "$FAMILY" = all ] || [ "$FAMILY" = plaited ]; then
    "${RENDER[@]}" templates_plaited_1_flat_floor blocks
    "${RENDER[@]}" --view bottom --name intrados templates_plaited_1_flat_floor blocks
    "${RENDER[@]}" templates_plaited_2_arch blocks
    "${RENDER[@]}" --view bottom --name intrados templates_plaited_2_arch blocks
    "${RENDER[@]}" templates_plaited_3_dome blocks
    "${RENDER[@]}" --view bottom --name intrados templates_plaited_3_dome blocks
    "${RENDER[@]}" templates_plaited_4_hotwire_blocks abeille_open sphera_open
    "${RENDER[@]}" --view top --name plan templates_plaited_4_hotwire_blocks abeille_open
    "${RENDER[@]}" --name arch_pinned templates_plaited_5_reciprocal_arch beams+frame+pins
    "${RENDER[@]}" --opacity 0.55 --wheel 8,0.48,0.38 --name pins_detail templates_plaited_5_reciprocal_arch beams+frame+pins
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = vidy ]; then
    "${RENDER[@]}" --name mesh_axes templates_vidy_1_main_mesh base_mesh+axes
    "${RENDER[@]}" --view top --name plan templates_vidy_1_main_mesh base_mesh+axes
    "${RENDER[@]}" --opacity 0.68 --name offset_shells templates_vidy_2_offset_mesh inner_surface+outer_surface+miter_planes
    "${RENDER[@]}" --opacity 0.72 templates_vidy_2_offset_mesh two_bay_shells
    "${RENDER[@]}" --name pavilion templates_vidy_3_panels inner_walls+outer_walls+inner_roof+outer_roof+border_boards
    "${RENDER[@]}" templates_vidy_3_panels two_bay_panels
    VIDY4_SCENE="$(mktemp -d)/vidy4.pb"
    "${RENDER[@]}" --scene "$VIDY4_SCENE" --name full_pavilion templates_vidy_4_section_joints inner_walls+outer_walls+inner_roof+outer_roof+border_boards
    "${RENDER[@]}" --scene "$VIDY4_SCENE" templates_vidy_4_section_joints first_row_section
    SHOT_ORBIT=95 "${RENDER[@]}" --scene "$VIDY4_SCENE" templates_vidy_4_section_joints module_1_haunch_wings module_2_haunches_joined module_3_inner_crowns_eaves module_4_outer_covers module_5_vertical_walls module_6_roof_onto_walls
    VIDY5_SCENE="$(mktemp -d)/vidy5.pb"
    "${RENDER[@]}" --scene "$VIDY5_SCENE" templates_vidy_5_parametric parametric_family compact_symmetric tapered_hall wide_shallow
    VIDY6_SCENE="$(mktemp -d)/vidy6.pb"
    "${RENDER[@]}" --scene "$VIDY6_SCENE" templates_vidy_6_origami_shapes prismatic_fold counter_fold
fi
# the library's trusses and the Pratt truss: every scene the examples write as truss_*.pb, one picture each
if [ "$FAMILY" = all ] || [ "$FAMILY" = truss ]; then
    rm -f data/output/pb/truss_*.pb
    "$(example_exe templates_truss_1_pratt)" > /dev/null
    cp data/output/pb/live.pb data/output/pb/truss_templates_truss_1_pratt.pb
    "$(example_exe templates_truss_2_library)" > /dev/null
    mv data/output/pb/truss_templates_truss_1_pratt.pb data/output/pb/templates_truss_1_pratt.pb
    # drawn like the joint library and the elements (see-through, as dark as solid, one isometric camera), not screenshot
    "${PYTHON:-python3}" tools/render_element_pictures.py --scenes docs/images/templates data/output/pb/truss_*.pb data/output/pb/templates_truss_1_pratt.pb
    # the joinery's nodes close up: the king post's heel, apex, foot and strut head, a Howe node, a kink and a crossing
    for node in "truss_king_post truss_node_heel 0,0,0 23" "truss_king_post truss_node_apex 4000,0,2667 23" \
        "truss_king_post truss_node_foot 4000,0,0 23" "truss_king_post truss_node_strut 2000,0,1333 23" \
        "truss_howe truss_node_howe 4000,0,0 23" "truss_roof_cambered_tie truss_node_kink 6000,0,500 22" \
        "truss_town_lattice truss_node_crossing 1556,0,1500 23"; do
        set -- $node
        "${PYTHON:-python3}" tools/render_element_pictures.py --close docs/images/templates "data/output/pb/$1.pb" "$2" "$3" "$4"
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
