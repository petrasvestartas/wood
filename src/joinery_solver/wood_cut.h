#pragma once

namespace wood_cut {

/// What each outline a joint emits does to the plate body; one per entry of m_outlines / f_outlines.
enum cut_type : int {
    /// No cut.
    nothing = 0,

    /// Plate: a hole through the face.
    hole = 1,

    /// Plate: an insertion along one edge.
    edge_insertion = 2,

    /// Plate: an insertion spanning several edges.
    insert_between_multiple_edges = 3,

    /// Beam: a slice through the volume.
    slice = 4,

    /// Beam: a slice projected and sheared.
    slice_projectsheer = 5,

    /// Beam: a milled pocket.
    mill = 6,

    /// Beam: a milled pocket projected onto the volume.
    mill_project = 7,

    /// Beam: a milled pocket projected and sheared.
    mill_projectsheer = 8,

    /// Beam: a cut.
    cut = 9,

    /// Beam: a cut projected onto the volume.
    cut_project = 10,

    /// Beam: a cut projected and sheared.
    cut_projectsheer = 11,

    /// Beam: a cut with the kept side reversed.
    cut_reverse = 12,

    /// Beam: a conic cut.
    conic = 13,

    /// Beam: a conic cut with the kept side reversed.
    conic_reverse = 14,

    /// Plate or beam: a vertical drill.
    drill = 15,
};

} // namespace wood_cut
