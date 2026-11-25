#include "pch.h"
#include "wood_module_sequence.h"
#include "wood_element_geometry.h"
#include <fmt/ranges.h>

namespace wood_session {

const double TOUCH_GAP = 1.5; // mm two faces may stand apart and still touch.
const double TOUCH_AREA = 4.0; // mm2 of shared area under which two faces do not touch.
const double BINDING = 12.0; // mm of shared wood over which a direction is blocked, not one that only just binds.
const double TURNS[] = {2.0, 5.0, 10.0, 15.0, 25.0, 40.0}; // Degrees a piece is turned out before it slides.

// ═══════════════════════════════════════════════════════════════════════════
// ModuleSequence
// ═══════════════════════════════════════════════════════════════════════════

ModuleSequence::ModuleSequence(const WoodSession& session, const AssemblyModule& root, double step, double play)
    : root(root), step(step), play(play) {

    // bodies: the solid of every element of the modules, its faces looking out
    bodies = compute_bodies(session);
    for (size_t i = 0; i < bodies.size(); i++)
        body_of[bodies[i].guid] = i;
    for (const Body& body : bodies)
        if (!body.solid->is_solid())
            open.push_back(body.name);

    piece_of.assign(bodies.size(), 0);
    std::vector<size_t> all(bodies.size());
    for (size_t i = 0; i < all.size(); i++)
        all[i] = i;
    reach = all.empty() ? 0.0 : 0.15 * bounds_of(all).diagonal();

    // touches and overlaps: where the faces of two solids lie on each other, and the wood two solids share as they stand
    for (size_t i = 0; i < bodies.size(); i++)
        for (size_t j = i + 1; j < bodies.size(); j++) {
            AABB near = bodies[j].solid->box();
            near.inflate(TOUCH_GAP);
            if (!bodies[i].solid->box().intersects(near))
                continue;
            touches[{i, j}] = compute_touches(i, j);
            const SharedWood shared = bodies[i].solid->shared_with(*bodies[j].solid, Xform());
            standing[{i, j}] = shared;
            if (shared.thickness > play && !bodies[i].pin && !bodies[j].pin)
                overlapping.push_back({bodies[i].name, bodies[j].name, shared.thickness});
        }

    // modules: every one taken apart on its own, the inner ones first
    add_module(this->root);
}

const std::vector<ModuleSteps>& ModuleSequence::modules() const {
    return every;
}

const std::vector<std::string>& ModuleSequence::locked() const {
    return stuck;
}

const std::vector<ModuleOverlap>& ModuleSequence::overlaps() const {
    return overlapping;
}

const std::vector<std::string>& ModuleSequence::open_solids() const {
    return open;
}

// ═══════════════════════════════════════════════════════════════════════════
// Bodies
// ═══════════════════════════════════════════════════════════════════════════

std::vector<ModuleSequence::Body> ModuleSequence::compute_bodies(const WoodSession& session) const {

    std::unordered_map<std::string, std::shared_ptr<Element>> elements;
    for (const std::shared_ptr<Element>& element : session.elements())
        elements[element->guid()] = element;
    const std::unordered_map<std::string, Xform> placements = session.world_xforms();

    std::vector<Body> found;
    for (const std::string& guid : root.every_element()) {
        const std::unordered_map<std::string, std::shared_ptr<Element>>::const_iterator element = elements.find(guid);
        if (element == elements.end())
            throw std::invalid_argument(fmt::format("ModuleSequence: no element {} in session {}", guid, session.name));
        const std::unordered_map<std::string, Xform>::const_iterator placed = placements.find(guid);
        Mesh mesh = element->second->model_geometry_mesh();

        // a curved plate's mesh is its patches side by side: its solid through the meridians is closed
        if (const std::shared_ptr<FaceCurvedElement> curved = std::dynamic_pointer_cast<FaceCurvedElement>(element->second))
            mesh = curved->closed_mesh();

        // any other whose faces stand side by side is closed where they meet
        else if (!manifold_solid(mesh))
            mesh = mesh.weld(0.05);
        if (placed != placements.end())
            mesh = mesh.transformed(placed->second);
        found.push_back({guid, element->second->name, std::make_shared<SolidBody>(mesh), compute_faces(mesh), std::dynamic_pointer_cast<JointPinElement>(element->second) != nullptr});
    }

    return found;
}

std::vector<ModuleSequence::BodyFace> ModuleSequence::compute_faces(const Mesh& mesh) {

    // the faces as the mesh lists them, and the volume they enclose by their own sense
    std::vector<BodyFace> faces;
    double volume = 0.0;
    for (const Polyline& outline : mesh.face_outlines()) {
        std::vector<Point> points = outline.get_points();
        if (points.size() < 4)
            continue;
        points.pop_back();
        Vector area(0.0, 0.0, 0.0);
        for (size_t i = 1; i + 1 < points.size(); i++)
            area += (points[i] - points[0]).cross(points[i + 1] - points[0]);
        if (area.magnitude() < TOUCH_AREA)
            continue;
        volume += area.dot(points[0] - Point(0.0, 0.0, 0.0));
        faces.push_back({outline, area.normalized(), points[0], AABB::from_points(points, TOUCH_GAP)});
    }

    // a mesh whose faces look in is turned out
    if (volume < 0.0)
        for (BodyFace& face : faces)
            face.normal = face.normal * -1.0;

    return faces;
}

std::vector<ModuleSequence::Touch> ModuleSequence::compute_touches(size_t first, size_t second) const {

    std::vector<Touch> found;
    for (const BodyFace& a : bodies[first].faces)
        for (const BodyFace& b : bodies[second].faces) {
            if (a.normal.dot(b.normal) > -0.9995 || std::abs((a.origin - b.origin).dot(b.normal)) > TOUCH_GAP || !a.box.intersects(b.box))
                continue;

            // the area both faces cover, in the second's plane
            const Vector x = b.normal.is_parallel_to(Vector::z_axis()) != 0 ? Vector::x_axis() : Vector::z_axis().cross(b.normal).normalized();
            const Vector y = b.normal.cross(x);
            const Clipper2Lib::PathsD common = Clipper2Lib::Intersect(
                {clipper_path(a.outline, b.origin, x, y, false)}, {clipper_path(b.outline, b.origin, x, y, false)}, Clipper2Lib::FillRule::NonZero, 4
            );
            if (std::abs(Clipper2Lib::Area(common)) < TOUCH_AREA)
                continue;
            Touch touch{b.normal, {}, std::abs(Clipper2Lib::Area(common))};
            for (const Clipper2Lib::PathD& path : common)
                for (const Clipper2Lib::PointD& point : path)
                    touch.points.push_back(b.origin + x * point.x + y * point.y);
            found.push_back(touch);
        }

    return found;
}

std::vector<ModuleSequence::Touch> ModuleSequence::touches_of(const std::vector<size_t>& piece, const std::vector<size_t>& others) const {

    std::vector<Touch> found;
    for (const size_t i : piece)
        for (const size_t j : others) {
            const std::map<std::pair<size_t, size_t>, std::vector<Touch>>::const_iterator pair = touches.find({std::min(i, j), std::max(i, j)});
            if (pair == touches.end())
                continue;
            for (Touch touch : pair->second) {
                if (i > j)
                    touch.normal = touch.normal * -1.0;
                found.push_back(touch);
            }
        }

    return found;
}

const SharedWood& ModuleSequence::standing_wood(size_t first, size_t second) {
    return standing[{std::min(first, second), std::max(first, second)}];
}

AABB ModuleSequence::bounds_of(const std::vector<size_t>& piece) const {

    AABB bounds = bodies[piece.front()].solid->box();
    for (const size_t i : piece)
        bounds = AABB::merge(bounds, bodies[i].solid->box());

    return bounds;
}

std::string ModuleSequence::kind_of(const std::string& name) {

    std::string kind;
    std::istringstream words(name);
    for (std::string word; std::getline(words, word, '_');)
        if (word.empty() || !std::isdigit(static_cast<unsigned char>(word[0])))
            kind += (kind.empty() ? "" : "_") + word;

    return kind;
}

// ═══════════════════════════════════════════════════════════════════════════
// Modules
// ═══════════════════════════════════════════════════════════════════════════

bool ModuleSequence::lower_piece(const Piece& left, const Piece& right) {
    return left.height < right.height;
}

void ModuleSequence::add_module(const AssemblyModule& module) {

    // the inner modules first, each then one piece of this one
    std::vector<Piece> pieces;
    for (const AssemblyModule& part : module.parts) {
        if (part.is_module())
            add_module(part);
        Piece piece{part.name, {}, part.ways, 0.0};
        for (const std::string& guid : part.every_element())
            piece.bodies.push_back(body_of.at(guid));
        if (piece.bodies.empty())
            continue;
        for (const size_t body : piece.bodies)
            piece_of[body] = pieces.size();
        piece.height = std::round(bounds_of(piece.bodies).cz);
        pieces.push_back(piece);
    }

    // the lowest first, to a millimetre
    if (module.raised)
        std::stable_sort(pieces.begin(), pieces.end(), lower_piece);

    // taken apart, then reversed: the piece left standing first
    ModuleSteps steps{module.name, {}, {}};
    const std::vector<ModuleMove> removal = compute_removal(pieces, steps);
    steps.moves.assign(removal.rbegin(), removal.rend());

    // the pieces that can go in at the same time
    compute_waves(steps);

    for (const std::string& name : steps.locked)
        stuck.push_back(fmt::format("{}: {}", module.name, name));
    every.push_back(steps);
}

std::vector<ModuleMove> ModuleSequence::compute_removal(const std::vector<Piece>& pieces, ModuleSteps& steps) {

    std::vector<ModuleMove> removal;
    std::vector<size_t> remaining(pieces.size());
    for (size_t i = 0; i < pieces.size(); i++)
        remaining[i] = i;

    // what blocked each direction of each piece: not tried again while that body is still in
    std::vector<std::map<std::string, size_t>> tried(pieces.size());

    while (!remaining.empty()) {
        bool taken = false;

        // the last piece in that a move frees goes out next, one that only goes out downwards when no other does
        for (size_t turn = 0; turn < 2 * remaining.size() && !taken; turn++) {
            const bool downwards = turn >= remaining.size();
            const size_t k = remaining.size() - 1 - turn % remaining.size();
            const Piece& piece = pieces[remaining[k]];
            std::vector<size_t> others;
            for (const size_t other : remaining)
                if (other != remaining[k])
                    others.insert(others.end(), pieces[other].bodies.begin(), pieces[other].bodies.end());

            Way way;
            if (!others.empty() && !find_way(piece, others, downwards, tried[remaining[k]], way))
                continue;

            ModuleMove move;
            move.piece = piece.name;
            for (const size_t body : piece.bodies)
                move.elements.push_back(bodies[body].guid);
            move.insertion = way.out * -1.0;
            for (std::vector<JointMove>::const_reverse_iterator leg = way.legs.rbegin(); leg != way.legs.rend(); ++leg)
                move.legs.push_back(leg->reversed());
            move.travel = way.travel;
            move.swept = way.swept;
            for (const size_t other : remaining)
                if (other != remaining[k] && !touches_of(piece.bodies, pieces[other].bodies).empty())
                    move.meets.push_back(pieces[other].name);
            removal.push_back(move);
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(k));
            taken = true;
        }

        // nothing comes out: the rest is locked, and stands first
        if (!taken) {
            for (std::vector<size_t>::const_reverse_iterator left = remaining.rbegin(); left != remaining.rend(); ++left) {
                ModuleMove move;
                move.piece = pieces[*left].name;
                for (const size_t body : pieces[*left].bodies)
                    move.elements.push_back(bodies[body].guid);
                removal.push_back(move);
                steps.locked.push_back(pieces[*left].name);
            }
            break;
        }
    }

    return removal;
}

// ═══════════════════════════════════════════════════════════════════════════
// Ways
// ═══════════════════════════════════════════════════════════════════════════

Vector ModuleSequence::nearest_direction(const Vector& hint, const std::vector<Touch>& touching) {

    // Hildreth's projection of the hint onto the directions no touch blocks
    Vector direction = hint;
    std::vector<double> pushed(touching.size(), 0.0);
    for (int sweep = 0; sweep < 400; sweep++) {
        double moved = 0.0;
        for (size_t i = 0; i < touching.size(); i++) {
            const double push = std::max(-pushed[i], -touching[i].normal.dot(direction));
            pushed[i] += push;
            direction += touching[i].normal * push;
            moved = std::max(moved, std::abs(push));
        }
        if (moved < 1e-10)
            break;
    }

    return direction.magnitude() < 1e-3 ? Vector(0.0, 0.0, 0.0) : direction.normalized();
}

std::vector<Vector> ModuleSequence::candidate_directions(const Piece& piece, const std::vector<size_t>& others, const std::vector<Touch>& touching) const {

    // the piece's own ways, then up, away from the pieces it touches, and the mean of the faces it stands on
    const Point centre = bounds_of(piece.bodies).center();
    Vector away(0.0, 0.0, 0.0);
    Vector mean(0.0, 0.0, 0.0);
    Vector weighted(0.0, 0.0, 0.0);
    std::set<size_t> touched;
    for (const Touch& touch : touching) {
        mean += touch.normal;
        weighted += touch.normal * touch.area;
    }
    for (const size_t i : piece.bodies)
        for (const size_t j : others) {
            const std::map<std::pair<size_t, size_t>, std::vector<Touch>>::const_iterator pair = touches.find({std::min(i, j), std::max(i, j)});
            if (pair == touches.end() || pair->second.empty())
                continue;
            touched.insert(piece_of[j]);
            away += (centre - bodies[j].solid->box().center()).normalized();
        }

    // a piece on one other opens straight off it, unless that is down
    const bool opens = touched.size() == 1 && mean.magnitude() > 1e-6 && mean.normalized()[2] > -0.2;
    std::vector<Vector> hints;
    for (const Vector& way : piece.ways)
        hints.push_back(way.normalized() * -1.0);
    const Vector up = Vector::z_axis();
    if (opens)
        hints.push_back(mean.normalized());
    for (const Vector& hint : {up, away, mean, up + away, up + mean, away + mean, weighted})
        if (hint.magnitude() > 1e-6)
            hints.push_back(hint.normalized());

    // each as it is, then turned as little as its touches ask; after them the touches' normals, their common directions and the axes
    std::vector<Vector> ranked;
    for (const Vector& hint : hints) {
        ranked.push_back(hint);
        const Vector nearest = nearest_direction(hint, touching);
        if (nearest.magnitude() > 0.5)
            ranked.push_back(nearest);
    }
    std::vector<Vector> normals;
    for (const Touch& touch : touching) {
        bool known = false;
        for (const Vector& normal : normals)
            known = known || touch.normal.dot(normal) > 0.9999;
        if (!known && normals.size() < 24)
            normals.push_back(touch.normal);
    }
    ranked.insert(ranked.end(), normals.begin(), normals.end());
    for (size_t i = 0; i < normals.size(); i++)
        for (size_t j = i + 1; j < normals.size(); j++) {
            const Vector along = normals[i].cross(normals[j]);
            if (along.magnitude() < 0.05)
                continue;
            ranked.push_back(along.normalized());
            ranked.push_back(along.normalized() * -1.0);
        }
    for (const Vector& axis : {Vector::z_axis(), Vector::x_axis(), Vector::y_axis()}) {
        ranked.push_back(axis);
        ranked.push_back(axis * -1.0);
    }

    // the piece's own ways as they are, the others where no touch blocks them, each once
    std::vector<Vector> kept;
    for (const Vector& way : piece.ways)
        kept.push_back(way.normalized() * -1.0);
    for (const Vector& direction : ranked) {
        bool fits = true;
        for (const Touch& touch : touching)
            fits = fits && touch.normal.dot(direction) > -2e-3;
        for (const Vector& other : kept)
            fits = fits && direction.dot(other) < 0.99999;
        if (fits)
            kept.push_back(direction);
    }

    return kept;
}

std::vector<Line> ModuleSequence::candidate_turns(const Piece& piece, const std::vector<Vector>& directions, const std::vector<Touch>& touching) const {

    const AABB bounds = bounds_of(piece.bodies);
    const Point centre = bounds.center();
    const double radius = 0.5 * bounds.diagonal();

    // per touch corner the row (normal, arm x normal / radius) a small motion's product with must not be negative
    std::vector<std::array<double, 6>> rows;
    for (const Touch& touch : touching)
        for (const Point& point : touch.points) {
            const Vector arm = (point - centre).cross(touch.normal) * (1.0 / radius);
            const double length = std::sqrt(1.0 + arm.dot(arm));
            rows.push_back({touch.normal[0] / length, touch.normal[1] / length, touch.normal[2] / length, arm[0] / length, arm[1] / length, arm[2] / length});
        }

    // the small motion nearest each hint that no touch blocks, as the line it turns about
    std::vector<Vector> hints = {Vector::z_axis(), Vector::x_axis(), Vector::y_axis(), Vector::x_axis() * -1.0, Vector::y_axis() * -1.0};
    for (const Vector& way : piece.ways)
        hints.push_back(way * -1.0);
    hints.insert(hints.end(), directions.begin(), directions.end());
    std::vector<Line> turns;
    for (const Vector& hint : hints) {
        std::array<double, 6> motion = {hint[0], hint[1], hint[2], 0.0, 0.0, 0.0};
        std::vector<double> pushed(rows.size(), 0.0);
        for (int sweep = 0; sweep < 400; sweep++) {
            double moved = 0.0;
            for (size_t i = 0; i < rows.size(); i++) {
                double along = 0.0;
                for (size_t k = 0; k < 6; k++)
                    along += rows[i][k] * motion[k];
                const double push = std::max(-pushed[i], -along);
                pushed[i] += push;
                for (size_t k = 0; k < 6; k++)
                    motion[k] += rows[i][k] * push;
                moved = std::max(moved, std::abs(push));
            }
            if (moved < 1e-10)
                break;
        }
        const Vector velocity(motion[0], motion[1], motion[2]);
        const Vector spin = Vector(motion[3], motion[4], motion[5]) * (1.0 / radius);
        if (spin.magnitude() * radius < 1e-3)
            continue;
        const Point pivot = centre + spin.cross(velocity) * (1.0 / spin.dot(spin));
        const Line axis = Line::from_points(pivot, pivot + spin.normalized() * 100.0);
        bool known = false;
        for (const Line& other : turns)
            known = known || (other.to_vector().normalized().dot(spin.normalized()) > 0.9999 && other.start().distance(pivot) < 1.0);
        if (!known)
            turns.push_back(axis);
    }

    return turns;
}

bool ModuleSequence::enters(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Xform& placed, size_t& blocker) {

    for (const size_t i : piece) {
        if (!bodies[i].solid->is_solid())
            continue;
        const AABB box = bodies[i].solid->box_at(placed);
        for (const size_t j : others) {
            if (!bodies[j].solid->is_solid() || !box.intersects(bodies[j].solid->box()))
                continue;
            const SharedWood shared = bodies[i].solid->shared_with(*bodies[j].solid, placed);
            if (shared.thickness < play || shared.volume < 10.0)
                continue;

            // a pin is driven into the wood it stands in, and wood the design itself shares is no collision until there is more of it
            const SharedWood& before = standing_wood(i, j);
            if (before.thickness > play && (bodies[i].pin || bodies[j].pin || shared.volume < 1.05 * before.volume))
                continue;
            blocker = j;
            return true;
        }
    }

    return false;
}

bool ModuleSequence::slides_out(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Xform& reached, const Vector& direction, Way& found, size_t& blocker) {

    // how far the piece has to go to leave the bounds of every other it could meet
    std::vector<AABB> boxes;
    for (const size_t i : piece)
        boxes.push_back(bodies[i].solid->box_at(reached));
    AABB whole = boxes.front();
    for (const AABB& box : boxes)
        whole = AABB::merge(whole, box);
    const double farthest = std::max(whole.diagonal() + 2.0 * step, reach);

    // pins are out once they have gone their own length: from there they are lifted away by hand
    bool pins = true;
    for (const size_t i : piece)
        pins = pins && bodies[i].pin;
    const double needed = pins ? 2.0 * (std::abs(direction[0]) * boxes.front().hx + std::abs(direction[1]) * boxes.front().hy + std::abs(direction[2]) * boxes.front().hz) + step : farthest;

    double distance = 1.0;
    double travel = 0.0;
    while (distance <= farthest) {
        const Xform placed = Xform::translation(direction[0] * distance, direction[1] * distance, direction[2] * distance) * reached;
        if (enters(piece, others, placed, blocker)) {
            if (distance <= needed)
                return false;
            found.travel = needed;
            found.swept = distance - step;
            return true;
        }

        // the last distance its bounds still meet another's
        for (const AABB& box : boxes) {
            AABB moved = box;
            moved.cx += direction[0] * distance;
            moved.cy += direction[1] * distance;
            moved.cz += direction[2] * distance;
            for (const size_t j : others)
                if (moved.intersects(bodies[j].solid->box()))
                    travel = distance;
        }
        distance += distance < step ? step - distance : distance < 20.0 * step ? step : 2.5 * step;
    }
    found.travel = std::min(farthest, travel + step);
    found.swept = farthest;

    return true;
}

double ModuleSequence::bound_by(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Vector& direction) {

    double thickest = 0.0;
    for (const double distance : {2.0, 6.0, 12.0, 20.0, 30.0, 45.0}) {
        const Xform placed = Xform::translation(direction[0] * distance, direction[1] * distance, direction[2] * distance);
        for (const size_t i : piece) {
            const AABB box = bodies[i].solid->box_at(placed);
            for (const size_t j : others) {
                if (!bodies[i].solid->is_solid() || !bodies[j].solid->is_solid() || !box.intersects(bodies[j].solid->box()))
                    continue;
                const SharedWood shared = bodies[i].solid->shared_with(*bodies[j].solid, placed);
                if (shared.volume > 10.0 && standing_wood(i, j).thickness <= play)
                    thickest = std::max(thickest, shared.thickness);
            }
        }
    }

    return thickest;
}

/// Whether left binds less than right.
static bool looser(const std::pair<double, Vector>& left, const std::pair<double, Vector>& right) {
    return left.first < right.first;
}

Vector ModuleSequence::refined_direction(const std::vector<size_t>& piece, const std::vector<size_t>& others, const std::vector<Vector>& directions) {

    // the three directions that bind least, none that is blocked outright
    std::vector<std::pair<double, Vector>> starts;
    for (const Vector& direction : directions)
        starts.emplace_back(bound_by(piece, others, direction), direction);
    std::stable_sort(starts.begin(), starts.end(), looser);

    for (size_t k = 0; k < starts.size() && k < 3 && starts[k].first < BINDING; k++) {
        double least = starts[k].first;
        Vector direction = starts[k].second;

        // turned either way about two axes across it, by half the angle whenever no turn is better
        for (double angle = 6.0; angle > 0.05 && least > 0.5 * play;) {
            const Vector across = (std::abs(direction[2]) < 0.9 ? Vector::z_axis() : Vector::x_axis()).cross(direction).normalized();
            bool better = false;
            for (const Vector& turn : {across, across * -1.0, direction.cross(across), direction.cross(across) * -1.0}) {
                const Vector tried = (direction + turn * std::tan(angle * Tolerance::TO_RADIANS)).normalized();
                const double bound = bound_by(piece, others, tried);
                if (bound < least) {
                    least = bound;
                    direction = tried;
                    better = true;
                    break;
                }
            }
            if (!better)
                angle *= 0.5;
        }
        if (least <= play)
            return direction;
    }

    return Vector(0.0, 0.0, 0.0);
}

bool ModuleSequence::find_way(const Piece& piece, const std::vector<size_t>& others, bool downwards, std::map<std::string, size_t>& tried, Way& found) {

    const std::set<size_t> in(others.begin(), others.end());
    const std::vector<Touch> touching = touches_of(piece.bodies, others);
    const std::vector<Vector> directions = candidate_directions(piece, others, touching);

    // a slide; the piece's own ways and pins may lead down
    for (const Vector& direction : directions) {
        bool own = bodies[piece.bodies.front()].pin;
        for (const Vector& way : piece.ways)
            own = own || direction.dot(way.normalized()) < -0.9999;
        if (!downwards && !own && direction[2] < -0.3)
            continue;
        const std::string key = fmt::format("{:.5f} {:.5f} {:.5f}", direction[0], direction[1], direction[2]);
        const std::map<std::string, size_t>::const_iterator known = tried.find(key);
        if (known != tried.end() && in.count(known->second))
            continue;
        size_t blocker = 0;
        if (slides_out(piece.bodies, others, Xform(), direction, found, blocker)) {
            found.out = direction;
            found.legs.clear();
            return true;
        }
        tried[key] = blocker;
    }

    // a slide close to one that only just binds, then a turn and a slide, once no plain slide frees any piece
    if (!downwards)
        return false;
    const std::map<std::string, size_t>::const_iterator refined = tried.find("refined");
    if (refined == tried.end() || !in.count(refined->second)) {
        const Vector direction = refined_direction(piece.bodies, others, directions);
        size_t blocker = others.front();
        if (direction.magnitude() > 0.5 && slides_out(piece.bodies, others, Xform(), direction, found, blocker)) {
            found.out = direction;
            found.legs.clear();
            return true;
        }
        tried["refined"] = blocker;
    }
    for (const Line& axis : candidate_turns(piece, directions, touching)) {
        const std::string key = fmt::format("turn {:.3f} {:.3f} {:.3f} {:.4f} {:.4f} {:.4f}", axis.start()[0], axis.start()[1], axis.start()[2], axis.to_vector()[0], axis.to_vector()[1], axis.to_vector()[2]);
        const std::map<std::string, size_t>::const_iterator known = tried.find(key);
        if (known != tried.end() && in.count(known->second))
            continue;
        size_t blocker = others.front();
        double turned = 0.0;
        for (const double angle : TURNS) {
            bool clear = true;
            for (double part = turned + 0.25; part <= angle + 1e-9 && clear; part += 0.25)
                clear = !enters(piece.bodies, others, JointMove::turn(axis, part).at(1.0), blocker);
            if (!clear)
                break;
            turned = angle;
            const Xform reached = JointMove::turn(axis, angle).at(1.0);
            for (const Vector& direction : directions) {
                size_t ahead = 0;
                if (!slides_out(piece.bodies, others, reached, direction, found, ahead))
                    continue;
                found.out = direction;
                found.legs = {JointMove::turn(axis, angle)};
                return true;
            }
        }
        tried[key] = blocker;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// Waves
// ═══════════════════════════════════════════════════════════════════════════

Xform ModuleSequence::placement(const ModuleMove& move, double share) {

    // out of its place: the legs undone first, then back along the slide
    double length = move.travel;
    for (const JointMove& leg : move.legs)
        length += leg.is_turn() ? move.travel * 0.5 : leg.shift.magnitude();
    double left = share * length;
    Xform placed;
    for (std::vector<JointMove>::const_reverse_iterator leg = move.legs.rbegin(); leg != move.legs.rend(); ++leg) {
        const double reach = leg->is_turn() ? move.travel * 0.5 : leg->shift.magnitude();
        placed = leg->reversed().at(reach > 0.0 ? std::min(left / reach, 1.0) : 1.0) * placed;
        left = std::max(left - reach, 0.0);
    }

    return Xform::translation(-move.insertion[0] * left, -move.insertion[1] * left, -move.insertion[2] * left) * placed;
}

void ModuleSequence::compute_waves(ModuleSteps& steps) {

    std::vector<std::vector<size_t>> moving(steps.moves.size());
    for (size_t k = 0; k < steps.moves.size(); k++)
        for (const std::string& guid : steps.moves[k].elements)
            moving[k].push_back(body_of.at(guid));

    size_t wave = 0;
    std::vector<size_t> going;
    for (size_t k = 0; k < steps.moves.size(); k++) {
        ModuleMove& move = steps.moves[k];
        const std::vector<size_t>& mine = moving[k];

        // with the wave so far, unless it stands first, touches one of it or meets one of it on its way
        bool joins = !going.empty() && move.travel > 0.0;
        for (size_t g = 0; g < going.size() && joins; g++) {
            const ModuleMove& other = steps.moves[going[g]];
            const std::vector<size_t>& theirs = moving[going[g]];
            joins = other.travel > 0.0 && touches_of(mine, theirs).empty();
            for (double share = 0.0; share <= 1.0 && joins; share += 0.05)
                for (const size_t i : mine)
                    for (const size_t j : theirs) {
                        const Xform placed = placement(move, share);
                        const Xform other_placed = placement(other, share);
                        if (!joins || !bodies[i].solid->box_at(placed).intersects(bodies[j].solid->box_at(other_placed)))
                            continue;
                        const SharedWood shared = bodies[i].solid->shared_with(*bodies[j].solid, placed, other_placed);
                        joins = shared.thickness < play || shared.volume < 10.0;
                    }
        }
        if (!joins) {
            wave += k > 0 ? 1 : 0;
            going.clear();
        }
        going.push_back(k);
        move.wave = wave;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON
// ═══════════════════════════════════════════════════════════════════════════

void ModuleSequence::json_dump(const std::string& filename) const {

    nlohmann::ordered_json json;
    json["root"] = root.name;
    json["modules"] = nlohmann::ordered_json::array();
    for (const ModuleSteps& steps : every) {
        nlohmann::ordered_json moves = nlohmann::ordered_json::array();
        for (const ModuleMove& move : steps.moves) {
            nlohmann::ordered_json legs = nlohmann::ordered_json::array();
            for (const JointMove& leg : move.legs) {
                if (!leg.is_turn()) {
                    legs.push_back({{"shift", {leg.shift[0], leg.shift[1], leg.shift[2]}}});
                    continue;
                }
                const Point pivot = leg.axis.start();
                const Vector along = leg.axis.to_vector().normalized();
                legs.push_back({{"line", {pivot[0], pivot[1], pivot[2], along[0], along[1], along[2]}}, {"angle", leg.angle}});
            }
            std::vector<size_t> inside;
            for (const std::string& guid : move.elements)
                inside.push_back(body_of.at(guid));
            const AABB box = bounds_of(inside);
            moves.push_back({
                {"piece", move.piece},
                {"elements", move.elements},
                {"insertion", {move.insertion[0], move.insertion[1], move.insertion[2]}},
                {"legs", legs},
                {"travel", move.travel},
                {"swept", move.swept},
                {"wave", move.wave},
                {"meets", move.meets},
                {"low", {box.cx - box.hx, box.cy - box.hy, box.cz - box.hz}},
                {"high", {box.cx + box.hx, box.cy + box.hy, box.cz + box.hz}},
            });
        }
        json["modules"].push_back({{"name", steps.name}, {"kind", kind_of(steps.name)}, {"moves", moves}, {"locked", steps.locked}});
    }

    std::ofstream(filename) << json.dump(1);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

bool ModuleSequence::thinner_overlap(const ModuleOverlap& left, const ModuleOverlap& right) {
    return left.thickness < right.thickness;
}

std::string ModuleSequence::str() const {

    size_t turned = 0;
    size_t moved = 0;
    for (const ModuleSteps& steps : every)
        for (const ModuleMove& move : steps.moves) {
            moved += move.travel > 0.0 ? 1 : 0;
            turned += move.legs.empty() ? 0 : 1;
        }
    std::string text = fmt::format("{}: {} elements, {} modules, {} moves ({} with a turn)\n", root.name, bodies.size(), every.size(), moved, turned);

    // every kind of module once
    std::set<std::string> shown;
    for (const ModuleSteps& steps : every) {
        if (!shown.insert(kind_of(steps.name)).second)
            continue;
        text += fmt::format("  {}: {} pieces in {} waves\n", steps.name, steps.moves.size(), steps.moves.empty() ? 0 : steps.moves.back().wave + 1);
        for (size_t k = 0; k < steps.moves.size() && k < 12; k++) {
            const ModuleMove& move = steps.moves[k];
            text += fmt::format("    {:<28} in along ({:6.3f}, {:6.3f}, {:6.3f}){}\n", move.piece, move.insertion[0], move.insertion[1], move.insertion[2], move.legs.empty() ? "" : fmt::format(", then turned {:.0f} degrees", move.legs.back().angle));
        }
    }
    text += fmt::format("  locked: {}\n", stuck.empty() ? "none" : fmt::format("{} ({})", stuck.size(), fmt::join(std::vector<std::string>(stuck.begin(), stuck.begin() + std::min<size_t>(stuck.size(), 12)), ", ")));
    if (!open.empty())
        text += fmt::format("  open solids: {}\n", fmt::join(open, ", "));
    const std::vector<ModuleOverlap>::const_iterator worst = std::max_element(overlapping.begin(), overlapping.end(), thinner_overlap);
    if (!overlapping.empty())
        text += fmt::format("  overlapping as they stand: {} pairs, the worst {} and {}, {:.2f} mm\n", overlapping.size(), worst->first, worst->second, worst->thickness);

    return text;
}

std::ostream& operator<<(std::ostream& os, const ModuleSequence& sequence) {
    return os << sequence.str();
}

}
