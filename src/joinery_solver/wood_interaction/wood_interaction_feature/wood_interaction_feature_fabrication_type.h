#pragma once

namespace wood_session {

/// What each outline a feature emits does to the element body when it is fabricated; one per entry of male_outlines / female_outlines.
enum FabricationType : int {
    nothing = 0,
    hole = 1,
    edge_insertion = 2,
    insert_between_multiple_edges = 3,
    slice = 4,
    slice_projectsheer = 5,
    mill = 6,
    mill_project = 7,
    mill_projectsheer = 8,
    cut = 9,
    cut_project = 10,
    cut_projectsheer = 11,
    cut_reverse = 12,
    conic = 13,
    conic_reverse = 14,
    drill = 15,
    drill_50 = 16,
    drill_10 = 17,
};

/// Whether a type is bored as a drill: drill and the two named bits of 2024.
inline bool is_drill(const int type) {
    return type == FabricationType::drill || type == FabricationType::drill_50 ||
           type == FabricationType::drill_10;
}

}
