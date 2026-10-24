#include "pch.h"
#include "wood_element_plate_curved.h"
#include "primitives.h"
#include <map>

namespace wood_session {

/// The curve with its domain set to [0, 1], the parametrisation the ruled patches between the curves use.
static NurbsCurve unit_domain(const NurbsCurve& curve) {

    NurbsCurve unit = curve;
    unit.set_domain(0.0, 1.0);

    return unit;
}

/// The straight segment between two points as a degree one curve.
static NurbsCurve segment(const Point& from, const Point& to) {
    return NurbsCurve::create(false, 1, {from, to});
}

/// The index of the solid's vertex nearest the point: the corner of a patch found among the eight corners of the plate.
static int corner_vertex(const BRep& solid, const Point& point) {

    int nearest = 0;

    for (int i = 1; i < static_cast<int>(solid.m_vertices.size()); i++)
        if (solid.m_vertices[i].point.distance(point) <
            solid.m_vertices[nearest].point.distance(point))
            nearest = i;

    return nearest;
}

/// The edge between two of the solid's vertices, in the table keyed by the smaller then the larger vertex index.
static int edge_between(const std::map<std::pair<int, int>, int>& edges, int a, int b) {
    return edges.at({std::min(a, b), std::max(a, b)});
}

/// A face on the patch's whole domain.
static int patch_face(
    BRep& solid, const NurbsSurface& patch, const std::map<std::pair<int, int>, int>& edges
) {

    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);
    const std::array<Point, 4> uv = {
        Point(du.first, dv.first, 0.0),
        Point(du.second, dv.first, 0.0),
        Point(du.second, dv.second, 0.0),
        Point(du.first, dv.second, 0.0),
    };
    const std::array<int, 4> corners = {
        corner_vertex(solid, patch.point_at_corner(0, 0)),
        corner_vertex(solid, patch.point_at_corner(1, 0)),
        corner_vertex(solid, patch.point_at_corner(1, 1)),
        corner_vertex(solid, patch.point_at_corner(0, 1)),
    };
    const int surface = solid.add_surface(patch);
    std::vector<BRepRef> wire;

    for (size_t k = 0; k < 4; k++) {
        const size_t next = (k + 1) % 4;
        const int edge = edge_between(edges, corners[k], corners[next]);
        const bool forward = solid.m_edges[edge].start_vertex == corners[k];
        solid.add_pcurve(
            edge,
            surface,
            solid.add_curve_2d(forward ? segment(uv[k], uv[next]) : segment(uv[next], uv[k]))
        );
        wire.push_back({edge, forward ? BRepOrientation::Forward : BRepOrientation::Reversed});
    }

    return solid.add_face(surface, {{solid.add_wire(wire), BRepOrientation::Forward}});
}

/// The point at the middle of the patch's domain.
static Point patch_centre(const NurbsSurface& patch) {

    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);

    return patch.point_at(0.5 * (du.first + du.second), 0.5 * (dv.first + dv.second));
}

/// The patch's own normal at the middle of its domain.
static Vector patch_normal(const NurbsSurface& patch) {

    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);

    return patch.normal_at(0.5 * (du.first + du.second), 0.5 * (dv.first + dv.second));
}

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════
PlateCurved::PlateCurved(
    const NurbsCurve& outer_a,
    const NurbsCurve& outer_b,
    const NurbsCurve& inner_a,
    const NurbsCurve& inner_b,
    const std::string& name
)
    : WoodElement(name), outer_a(outer_a), outer_b(outer_b), inner_a(inner_a), inner_b(inner_b) {

    if (!outer_a.is_valid() || !outer_b.is_valid() || !inner_a.is_valid() || !inner_b.is_valid())
        throw std::invalid_argument("A curved plate needs four valid meridians");
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════
std::array<NurbsSurface, 6> PlateCurved::compute_patches() const {

    const NurbsCurve outer_start =
        segment(outer_a.point_at(outer_a.domain_start()), outer_b.point_at(outer_b.domain_start()));
    const NurbsCurve inner_start =
        segment(inner_a.point_at(inner_a.domain_start()), inner_b.point_at(inner_b.domain_start()));
    const NurbsCurve outer_end =
        segment(outer_a.point_at(outer_a.domain_end()), outer_b.point_at(outer_b.domain_end()));
    const NurbsCurve inner_end =
        segment(inner_a.point_at(inner_a.domain_end()), inner_b.point_at(inner_b.domain_end()));

    // every patch ruled between two boundary curves, the faces opposite each other in pairs: outer and inner, side a and side b, start cap and end cap
    return {
        Primitives::create_ruled(outer_a, outer_b),
        Primitives::create_ruled(inner_a, inner_b),
        Primitives::create_ruled(outer_a, inner_a),
        Primitives::create_ruled(outer_b, inner_b),
        Primitives::create_ruled(outer_start, inner_start),
        Primitives::create_ruled(outer_end, inner_end),
    };
}

BRep PlateCurved::compute_solid(const std::array<NurbsSurface, 6>& patches) const {

    BRep solid;
    solid.name = name;

    // vertices: the eight corners, the start and the end of every meridian, vertex 2k and 2k + 1 of meridian k
    const std::array<NurbsCurve, 4> meridians = {outer_a, outer_b, inner_a, inner_b};

    for (const NurbsCurve& meridian : meridians) {
        solid.add_vertex(meridian.point_at(meridian.domain_start()));
        solid.add_vertex(meridian.point_at(meridian.domain_end()));
    }

    // edges: the four meridians on [0, 1] between their ends, then at each end the four rulings between the meridians' starts, then their ends: outer, inner, side a and side b
    const std::array<std::pair<int, int>, 4> rulings = {{{0, 2}, {4, 6}, {0, 4}, {2, 6}}};
    std::map<std::pair<int, int>, int> edges;

    for (int k = 0; k < 4; k++)
        edges[{2 * k, 2 * k + 1}] =
            solid.add_edge(solid.add_curve_3d(unit_domain(meridians[k])), 2 * k, 2 * k + 1);

    for (int end = 0; end < 2; end++)
        for (const std::pair<int, int>& ruling : rulings) {
            const int start = ruling.first + end;
            const int stop = ruling.second + end;
            const NurbsCurve line =
                segment(solid.m_vertices[start].point, solid.m_vertices[stop].point);
            edges[{start, stop}] = solid.add_edge(solid.add_curve_3d(line), start, stop);
        }

    // faces: every patch on its whole domain, bounded by the edges between its corners
    std::array<int, 6> faces;

    for (size_t i = 0; i < 6; i++)
        faces[i] = patch_face(solid, patches[i], edges);

    // shell: every face wound outwards, forward where the patch's own normal points away from the face opposite it
    std::vector<BRepRef> shell;

    for (size_t i = 0; i < 6; i++) {
        const Vector away = patch_centre(patches[i]) - patch_centre(patches[i ^ 1]);
        const bool outward = patch_normal(patches[i]).dot(away) > 0.0;
        shell.push_back({faces[i], outward ? BRepOrientation::Forward : BRepOrientation::Reversed});
    }

    solid.add_solid({{solid.add_shell(shell), BRepOrientation::Forward}});

    return solid;
}

const Mesh& PlateCurved::element_geometry_mesh() const {

    if (!_element_geometry_mesh) {
        Mesh mesh;

        for (const Mesh& face : element_geometry_brep().face_meshes()) {
            std::map<size_t, size_t> keys;

            for (const std::pair<const size_t, std::vector<size_t>>& entry : face.face) {
                std::vector<size_t> corners;

                for (const size_t vertex : entry.second) {
                    if (!keys.count(vertex))
                        keys[vertex] = mesh.add_vertex(face.vertex.at(vertex).position());

                    corners.push_back(keys[vertex]);
                }

                mesh.add_face(corners);
            }
        }

        _element_geometry_mesh = mesh;
    }

    return *_element_geometry_mesh;
}

const BRep& PlateCurved::element_geometry_brep() const {

    if (!_element_geometry_brep)
        _element_geometry_brep = compute_solid(compute_patches());

    return *_element_geometry_brep;
}

void PlateCurved::place(const Xform& xform) {

    Element::place(xform);
    outer_a.transform(xform);
    outer_b.transform(xform);
    inner_a.transform(xform);
    inner_b.transform(xform);

    for (InteractionFeatureSolid& feature : solid_features)
        feature = feature.transformed(xform);

    for (InteractionFeaturePlane& feature : plane_features)
        feature = feature.transformed(xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
}

void PlateCurved::compute_geometry_mesh_impl() {
    set_geometry(model_geometry_mesh());
    set_features(session_features(*this));
}

void PlateCurved::compute_geometry_brep_impl() {
    set_geometry(model_geometry_brep());
    set_features(session_features(*this));
}

}
