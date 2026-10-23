#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A folded plate whose profile changes along the span, after Engel, Structure Systems, Folded plate structures (pp. 222-224):
/// fold profiles at stations along x, counter-running (the ridges of one where the next has its valleys), elevated or deepened in
/// the middle, their points joined station to station by straight fold lines. A strip between two fold lines is then warped, so it
/// folds along one of its diagonals into two planar triangles: through its high corners (ridge-to-ridge folding), through its low
/// corners (ridge-to-valley folding) or fold by fold the one and the other (alternate folding). A strip still planar stays one plate,
/// and two triangles that lie in one plane, as either side of the straight fold lines of the elevated centre, are one plate too.
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

    /// A zigzag of ridges ridges at x, every plate fold_width across in plan and depth high, its valleys on elevation; counter starts
    /// it on a ridge, so its ridges stand where the plain one has its valleys.
    static Polyline zigzag(double x, size_t ridges, double fold_width, double depth, double elevation, bool counter);

    /// Counter-running folding of the same depth and elevation (p. 222): the zigzag at the start, its counter profile at the end.
    static std::vector<Polyline> counter_running(size_t ridges, double fold_width, double depth, double span);

    /// The counter profile in the centre (p. 222): the zigzag at both ends, its counter profile at mid-span, the same depth and
    /// elevation, so every fold line rises or falls by the depth to the centre; folded ridge to valley the diagonals run from the
    /// edges' valleys to the centre's, where they cross in the stars of Engel's plan.
    static std::vector<Polyline> counter_centre(size_t ridges, double fold_width, double depth, double span);

    /// The centre fold elevated over the edge folds (pp. 223, 224): the zigzag at both ends starting on a ridge, and at mid-span its
    /// counter profile lifted by the depth, so its valleys stand level with the edges' ridges and the even fold lines run straight
    /// through, while the odd ones rise from the edges' valleys to the centre's ridges middle_depth higher: the same depth as the
    /// edges on p. 223, a larger one on p. 224.
    static std::vector<Polyline> elevated_centre(size_t ridges, double fold_width, double depth, double span, double middle_depth);

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

    // mesh: every strip one planar quad or two triangles on the folding's diagonal, a point's height its z; two faces in one plane
    // joined, so a fold line the folding leaves straight is no fold
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

    // plates: one plate per face, mitered at the folds; only at a node of three or more plates whose inner corners spread apart
    // the corners are cut back, by just that spread, so they do not pass through each other; the free edges cut flush by the side planes
    // y 0 and y width and by the end stations' planes
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    const double width = stations[0][stations[0].point_count() - 1][1];
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(stations.front()[0], Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(stations.back()[0], Vector(1.0, 0.0, 0.0)),
    };

    for (const std::shared_ptr<Plate>& plate : node_cut_plates(_mesh, thickness, sides))
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

inline std::vector<Polyline> CounterFold::counter_centre(size_t ridges, double fold_width, double depth, double span) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, false),
        zigzag(0.5 * span, ridges, fold_width, depth, 0.0, true),
        zigzag(span, ridges, fold_width, depth, 0.0, false),
    };
}

inline std::vector<Polyline> CounterFold::elevated_centre(size_t ridges, double fold_width, double depth, double span, double middle_depth) {
    return {
        zigzag(0.0, ridges, fold_width, depth, 0.0, true),
        zigzag(0.5 * span, ridges, fold_width, middle_depth, depth, false),
        zigzag(span, ridges, fold_width, depth, 0.0, true),
    };
}
