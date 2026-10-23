#pragma once
#include "wood_session.h"
#include "wood_profile.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A singly folded plate after Engel, Structure Systems, Folded plate structures (pp. 219-221): a fold profile across y carried
/// straight along x over the span, one plate per fold. Each plate carries as a slab between its folds, as a deep beam along the span
/// and with its neighbours as a truss across it. Transverse diaphragms under the folds stiffen the profile against changing its fold
/// angles, edge beams the free edges against buckling: a vertical beam beside a shallow fold, a horizontal one beside a steep fold.
///
/// Fields: the profile, the span and the member sizes; the profile, the fold lines, the mesh, the plates, the diaphragms and the
/// edge beams are in the session, each in its group.
class PrismaticFold : public WoodSession {
public:
    const Polyline profile; // The fold profile in the yz plane at x 0, running along +y, a fold at every inner point.
    const double span; // mm along x, the folds' length between the end supports.
    const double thickness; // mm, every plate and diaphragm.
    const size_t diaphragms; // Transverse diaphragms under the folds, one at each end and the rest evenly between; 0 for none.
    const double edge_beam_width; // mm, the edge beams' smaller side; 0 for none.
    const double edge_beam_depth; // mm, the edge beams' larger side, vertical beside a shallow fold, horizontal beside a steep one.

    /// The folded plate of profile over span as the session named name.
    explicit PrismaticFold(
        const Polyline& profile = zigzag(4, 1500.0, 1200.0),
        double span = 12000.0,
        double thickness = 60.0,
        size_t diaphragms = 3,
        double edge_beam_width = 200.0,
        double edge_beam_depth = 400.0,
        const std::string& name = "prismatic_fold"
    );

    /// Not copied, as a session is not.
    PrismaticFold(const PrismaticFold&) = delete;

    /// Not assigned, as it is not copied.
    PrismaticFold& operator=(const PrismaticFold&) = delete;

    /// The folded quad mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// The zigzag of ridges ridges between valleys on z 0 (p. 219), every plate fold_width across in plan and depth high.
    static Polyline zigzag(size_t ridges, double fold_width, double depth);

    /// The polygon of folds folds over width, its corners on a circular arc rising rise (p. 220): one fold a ridge, two a
    /// trapezoid, many a folded barrel approaching a cylindrical shell.
    static Polyline polygon(size_t folds, double width, double rise);

private:
    Mesh _mesh;

    /// The profile's points moved onto the plates' inner faces: every fold along its bisector, the free edges square to their plate.
    std::vector<Point> compute_inner_profile() const;

    /// One quad per profile segment between its two fold lines, facing in, so the plates grow inwards from it.
    Mesh compute_mesh() const;

    /// The diaphragms' outlines, one per run of the inner profile between two points within a plate thickness of its lowest level,
    /// closed along the lowest level.
    std::vector<Polyline> compute_diaphragm_outlines(const std::vector<Point>& inner) const;
};

inline PrismaticFold::PrismaticFold(
    const Polyline& profile,
    double span,
    double thickness,
    size_t diaphragms,
    double edge_beam_width,
    double edge_beam_depth,
    const std::string& name
)
    : WoodSession(name),
      profile(profile),
      span(span),
      thickness(thickness),
      diaphragms(diaphragms),
      edge_beam_width(edge_beam_width),
      edge_beam_depth(edge_beam_depth) {

    const Vector x(1.0, 0.0, 0.0);
    const Vector z(0.0, 0.0, 1.0);

    // profile: the fold profile at the start of the span
    add_polyline(profile, add_group("profile"));

    // folds: a fold line along the span at every profile point, ridges, valleys and the two free edges
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (size_t i = 0; i < profile.point_count(); i++)
        add_polyline(Polyline({profile[i], profile[i] + x * span}), folds);

    // mesh: one quad per profile segment between its fold lines, facing in
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per fold, mitered at the folds
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : mitered_plates(_mesh, thickness, 0.0, 0.0, 180.0))
        add(plate, plates);

    // diaphragms: under the folds at both ends and evenly between, each filling the profile's inside down to its lowest level
    const std::vector<Point> inner = compute_inner_profile();
    const std::vector<Polyline> outlines = compute_diaphragm_outlines(inner);
    const std::shared_ptr<TreeNode> stiffeners = add_group("diaphragms");

    for (size_t k = 0; k < diaphragms; k++) {
        const double along = diaphragms == 1 ? 0.5 * (span - thickness) : (span - thickness) * static_cast<double>(k) / static_cast<double>(diaphragms - 1);

        for (size_t i = 0; i < outlines.size(); i++) {
            const Polyline bottom = outlines[i].transformed(Xform::translation(along, 0.0, 0.0));
            const Polyline top = outlines[i].transformed(Xform::translation(along + thickness, 0.0, 0.0));
            add(std::make_shared<Plate>(bottom, top, fmt::format("diaphragm_{}_{}", k, i)), stiffeners);
        }
    }

    // edge beams: along both free edges under the plate, vertical beside a shallow fold, horizontal and reaching out beside a steep one
    if (edge_beam_width > 0.0 && edge_beam_depth > 0.0) {
        const std::shared_ptr<TreeNode> beams = add_group("edge_beams");
        const size_t last = profile.point_count() - 1;

        for (size_t side = 0; side < 2; side++) {
            const Point edge = inner[side == 0 ? 0 : last];
            const Vector slope = side == 0 ? profile[1] - profile[0] : profile[last - 1] - profile[last];
            const double outward = side == 0 ? -1.0 : 1.0;
            const bool steep = std::abs(slope[2]) > std::abs(slope[1]);
            const double across = steep ? edge_beam_depth : edge_beam_width;
            const double up = steep ? edge_beam_width : edge_beam_depth;
            const Point start = Point(0.0, edge[1] + outward * (steep ? 0.5 * across : 0.0), edge[2] - 0.5 * up);
            const Polyline axis({start, start + x * span});
            add(std::make_shared<Beam>(axis, profile_rectangle(across, up), std::vector<Vector>{z}, fmt::format("edge_beam_{}", side)), beams);
        }
    }
}

inline const Mesh& PrismaticFold::mesh() const {
    return _mesh;
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

    const size_t count = profile.point_count();
    std::vector<Vector> normals;

    // segment normals: the profile's direction turned to its inside, -z for a profile running along +y
    for (size_t i = 0; i + 1 < count; i++) {
        const Vector along = (profile[i + 1] - profile[i]).normalized();
        normals.push_back(Vector(0.0, along[2], -along[1]));
    }

    std::vector<Point> inner;

    for (size_t i = 0; i < count; i++) {
        if (i == 0 || i + 1 == count) {
            inner.push_back(profile[i] + normals[i == 0 ? 0 : i - 1] * thickness);
            continue;
        }

        Vector bisector = normals[i - 1] + normals[i];
        bisector.normalize_self();
        inner.push_back(profile[i] + bisector * (thickness / bisector.dot(normals[i])));
    }

    return inner;
}

inline Mesh PrismaticFold::compute_mesh() const {

    const Vector x(1.0, 0.0, 0.0);
    const size_t count = profile.point_count();
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 0; i < count; i++) {
        points.push_back(profile[i]);
        points.push_back(profile[i] + x * span);
    }

    for (size_t i = 0; i + 1 < count; i++)
        faces.push_back({2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1});

    return Mesh::from_vertices_and_faces(points, faces);
}

inline std::vector<Polyline> PrismaticFold::compute_diaphragm_outlines(const std::vector<Point>& inner) const {

    double lowest = inner[0][2];

    for (const Point& point : inner)
        lowest = std::min(lowest, point[2]);

    // runs: from one point on the lowest level to the next, each with a point above it a diaphragm
    std::vector<Polyline> outlines;
    std::vector<Point> run;

    for (const Point& point : inner) {
        const bool base = point[2] - lowest < thickness;

        if (!base && run.empty())
            continue;

        run.push_back(point);

        // a run closed along the lowest level, a free edge above it dropped onto it
        if (base && run.size() > 2) {
            std::vector<Point> outline = run;

            if (outline.back()[2] - lowest > 1.0)
                outline.push_back(Point(outline.back()[0], outline.back()[1], lowest));

            if (outline.front()[2] - lowest > 1.0)
                outline.push_back(Point(outline.front()[0], outline.front()[1], lowest));

            outline.push_back(outline.front());
            outlines.push_back(Polyline(outline));
        }

        if (base)
            run = {point};
    }

    return outlines;
}
