#include "src/templates/folding/folded_frame.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
FoldedFrame::FoldedFrame(const FoldedFrameGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // zigzag: the fold line of half a bay from the foot to the crown
    add_polyline(guide.zigzag, add_group("zigzag"));

    // mesh: one planar triangle per point of the zigzag with the last valley and the last ridge, mirrored and repeated bay by bay
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the free edges cut flush by the side planes, the feet by the floor
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const double width = guide.bay * static_cast<double>(guide.bays);
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0)),
    };

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, sides))
        add(plate, plates);

    // assembly: the lower and the upper half of the zigzag's triangles make a row each along the bays, one row when they are three,
    // the crown plates with the first side; the rows put one after another from one foot over the crown to the other
    const std::vector<std::array<Point, 3>> triangles = compute_triangles();
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    const size_t rows = triangles.size() < 4 ? 1 : 2;
    const double middle = guide.zigzag[guide.zigzag.point_count() - 1][0];
    std::vector<size_t> keys;
    for (const std::shared_ptr<FacePlanarElement>& plate : placed) {
        const Point centre = plate->polylines[0].center();
        const double across = middle - std::abs(centre[0] - middle);
        size_t nearest = 0;
        double nearest_distance = std::numeric_limits<double>::max();
        for (size_t t = 0; t < triangles.size(); t++) {
            const double x = (triangles[t][0][0] + triangles[t][1][0] + triangles[t][2][0]) / 3.0;
            const double z = (triangles[t][0][2] + triangles[t][1][2] + triangles[t][2][2]) / 3.0;
            const double distance = std::hypot(across - x, centre[2] - z);
            if (distance < nearest_distance) {
                nearest = t;
                nearest_distance = distance;
            }
        }
        const size_t row = nearest * rows / triangles.size();
        keys.push_back(centre[0] < middle + 1.0 ? row : 2 * rows - 1 - row);
    }
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("row", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& FoldedFrame::mesh() const {
    return _mesh;
}

std::vector<std::array<Point, 3>> FoldedFrame::compute_triangles() const {

    std::vector<std::array<Point, 3>> triangles;
    std::optional<Point> valley;
    std::optional<Point> ridge;

    for (size_t i = 0; i < guide.zigzag.point_count(); i++) {
        const Point& point = guide.zigzag[i];

        if (valley && ridge)
            triangles.push_back({*valley, *ridge, point});

        if (std::abs(point[1] - 0.5 * guide.bay) < std::abs(point[1]))
            ridge = point;
        else
            valley = point;
    }

    return triangles;
}

Mesh FoldedFrame::compute_mesh() const {

    const std::vector<std::array<Point, 3>> triangles = compute_triangles();
    const double middle = guide.zigzag[guide.zigzag.point_count() - 1][0];
    double top = 0.0;

    for (size_t i = 0; i < guide.zigzag.point_count(); i++)
        top = std::max(top, guide.zigzag[i][2]);

    // faces: the triangles mirrored over mid-span and the bay's centre, bay by bay, facing away from the inside
    FoldMesh fold;
    std::vector<std::vector<size_t>> faces;

    for (size_t b = 0; b < guide.bays; b++)
        for (const bool over_span : {false, true})
            for (const bool over_bay : {false, true}) {
                const double y0 = guide.bay * static_cast<double>(b);
                const Point inside(middle, y0 + 0.5 * guide.bay, 0.5 * top);

                for (const std::array<Point, 3>& triangle : triangles) {
                    std::array<Point, 3> placed;
                    std::vector<size_t> face;

                    for (size_t k = 0; k < 3; k++) {
                        const double x = over_span ? 2.0 * middle - triangle[k][0] : triangle[k][0];
                        const double y =
                            over_bay ? y0 + guide.bay - triangle[k][1] : y0 + triangle[k][1];
                        placed[k] = Point(x, y, triangle[k][2]);
                        face.push_back(fold.add(placed[k]));
                    }

                    const Vector normal = (placed[1] - placed[0]).cross(placed[2] - placed[0]);
                    const double centroid_x = (placed[0][0] + placed[1][0] + placed[2][0]) / 3.0;
                    const double centroid_y = (placed[0][1] + placed[1][1] + placed[2][1]) / 3.0;
                    const double centroid_z = (placed[0][2] + placed[1][2] + placed[2][2]) / 3.0;
                    const Point centroid(centroid_x, centroid_y, centroid_z);

                    if (normal.dot(centroid - inside) < 0.0)
                        std::swap(face[1], face[2]);

                    faces.push_back(face);
                }
            }

    // neighbours wound alike: the first face's side, where a fold is so sharp that the inside tells its two faces apart
    Mesh mesh = Mesh::from_vertices_and_faces(fold.points(), faces);
    mesh.unify_winding();

    return mesh;
}

}
