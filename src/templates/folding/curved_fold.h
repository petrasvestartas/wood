#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folded tent roof after Knie's Zauberhut, Rapperswil (Blumer Lehmann, Carlos Martinez Architekten, 2020): a roof of 32 m
/// diameter rising in a trumpet curve into a tower 26 m high, folded into sectors whose valleys run down to the columns and whose
/// ridges end in a gable over the eaves, the folds shrinking into the tower. The curved folds are discretised in rings: every ring the
/// same star of ridge and valley points around the axis scaled by the trumpet curve's radius and lifted to its height, the fold's
/// depth and the valleys' inset scaling with it, so the two ring segments of a band between two rings are parallel and every band is a
/// planar trapezoid, a ruled strip of the curved fold; a band whose segments are not parallel would fold into two planar triangles.
/// Every plate is planar, whatever its thickness.
///
/// Fields: the plan, the curve, the fold and the thickness; the meridians, the mesh and the plates are in the session.
class CurvedFold : public WoodSession {
public:
    const size_t sectors; // Folds around the axis, a ridge and a valley each.
    const double radius; // mm, the eaves' radius at the ridges.
    const double top_radius; // mm, the round tower's radius at the top.
    const double eave; // mm, the valleys' height at the eaves, on the columns.
    const double height; // mm, the tower's top.
    const double gable; // mm, how far a ridge rises over its valleys at the eaves, the fold's depth, scaling with the radius up the tower.
    const double inset; // The valleys' radius inside the ridges' at the eaves, a fraction of the radius.
    const size_t rings; // Rings from the eaves to the top, the curve's discretisation.
    const double thickness; // mm, every plate.

    /// The curved fold as the session named name.
    explicit CurvedFold(
        size_t sectors = 12,
        double radius = 16000.0,
        double top_radius = 3500.0,
        double eave = 4000.0,
        double height = 26000.0,
        double gable = 4000.0,
        double inset = 0.1,
        size_t rings = 14,
        double thickness = 120.0,
        const std::string& name = "curved_fold"
    );

    /// Not copied, as a session is not.
    CurvedFold(const CurvedFold&) = delete;

    /// Not assigned, as it is not copied.
    CurvedFold& operator=(const CurvedFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh;

    /// Per ring from the eaves up its star of 2 sectors points, a ridge and a valley in turn: the trumpet curve's radius, flat at the
    /// eaves and vertical at the tower, its height, the fold's depth and the valleys' inset scaled with the radius.
    std::vector<std::vector<Point>> compute_rings() const;
};

inline CurvedFold::CurvedFold(
    size_t sectors,
    double radius,
    double top_radius,
    double eave,
    double height,
    double gable,
    double inset,
    size_t rings,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      sectors(sectors),
      radius(radius),
      top_radius(top_radius),
      eave(eave),
      height(height),
      gable(gable),
      inset(inset),
      rings(std::max<size_t>(rings, 2)),
      thickness(thickness) {

    // meridians: the ridge and valley curves from the eaves to the top through every ring
    const std::vector<std::vector<Point>> stars = compute_rings();
    const size_t count = 2 * sectors;
    const std::shared_ptr<TreeNode> meridians = add_group("meridians");

    for (size_t j = 0; j < count; j++) {
        std::vector<Point> curve;

        for (const std::vector<Point>& star : stars)
            curve.push_back(star[j]);

        add_polyline(Polyline(curve), meridians);
    }

    // mesh: per band between two rings and two meridians a planar quad where its ring segments are parallel, else two triangles
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (const std::vector<Point>& star : stars)
        points.insert(points.end(), star.begin(), star.end());

    for (size_t k = 0; k + 1 < stars.size(); k++)
        for (size_t j = 0; j < count; j++) {
            const size_t next = (j + 1) % count;
            const size_t a = k * count + j;
            const size_t b = k * count + next;
            const size_t c = (k + 1) * count + next;
            const size_t d = (k + 1) * count + j;
            const Vector normal = (points[d] - points[a]).cross(points[b] - points[a]).normalized();

            if (std::abs((points[c] - points[a]).dot(normal)) < 1.0) {
                faces.push_back({a, d, c, b});
                continue;
            }

            faces.push_back({a, d, c});
            faces.push_back({a, c, b});
        }

    _mesh = Mesh::from_vertices_and_faces(points, faces);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per face, mitered at the folds; only at a node of three or more plates whose inner corners spread apart the
    // corners are cut back, by just that spread
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : node_cut_plates(_mesh, thickness))
        add(plate, plates);
}

inline const Mesh& CurvedFold::mesh() const {
    return _mesh;
}

inline std::vector<std::vector<Point>> CurvedFold::compute_rings() const {

    const size_t count = 2 * sectors;
    std::vector<std::vector<Point>> stars;

    for (size_t k = 0; k < rings; k++) {
        const double t = static_cast<double>(k) / static_cast<double>(rings - 1);
        const double ring_radius = top_radius + (radius - top_radius) * (1.0 - t) * (1.0 - t);
        const double ring_height = eave + (height - eave) * t;
        const double fade = ring_radius / radius;
        std::vector<Point> star;

        for (size_t j = 0; j < count; j++) {
            const double angle = 2.0 * Tolerance::PI * static_cast<double>(j) / static_cast<double>(count);
            const bool ridge = j % 2 == 0;
            const double r = ridge ? ring_radius : ring_radius * (1.0 - inset);
            const double z = ring_height + (ridge ? gable * fade : 0.0);
            star.push_back(Point(r * std::cos(angle), r * std::sin(angle), z));
        }

        stars.push_back(star);
    }

    return stars;
}
