#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A linear structure system of folded surfaces after Engel, Structure Systems, Folded plate structures (pp. 225-227): a frame or an
/// arch, its inner line in the xz plane, the valleys of its folds, whose cross section is a fold profile running along y. At every corner of the frame line the
/// profile stands in the plane that halves the corner, its ridges offset outwards by the corner's depth along the bisector, so that
/// between two corners of one phase every ridge line is parallel to the frame line and every plate a planar quad. The depth follows
/// the moments: small at a hinge, deepest at the knees; and the phase turns at every corner, the counter profile standing where the
/// last had its valleys, so the strips warp and fold into planar triangles, the diamonds of Engel's elevations.
///
/// Fields: the frame line, the profile sizes, the depths and the folding; the frame line, the stations, the mesh, the folds and the
/// plates are in the session.
class FoldedFrame : public WoodSession {
public:
    const Polyline frame; // The frame's or arch's inner line in the xz plane, from one support to the other, the valleys of its folds.
    const size_t ridges; // Ridges of the fold profile along y.
    const double fold_width; // mm, every plate of the profile across in plan.
    const std::vector<double> depths; // mm, the fold depth at every corner of the frame line, small at a hinge.
    const bool counter; // Whether the phase turns at every corner of the frame line.
    const Folding folding; // The diagonal every warped strip folds along.
    const double thickness; // mm, every plate.

    /// The folded frame along frame as the session named name.
    explicit FoldedFrame(
        const Polyline& frame = portal(30000.0, 5000.0, 2),
        size_t ridges = 4,
        double fold_width = 2000.0,
        const std::vector<double>& depths = {300.0, 2000.0, 900.0, 3200.0, 900.0, 2000.0, 300.0},
        bool counter = true,
        Folding folding = Folding::ridge_to_ridge,
        double thickness = 480.0,
        const std::string& name = "folded_frame"
    );

    /// Not copied, as a session is not.
    FoldedFrame(const FoldedFrame&) = delete;

    /// Not assigned, as it is not copied.
    FoldedFrame& operator=(const FoldedFrame&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// The inner line of a portal frame span wide and height high, the valleys of its folds (pp. 225, 226): each column straight with a
    /// corner at mid-height, where its ridges bulge out, and the ceiling straight with roof_segments corners, where the roof's ridges rise.
    static Polyline portal(double span, double height, size_t roof_segments);

    /// The frame line of an arch span wide and rise high, its corners on a circular arc, segments of them (p. 227).
    static Polyline arch(double span, double rise, size_t segments);

private:
    Mesh _mesh;

    /// Per corner of the frame line its station: the profile in the corner's bisector plane, the valleys on the frame line, the ridges
    /// offset outwards along the bisector by the depth over the cosine of the half corner, and the points' heights out of the fold.
    std::pair<std::vector<Polyline>, std::vector<std::vector<double>>> compute_stations() const;
};

inline FoldedFrame::FoldedFrame(
    const Polyline& frame,
    size_t ridges,
    double fold_width,
    const std::vector<double>& depths,
    bool counter,
    Folding folding,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      frame(frame),
      ridges(ridges),
      fold_width(fold_width),
      depths(depths),
      counter(counter),
      folding(folding),
      thickness(thickness) {

    // frame: the frame or arch line at the start of the fold
    add_polyline(frame, add_group("frame"));

    // stations: the fold profile at every corner of the frame line, in its bisector plane, the phase turning corner by corner
    const std::pair<std::vector<Polyline>, std::vector<std::vector<double>>> stations = compute_stations();
    const std::shared_ptr<TreeNode> profiles = add_group("stations");

    for (const Polyline& station : stations.first)
        add_polyline(station, profiles);

    // mesh: between two corners of one phase planar quads, else two triangles per strip on the folding's diagonal
    _mesh = fold_strips(stations.first, stations.second, folding);
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face, mitered at the folds, every corner cut back by the thickness on both faces, as the reflex fold
    // does, so the plates meeting at a vertex do not pass through each other
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, thickness, thickness, 180.0))
        add(plate, plates);
}

inline const Mesh& FoldedFrame::mesh() const {
    return _mesh;
}

inline Polyline FoldedFrame::portal(double span, double height, size_t roof_segments) {

    std::vector<Point> points = {Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 0.5 * height)};

    for (size_t i = 0; i <= roof_segments; i++)
        points.push_back(Point(span * static_cast<double>(i) / static_cast<double>(roof_segments), 0.0, height));

    points.push_back(Point(span, 0.0, 0.5 * height));
    points.push_back(Point(span, 0.0, 0.0));
    return Polyline(points);
}

inline Polyline FoldedFrame::arch(double span, double rise, size_t segments) {

    // the arch through both supports and the crown: its radius and its centre under the crown
    const double half = 0.5 * span;
    const double radius = (half * half + rise * rise) / (2.0 * rise);
    const double opening = std::atan2(half, radius - rise);
    std::vector<Point> points;

    for (size_t i = 0; i <= segments; i++) {
        const double angle = -opening + 2.0 * opening * static_cast<double>(i) / static_cast<double>(segments);
        points.push_back(Point(half + radius * std::sin(angle), 0.0, rise - radius + radius * std::cos(angle)));
    }

    return Polyline(points);
}

inline std::pair<std::vector<Polyline>, std::vector<std::vector<double>>> FoldedFrame::compute_stations() const {

    const size_t count = frame.point_count();
    const Vector y(0.0, 1.0, 0.0);
    std::vector<Vector> normals;

    // segment normals: the frame line's direction turned outwards, to its left in the xz plane
    for (size_t k = 0; k + 1 < count; k++) {
        const Vector along = (frame[k + 1] - frame[k]).normalized();
        normals.push_back(Vector(-along[2], 0.0, along[0]));
    }

    std::vector<Polyline> stations;
    std::vector<std::vector<double>> heights;

    for (size_t k = 0; k < count; k++) {
        const Vector before = normals[k == 0 ? 0 : k - 1];
        const Vector after = normals[k + 1 == count ? k - 1 : k];
        Vector bisector = before + after;
        bisector.normalize_self();
        const double miter = 1.0 / bisector.dot(after);
        const bool turned = counter && k % 2 == 1;
        std::vector<Point> points;
        heights.push_back({});

        for (size_t i = 0; i <= 2 * ridges; i++) {
            const double height = (i % 2 == 1) != turned ? depths[k] : 0.0;
            points.push_back(frame[k] + y * (fold_width * static_cast<double>(i)) + bisector * (height * miter));
            heights.back().push_back(height);
        }

        stations.push_back(Polyline(points));
    }

    return {stations, heights};
}
