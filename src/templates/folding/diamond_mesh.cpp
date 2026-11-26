#include "src/templates/folding/diamond_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
DiamondMesh::DiamondMesh(const DiamondMeshGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // surface: the NURBS surface the pattern is laid on
    add_nurbssurface(guide.surface, add_group("surface"));

    // mesh: the surface's cells split into rhombi between cell centres, two triangles each
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per triangle, mitered at the folds, its corners chamfered
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<std::shared_ptr<FacePlanarElement>> triangles =
        mitered_plates(_mesh, guide.thickness, guide.chamfer, guide.chamfer, guide.chamfer_angle);

    for (const std::shared_ptr<FacePlanarElement>& plate : triangles)
        add(plate, plates);

    // assembly: the plates of every cell across u make one row along the length, the rows put one after another from one foot over the crown to the other
    std::vector<size_t> keys;
    for (int i = 0; i < guide.u_divisions; i++)
        for (int j = 0; j < guide.v_divisions; j++)
            keys.insert(keys.end(), j == 0 ? 6 : 4, static_cast<size_t>(j));
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("row", triangles, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& DiamondMesh::mesh() const {
    return _mesh;
}

Mesh DiamondMesh::compute_mesh() const {

    const std::pair<double, double> domain_u = guide.surface.domain(0);
    const std::pair<double, double> domain_v = guide.surface.domain(1);
    const double step_u = (domain_u.second - domain_u.first) / guide.u_divisions;
    const double step_v = (domain_v.second - domain_v.first) / guide.v_divisions;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (int i = 0; i < guide.u_divisions; i++) {
        const double u = domain_u.first + i * step_u;
        const double middle = u + step_u * 0.5;

        for (int j = 0; j < guide.v_divisions; j++) {
            const double v = domain_v.first + j * step_v;
            const Point a = guide.surface.point_at(u, v);
            const Point b = guide.surface.point_at(u + step_u, v);
            const Point c = guide.surface.point_at(u, v + step_v);
            const Point d = guide.surface.point_at(u + step_u, v + step_v);
            const Point centre = guide.surface.point_at(middle, v + step_v * 0.5);
            std::vector<std::array<Point, 3>> triangles;

            if (j == 0) {
                const Point start = guide.surface.point_at(middle, domain_v.first);
                triangles.push_back({b, centre, start});
                triangles.push_back({start, centre, a});
            }

            triangles.push_back({b, d, centre});
            triangles.push_back({a, centre, c});

            if (j + 1 < guide.v_divisions) {
                const Point next = guide.surface.point_at(middle, v + step_v * 1.5);
                triangles.push_back({d, next, centre});
                triangles.push_back({next, c, centre});
            } else {
                const Point end = guide.surface.point_at(middle, domain_v.second);
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

}
