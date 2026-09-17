#pragma once

namespace wood_cut {

/// What each outline a joint emits does to the plate body; one per entry of m_outlines / f_outlines.
enum cut_type : int {
    nothing                       = 0,

    // Plates (faces, top/bottom + edges)
    hole                          = 1,
    edge_insertion                = 2,
    insert_between_multiple_edges = 3,

    // Beams (always projected or inside volume)
    slice                         = 4,
    slice_projectsheer            = 5,
    mill                          = 6,
    mill_project                  = 7,
    mill_projectsheer             = 8,
    cut                           = 9,
    cut_project                   = 10,
    cut_projectsheer              = 11,
    cut_reverse                   = 12,
    conic                         = 13,
    conic_reverse                 = 14,

    // Vertical drill (plates & beams)
    drill                         = 15,
};

} // namespace wood_cut
