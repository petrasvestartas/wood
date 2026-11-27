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
    "${RENDER[@]}" --name plates templates_developable_folding_2_curved_crease_unfolding stage_1
    "${RENDER[@]}" --name closed templates_developable_folding_2_curved_crease_unfolding stage_0
    "${RENDER[@]}" --view top templates_developable_folding_2_curved_crease_unfolding flat
    "${RENDER[@]}" --name stages templates_developable_folding_3_unfolding_sequence stage_0+stage_1+stage_2+stage_3
    "${RENDER[@]}" --aspect 1.5 templates_developable_folding_4_crease_quadrants quadrant_1 quadrant_2 quadrant_3 quadrant_4
    "${RENDER[@]}" --name plates templates_developable_folding_5_synclastic_unfolding stage_1
    "${RENDER[@]}" --name closed templates_developable_folding_5_synclastic_unfolding stage_0
    "${RENDER[@]}" templates_developable_folding_6_cellular_tube_vault plates
    "${RENDER[@]}" --view top templates_developable_folding_6_cellular_tube_vault flat
    "${RENDER[@]}" --name plates templates_developable_folding_9_alternating_unfolding stage_1
    "${RENDER[@]}" --name closed templates_developable_folding_9_alternating_unfolding stage_0
    "${RENDER[@]}" --name plates templates_developable_folding_11_corrugated_dome stage_1
    "${RENDER[@]}" --name closed templates_developable_folding_11_corrugated_dome stage_0
    "${RENDER[@]}" --name plates templates_developable_folding_12_corrugated_funnel stage_1
    "${RENDER[@]}" --name closed templates_developable_folding_12_corrugated_funnel stage_0
    "${RENDER[@]}" --name design templates_developable_folding_13_space_curve_fold stage_0_quads
    "${RENDER[@]}" --name stages templates_developable_folding_13_space_curve_fold stage_0_quads+stage_1_quads+stage_2_quads+stage_3_quads+stage_4_quads
    "${RENDER[@]}" --name design templates_developable_folding_14_curved_miura stage_0
    "${RENDER[@]}" --name stages templates_developable_folding_14_curved_miura stage_0+stage_1+stage_2
    "${RENDER[@]}" --name design templates_developable_folding_15_smooth_space_curve_fold stage_0
    "${RENDER[@]}" --name stages templates_developable_folding_15_smooth_space_curve_fold stage_0+stage_1+stage_2+stage_3
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
    "${RENDER[@]}" templates_module_2_cassette_shell all
    "${RENDER[@]}" templates_module_3_cassette_house all
    "${RENDER[@]}" --view right --name section templates_module_3_cassette_house all
    "${RENDER[@]}" templates_module_4_room_module all
    "${RENDER[@]}" --view right --name section templates_module_4_room_module all
    "${RENDER[@]}" templates_module_5_module_block all
    "${RENDER[@]}" templates_module_6_bank_pavilion all
    PAVILION=module_0_0_floor+module_0_0_posts+module_0_0_wall_front+module_0_0_wall_back+module_0_0_wall_start+module_0_0_wall_end
    PAVILION+=+module_0_1_floor+module_0_1_posts+module_0_1_wall_start+module_0_1_wall_end
    PAVILION+=+module_0_2_floor+module_0_2_posts+module_0_2_wall_start+module_0_2_wall_end
    PAVILION+=+module_0_3_floor+module_0_3_posts+module_0_3_wall_front+module_0_3_wall_back+module_0_3_wall_start+module_0_3_wall_end
    "${RENDER[@]}" templates_module_7_module_types all
    "${RENDER[@]}" templates_module_8_open_module all
    "${RENDER[@]}" templates_module_9_stacked_modules all
    "${RENDER[@]}" --view right --turn 180 --name front templates_module_9_stacked_modules all
    "${RENDER[@]}" templates_module_10_module_rack all
    "${RENDER[@]}" templates_module_11_module_frame all
    "${RENDER[@]}" templates_module_12_module_levels all
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
    "${RENDER[@]}" templates_timber_floor_1_structures all
    "${RENDER[@]}" --view front --name section_a templates_timber_floor_1_structures joists+ribbed+box+hollow
    "${RENDER[@]}" --view front --name section_b templates_timber_floor_1_structures dowelled+cross_laminated+composite
    "${RENDER[@]}" templates_timber_floor_2_spans all
    "${RENDER[@]}" --view front --name section_a templates_timber_floor_2_spans baubuche_6+vmg_6+kerto_ripa_6+kerto_box_6
    "${RENDER[@]}" --view front --name section_b templates_timber_floor_2_spans clt_ribs_6+clt_box_6+novatop_6+lignatur_6
    "${RENDER[@]}" --aspect 2.5 --name bearings templates_timber_floor_3_joints resting+rebated+battens
    "${RENDER[@]}" --view front --aspect 4 --name edges templates_timber_floor_3_joints tongue+strip+board+keys
    "${RENDER[@]}" --aspect 2 --name concrete templates_timber_floor_3_joints glued_beams+glued_panels+notched_beams+notched_panels+pinned_beams+pinned_ribs+pinned_boards+pinned_joints+locked_beams+locked_panels+locked_joints
    "${RENDER[@]}" --aspect 1.5 --name open templates_timber_floor_4_earth arch_bearings+arch_beams+arch_earth
    "${RENDER[@]}" --view front --aspect 4 --name section templates_timber_floor_4_earth all
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
    "${RENDER[@]}" --scene "$GRID_SCENES/beam_grids.pb" --aspect 1.5 templates_grid_6_beam_grids built_up_profiles belly_spans haunched_cantilever gerber_hall diagonal_grillage concentric_lantern ramified_floor
    "${RENDER[@]}" --scene "$GRID_SCENES/frames_slabs.pb" --opacity 0.52 --aspect 1.5 templates_grid_7_frames_and_slabs mushroom_drop_slab ribbed_cantilever_slab waffle_coffered_slab
    "${RENDER[@]}" --scene "$GRID_SCENES/frames_slabs.pb" --aspect 1.5 templates_grid_7_frames_and_slabs umbrella_canopies tapered_portal_hall shear_vierendeel concentric_vierendeel mega_transfer_tower
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
    VAULT10_SCENE="$(mktemp -d)/vault10.pb"
    "${RENDER[@]}" --scene "$VAULT10_SCENE" templates_vault_10_surface_subdivisions hex_saddle diamond_torus sewn_groin sewn_funnel courses_funnel chebyshev_dome
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name hex_saddle_bottom templates_vault_10_surface_subdivisions hex_saddle
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name diamond_torus_bottom templates_vault_10_surface_subdivisions diamond_torus
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name sewn_groin_bottom templates_vault_10_surface_subdivisions sewn_groin
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name sewn_funnel_bottom templates_vault_10_surface_subdivisions sewn_funnel
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name courses_funnel_bottom templates_vault_10_surface_subdivisions courses_funnel
    "${RENDER[@]}" --scene "$VAULT10_SCENE" --view bottom --name chebyshev_dome_bottom templates_vault_10_surface_subdivisions chebyshev_dome
    VAULT11_SCENE="$(mktemp -d)/vault11.pb"
    "${RENDER[@]}" --scene "$VAULT11_SCENE" --opacity 0.78 templates_vault_11_shifted_facets shifted_hex bevelled_hex_dome shifted_groin shifted_chebyshev
    "${RENDER[@]}" --scene "$VAULT11_SCENE" --view bottom --name shifted_hex_bottom templates_vault_11_shifted_facets shifted_hex
    "${RENDER[@]}" --scene "$VAULT11_SCENE" --view bottom --name bevelled_hex_dome_bottom templates_vault_11_shifted_facets bevelled_hex_dome
    "${RENDER[@]}" --scene "$VAULT11_SCENE" --view bottom --name shifted_groin_bottom templates_vault_11_shifted_facets shifted_groin
    "${RENDER[@]}" --scene "$VAULT11_SCENE" --view bottom --name shifted_chebyshev_bottom templates_vault_11_shifted_facets shifted_chebyshev
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
