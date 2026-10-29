#include "pch.h"
#include "src/templates/truss/truss_graph.h"
#include "json.h"

#include <fstream>

namespace wood_truss {

// ═══════════════════════════════════════════════════════════════════════════
// TrussGraph
// ═══════════════════════════════════════════════════════════════════════════

TrussGuide TrussGraph::guide() const {

    // the nodes in mm, in the xz plane
    std::vector<Point> points;
    for (const std::array<double, 2>& node : nodes)
        points.push_back(Point(node[0] * span, 0.0, node[1] * span));

    // the chords through their nodes, the webs and posts between theirs
    std::vector<Polyline> chord_lines;
    for (const std::vector<size_t>& chord : chords) {
        std::vector<Point> chord_points;
        for (size_t node : chord)
            chord_points.push_back(points[node]);
        chord_lines.push_back(Polyline(chord_points));
    }

    std::vector<Line> web_lines;
    for (const std::array<size_t, 2>& web : webs)
        web_lines.push_back(Line::from_points(points[web[0]], points[web[1]]));

    std::vector<Line> post_lines;
    for (const std::array<size_t, 2>& post : posts)
        post_lines.push_back(Line::from_points(points[post[0]], points[post[1]]));

    const double chord_radius = *std::max_element(chord_radii.begin(), chord_radii.end());
    const double web_radius = web_radii.empty() ? chord_radius : *std::max_element(web_radii.begin(), web_radii.end());
    return TrussGuide(chord_lines, web_lines, post_lines, chord_radius, web_radius, chord_radii, web_radii, post_radii);
}

std::vector<TrussGraph> TrussGraph::library(const std::filesystem::path& json_path) {

    std::ifstream file(json_path);
    if (!file)
        return {};

    nlohmann::json data;
    file >> data;
    std::vector<TrussGraph> graphs;
    for (const nlohmann::json& entry : data["trusses"]) {
        TrussGraph graph;
        graph.name = entry["name"];
        graph.number = entry["number"];
        graph.nodes = entry["nodes"].get<std::vector<std::array<double, 2>>>();
        graph.chords = entry["chords"].get<std::vector<std::vector<size_t>>>();
        graph.webs = entry["webs"].get<std::vector<std::array<size_t, 2>>>();
        graph.posts = entry["posts"].get<std::vector<std::array<size_t, 2>>>();
        graph.span = entry["span"];
        graph.chord_radii = entry["chord_radii"].get<std::vector<double>>();
        graph.web_radii = entry["web_radii"].get<std::vector<double>>();
        graph.post_radii = entry["post_radii"].get<std::vector<double>>();
        graphs.push_back(graph);
    }

    return graphs;
}

}
