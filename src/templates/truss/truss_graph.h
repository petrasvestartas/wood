#pragma once
#include "src/templates/truss/truss_guide.h"

#include <filesystem>

using namespace session_cpp;
using namespace wood_session;

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGraph
// ═══════════════════════════════════════════════════════════════════════════

/// A truss as a graph: its nodes in spans, x along the span from the left support and z up, and the members between them.
/// Any truss is one topology of this kind; the Truss template makes its members and their cuts from the guide.
class TrussGraph {
public:
    std::string name; // Scene name, e.g. bow_64.
    std::string number; // Number in its source, e.g. 64 or 200½.
    std::vector<std::array<double, 2>> nodes; // x, z in spans: the supports at (0, 0) and (1, z).
    std::vector<std::vector<size_t>> chords; // Each chord through its nodes, end to end.
    std::vector<std::array<size_t, 2>> webs; // Each web between two nodes.
    std::vector<std::array<size_t, 2>> posts; // Each post between two nodes.
    double span = 12000.0; // mm, support to support.
    std::vector<double> chord_radii; // mm, half the square section of each chord.
    std::vector<double> web_radii; // mm, of each web.
    std::vector<double> post_radii; // mm, of each post.
    std::vector<std::array<size_t, 2>> runs_on; // Chord ends (chord, 0 start or 1 end) running on where chords end together.
    std::vector<int> chord_levels; // 1 primary, 2 secondary, 3 tertiary, per chord.
    std::vector<int> web_levels; // Per web.
    std::vector<int> post_levels; // Per post.

    /// The guide of this graph at its span, each member with its own section.
    TrussGuide guide() const;

    /// Every graph of a library file such as data/templates/truss_bow_1873.json, an empty list when the file is missing.
    static std::vector<TrussGraph> library(const std::filesystem::path& json_path);
};

}
