#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A diamond shell: a NURBS surface split into rhombi of two triangles each, one plate per triangle.
///
/// Fields: the surface, its divisions and the plate sizes; the surface, the mesh and the plates are in the session.
class DiamondMesh : public WoodSession {
public:
    const NurbsSurface surface; // The surface the pattern is laid on.
    const int u_divisions; // Cells across u.
    const int v_divisions; // Cells along v.
    const double thickness; // mm, every plate.
    const double chamfer; // mm cut back at the corners of both outlines.
    const double chamfer_angle; // Degrees, corners sharper than this are chamfered.

    /// The diamond pattern on surface as the session named name.
    explicit DiamondMesh(
        const NurbsSurface& surface = default_surface(),
        int u_divisions = 8,
        int v_divisions = 4,
        double thickness = 10.0,
        double chamfer = 1.0,
        double chamfer_angle = 180.0,
        const std::string& name = "diamond_mesh"
    );

    /// Not copied, as a session is not.
    DiamondMesh(const DiamondMesh&) = delete;

    /// Not assigned, as it is not copied.
    DiamondMesh& operator=(const DiamondMesh&) = delete;

    /// The welded triangle mesh, one face per plate.
    const Mesh& mesh() const;

    /// A bicubic arch, 3000 across, 5000 along v and 1500 high.
    static NurbsSurface default_surface();

private:
    Mesh _mesh;

    /// Per cell four triangles around its centre to the next cell's centre, six on the first row, welded.
    Mesh compute_mesh() const;
};

inline DiamondMesh::DiamondMesh(
    const NurbsSurface& surface,
    int u_divisions,
    int v_divisions,
    double thickness,
    double chamfer,
    double chamfer_angle,
    const std::string& name
)
    : WoodSession(name),
      surface(surface),
      u_divisions(std::max(u_divisions, 1)),
      v_divisions(std::max(v_divisions, 1)),
      thickness(thickness),
      chamfer(chamfer),
      chamfer_angle(chamfer_angle) {

    // surface: the NURBS surface the pattern is laid on
    add_nurbssurface(surface, add_group("surface"));

    // mesh: the surface's cells split into rhombi between cell centres, two triangles each
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per triangle, mitred at the folds, its corners chamfered
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<std::shared_ptr<Plate>> triangles = mitred_plates(
        _mesh,
        thickness,
        chamfer,
        chamfer,
        chamfer_angle
    );

    for (const std::shared_ptr<Plate>& plate : triangles)
        add(plate, plates);
}

inline const Mesh& DiamondMesh::mesh() const {
    return _mesh;
}

inline NurbsSurface DiamondMesh::default_surface() {

    const double width = 3000.0;
    const double length = 5000.0;
    const double height = 1500.0;
    std::vector<Point> points;

    for (int i = 0; i < 4; i++) {
        const double z = (i == 1 || i == 2) ? height * 4.0 / 3.0 : 0.0;

        for (int j = 0; j < 4; j++)
            points.push_back({width * i / 3.0, length * j / 3.0, z});
    }

    NurbsSurface arch = NurbsSurface::create(
        false,
        false,
        3,
        3,
        4,
        4,
        points
    );
    arch.set_domain(0, 0.0, width);
    arch.set_domain(1, 0.0, length);
    arch.transpose();
    return arch;
}

inline Mesh DiamondMesh::compute_mesh() const {

    const std::pair<double, double> domain_u = surface.domain(0);
    const std::pair<double, double> domain_v = surface.domain(1);
    const double step_u = (domain_u.second - domain_u.first) / u_divisions;
    const double step_v = (domain_v.second - domain_v.first) / v_divisions;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (int i = 0; i < u_divisions; i++) {
        const double u = domain_u.first + i * step_u;
        const double middle = u + step_u * 0.5;

        for (int j = 0; j < v_divisions; j++) {
            const double v = domain_v.first + j * step_v;
            const Point a = surface.point_at(u, v);
            const Point b = surface.point_at(u + step_u, v);
            const Point c = surface.point_at(u, v + step_v);
            const Point d = surface.point_at(u + step_u, v + step_v);
            const Point centre = surface.point_at(middle, v + step_v * 0.5);
            std::vector<std::array<Point, 3>> triangles;

            if (j == 0) {
                const Point start = surface.point_at(middle, domain_v.first);
                triangles.push_back({b, centre, start});
                triangles.push_back({start, centre, a});
            }

            triangles.push_back({b, d, centre});
            triangles.push_back({a, centre, c});

            if (j + 1 < v_divisions) {
                const Point next = surface.point_at(middle, v + step_v * 1.5);
                triangles.push_back({d, next, centre});
                triangles.push_back({next, c, centre});
            } else {
                const Point end = surface.point_at(middle, domain_v.second);
                triangles.push_back({d, end, centre});
                triangles.push_back({end, c, centre});
            }

            for (const std::array<Point, 3>& triangle : triangles) {
                const size_t start = points.size();
                faces.push_back({start, start + 1, start + 2});
                points.insert(points.end(), triangle.begin(), triangle.end());
            }
        }
    }

    const Mesh triangles = Mesh::from_vertices_and_faces(points, faces);
    return triangles.weld(0.01);
}
