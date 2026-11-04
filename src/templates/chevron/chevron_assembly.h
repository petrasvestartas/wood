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
    std::vector<std::array<double, 2>> loop(size_t k, double s) const {
        std::vector<std::array<double, 2>> points(bottom[k].size());
        for (size_t i = 0; i < points.size(); i++)
            for (int c = 0; c < 2; c++)
                points[i][c] = bottom[k][i][c] + (top[k][i][c] - bottom[k][i][c]) * s;
        return points;
    }

    /// Whether (u, v) lies inside the loop, by the crossings of a ray along u.
    static bool inside(const std::vector<std::array<double, 2>>& points, double u, double v) {
        bool in = false;
        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            const std::array<double, 2>& a = points[i];
            const std::array<double, 2>& b = points[j];
            if ((a[1] > v) != (b[1] > v) && u < a[0] + (b[0] - a[0]) * (v - a[1]) / (b[1] - a[1]))
                in = !in;
        }
        return in;
    }

    /// How far p lies inside the prism: the least of its distances to the faces, the outer loop and the holes at its height; 0 outside.
    double depth(const Point& p) const {
        if (!contains(p))
            return 0.0;
        const Vector d = p - base.origin();
        const double h = d.dot(up);
        const double s = h / thickness;
        const double u = d.dot(base.x_axis());
        const double v = d.dot(base.y_axis());
        double least = std::min(h, thickness - h);
        for (size_t k = 0; k < bottom.size(); k++) {
            const std::vector<std::array<double, 2>> points = loop(k, s);
            for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
                const double ex = points[i][0] - points[j][0];
                const double ey = points[i][1] - points[j][1];
                const double length2 = ex * ex + ey * ey;
                const double t =
                    length2 < 1e-18
                        ? 0.0
                        : std::clamp(
                              ((u - points[j][0]) * ex + (v - points[j][1]) * ey) / length2,
                              0.0,
                              1.0
                          );
                const double dx = u - (points[j][0] + ex * t);
                const double dy = v - (points[j][1] + ey * t);
                least = std::min(least, std::sqrt(dx * dx + dy * dy));
            }
        }
        return least;
    }

    /// Whether p lies in the prism: between the faces, inside the outer loop and outside every hole at its height.
    bool contains(const Point& p) const {
        if (p[0] < low[0] || p[0] > high[0] || p[1] < low[1] || p[1] > high[1] || p[2] < low[2] ||
            p[2] > high[2])
            return false;
        const Vector d = p - base.origin();
        const double h = d.dot(up);
        if (h < 0.0 || h > thickness)
            return false;
        const double s = h / thickness;
        const double u = d.dot(base.x_axis());
        const double v = d.dot(base.y_axis());
        if (!inside(loop(0, s), u, v))
            return false;
        for (size_t k = 1; k < bottom.size(); k++)
            if (inside(loop(k, s), u, v))
                return false;
        return true;
    }
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
};

inline Assembly::Assembly(const WoodSession& session, double reach, double clearance)
    : reach(reach), clearance(clearance) {

    // plates: the session's, each with its box from the groups box_<i>
    _plates = session.world_elements<FacePlanarElement>();
    const std::vector<std::vector<int>> box_plates = compute_box_plates(session);
    _boxes.resize(box_plates.size());
    _box_of.assign(_plates.size(), -1);

    for (size_t box = 0; box < box_plates.size(); box++) {
        _boxes[box].plates = box_plates[box];
        _boxes[box].low = Point(1e300, 1e300, 1e300);
        _boxes[box].high = Point(-1e300, -1e300, -1e300);

        for (const int plate : box_plates[box]) {
            _box_of[plate] = static_cast<int>(box);

            for (int face = 0; face < 2 && face < (int)_plates[plate]->polylines.size(); face++)
                for (const Point& point : _plates[plate]->polylines[face].get_points())
                    for (int k = 0; k < 3; k++) {
                        _boxes[box].low[k] = std::min(_boxes[box].low[k], point[k]);
                        _boxes[box].high[k] = std::max(_boxes[box].high[k], point[k]);
                    }
        }
    }

    // directions: one line per box in the insertion group
    const std::vector<Vector> directions = compute_directions(session);

    for (size_t box = 0; box < _boxes.size(); box++)
        _boxes[box].direction = directions[box];

    // constraints: from the top-side joints between two boxes
    const AssemblyConstraints constraints = compute_before(session);
    _unenterable = constraints.unenterable;

    for (size_t box = 0; box < _boxes.size(); box++)
        _boxes[box].before = constraints.before[box];

    // order: every box after the boxes its joints put before it
    _order = compute_order();

    // collisions: every box swept along its vector into the boxes placed before it
    _collisions = compute_collisions();
}

inline const std::vector<std::shared_ptr<FacePlanarElement>>& Assembly::plates() const {
    return _plates;
}

inline const std::vector<AssemblyBox>& Assembly::boxes() const {
    return _boxes;
}

inline const std::vector<std::pair<int, int>>& Assembly::unenterable() const {
    return _unenterable;
}

inline const std::vector<int>& Assembly::order() const {
    return _order;
}

inline const std::vector<AssemblyCollision>& Assembly::collisions() const {
    return _collisions;
}

// ═══════════════════════════════════════════════════════════════════════════
// Boxes
// ═══════════════════════════════════════════════════════════════════════════
inline std::vector<std::vector<int>>
Assembly::compute_box_plates(const WoodSession& session) const {

    std::unordered_map<std::string, int> index;

    for (size_t i = 0; i < _plates.size(); i++)
        index[_plates[i]->guid()] = static_cast<int>(i);

    std::vector<std::vector<int>> plates;
    std::function<void(const TreeNode*, int)> visit = [&](const TreeNode* node, int box) {
        if (node->name.rfind("box_", 0) == 0) {
            box = std::stoi(node->name.substr(4));

            if (box >= (int)plates.size())
                plates.resize(box + 1);
        } else if (box >= 0) {
            const std::unordered_map<std::string, int>::const_iterator found =
                index.find(node->name);

            if (found != index.end())
                plates[box].push_back(found->second);
        }

        for (const TreeNode* child : node->children())
            visit(child, box);
    };
    visit(session.tree.root().get(), -1);

    return plates;
}

inline std::vector<Vector> Assembly::compute_directions(const WoodSession& session) const {

    // the lines of the insertion group
    std::vector<Line> lines;
    std::function<void(const TreeNode*, bool)> visit = [&](const TreeNode* node, bool inside) {
        inside = inside || node->name == "insertion";

        if (inside && node->is_leaf())
            if (const std::shared_ptr<const Line> line = session.get_object<Line>(node->name))
                lines.push_back(*line);

        for (const TreeNode* child : node->children())
            visit(child, inside);
    };
    visit(session.tree.root().get(), false);

    std::vector<Vector> directions(_boxes.size(), Vector(0.0, 0.0, 0.0));

    for (size_t box = 0; box < _boxes.size(); box++) {
        if (_boxes[box].plates.empty())
            continue;

        // the centre of the box's plates, and the line starting nearest it
        Vector centre(0.0, 0.0, 0.0);

        for (const int plate : _boxes[box].plates) {
            const Point point = _plates[plate]->polylines[0].center();
            centre = centre + Vector(point[0], point[1], point[2]);
        }

        const Point middle = Point(0.0, 0.0, 0.0) + centre * (1.0 / _boxes[box].plates.size());
        double nearest = std::numeric_limits<double>::infinity();

        for (const Line& line : lines) {
            const double distance = Point::distance(middle, line.start());

            if (distance < nearest) {
                nearest = distance;
                directions[box] = line.to_vector();
            }
        }

        // no line: the first insertion vector a plate of the box carries on a side
        for (size_t p = 0; p < _boxes[box].plates.size() && directions[box].magnitude() < 1e-9;
             p++) {
            const std::vector<Vector>& vectors =
                _plates[_boxes[box].plates[p]]->insertion_vectors();

            for (size_t face = 2; face < vectors.size() && directions[box].magnitude() < 1e-9;
                 face++)
                directions[box] = vectors[face];
        }

        directions[box].normalize_self();
    }

    return directions;
}

// ═══════════════════════════════════════════════════════════════════════════
// Order
// ═══════════════════════════════════════════════════════════════════════════
inline AssemblyConstraints Assembly::compute_before(const WoodSession& session) const {

    std::unordered_map<std::string, int> index;

    for (size_t i = 0; i < _plates.size(); i++)
        index[_plates[i]->guid()] = static_cast<int>(i);

    AssemblyConstraints constraints;
    constraints.before.resize(_boxes.size());

    for (const InteractionFeaturePlate& joint : session.get_plate_features()) {
        const std::unordered_map<std::string, int>::const_iterator a = index.find(joint.element_a);
        const std::unordered_map<std::string, int>::const_iterator b = index.find(joint.element_b);

        if (a == index.end() || b == index.end())
            continue;

        const int male = a->second;
        const int female = b->second;
        const int box_a = _box_of[male];
        const int box_b = _box_of[female];

        if (box_a < 0 || box_b < 0 || box_a == box_b)
            continue;

        // the tenon's own direction on the male's side
        Vector tenon = _plates[male]->insertion_vector(joint.contact.face_a);

        if (joint.joint_type != 20 || !tenon.normalize_self()) {
            constraints.unenterable.emplace_back(male, female);
            continue;
        }

        // the box whose vector the tenon runs along moves last: the nearer of the two, as the vectors of two cells of one column differ by the surface's turn alone
        const double along_a = std::abs(tenon.dot(_boxes[box_a].direction));
        const double along_b = std::abs(tenon.dot(_boxes[box_b].direction));

        if (std::max(along_a, along_b) <= 0.999)
            constraints.unenterable.emplace_back(male, female);
        else if (along_a >= along_b)
            constraints.before[box_a].push_back(box_b);
        else
            constraints.before[box_b].push_back(box_a);
    }

    for (std::vector<int>& list : constraints.before) {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
    }

    return constraints;
}

inline std::vector<int> Assembly::compute_order() const {

    std::vector<int> pending(_boxes.size(), 0);
    std::vector<std::vector<int>> after(_boxes.size());

    for (size_t box = 0; box < _boxes.size(); box++) {
        pending[box] = static_cast<int>(_boxes[box].before.size());

        for (const int earlier : _boxes[box].before)
            after[earlier].push_back(static_cast<int>(box));
    }

    std::vector<int> order;
    std::deque<int> ready;

    for (size_t box = 0; box < _boxes.size(); box++)
        if (pending[box] == 0)
            ready.push_back(static_cast<int>(box));

    while (!ready.empty()) {
        const int box = ready.front();
        ready.pop_front();
        order.push_back(box);

        for (const int later : after[box])
            if (--pending[later] == 0)
                ready.push_back(later);
    }

    return order;
}

// ═══════════════════════════════════════════════════════════════════════════
// Collisions
// ═══════════════════════════════════════════════════════════════════════════
inline std::vector<AssemblySolid> Assembly::compute_solids() const {

    std::vector<AssemblySolid> solids(_plates.size());

    for (size_t i = 0; i < _plates.size(); i++) {
        const FacePlanarElement& plate = *_plates[i];
        if (plate.planes.size() < 2 || plate.polylines.size() < 2)
            continue;

        AssemblySolid& solid = solids[i];
        solid.base = plate.planes[0];
        solid.up = plate.planes[0].z_axis() * -1.0;
        solid.thickness =
            std::abs((plate.planes[1].origin() - plate.planes[0].origin()).dot(solid.up));
        if (solid.thickness < 1e-9)
            continue;

        // the loops on both faces in the frame, the merged outlines after the joints, the plain outlines before
        const std::vector<Polyline>& bottoms = plate.features.bottom.empty()
                                                   ? std::vector<Polyline>{plate.polylines[0]}
                                                   : plate.features.bottom;
        const std::vector<Polyline>& tops = plate.features.top.empty()
                                                ? std::vector<Polyline>{plate.polylines[1]}
                                                : plate.features.top;
        const auto flat = [&](const Polyline& polyline) {
            std::vector<std::array<double, 2>> points;
            const std::vector<Point> source = polyline.get_points();
            const size_t count =
                source.size() > 1 && Point::distance(source.front(), source.back()) < 1e-9
                    ? source.size() - 1
                    : source.size();
            for (size_t k = 0; k < count; k++) {
                const Vector d = source[k] - solid.base.origin();
                points.push_back({d.dot(solid.base.x_axis()), d.dot(solid.base.y_axis())});
            }
            return points;
        };

        for (size_t k = 0; k < bottoms.size() && k < tops.size(); k++) {
            // the loop nearer the first face first, whichever face the merge listed it under: the holes lean, so the faces must not swap
            const Polyline* lower = &bottoms[k];
            const Polyline* upper = &tops[k];
            if (lower->point_count() > 0 &&
                std::abs((lower->get_point(0) - plate.planes[0].origin()).dot(solid.up)) >
                    std::abs((lower->get_point(0) - plate.planes[1].origin()).dot(solid.up)))
                std::swap(lower, upper);
            std::vector<std::array<double, 2>> bottom = flat(*lower);
            std::vector<std::array<double, 2>> top = flat(*upper);
            if (bottom.size() < 3 || top.size() < 3)
                continue;
            if (bottom.size() != top.size())
                top = bottom;
            solid.bottom.push_back(bottom);
            solid.top.push_back(top);
        }
        if (solid.bottom.empty())
            continue;

        // the bounds in space
        solid.low = Point(1e300, 1e300, 1e300);
        solid.high = Point(-1e300, -1e300, -1e300);
        for (const std::vector<Polyline>* loops : {&bottoms, &tops})
            for (const Polyline& polyline : *loops)
                for (const Point& point : polyline.get_points())
                    for (int c = 0; c < 3; c++) {
                        solid.low[c] = std::min(solid.low[c], point[c]);
                        solid.high[c] = std::max(solid.high[c], point[c]);
                    }

        // the samples: a grid over both faces just inside them, and along every edge of every loop at three heights, clearance into the material
        const auto at = [&](double u, double v, double h) {
            return solid.base.origin() + solid.base.x_axis() * u + solid.base.y_axis() * v +
                   solid.up * h;
        };
        double u0 = 1e300;
        double u1 = -1e300;
        double v0 = 1e300;
        double v1 = -1e300;
        for (const std::array<double, 2>& point : solid.bottom[0]) {
            u0 = std::min(u0, point[0]);
            u1 = std::max(u1, point[0]);
            v0 = std::min(v0, point[1]);
            v1 = std::max(v1, point[1]);
        }
        const double pitch = 20.0;
        for (const double h : {clearance, solid.thickness - clearance})
            for (double u = u0 + pitch * 0.5; u < u1; u += pitch)
                for (double v = v0 + pitch * 0.5; v < v1; v += pitch)
                    if (solid.contains(at(u, v, h)))
                        solid.samples.push_back(at(u, v, h));
        for (const double s : {0.25, 0.5, 0.75})
            for (size_t k = 0; k < solid.bottom.size(); k++) {
                const std::vector<std::array<double, 2>> points = solid.loop(k, s);
                double area = 0.0;
                for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++)
                    area += points[j][0] * points[i][1] - points[i][0] * points[j][1];
                const double side = (area > 0.0 ? 1.0 : -1.0) * (k == 0 ? 1.0 : -1.0);
                for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
                    const double ex = points[i][0] - points[j][0];
                    const double ey = points[i][1] - points[j][1];
                    const double length = std::sqrt(ex * ex + ey * ey);
                    if (length < 1e-9)
                        continue;
                    const double nx = -ey / length * side * clearance;
                    const double ny = ex / length * side * clearance;
                    for (const double t : {0.25, 0.5, 0.75}) {
                        const Point sample =
                            at(points[j][0] + ex * t + nx,
                               points[j][1] + ey * t + ny,
                               s * solid.thickness);
                        if (solid.contains(sample))
                            solid.samples.push_back(sample);
                    }
                }
            }
        solid.ready = true;
    }

    return solids;
}

inline std::vector<AssemblyCollision> Assembly::compute_collisions() const {

    const std::vector<AssemblySolid> solids = compute_solids();

    // every box in order: each of its plates from reach behind its place along the vector, step by step, against every placed plate on the
    // way; the first contact of a pair is kept with the distance still to go, and the points inside each other with the box in place
    std::vector<AssemblyCollision> collisions;
    std::vector<bool> placed(_plates.size(), false);
    const double step = 10.0;
    const int steps = static_cast<int>(std::ceil(reach / step));

    for (const int box : _order) {
        const Vector& v = _boxes[box].direction;
        if (v.magnitude() < 1e-9)
            continue;

        for (const int moving : _boxes[box].plates) {
            const AssemblySolid& m = solids[moving];
            if (!m.ready)
                continue;

            Point low = m.low;
            Point high = m.high;
            for (int c = 0; c < 3; c++) {
                low[c] = std::min(low[c], low[c] - v[c] * reach);
                high[c] = std::max(high[c], high[c] - v[c] * reach);
            }

            for (size_t other = 0; other < _plates.size(); other++) {
                const AssemblySolid& q = solids[other];
                if (!placed[other] || !q.ready || q.low[0] > high[0] || q.high[0] < low[0] ||
                    q.low[1] > high[1] || q.high[1] < low[1] || q.low[2] > high[2] ||
                    q.high[2] < low[2])
                    continue;

                std::optional<AssemblyCollision> met;
                int inside = 0;

                // the moving plate's points along the way through the placed plate, and the placed plate's points through the moving one
                double deepest = 0.0;
                for (const Point& sample : m.samples)
                    for (int i = steps; i >= 0; i--) {
                        const double d = std::min(reach, i * step);
                        const double depth = q.depth(sample - v * d);
                        if (depth <= 0.0)
                            continue;
                        if (!met || d > met->offset)
                            met = AssemblyCollision{
                                box, moving, static_cast<int>(other), d, sample - v * d, 0, 0.0
                            };
                        deepest = std::max(deepest, depth);
                        if (d == 0.0)
                            inside++;
                    }
                for (const Point& sample : q.samples)
                    for (int i = steps; i >= 0; i--) {
                        const double d = std::min(reach, i * step);
                        const double depth = m.depth(sample + v * d);
                        if (depth <= 0.0)
                            continue;
                        if (!met || d > met->offset)
                            met = AssemblyCollision{
                                box, moving, static_cast<int>(other), d, sample, 0, 0.0
                            };
                        deepest = std::max(deepest, depth);
                        if (d == 0.0)
                            inside++;
                    }

                if (met) {
                    met->inside = inside;
                    met->depth = deepest;
                    collisions.push_back(*met);
                }
            }
        }

        for (const int plate : _boxes[box].plates)
            placed[plate] = true;
    }

    return collisions;
}

// ═══════════════════════════════════════════════════════════════════════════
// Report
// ═══════════════════════════════════════════════════════════════════════════
inline std::string Assembly::report() const {

    int binding = 0;
    double deepest = 0.0;
    for (const AssemblyCollision& collision : _collisions) {
        binding += collision.depth > 1.0 ? 1 : 0;
        deepest = std::max(deepest, collision.depth);
    }
    std::string text = fmt::format(
        "assembly: {} boxes, {} in order, {} kept out by a cycle, {} joints between boxes no vector enters, {} plate pairs meeting along the insertions, {} of them deeper than 1 mm, the deepest {:.2f} mm",
        _boxes.size(),
        _order.size(),
        _boxes.size() - _order.size(),
        _unenterable.size(),
        _collisions.size(),
        binding,
        deepest
    );

    std::vector<AssemblyCollision> listed = _collisions;
    std::sort(
        listed.begin(), listed.end(), [](const AssemblyCollision& a, const AssemblyCollision& b) {
            return a.depth > b.depth;
        }
    );

    for (size_t i = 0; i < listed.size() && i < 10; i++) {
        const AssemblyCollision& collision = listed[i];
        text += fmt::format(
            "\n  box {} {} meets {} from {:.0f} mm to go at ({:.0f}, {:.0f}, {:.0f}), {:.2f} mm deep, {} points inside in place",
            collision.box,
            _plates[collision.plate]->name,
            _plates[collision.other]->name,
            collision.offset,
            collision.point[0],
            collision.point[1],
            collision.point[2],
            collision.depth,
            collision.inside
        );
    }

    return text;
}

inline std::string Assembly::groups(const std::vector<int>& block) {

    std::string text;

    for (const int box : block)
        text += fmt::format("{}box_{}", text.empty() ? "" : "+", box);

    return text;
}

inline void Assembly::write_json(const std::string& path, const std::vector<int>& block) const {

    nlohmann::ordered_json json;
    json["reach"] = reach;
    json["order"] = _order;
    json["block"] = block;
    json["collisions"] = _collisions.size();
    double deepest = 0.0;
    for (const AssemblyCollision& collision : _collisions)
        deepest = std::max(deepest, collision.depth);
    json["deepest"] = deepest;
    json["meetings"] = nlohmann::ordered_json::array();

    for (const AssemblyCollision& collision : _collisions)
        json["meetings"].push_back(
            {{"box", collision.box},
             {"plate", _plates[collision.plate]->name},
             {"other", _plates[collision.other]->name},
             {"offset", collision.offset},
             {"depth", collision.depth},
             {"inside", collision.inside}}
        );

    json["boxes"] = nlohmann::ordered_json::array();

    for (const AssemblyBox& box : _boxes) {
        nlohmann::ordered_json entry;
        entry["plates"] = nlohmann::ordered_json::array();

        for (const int plate : box.plates)
            entry["plates"].push_back(_plates[plate]->name);

        entry["direction"] = {box.direction[0], box.direction[1], box.direction[2]};
        entry["low"] = {box.low[0], box.low[1], box.low[2]};
        entry["high"] = {box.high[0], box.high[1], box.high[2]};
        json["boxes"].push_back(entry);
    }

    std::ofstream file(path);
    file << json.dump(1);
}

}
