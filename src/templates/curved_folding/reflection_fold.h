#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folding by sequential reflection, after Mitani and Igarashi, "Interactive Design of Planar Curved Folding by Reflection"
/// (Pacific Graphics 2011) and Huffman's "Curvature and Creases" (1976): a developable sheet, a planar profile swept along one direction
/// into a cylinder, is cut by a plane and the part beyond the plane is mirrored in it. The crease is the plane's section of the sheet, a
/// planar curve; the mirrored part meets the rest without a gap and its development is unchanged, so the folded sheet still unfolds flat
/// (Mitani's closure theorem). Repeating with one plane after another, each acting on the sheet as it lies after the folds before it,
/// gives a sheet with as many planar curved creases as planes; the planes are given on the unfolded sheet and the fold beyond plane j is
/// the composition of the reflections 1 to j. The same construction underlies Buri, Stotz and Weinand's curved folded plates (IBOIS,
/// IABSE-IASS 2011), where an undulating profile is folded at the reverse folds of an arch, and Engel's linear systems of folded
/// cylindrical surfaces (Structure Systems pp. 242-244), where a profile of arcs is folded into frames and arches. A frame's
/// symmetric ridge is the other planar curved crease: not the sheet reflected beyond the plane, but the folded half and its mirror
/// image meeting in the plane of symmetry along the plane's section of the sheet, straight in plan; given the `symmetry` plane, the
/// sheet ends on it and the whole folded half is mirrored in it (Engel's two-hinged frame, arches and A-frame, p. 242-244).
///
/// Every part of the sheet between two planes, and between two consecutive curves of the profile, is one lamella, a PlateCurved: its
/// outer face the exact cylinder between the two creases, its inner face the thickness under it on the sheet's physical underside (a
/// reflection swaps the sheet's faces, so the offset alternates sides on the unfolded sheet), its side faces at the creases the mitre
/// of thick folded plates, the fold's bisector surface, where it meets its neighbour, and its caps the rulings at the profile
/// curves' ends, where the lamellas of one fold meet along a straight line. Every lamella develops flat without distortion, being a
/// cylinder strip; the development of each is checked and drawn in the flat cutting pattern beside the model.
///
/// Fields: the profile, the rulings' direction and length, the planes, the thickness and the sampling; the sheet, the creases, the
/// lamellas and the flat pattern are in the session.
class ReflectionFold : public WoodSession {
public:
    const std::vector<NurbsCurve> profile; // The sheet's section: planar curves one after another sharing their ends, running one way, each a row of lamellas.
    const Vector direction; // The rulings' unit direction, the profile swept along it.
    const double length; // mm, the sheet along the rulings; with a symmetry plane, how far the mirror planes may reach at most.
    const std::vector<Plane> planes; // The mirror planes on the unfolded sheet, in order along the rulings.
    const std::optional<Plane> symmetry; // The folded sheet's plane of symmetry, where the sheet ends and its mirror image begins; none for a sheet with two free ends.
    const double thickness; // mm, every lamella, on the sheet's physical underside: the side away from the profile's tangent crossed with the direction.
    const size_t samples; // Rulings per profile curve the offsets and mitres are interpolated through.

    /// The reflection fold as the session named name.
    ReflectionFold(
        const std::vector<NurbsCurve>& profile,
        const Vector& direction,
        double length,
        const std::vector<Plane>& planes,
        const std::optional<Plane>& symmetry = std::nullopt,
        double thickness = 120.0,
        const std::string& name = "reflection_fold",
        size_t samples = 24
    );

    /// Not copied, as a session is not.
    ReflectionFold(const ReflectionFold&) = delete;

    /// Not assigned, as it is not copied.
    ReflectionFold& operator=(const ReflectionFold&) = delete;

    /// The largest relative length error of any lamella's development, the proof every lamella is cut flat.
    double distortion() const {
        return _distortion;
    }

    /// The largest twist of any lamella's rulings out of the tangent planes, 0 on a cylinder.
    double twist() const {
        return _twist;
    }

    /// Mirror planes on the unfolded sheet that turn the folded sheet about axis by the angles (radians, as seen on the folded model,
    /// positive by the right hand), one at each station along the ruling through origin; the axis lies in every plane.
    static std::vector<Plane> turns(const Point& origin, const Vector& direction, const Vector& axis, const std::vector<double>& stations, const std::vector<double>& angles);

    /// A profile of count circular arcs side by side from start along across, each width wide and bulging rise toward up: Engel's rows of barrels.
    static std::vector<NurbsCurve> arcs(const Point& start, const Vector& across, const Vector& up, size_t count, double width, double rise);

    /// An undulating profile from start along across: half_waves half sine waves, each half_wavelength long and amplitude high toward up, first up: Buri's corrugation.
    static std::vector<NurbsCurve> waves(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double amplitude);

private:
    double _distortion = 0.0;
    double _twist = 0.0;

    /// The planes that bound the folds: the profile's plane at the start, the mirror planes, and at the end the profile's plane moved by
    /// length, or the symmetry plane carried back onto the unfolded sheet by the last fold; each checked to cut every ruling of the sheet
    /// in this order and never to stand on the sheet, where the fold would be flat.
    std::vector<Plane> compute_cuts(const std::vector<Xform>& folds) const;

    /// Fold j for the sheet beyond mirror plane j: the reflections in the planes 1 to j composed, fold 0 the identity.
    std::vector<Xform> compute_folds() const;

    /// The profile moved the thickness to one side of the sheet, side -1 under and +1 over, curve by curve through the sampled points,
    /// each curve's ends mitred with its neighbour where the profile has a kink.
    std::vector<NurbsCurve> compute_offset(double side) const;

    /// The inner meridian of a plate at a mirror plane, on the unfolded sheet: every ruling of the plate's offset, the curve the thickness
    /// to one side of the profile curve swept along the direction, cut by the fold's bisector surface, the surface through the crease
    /// perpendicular to the mirror plane. The two plates at a crease lie on the two sides of that surface, so they neither overlap nor
    /// leave a gap, each keeps its exact thickness, and their side faces lie in one surface; a thick folded plate's mitre.
    NurbsCurve compute_mitre(const NurbsCurve& curve, const NurbsCurve& offset, const Plane& cut) const;

    /// The unfolded sheet with the creases drawn on it, in the group sheet.
    void add_sheet(const std::vector<Plane>& cuts);

    /// Every crease on the folded sheet, in the group creases.
    void add_creases(const std::vector<Plane>& cuts, const std::vector<Xform>& folds);

    /// Every lamella, lamella_<fold>_<row>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_lamellas(const std::vector<Plane>& cuts, const std::vector<Xform>& folds, const std::vector<NurbsCurve>& under, const std::vector<NurbsCurve>& over);

    /// The flat cutting pattern beside the model, every lamella's development where it lies on the unfolded sheet, in the group flat.
    void add_flat(const std::vector<Development>& developments, const std::vector<Plane>& cuts, double beside);

    /// The mirror image of every lamella and crease in the symmetry plane, lamella_<2 folds - 1 - j>_<row>, the plane's own section of
    /// the sheet as the last crease, and the mirrored half's cutting pattern beyond the first's.
    void add_mirrored(const std::vector<Plane>& cuts, const std::vector<Xform>& folds);

    /// The reflection in the plane.
    static Xform reflection(const Plane& plane);

    /// The vector turned about axis by angle (radians).
    static Vector turned(const Vector& vector, const Vector& axis, double angle);
};

inline ReflectionFold::ReflectionFold(
    const std::vector<NurbsCurve>& profile,
    const Vector& direction,
    double length,
    const std::vector<Plane>& planes,
    const std::optional<Plane>& symmetry,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      profile(profile),
      direction(direction.normalized()),
      length(length),
      planes(planes),
      symmetry(symmetry),
      thickness(thickness),
      samples(std::max<size_t>(samples, 4)) {

    if (profile.empty() || length <= 0.0 || thickness <= 0.0)
        throw std::invalid_argument("A reflection fold needs a profile, a length and a thickness");

    // folds: the sheet beyond mirror plane j carried by the reflections 1 to j, Mitani's sequential reflection
    const std::vector<Xform> folds = compute_folds();

    // cuts: the sheet's end planes with the mirror planes between them, every one across all the rulings, in order; the end the symmetry plane pulled back onto the unfolded sheet when there is one
    const std::vector<Plane> cuts = compute_cuts(folds);

    // offsets: the profile the thickness under the sheet and over it, the plates' inner faces on the unfolded sheet, since every reflection swaps the sheet's faces
    const std::vector<NurbsCurve> under = compute_offset(-1.0);
    const std::vector<NurbsCurve> over = compute_offset(1.0);

    // sheet: the unfolded developable with its creases, Mitani's left column
    add_sheet(cuts);

    // creases: every mirror plane's section of the sheet, folded
    add_creases(cuts, folds);

    // lamellas: between two cuts and two profile curves one curved plate, its creases mitred with its neighbours
    const std::vector<Development> developments = add_lamellas(cuts, folds, under, over);

    // flat: the cutting pattern beside the model, every lamella unfolded by its rulings
    add_flat(developments, cuts, 0.1 * length);

    // symmetry: the folded half mirrored in its plane of symmetry, meeting itself along the plane's section of the sheet
    if (symmetry)
        add_mirrored(cuts, folds);
}

inline std::vector<Plane> ReflectionFold::compute_cuts(const std::vector<Xform>& folds) const {

    // the profile's plane: the normal from the two most different tangents, or across the direction when the profile is straight
    std::vector<Point> points;
    std::vector<Vector> tangents;

    for (const NurbsCurve& curve : profile)
        for (size_t k = 0; k <= samples; k++) {
            const double t = curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
            points.push_back(curve.point_at(t));
            tangents.push_back(curve.tangent_at(t).normalized());
        }

    Vector normal = tangents.front().cross(tangents.back());

    for (const Vector& tangent : tangents)
        if (tangents.front().cross(tangent).magnitude() > normal.magnitude())
            normal = tangents.front().cross(tangent);

    if (normal.magnitude() < 1e-6)
        normal = direction - tangents.front() * direction.dot(tangents.front());

    normal.normalize_self();

    if (std::abs(normal.dot(direction)) < 1e-6)
        throw std::invalid_argument("The profile's plane contains the direction, so the sheet has no width");

    std::vector<Plane> cuts = {Plane::from_point_normal(points.front(), normal)};
    cuts.insert(cuts.end(), planes.begin(), planes.end());

    if (symmetry) {
        const std::optional<Xform> back = folds.back().inverse();

        if (!back)
            throw std::invalid_argument("The folds cannot be undone");

        cuts.push_back(Plane::from_point_normal(back->transform_point(symmetry->origin()), back->transform_vector(symmetry->z_axis())));
    } else {
        cuts.push_back(Plane::from_point_normal(points.front() + direction * length, normal));
    }

    // every ruling meets the cuts in order within the sheet, and no mirror plane stands on the sheet
    for (size_t k = 0; k < points.size(); k++) {
        double before = -1.0;
        const Vector sheet_normal = tangents[k].cross(direction).normalized();

        for (size_t j = 0; j < cuts.size(); j++) {
            const double slope = cuts[j].z_axis().dot(direction);

            if (std::abs(slope) < 1e-9)
                throw std::invalid_argument(fmt::format("Cut plane {} runs along the rulings", j));

            const double along = cuts[j].z_axis().dot(cuts[j].origin() - points[k]) / slope;

            if (along < before - 1e-6 || (along > length + 1e-6 && !(symmetry && j + 1 == cuts.size())))
                throw std::invalid_argument(fmt::format("Cut plane {} leaves the sheet or crosses the cut before it at ruling {}", j, k));

            if (j > 0 && j + 1 < cuts.size() && std::abs(cuts[j].z_axis().dot(sheet_normal)) < 0.05)
                throw std::invalid_argument(fmt::format("Mirror plane {} stands on the sheet at ruling {}, where its fold would be flat", j, k));

            before = along;
        }
    }

    return cuts;
}

inline std::vector<Xform> ReflectionFold::compute_folds() const {

    std::vector<Xform> folds = {Xform::identity()};

    for (const Plane& plane : planes)
        folds.push_back(folds.back() * reflection(plane));

    return folds;
}

inline std::vector<NurbsCurve> ReflectionFold::compute_offset(double side) const {

    // the sheet's top normal at every sample of every curve: the tangent crossed with the direction
    std::vector<std::vector<Point>> points(profile.size());
    std::vector<std::vector<Vector>> normals(profile.size());

    for (size_t i = 0; i < profile.size(); i++)
        for (size_t k = 0; k <= samples; k++) {
            const double t = profile[i].domain_start() + (profile[i].domain_end() - profile[i].domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
            points[i].push_back(profile[i].point_at(t));
            normals[i].push_back(profile[i].tangent_at(t).cross(direction).normalized());
        }

    // every curve moved along its normals, its ends along the mitre of its own and its neighbour's normal
    std::vector<NurbsCurve> offset;

    for (size_t i = 0; i < profile.size(); i++) {
        std::vector<Point> moved;

        for (size_t k = 0; k <= samples; k++) {
            const Vector& own = normals[i][k];
            const Vector& neighbour = k == 0 && i > 0 ? normals[i - 1].back() : k == samples && i + 1 < profile.size() ? normals[i + 1].front() : own;
            const Vector mitre = (own + neighbour) * (1.0 / (1.0 + own.dot(neighbour)));
            moved.push_back(points[i][k] + mitre * (side * thickness));
        }

        offset.push_back(NurbsCurve::create_interpolated(moved, CurveNurbsKnotStyle::Uniform));
    }

    return offset;
}

inline NurbsCurve ReflectionFold::compute_mitre(const NurbsCurve& curve, const NurbsCurve& offset, const Plane& cut) const {

    const Xform to_cut = Xform::project_to_plane_by_axis(cut, direction);
    const Vector normal = cut.z_axis().normalized();
    std::vector<Point> points;

    for (size_t k = 0; k <= samples; k++) {
        // the crease point on this ruling and the crease's tangent there, the bisector surface's plane through them
        const double t = curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
        const double s = offset.domain_start() + (offset.domain_end() - offset.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
        const Point crease = to_cut.transform_point(curve.point_at(t));
        const Vector along = to_cut.transform_vector(curve.tangent_at(t));
        const Vector bisector = along.cross(normal);
        const double speed = bisector.dot(direction);

        if (bisector.magnitude() < 1e-12 || std::abs(speed) < 1e-9 * bisector.magnitude())
            throw std::invalid_argument(fmt::format("The fold's bisector surface runs along the rulings at ruling {}", k));

        // the offset's ruling where it crosses the bisector surface
        const Point start = offset.point_at(s);
        points.push_back(start + direction * (bisector.dot(crease - start) / speed));
    }

    return NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform);
}

inline void ReflectionFold::add_sheet(const std::vector<Plane>& cuts) {

    const std::shared_ptr<TreeNode> sheet = add_group("sheet");

    // the outline: the profile at the start, the last ruling, the profile at the end back and the first ruling
    std::vector<Point> start;
    std::vector<Point> end;
    const Xform to_end = Xform::project_to_plane_by_axis(cuts.back(), direction);

    for (const NurbsCurve& curve : profile)
        for (size_t k = 0; k <= samples; k++) {
            const double t = curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
            start.push_back(curve.point_at(t));
            end.push_back(to_end.transform_point(start.back()));
        }

    std::vector<Point> outline = start;
    outline.insert(outline.end(), end.rbegin(), end.rend());
    outline.push_back(start.front());
    Polyline polyline(outline);
    polyline.name = "sheet_outline";
    add_polyline(polyline, sheet);

    // every crease where its mirror plane cuts the unfolded sheet
    for (size_t j = 1; j + 1 < cuts.size(); j++) {
        const Xform to_plane = Xform::project_to_plane_by_axis(cuts[j], direction);
        std::vector<Point> crease;

        for (const Point& point : start)
            crease.push_back(to_plane.transform_point(point));

        Polyline drawn(crease);
        drawn.name = fmt::format("sheet_crease_{}", j);
        add_polyline(drawn, sheet);
    }
}

inline void ReflectionFold::add_creases(const std::vector<Plane>& cuts, const std::vector<Xform>& folds) {

    const std::shared_ptr<TreeNode> creases = add_group("creases");

    for (size_t j = 1; j + 1 < cuts.size(); j++) {
        const Xform to_crease = folds[j] * Xform::project_to_plane_by_axis(cuts[j], direction);
        std::vector<Point> crease;

        for (const NurbsCurve& curve : profile)
            for (size_t k = 0; k <= samples; k++) {
                const double t = curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
                crease.push_back(to_crease.transform_point(curve.point_at(t)));
            }

        Polyline drawn(crease);
        drawn.name = fmt::format("crease_{}", j);
        add_polyline(drawn, creases);
    }
}

inline std::vector<Development> ReflectionFold::add_lamellas(const std::vector<Plane>& cuts, const std::vector<Xform>& folds, const std::vector<NurbsCurve>& under, const std::vector<NurbsCurve>& over) {

    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");
    std::vector<Development> developments;

    for (size_t j = 0; j + 1 < cuts.size(); j++) {
        // this fold's inner side on the unfolded sheet, the other one than its neighbours'
        const std::vector<NurbsCurve>& mine = j % 2 == 0 ? under : over;
        const Xform to_start = Xform::project_to_plane_by_axis(cuts[j], direction);
        const Xform to_end = Xform::project_to_plane_by_axis(cuts[j + 1], direction);

        for (size_t i = 0; i < profile.size(); i++) {
            // the outer meridians: the profile curve on the two cuts, exact; the inner: the offset cut flush at the sheet's ends, by the fold's bisector at the creases
            const NurbsCurve outer_a = profile[i].transformed(folds[j] * to_start);
            const NurbsCurve outer_b = profile[i].transformed(folds[j] * to_end);
            const NurbsCurve inner_a = j == 0 ? mine[i].transformed(folds[j] * to_start) : compute_mitre(profile[i], mine[i], cuts[j]).transformed(folds[j]);
            const NurbsCurve inner_b = j + 2 == cuts.size() ? mine[i].transformed(folds[j] * to_end) : compute_mitre(profile[i], mine[i], cuts[j + 1]).transformed(folds[j]);
            add(std::make_shared<PlateCurved>(outer_a, outer_b, inner_a, inner_b, fmt::format("lamella_{}_{}", j, i)), lamellas);

            // its development, the proof it is cut flat
            developments.emplace_back(outer_a, outer_b, samples);
            _distortion = std::max(_distortion, developments.back().distortion);
            _twist = std::max(_twist, developments.back().twist);
        }
    }

    return developments;
}

inline void ReflectionFold::add_flat(const std::vector<Development>& developments, const std::vector<Plane>& cuts, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    // the pattern starts beside the model's largest x; its x runs along the profile's development, its y along the rulings
    double largest = -std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Element>& element : *objects.elements)
        if (const std::shared_ptr<PlateCurved> lamella = std::dynamic_pointer_cast<PlateCurved>(element))
            for (const NurbsCurve* curve : {&lamella->outer_a, &lamella->outer_b})
                for (size_t k = 0; k <= samples; k++)
                    largest = std::max(largest, curve->point_at(curve->domain_start() + (curve->domain_end() - curve->domain_start()) * static_cast<double>(k) / static_cast<double>(samples))[0]);

    // every profile curve's start along the development: the lengths of the curves before it across the rulings
    const Xform across = Xform::project_to_plane_by_axis(Plane::from_point_normal(Point(0.0, 0.0, 0.0), direction), direction);
    std::vector<double> starts = {0.0};

    for (const NurbsCurve& curve : profile)
        starts.push_back(starts.back() + curve.transformed(across).length());

    // every lamella's outline moved to its place: its row's start along x, its crease's distance along the ruling from the sheet's start along y
    size_t index = 0;

    for (size_t j = 0; j + 1 < cuts.size(); j++) {
        const Xform to_cut = Xform::project_to_plane_by_axis(cuts[j], direction);
        const Xform to_sheet = Xform::project_to_plane_by_axis(cuts.front(), direction);

        for (size_t i = 0; i < profile.size(); i++, index++) {
            const Point first = profile[i].point_at(profile[i].domain_start());
            const double along = (to_cut.transform_point(first) - to_sheet.transform_point(first)).dot(direction);
            Polyline outline = developments[index].outline.transformed(Xform::translation(largest + beside + starts[i], along, 0.0));
            outline.name = fmt::format("flat_{}_{}", j, i);
            add_polyline(outline, flat);
        }
    }
}

inline void ReflectionFold::add_mirrored(const std::vector<Plane>& cuts, const std::vector<Xform>& folds) {

    const Xform mirror = reflection(*symmetry);
    const size_t folded = cuts.size() - 1;

    // the lamellas: the mirror image of every plate, the pair at the symmetry plane sharing its side face there
    const std::shared_ptr<TreeNode> lamellas = group_named("lamellas");
    const std::vector<std::shared_ptr<PlateCurved>> plates = get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& plate : plates) {
        const size_t fold = std::stoul(plate->name.substr(8, plate->name.find('_', 8) - 8));
        const std::string row = plate->name.substr(plate->name.find('_', 8) + 1);
        add(std::make_shared<PlateCurved>(plate->outer_a.transformed(mirror), plate->outer_b.transformed(mirror), plate->inner_a.transformed(mirror), plate->inner_b.transformed(mirror), fmt::format("lamella_{}_{}", 2 * folded - 1 - fold, row)), lamellas);
    }

    // the creases: the symmetry plane's own section of the sheet, then every crease mirrored
    const std::shared_ptr<TreeNode> creases = group_named("creases");
    const Xform to_end = folds.back() * Xform::project_to_plane_by_axis(cuts.back(), direction);
    std::vector<Point> ridge;

    for (const NurbsCurve& curve : profile)
        for (size_t k = 0; k <= samples; k++) {
            const double t = curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
            ridge.push_back(to_end.transform_point(curve.point_at(t)));
        }

    Polyline drawn(ridge);
    drawn.name = fmt::format("crease_{}", folded);
    add_polyline(drawn, creases);

    std::vector<Polyline> folded_creases;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("crease_", 0) == 0 && polyline->name != drawn.name)
            folded_creases.push_back(*polyline);

    for (const Polyline& crease : folded_creases) {
        Polyline mirrored = crease.transformed(mirror);
        mirrored.name = fmt::format("crease_{}", 2 * folded - std::stoul(crease.name.substr(7)));
        add_polyline(mirrored, creases);
    }

    // the flat pattern: the mirrored half's developments beyond the first's, turned over so the two halves face each other along the symmetry plane's section
    const std::shared_ptr<TreeNode> flat = group_named("flat");
    std::vector<Polyline> outlines;
    double top = -std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("flat_", 0) == 0) {
            outlines.push_back(*polyline);

            for (size_t k = 0; k < polyline->point_count(); k++)
                top = std::max(top, polyline->get_point(k)[1]);
        }

    const Xform turn_over = Xform::scale_non_uniform(Point(0.0, top + 0.05 * length, 0.0), 1.0, -1.0, 1.0);

    for (const Polyline& outline : outlines) {
        const size_t fold = std::stoul(outline.name.substr(5, outline.name.find('_', 5) - 5));
        Polyline mirrored = outline.transformed(turn_over);
        mirrored.name = fmt::format("flat_{}_{}", 2 * folded - 1 - fold, outline.name.substr(outline.name.find('_', 5) + 1));
        add_polyline(mirrored, flat);
    }
}

inline Xform ReflectionFold::reflection(const Plane& plane) {

    const Vector n = plane.z_axis().normalized();
    const double h = n.dot(Vector(plane.origin()[0], plane.origin()[1], plane.origin()[2]));
    std::array<double, 16> m = {};

    for (int column = 0; column < 3; column++)
        for (int row = 0; row < 3; row++)
            m[column * 4 + row] = (row == column ? 1.0 : 0.0) - 2.0 * n[row] * n[column];

    for (int row = 0; row < 3; row++)
        m[12 + row] = 2.0 * h * n[row];

    m[15] = 1.0;

    return Xform::from_matrix(m);
}

inline Vector ReflectionFold::turned(const Vector& vector, const Vector& axis, double angle) {

    const Vector k = axis.normalized();

    return vector * std::cos(angle) + k.cross(vector) * std::sin(angle) + k * (k.dot(vector) * (1.0 - std::cos(angle)));
}

inline std::vector<Plane> ReflectionFold::turns(const Point& origin, const Vector& direction, const Vector& axis, const std::vector<double>& stations, const std::vector<double>& angles) {

    if (stations.size() != angles.size())
        throw std::invalid_argument("Every station needs its angle");

    // a reflection reverses the sense of every turn after it, so the angle on the unfolded sheet alternates its sign
    const Vector d = direction.normalized();
    std::vector<Plane> planes;

    for (size_t j = 0; j < stations.size(); j++) {
        const double angle = (j % 2 == 0 ? 1.0 : -1.0) * angles[j];
        const Vector normal = d - turned(d, axis, angle);

        if (normal.magnitude() < 1e-9)
            throw std::invalid_argument(fmt::format("Turn {} is no turn", j));

        planes.push_back(Plane::from_point_normal(origin + d * stations[j], normal));
    }

    return planes;
}

inline std::vector<NurbsCurve> ReflectionFold::arcs(const Point& start, const Vector& across, const Vector& up, size_t count, double width, double rise) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < count; i++) {
        const Point from = start + a * (width * static_cast<double>(i));
        curves.push_back(Primitives::arc(from, from + a * (0.5 * width) + u * rise, from + a * width));
    }

    return curves;
}

inline std::vector<NurbsCurve> ReflectionFold::waves(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double amplitude) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    const size_t points = 12;
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < half_waves; i++) {
        const double sign = i % 2 == 0 ? 1.0 : -1.0;
        std::vector<Point> wave;

        for (size_t k = 0; k <= points; k++) {
            const double s = static_cast<double>(k) / static_cast<double>(points);
            wave.push_back(start + a * (half_wavelength * (static_cast<double>(i) + s)) + u * (sign * amplitude * std::sin(Tolerance::PI * s)));
        }

        curves.push_back(NurbsCurve::create_interpolated(wave));
    }

    return curves;
}
