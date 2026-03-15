#pragma once
#include "nurbssurface.h"
#include "mesh.h"
#include "polyline.h"
#include "line.h"
#include "plane.h"
#include <vector>
#include <array>
#include <map>

namespace session_cpp {

Mesh chevron_mesh(const NurbsSurface& surface,
                  int u_divisions = 4,
                  double v_division_dist = 900.0,
                  double shift = 0.5,
                  double scale = 0.05799);

std::vector<NurbsSurface> annen_surfaces();

/// Diamond subdivision of a NURBS surface into folded timber plates.
class FoldedPlates {
public:
    Mesh mesh;
    std::vector<bool> flags;
    std::vector<std::array<int, 4>> adjacency;
    std::vector<std::vector<Polyline>> polylines;
    std::vector<Line> insertion_lines;

    FoldedPlates(const NurbsSurface& surface, int u_divisions, int v_divisions,
                 double thickness, double chamfer = 0.0,
                 const std::vector<Plane>& base_planes = {},
                 const std::vector<double>& face_positions = {0.0});

private:
    NurbsSurface _srf;
    int _udiv, _vdiv;
    double _thick, _cham;
    std::vector<Plane> _base_planes;
    std::vector<double> _face_pos;

    int _f;
    std::vector<size_t> _fkeys;
    std::vector<std::vector<size_t>> _fv;
    std::vector<Plane> _fplanes;
    std::vector<std::vector<Plane>> _fe_planes;
    std::vector<Plane> _eplanes;
    std::map<std::pair<size_t,size_t>, int> _eidx;
    std::vector<std::vector<int>> _ef;

    void diamond_subdivision();
    void build_topology();
    void compute_face_planes();
    void compute_edge_planes();
    void compute_face_edge_planes();
    void compute_face_polylines();
    void compute_insertion_vectors();

    static Point closest_on_line(const Point& pt, const Point& a, const Point& b);
    static bool line_plane_t(const Point& a, const Point& b, const Plane& pl, double& t);
    static bool intersect_3_planes(const Plane& p0, const Plane& p1, const Plane& p2, Point& result);
    static Polyline chamfer_polyline(const Polyline& pl, double value);
};

/// Cross-connectors for mesh: face plates + edge connectors.
class CrossConnectors {
public:
    Mesh mesh;
    std::vector<Plane> face_planes;
    std::vector<std::vector<Plane>> fe_planes;
    std::vector<std::vector<Plane>> bisector_planes;
    std::vector<std::vector<Polyline>> face_polylines;
    std::vector<std::pair<size_t,size_t>> edges;
    std::vector<std::vector<int>> edge_faces;
    std::vector<Plane> edge_planes;
    std::vector<std::vector<Polyline>> edge_polylines;

    CrossConnectors(const Mesh& m,
                    double face_thickness,
                    const std::vector<double>& face_positions = {0.0},
                    int edge_divisions = 2,
                    double rect_width = 10.0,
                    double rect_height = 10.0,
                    double rect_thickness = 1.0,
                    double chamfer = 0.0);

private:
    int _f;
    double _thick, _rect_w, _rect_h, _rect_t, _cham;
    int _edge_div;
    std::vector<double> _face_pos;
    std::vector<size_t> _fkeys;
    std::vector<std::vector<size_t>> _fv;
    std::map<std::pair<size_t,size_t>, int> _eidx;
    std::vector<std::vector<Plane>> _e90_multiple_planes;

    void build_topology();
    void compute_face_planes();
    void compute_face_edge_planes();
    void compute_bisector_planes();
    void compute_face_polylines();
    void compute_edges();
    void compute_edge_faces();
    void compute_edge_planes_method();
    void compute_connectors();

    static Plane dihedral_plane(const Plane& p0, const Plane& p1);
    static Plane move_plane_by_axis(const Plane& pl, double dist, int axis = 2);
    static Polyline outline_from_planes(const Plane& face_plane, const std::vector<Plane>& edge_planes, const std::vector<Plane>& bise_planes);
    static Polyline make_rectangle(const Plane& pl, double w, double h);
};

} // namespace session_cpp