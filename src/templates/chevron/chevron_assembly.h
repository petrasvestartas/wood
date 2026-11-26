#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_chevron {

/// A box of the shell in its assembly: its plates, the one vector it slides in along and the boxes that stand before it.
struct AssemblyBox {
    std::vector<int> plates; // Indices into the assembly's plates.
    Vector direction; // The unit insertion vector, its line in the session's insertion group.
    std::vector<int>
        before; // The boxes whose walls its tenons enter along this vector, and whose tenons its walls take along it.
    Point low; // The corner of its plates' bounds in place with the smallest coordinates.
    Point high; // The corner with the largest.
};

/// A plate of a box sliding in passing through a plate of a box already placed.
struct AssemblyCollision {
    int box; // The box sliding in.
    int plate; // Its plate, by index.
    int other; // The placed plate it passes through, by index.
    double offset; // mm the box still had to go.
    Point point; // Where the plates met.
    int inside; // Sample points of either plate inside the other with the box in place: the overlap the design itself has there.
    double
        depth; // mm the deepest sample point of either plate stood inside the other anywhere along the way.
};

/// What the through-tenons dictate: per box the boxes that must be placed before it, and the joints no box's vector enters.
struct AssemblyConstraints {
    std::vector<std::vector<int>> before;
    std::vector<std::pair<int, int>> unenterable;
};

/// A plate as the prism its merged outlines bound: its first face as the frame, the outer loop and the holes on both faces in it, the
/// same count per loop so the side faces rule between them, and points clearance inside it on a grid over both faces and along every edge.
struct AssemblySolid {
    Plane base;
    Vector up;
    double thickness = 0.0;
    std::vector<std::vector<std::array<double, 2>>> bottom;
    std::vector<std::vector<std::array<double, 2>>> top;
    std::vector<Point> samples;
    Point low;
    Point high;
    bool ready = false;

    /// The loop at the height s between the faces.
    std::vector<std::array<double, 2>> loop(size_t k, double s) const;

    /// Whether (u, v) lies inside the loop, by the crossings of a ray along u.
    static bool inside(const std::vector<std::array<double, 2>>& points, double u, double v);

    /// How far p lies inside the prism: the least of its distances to the faces, the outer loop and the holes at its height; 0 outside.
    double depth(const Point& p) const;

    /// Whether p lies in the prism: between the faces, inside the outer loop and outside every hole at its height.
    bool contains(const Point& p) const;

    /// Project a polyline onto the base frame as 2D coordinates, dropping a duplicate closing vertex.
    std::vector<std::array<double, 2>> flat(const Polyline& polyline) const;

    /// Evaluate a 3D point from in-plane coordinates (u, v) and height h along up.
    Point at(double u, double v, double h) const;
};

/// The assembly of a shell box by box: every box slides in along its one insertion vector into the boxes already placed, in an order where
/// every through-tenon meets its wall along the vector of the box that moves last, and each box is swept along its vector against every
/// plate already placed, points just inside its plates stepped along the way through the prisms the placed plates' merged outlines bound. Built on a template whose plates stand in groups
/// box_<i> and whose insertion group holds one line per box.
class Assembly {
public:
    const double reach; // mm a box starts away from its place along its vector.
    const double
        clearance; // mm a sample point stands inside its own plate, so faces coming to rest on each other are no collision.

    /// The assembly of the boxes of session.
    explicit Assembly(const WoodSession& session, double reach = 400.0, double clearance = 0.2);

    /// The plates by index, the session's.
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates() const;

    /// By box index, the group box_<i>.
    const std::vector<AssemblyBox>& boxes() const;

    /// Pairs (male plate, female plate) of joints between two boxes that neither box's vector enters.
    const std::vector<std::pair<int, int>>& unenterable() const;

    /// The boxes in assembly order; shorter than boxes() when the constraints cycle.
    const std::vector<int>& order() const;

    /// Every penetration found along the insertions, one per plate pair.
    const std::vector<AssemblyCollision>& collisions() const;

    /// The counts and the first collisions, one per line.
    std::string report() const;

    /// The groups of block joined by +, as tools/render_template_docs.py takes them.
    static std::string groups(const std::vector<int>& block);

    /// The reach, the order, every box's plates, vector and bounds, and the block as JSON, for tools/render_assembly_gif.py.
    void write_json(const std::string& path, const std::vector<int>& block) const;

private:
    std::vector<std::shared_ptr<FacePlanarElement>> _plates;
    std::vector<int> _box_of;
    std::vector<AssemblyBox> _boxes;
    std::vector<std::pair<int, int>> _unenterable;
    std::vector<int> _order;
    std::vector<AssemblyCollision> _collisions;

    /// The plates under every box_<i> group of the tree, by box index.
    std::vector<std::vector<int>> compute_box_plates(const WoodSession& session) const;

    /// Every box's vector: the line of the insertion group starting nearest the centre of its plates, else the first insertion vector its plates carry.
    std::vector<Vector> compute_directions(const WoodSession& session) const;

    /// Every box's predecessors from the top-side joints between two boxes: the female's box before the male's when the tenon runs along the male's vector, the other way round when it runs along the female's; a tenon along neither, or a joint of another kind between boxes, is unenterable.
    AssemblyConstraints compute_before(const WoodSession& session) const;

    /// Every plate as the prism its merged outlines bound, with its sample points.
    std::vector<AssemblySolid> compute_solids() const;

    /// The boxes in an order where every predecessor comes first; the boxes a cycle keeps out are left out.
    std::vector<int> compute_order() const;

    /// Every box swept along its vector into the boxes before it in the order.
    std::vector<AssemblyCollision> compute_collisions() const;

    /// Order collisions from deepest penetration to shallowest.
    static bool deeper_collision(const AssemblyCollision& a, const AssemblyCollision& b);
};

}
