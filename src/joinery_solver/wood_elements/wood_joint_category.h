#pragma once

#include "pch.h"

namespace wood_session {

/// What meets in a joint: beams by their ends and sides, plates by their edges (side) and faces (top), and the two mixed.
enum class JointCategory : int {
    none,
    end_end_inplane,
    end_end_outplane,
    side_end,
    side_side,
    cross,
    border,
    side_side_inplane,
    side_side_outplane,
    side_side_rotated,
    side_top,
    top_top,
    end_top,
};

/// The family the Joinery Compendium files a joint under.
enum class JointFamily : int {
    none,
    splicing,
    corner,
    t,
    angled,
    cross_lap,
    specialty,
    edge,
    border,
    lap,
};

/// The category's name: "side_end", "end_end_outplane", ...
inline std::string_view to_string(JointCategory category) {

    static constexpr std::array<std::string_view, 13> NAMES = {
        "none", "end_end_inplane", "end_end_outplane", "side_end", "side_side", "cross", "border",
        "side_side_inplane", "side_side_outplane", "side_side_rotated", "side_top", "top_top", "end_top",
    };
    return NAMES[static_cast<size_t>(category)];
}

/// The family's name: "splicing", "corner", "t", "angled", ...
inline std::string_view to_string(JointFamily family) {

    static constexpr std::array<std::string_view, 10> NAMES = {
        "none", "splicing", "corner", "t", "angled", "cross_lap", "specialty", "edge", "border", "lap",
    };
    return NAMES[static_cast<size_t>(family)];
}

}
