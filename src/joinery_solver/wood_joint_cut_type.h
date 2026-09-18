#pragma once

namespace wood_session {

/// What each outline a joint emits does to the plate body; one per entry of male_outlines / female_outlines.
enum CutType : int {
    nothing = 0, // No cut.
    hole = 1, // Plate: a hole through the face.
    edge_insertion = 2, // Plate: an insertion along one edge.
    insert_between_multiple_edges = 3, // Plate: an insertion spanning several edges.
    slice = 4, // Beam: a slice through the volume.
    slice_projectsheer = 5, // Beam: a slice projected and sheared.
    mill = 6, // Beam: a milled pocket.
    mill_project = 7, // Beam: a milled pocket projected onto the volume.
    mill_projectsheer = 8, // Beam: a milled pocket projected and sheared.
    cut = 9, // Beam: a cut.
    cut_project = 10, // Beam: a cut projected onto the volume.
    cut_projectsheer = 11, // Beam: a cut projected and sheared.
    cut_reverse = 12, // Beam: a cut with the kept side reversed.
    conic = 13, // Beam: a conic cut.
    conic_reverse = 14, // Beam: a conic cut with the kept side reversed.
    drill = 15, // Plate or beam: a vertical drill.
};

} // namespace wood_session
