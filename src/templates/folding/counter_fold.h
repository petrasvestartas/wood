#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A folded plate whose profile changes along the span, after Engel, Structure Systems, Folded plate structures (pp. 222-224):
/// fold profiles at stations along x, counter-running (the ridges of one where the next has its valleys), raised or deepened towards
/// the middle, their points joined station to station by straight fold lines. A strip between two fold lines is then warped, so it
/// folds along one of its diagonals into two planar triangles: through its high corners (ridge-to-ridge folding), through its low
/// corners (ridge-to-valley folding) or the one and the other strip by strip (alternate folding). A strip still planar stays one plate.
/// Every face is planar, so every plate is, whatever its thickness.
///
/// Fields: the stations, the folding and the thickness; the stations, the fold lines, the mesh and the plates are in the session.
class CounterFold : public WoodSession {
public:
    const std::vector<Polyline> stations; // The fold profiles along +x, each in a yz plane running along +y, all with one point count.
    const Folding folding; // The diagonal every warped strip folds along.
    const double thickness; // mm, every plate.

    /// The fold through stations as the session named name.
    explicit CounterFold(
        const std::vector<Polyline>& stations = counter_running(5, 1500.0, 1500.0, 18000.0),
        Folding folding = Folding::ridge_to_ridge,
        double thickness = 60.0,
        const std::string& name = "counter_fold"
    );

    /// Not copied, as a session is not.
    CounterFold(const CounterFold&) = delete;

    /// Not assigned, as it is not copied.
    CounterFold& operator=(const CounterFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// A zigzag of ridges ridges at x, every plate fold_width across in plan and depth high, its valleys on elevation; counter starts
    /// it on a ridge, so its ridges stand where the plain one has its valleys.
    static Polyline zigzag(double x, size_t ridges, double fold_width, double depth, double elevation, bool counter);

    /// Counter-running folding of the same depth and elevation (p. 222): the zigzag at the start, its counter profile at the end.
    static std::vector<Polyline> counter_running(size_t ridges, double fold_width, double depth, double span);

    /// The counter profile in the middle (pp. 222, 223): the zigzag at both ends, its counter profile at mid-span rise higher and
    /// middle_depth deep; rise 0 and middle_depth depth is the counter profile in centre, rise above 0 the centre fold elevated.
    static std::vector<Polyline> counter_centre(size_t ridges, double fold_width, double depth, double span, double rise, double middle_depth);

    /// Conical folding (p. 224): one zigzag throughout, its valleys level, its ridges cut by sloping planes, depth at both ends and
    /// middle_depth at mid-span.
    static std::vector<Polyline> conical(size_t ridges, double fold_width, double depth, double span, double middle_depth);

private:
    Mesh _mesh;

};

inline CounterFold::CounterFold(
    const std::vector<Polyline>& stations,
    Folding folding,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      stations(stations),
      folding(folding),
      thickness(thickness) {

    // stations: the fold profile at every station along the span
    const std::shared_ptr<TreeNode> profiles = add_group("stations");

    for (const Polyline& station : stations)
        add_polyline(station, profiles);

    // mesh: every strip one planar quad or two triangles on the folding's diagonal, a point's height its z
    std::vector<std::vector<double>> heights;

    for (const Polyline& station : stations) {
        heights.push_back({});

        for (size_t i = 0; i < station.point_count(); i++)
            heights.back().push_back(station[i][2]);
    }

    _mesh = fold_strips(stations, heights, folding);
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh, the fold lines between stations and the diagonals
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face, mitered at the folds
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, 0.0, 0.0, 180.0))
        add(plate, plates);
}

inline const Mesh& CounterFold::mesh() const {
    return _mesh;
}

inline Polyline CounterFold::zigzag(double x, size_t ridges, double fold_width, double depth, double elevation, bool counter) {

    std::vector<Point> points;

    for (size_t i = 0; i <= 2 * ridges; i++)
        points.push_back(Point(x, fold_width * static_cast<double>(i), elevation + ((i % 2 == 1) != counter ? depth : 0.0)));

    return Polyline(points);
}

inline std::vector<Polyline> CounterFold::counter_running(size_t ridges, double fold_width, double depth, double span) {
    return {zigzag(0.0, ridges, fold_width, depth, 0.0, false), zigzag(span, ridges, fold_width, depth, 0.0, true)};
}

inline std::vector<Polyline> CounterFold::counter_centre(size_t ridges, double fold_width, double depth, double span, double rise, double middle_depth) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, false),
        zigzag(0.5 * span, ridges, fold_width, middle_depth, rise, true),
        zigzag(span, ridges, fold_width, depth, 0.0, false),
    };
}

inline std::vector<Polyline> CounterFold::conical(size_t ridges, double fold_width, double depth, double span, double middle_depth) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, false),
        zigzag(0.5 * span, ridges, fold_width, middle_depth, 0.0, false),
        zigzag(span, ridges, fold_width, depth, 0.0, false),
    };
}
