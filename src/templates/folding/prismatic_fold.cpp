#include "src/templates/folding/prismatic_fold.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
PrismaticFold::PrismaticFold(const PrismaticFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    const Vector x(1.0, 0.0, 0.0);

    // profile: the fold profile at the start of the span, a flange folded on at each free edge
    _folded = compute_folded_profile();
    add_polyline(_folded, add_group("profile"));

    // folds: a fold line along the span at every profile point, ridges, valleys, the flanges' folds and the two free edges
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (size_t i = 0; i < _folded.point_count(); i++)
        add_polyline(Polyline({_folded[i], _folded[i] + x * guide.span}), folds);

    // mesh: one quad per profile segment between its fold lines, facing in
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per fold, mitered at the folds, its ends cut flush by the planes x 0 and x span
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    const std::vector<Plane> ends = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), x),
        Plane::from_point_normal(Point(guide.span, 0.0, 0.0), x),
    };
    const std::vector<std::shared_ptr<FacePlanarElement>> fold_plates =
        mitered_plates(_mesh, guide.thickness, 0.0, 0.0, 180.0, ends);

    for (const std::shared_ptr<FacePlanarElement>& plate : fold_plates)
        add(plate, plates);

    // diaphragms: under the folds at both ends and evenly between, one plate each filling the profile under the plates
    const Polyline outline = compute_diaphragm_outline(compute_inner_profile());
    const std::shared_ptr<TreeNode> stiffeners = add_group("diaphragms");

    for (size_t k = 0; k < guide.diaphragms; k++) {
        const double along = guide.diaphragms == 1
                                 ? 0.5 * (guide.span - guide.thickness)
                                 : (guide.span - guide.thickness) * static_cast<double>(k) /
                                       static_cast<double>(guide.diaphragms - 1);
        const Polyline bottom = outline.transformed(Xform::translation(along, 0.0, 0.0));
        const Polyline top =
            outline.transformed(Xform::translation(along + guide.thickness, 0.0, 0.0));
        add(std::make_shared<FacePlanarElement>(bottom, top, fmt::format("diaphragm_{}", k)),
            stiffeners);
    }

    // assembly: the diaphragms stand, the plates, each a row along the length, put on them one after another from one side to the other
    std::vector<AssemblyModule> whole =
        AssemblyModule::parts_of(get_elements_numbered<FacePlanarElement>("diaphragm"));
    const std::vector<AssemblyModule> rows =
        AssemblyModule::parts_of(fold_plates, {Vector(0.0, 0.0, -1.0)});
    whole.insert(whole.end(), rows.begin(), rows.end());
    assemblies = {AssemblyModule::module(name, whole)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Polyline& PrismaticFold::folded_profile() const {
    return _folded;
}

const Mesh& PrismaticFold::mesh() const {
    return _mesh;
}

Polyline PrismaticFold::compute_folded_profile() const {

    if (guide.edge_flange <= 0.0 || guide.profile.point_count() < 2)
        return guide.profile;

    // a flange per free edge: down beside a shallow end plate, outwards along y beside a steep one
    const size_t last = guide.profile.point_count() - 1;
    std::vector<Point> points = guide.profile.get_points();
    const Vector start_slope = guide.profile[1] - guide.profile[0];
    const Vector end_slope = guide.profile[last - 1] - guide.profile[last];
    const bool start_steep = std::abs(start_slope[2]) > std::abs(start_slope[1]);
    const bool end_steep = std::abs(end_slope[2]) > std::abs(end_slope[1]);
    const Vector start_flange =
        start_steep ? Vector(0.0, -guide.edge_flange, 0.0) : Vector(0.0, 0.0, -guide.edge_flange);
    const Vector end_flange =
        end_steep ? Vector(0.0, guide.edge_flange, 0.0) : Vector(0.0, 0.0, -guide.edge_flange);
    points.insert(points.begin(), guide.profile[0] + start_flange);
    points.push_back(guide.profile[last] + end_flange);
    return Polyline(points);
}

std::vector<Point> PrismaticFold::compute_inner_profile() const {

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
            inner.push_back(_folded[i] + normals[i == 0 ? 0 : i - 1] * guide.thickness);
            continue;
        }

        Vector bisector = normals[i - 1] + normals[i];
        bisector.normalize_self();
        inner.push_back(_folded[i] + bisector * (guide.thickness / bisector.dot(normals[i])));
    }

    return inner;
}

Mesh PrismaticFold::compute_mesh() const {

    const Vector x(1.0, 0.0, 0.0);
    const size_t count = _folded.point_count();
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 0; i < count; i++) {
        points.push_back(_folded[i]);
        points.push_back(_folded[i] + x * guide.span);
    }

    for (size_t i = 0; i + 1 < count; i++)
        faces.push_back({2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1});

    return Mesh::from_vertices_and_faces(points, faces);
}

Polyline PrismaticFold::compute_diaphragm_outline(const std::vector<Point>& inner) const {

    double lowest = _folded[0][2];

    for (size_t i = 0; i < _folded.point_count(); i++)
        lowest = std::min(lowest, _folded[i][2]);

    std::vector<Point> points = inner;

    if (points.back()[2] - lowest > 1.0)
        points.push_back(Point(points.back()[0], points.back()[1], lowest));

    if (points.front()[2] - lowest > 1.0)
        points.insert(points.begin(), Point(points.front()[0], points.front()[1], lowest));

    points.push_back(points.front());
    return Polyline(points);
}

}
