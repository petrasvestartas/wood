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
    "${RENDER[@]}" templates_folding_1_reflex_fold curves fold_planes mesh plates
    "${RENDER[@]}" templates_folding_2_diamond_mesh surface mesh plates
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = chevron ]; then
    "${RENDER[@]}" templates_chevron_1_chevron surface mesh strips edge_planes bisectors plates insertion
fi

if [ "$FAMILY" = all ] || [ "$FAMILY" = cross ]; then
    "${RENDER[@]}" templates_cross_1_vda_mesh all mesh face_planes edge_planes bisector_planes plates connector_frames connectors
    "${RENDER[@]}" --size 4800x3200 --crop 0.25,0.05,0.75,0.55 --name cross_laps templates_cross_1_vda_mesh plates
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
    "${RENDER[@]}" --opacity 0.95 --name gable_roof templates_module_3_cassette_house cassette_4+cassette_5+cassette_0+cassette_3
fi
