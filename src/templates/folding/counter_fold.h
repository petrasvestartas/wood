#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A folded plate whose profile changes along the span, every warped strip folded on one diagonal.
class CounterFold : public WoodSession {
public:
    const std::vector<Polyline>
        stations; // The fold profiles along +x, each in a yz plane along +y, all with one point count.
    const Folding folding; // The diagonal every warped strip folds along.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The fold through stations as the session named name.
    explicit CounterFold(
        const std::vector<Polyline>& stations = counter_running(4, 1500.0, 1500.0, 13500.0),
        Folding folding = Folding::ridge_to_ridge,
        double thickness = 120.0,
        const std::string& name = "counter_fold"
    );

    /// Not copied, as a session is not.
    CounterFold(const CounterFold&) = delete;

    /// Not assigned, as it is not copied.
    CounterFold& operator=(const CounterFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// A zigzag of ridges ridges at x, its valleys on elevation, counter starting it on a ridge.
    static Polyline zigzag(
        double x, size_t ridges, double fold_width, double depth, double elevation, bool counter
    );

    /// Counter-running folding: the zigzag at the start, its counter profile at the end.
    static std::vector<Polyline>
    counter_running(size_t ridges, double fold_width, double depth, double span);

    /// The counter profile in the centre: the zigzag at both ends, its counter profile at mid-span.
    static std::vector<Polyline>
    counter_centre(size_t ridges, double fold_width, double depth, double span);

    /// The centre fold elevated over the edge folds, its ridges middle_depth over the edges' ridges.
    static std::vector<Polyline> elevated_centre(
        size_t ridges, double fold_width, double depth, double span, double middle_depth
    );
};

inline CounterFold::CounterFold(
    const std::vector<Polyline>& stations,
    Folding folding,
    double thickness,
    const std::string& name
)
    : WoodSession(name), stations(stations), folding(folding), thickness(thickness) {

    // stations: the fold profile at every station along the span
    const std::shared_ptr<TreeNode> profiles = add_group("stations");

    for (const Polyline& station : stations)
        add_polyline(station, profiles);

    // mesh: every strip one planar quad or two triangles on the folding's diagonal, two faces in one plane joined
    std::vector<std::vector<double>> heights;

    for (const Polyline& station : stations) {
        heights.push_back({});

        for (size_t i = 0; i < station.point_count(); i++)
            heights.back().push_back(station[i][2]);
    }

    _mesh = merge_coplanar(fold_strips(stations, heights, folding));
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh, the fold lines between stations and the diagonals
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the free edges cut flush by the side planes and the end stations' planes
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const double width = stations[0][stations[0].point_count() - 1][1];
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(stations.front()[0], Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(stations.back()[0], Vector(1.0, 0.0, 0.0)),
    };

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& CounterFold::mesh() const {
    return _mesh;
}

inline Polyline CounterFold::zigzag(
    double x, size_t ridges, double fold_width, double depth, double elevation, bool counter
) {

    std::vector<Point> points;

    for (size_t i = 0; i <= 2 * ridges; i++) {
        const bool ridge = (i % 2 == 1) != counter;
        points.push_back(
            Point(x, fold_width * static_cast<double>(i), elevation + (ridge ? depth : 0.0))
        );
    }

    return Polyline(points);
}

inline std::vector<Polyline>
CounterFold::counter_running(size_t ridges, double fold_width, double depth, double span) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, false),
        zigzag(span, ridges, fold_width, depth, 0.0, true),
    };
}

inline std::vector<Polyline>
CounterFold::counter_centre(size_t ridges, double fold_width, double depth, double span) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, false),
        zigzag(0.5 * span, ridges, fold_width, depth, 0.0, true),
        zigzag(span, ridges, fold_width, depth, 0.0, false),
    };
}

inline std::vector<Polyline> CounterFold::elevated_centre(
    size_t ridges, double fold_width, double depth, double span, double middle_depth
) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, true),
        zigzag(0.5 * span, ridges, fold_width, middle_depth, depth, false),
        zigzag(span, ridges, fold_width, depth, 0.0, true),
    };
}
