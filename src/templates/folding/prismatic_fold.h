#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A singly folded plate after Engel (pp. 219-221): a fold profile carried straight along the span, diaphragms under the folds.
class PrismaticFold : public WoodSession {
public:
    const Polyline profile; // The fold profile in the yz plane at x 0, running along +y, a fold at every inner point.
    const double span; // mm along x, the folds' length between the end supports.
    const double thickness; // mm, every plate and diaphragm.
    const size_t diaphragms; // Transverse diaphragms under the folds, one at each end and the rest evenly between; 0 for none.
    const double edge_flange; // mm, the flange folded on at each free edge; 0 for none.

private:
    Polyline _folded; // The profile with its edge flanges.
    Mesh _mesh; // The folded quad mesh, the plates' outer faces.

public:
    /// The folded plate of profile over span as the session named name.
    explicit PrismaticFold(
        const Polyline& profile = zigzag(4, 1500.0, 1200.0),
        double span = 12000.0,
        double thickness = 120.0,
        size_t diaphragms = 3,
        double edge_flange = 400.0,
        const std::string& name = "prismatic_fold"
    );

    /// Not copied, as a session is not.
    PrismaticFold(const PrismaticFold&) = delete;

    /// Not assigned, as it is not copied.
    PrismaticFold& operator=(const PrismaticFold&) = delete;

    /// The profile with its edge flanges, the cross section of every plate.
    const Polyline& folded_profile() const;

    /// The folded quad mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// The zigzag of ridges ridges between valleys on z 0 (p. 219), every plate fold_width across in plan and depth high.
    static Polyline zigzag(size_t ridges, double fold_width, double depth);

    /// The polygon of folds folds over width, its corners on a circular arc rising rise (p. 220).
    static Polyline polygon(size_t folds, double width, double rise);

private:
    /// The profile with a flange at each free edge: down beside a shallow fold, outwards beside a steep one.
    Polyline compute_folded_profile() const;

    /// The folded profile's points moved onto the plates' inner faces.
    std::vector<Point> compute_inner_profile() const;

    /// One quad per profile segment between its two fold lines, facing in, so the plates grow inwards from it.
    Mesh compute_mesh() const;

    /// The diaphragm's outline: the inner profile closed along the folded profile's lowest level.
    Polyline compute_diaphragm_outline(const std::vector<Point>& inner) const;
};

inline PrismaticFold::PrismaticFold(
    const Polyline& profile,
    double span,
    double thickness,
    size_t diaphragms,
    double edge_flange,
    const std::string& name
)
    : WoodSession(name),
      profile(profile),
      span(span),
      thickness(thickness),
      diaphragms(diaphragms),
      edge_flange(edge_flange) {

    const Vector x(1.0, 0.0, 0.0);

    // profile: the fold profile at the start of the span, a flange folded on at each free edge
    _folded = compute_folded_profile();
    add_polyline(_folded, add_group("profile"));

    // folds: a fold line along the span at every profile point, ridges, valleys, the flanges' folds and the two free edges
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (size_t i = 0; i < _folded.point_count(); i++)
        add_polyline(Polyline({_folded[i], _folded[i] + x * span}), folds);

    // mesh: one quad per profile segment between its fold lines, facing in
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per fold, mitered at the folds, its ends cut flush by the planes x 0 and x span
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    const std::vector<Plane> ends = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), x),
        Plane::from_point_normal(Point(span, 0.0, 0.0), x),
    };
    const std::vector<std::shared_ptr<Plate>> fold_plates = mitered_plates(
        _mesh,
        thickness,
        0.0,
        0.0,
        180.0,
        ends
    );

    for (const std::shared_ptr<Plate>& plate : fold_plates)
        add(plate, plates);

    // diaphragms: under the folds at both ends and evenly between, one plate each filling the profile under the plates
    const Polyline outline = compute_diaphragm_outline(compute_inner_profile());
    const std::shared_ptr<TreeNode> stiffeners = add_group("diaphragms");

    for (size_t k = 0; k < diaphragms; k++) {
        const double along = diaphragms == 1
            ? 0.5 * (span - thickness)
            : (span - thickness) * static_cast<double>(k) / static_cast<double>(diaphragms - 1);
        const Polyline bottom = outline.transformed(Xform::translation(along, 0.0, 0.0));
        const Polyline top = outline.transformed(Xform::translation(along + thickness, 0.0, 0.0));
        add(std::make_shared<Plate>(bottom, top, fmt::format("diaphragm_{}", k)), stiffeners);
    }
}

inline const Polyline& PrismaticFold::folded_profile() const {
    return _folded;
}

inline const Mesh& PrismaticFold::mesh() const {
    return _mesh;
}

inline Polyline PrismaticFold::compute_folded_profile() const {

    if (edge_flange <= 0.0)
        return profile;

    // a flange per free edge: down beside a shallow end plate, outwards along y beside a steep one
    const size_t last = profile.point_count() - 1;
    std::vector<Point> points = profile.get_points();
    const Vector start_slope = profile[1] - profile[0];
    const Vector end_slope = profile[last - 1] - profile[last];
    const bool start_steep = std::abs(start_slope[2]) > std::abs(start_slope[1]);
    const bool end_steep = std::abs(end_slope[2]) > std::abs(end_slope[1]);
    const Vector start_flange = start_steep ? Vector(0.0, -edge_flange, 0.0) : Vector(0.0, 0.0, -edge_flange);
    const Vector end_flange = end_steep ? Vector(0.0, edge_flange, 0.0) : Vector(0.0, 0.0, -edge_flange);
    points.insert(points.begin(), profile[0] + start_flange);
    points.push_back(profile[last] + end_flange);
    return Polyline(points);
}

inline Polyline PrismaticFold::zigzag(size_t ridges, double fold_width, double depth) {

    std::vector<Point> points;

    for (size_t i = 0; i <= 2 * ridges; i++)
        points.push_back(Point(0.0, fold_width * static_cast<double>(i), i % 2 == 1 ? depth : 0.0));

    return Polyline(points);
}

inline Polyline PrismaticFold::polygon(size_t folds, double width, double rise) {

    // the arch through both supports and the crown: its radius and its centre under the crown
    const double half = 0.5 * width;
    const double radius = (half * half + rise * rise) / (2.0 * rise);
    const double opening = std::asin(half / radius);
    std::vector<Point> points;

    for (size_t i = 0; i <= folds + 1; i++) {
        const double angle = -opening + 2.0 * opening * static_cast<double>(i) / static_cast<double>(folds + 1);
        points.push_back(Point(0.0, half + radius * std::sin(angle), rise - radius + radius * std::cos(angle)));
    }

    return Polyline(points);
}

inline std::vector<Point> PrismaticFold::compute_inner_profile() const {

    // normals: every segment's direction turned to the profile's inside, -z for a profile along +y
    const size_t count = _folded.point_count();
    std::vector<Vector> normals;

    for (size_t i = 0; i + 1 < count; i++) {
        const Vector along = (_folded[i + 1] - _folded[i]).normalized();
        normals.push_back(Vector(0.0, along[2], -along[1]));
    }

    // inner: every fold moved along its bisector, the free edges square to their plate
    std::vector<Point> inner;

    for (size_t i = 0; i < count; i++) {
        if (i == 0 || i + 1 == count) {
            inner.push_back(_folded[i] + normals[i == 0 ? 0 : i - 1] * thickness);
            continue;
        }

        Vector bisector = normals[i - 1] + normals[i];
        bisector.normalize_self();
        inner.push_back(_folded[i] + bisector * (thickness / bisector.dot(normals[i])));
    }

    return inner;
}

inline Mesh PrismaticFold::compute_mesh() const {

    const Vector x(1.0, 0.0, 0.0);
    const size_t count = _folded.point_count();
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 0; i < count; i++) {
        points.push_back(_folded[i]);
        points.push_back(_folded[i] + x * span);
    }

    for (size_t i = 0; i + 1 < count; i++)
        faces.push_back({2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1});

    return Mesh::from_vertices_and_faces(points, faces);
}

inline Polyline PrismaticFold::compute_diaphragm_outline(const std::vector<Point>& inner) const {

    double lowest = _folded[0][2];

    for (size_t i = 0; i < _folded.point_count(); i++)
        lowest = std::min(lowest, _folded[i][2]);

    // the inner profile, a free edge above the lowest level dropped onto it, closed along it
    std::vector<Point> points = inner;

    if (points.back()[2] - lowest > 1.0)
        points.push_back(Point(points.back()[0], points.back()[1], lowest));

    if (points.front()[2] - lowest > 1.0)
        points.insert(points.begin(), Point(points.front()[0], points.front()[1], lowest));

    points.push_back(points.front());
    return Polyline(points);
}
