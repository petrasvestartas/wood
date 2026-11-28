#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_module_sequence.h"
#include "src/templates/cross/vda_mesh.h"
#include "src/templates/developable_folding/crease_quadrants.h"
#include "src/templates/developable_folding/curved_crease_tube.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"
#include "src/templates/developable_folding/curved_fold.h"
#include "src/templates/developable_folding/curved_miura.h"
#include "src/templates/developable_folding/space_curve_fold.h"
#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/folded_frame.h"
#include "src/templates/frame/frame.h"
#include "src/templates/grid/grid.h"
#include "src/templates/module/cassette_shell.h"
#include "src/templates/plaited/plaited.h"
#include "src/templates/vault/surface_vault.h"
#include "src/templates/vault/tile_pattern.h"
#include "src/templates/vidy/vidy.h"

using namespace laika_cpp;
using namespace wood_session;

/// Throws with the message when the condition fails.
static void check(bool ok, const std::string& message) {

    if (!ok)
        throw std::runtime_error(message);
}

/// A block of the box from low to high.
static std::shared_ptr<FacePlanarElement> block(const Point& low, const Point& high, const std::string& name) {

    const Polyline bottom = Polyline::rectangle(low, Vector::x_axis(), Vector::y_axis(), high[0] - low[0], high[1] - low[1]);

    return std::make_shared<FacePlanarElement>(bottom, bottom.translated(Vector(0.0, 0.0, high[2] - low[2])), name);
}

/// A block on a block lifts off; one walled in on every side is locked, and said to be.
static void blocks() {

    WoodSession scene("blocks");
    const std::shared_ptr<FacePlanarElement> under = block(Point(0.0, 0.0, 0.0), Point(400.0, 400.0, 100.0), "under");
    const std::shared_ptr<FacePlanarElement> core = block(Point(100.0, 100.0, 100.0), Point(300.0, 300.0, 200.0), "core");
    scene.add(under);
    scene.add(core);
    const ModuleSequence pair(scene, AssemblyModule::module("pair", {AssemblyModule::part(under), AssemblyModule::part(core)}));
    check(pair.locked().empty() && pair.modules()[0].moves[1].piece == "core", "blocks: the core goes in last");
    check(pair.modules()[0].moves[1].insertion.dot(Vector::z_axis()) < -0.999, "blocks: the core goes in straight down");
    check(pair.overlaps().empty() && pair.open_solids().empty(), "blocks: two closed solids that touch");

    // walls around the core and a lid on it, held as one piece
    std::vector<std::shared_ptr<Element>> shell = {under};
    shell.push_back(block(Point(0.0, 0.0, 100.0), Point(100.0, 400.0, 200.0), "left"));
    shell.push_back(block(Point(300.0, 0.0, 100.0), Point(400.0, 400.0, 200.0), "right"));
    shell.push_back(block(Point(100.0, 0.0, 100.0), Point(300.0, 100.0, 200.0), "front"));
    shell.push_back(block(Point(100.0, 300.0, 100.0), Point(300.0, 400.0, 200.0), "back"));
    shell.push_back(block(Point(0.0, 0.0, 200.0), Point(400.0, 400.0, 300.0), "lid"));
    for (size_t i = 1; i < shell.size(); i++)
        scene.add(shell[i]);
    const ModuleSequence closed(scene, AssemblyModule::module("closed", {AssemblyModule::part("shell", shell), AssemblyModule::part(core)}));
    check(closed.locked().size() == 2, "blocks: the core inside the shell is locked");
}

/// The sweep stays clear: every move of every module sampled with solid_boolean on the elements' own meshes, the solver's SolidBody aside.
static void check_swept(const WoodSession& session, const ModuleSequence& sequence) {

    std::map<std::string, Mesh> meshes;
    for (const std::shared_ptr<Element>& element : session.elements()) {
        const std::shared_ptr<FaceCurvedElement> curved = std::dynamic_pointer_cast<FaceCurvedElement>(element);
        meshes[element->guid()] = curved ? curved->closed_mesh() : element->model_geometry_mesh();
    }
    for (const ModuleSteps& steps : sequence.modules()) {
        std::vector<std::string> placed;
        for (const ModuleMove& move : steps.moves) {
            for (const double distance : {3.0, 17.0, 55.0, 130.0}) {
                if (distance > move.travel)
                    continue;
                Xform at;
                for (std::vector<JointMove>::const_reverse_iterator leg = move.legs.rbegin(); leg != move.legs.rend(); ++leg)
                    at = leg->reversed().at(1.0) * at;
                at = Xform::translation(-move.insertion[0] * distance, -move.insertion[1] * distance, -move.insertion[2] * distance) * at;
                for (const std::string& guid : move.elements) {
                    const Mesh moved = meshes[guid].transformed(at);
                    const AABB box = AABB::from_mesh(moved);
                    for (const std::string& other : placed) {
                        if (!box.intersects(AABB::from_mesh(meshes[other])))
                            continue;
                        const Mesh common = solid_boolean(moved, meshes[other], SolidOperation::intersect);
                        const double volume = common.number_of_faces() ? compute_volume(common) : 0.0;
                        check(volume < 0.5 * sequence.play * common.area() + 10.0, fmt::format("{}: {} shares {:.0f} mm3 with a piece already in, {} mm before its place", steps.name, move.piece, volume, distance));
                    }
                }
            }
            placed.insert(placed.end(), move.elements.begin(), move.elements.end());
        }
    }
}

/// Every assembly of the template: nothing locked, every solid closed, every sampled move clear.
static void check_template(const WoodSession& session, size_t whole) {

    check(session.assemblies.size() == 1, fmt::format("{}: one assembly", session.name));
    const ModuleSequence sequence(session, session.assemblies[0], 10.0, session.assemblies[0].play);
    std::cout << sequence;
    check(sequence.locked().empty(), fmt::format("{}: {} pieces locked", session.name, sequence.locked().size()));
    check(sequence.open_solids().empty(), fmt::format("{}: open solids", session.name));
    check(sequence.modules().back().moves.size() == whole, fmt::format("{}: {} pieces in the whole, not {}", session.name, sequence.modules().back().moves.size(), whole));
    check_swept(session, sequence);
}

int main() {

    blocks();

    // a pavilion of two rows: a row's two walls of plates, its roof lowered on them, then the next row's
    const wood_vidy::VidyPavilion pavilion(wood_vidy::VidyGuide(2));
    check_template(pavilion, 6);
    const std::vector<ModuleMove> rows = ModuleSequence(pavilion, pavilion.assemblies[0]).modules().back().moves;
    check(rows[2].piece == "roof_0" && rows[5].piece == "roof_1", "vidy: a row's roof goes on after its walls, before the next row's");
    check(rows[2].insertion.dot(Vector::z_axis()) < -0.999, "vidy: the roof is lowered on the walls");

    check_template(wood_folding::CounterFold(wood_folding::CounterFoldGuide::elevated_fold()), 4);

    // a folded frame: four rows along its bays, one after another from one foot over the crown to the other
    const wood_folding::FoldedFrame frame;
    check_template(frame, 4);
    const std::vector<ModuleMove> along = ModuleSequence(frame, frame.assemblies[0]).modules().back().moves;
    for (size_t i = 0; i < along.size(); i++)
        check(along[i].piece == fmt::format("row_{}", i) && along[i].wave == i, "folded frame: the rows go in one by one from foot to foot");
    check_template(wood_module::CassetteShell(), 15);
    check_template(wood_plaited::PlaitedStereotomy(wood_plaited::flat_quad_mesh(3600.0, 3600.0, 6, 6, 0.0, 220.0), wood_plaited::PlaitedPattern::Abeille, 220.0, 0.48), 36);

    // the connectors turned along the slides: every plate, its connectors to the plates after it on it, onto all of its own at once
    check_template(wood_cross::VdaMesh(wood_cross::VdaMeshGuide::dome_assembly()), 15);

    // curved-crease unfolding: target mitered vault, closed flat-packed stack (w = 0), level crown and four crease quadrants
    wood_developable_folding::CurvedCreaseUnfoldingGuide ccu = wood_developable_folding::CurvedCreaseUnfoldingGuide::monoclastic();
    ccu.planes = wood_developable_folding::CurvedCreaseUnfoldingGuide::vault(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(1.0, 0.0, 0.0), 8, 450.0, 60.0);
    const wood_developable_folding::CurvedCreaseUnfolding unfolded(ccu);
    check(unfolded.distortion() < 1e-4 && unfolded.drift() < 1e-3, "ccu: zero distortion and floor drift on synclastic vault");
    check(unfolded.get_contacts().size() == 7, "ccu: 7 shared miter contacts across 8 plates");
    const std::shared_ptr<FaceCurvedElement> folded_plate = unfolded.get_element_by_name<FaceCurvedElement>("plate_4");
    const Development flat_dev(*folded_plate, 48);
    check(flat_dev.piece && flat_dev.piece->element_geometry_brep().is_solid(), "ccu unfold: flat piece is a closed 6-patch BRep solid");
    check(std::abs(flat_dev.thickness - 30.0) < 1e-3, "ccu unfold: flat piece preserves 30 mm thickness");
    check(std::abs(flat_dev.bevel_a - 30.0 / std::tan(55.0 * Tolerance::TO_RADIANS)) < 0.1, "ccu unfold: flat piece preserves t * cot(alpha) miter bevel");
    check(flat_dev.distortion < 1e-4, "ccu unfold: sector-angle unfolding has zero distortion");
    check_template(unfolded, 4);

    wood_developable_folding::CurvedCreaseUnfoldingGuide stack = ccu;
    stack.deployments = {0.0};
    const wood_developable_folding::CurvedCreaseUnfolding closed_stack(stack, "closed_stack");
    check(closed_stack.distortion() < 1e-4, "ccu closed: planar strips remain developable");
    double min_stack_y = std::numeric_limits<double>::infinity();
    double max_stack_y = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < 8; i++) {
        const std::shared_ptr<FaceCurvedElement> plate = closed_stack.get_element_by_name<FaceCurvedElement>(fmt::format("plate_{}", i));
        check(plate && plate->element_geometry_brep().is_solid(), "ccu closed: every flat-packed plate is a closed solid");
        for (const Point& corner : plate->aabb().corners()) {
            min_stack_y = std::min(min_stack_y, corner[1]);
            max_stack_y = std::max(max_stack_y, corner[1]);
        }
    }
    check(std::abs((max_stack_y - min_stack_y) - 8.0 * 30.0) < 1.0, "ccu closed: flat-folded stack thickness equals n * t (240 mm)");
    check_template(closed_stack, 8);

    // without thickness: the strips, creases and crease pattern alone, no plate, the same development
    wood_developable_folding::CurvedCreaseUnfoldingGuide bare = ccu;
    bare.thickness = 0.0;
    const wood_developable_folding::CurvedCreaseUnfolding surfaces(bare, "surfaces");
    check(surfaces.elements().size() == 0 && surfaces.assemblies.empty(), "ccu thickness 0: no plate is made");
    check(surfaces.distortion() == unfolded.distortion() && surfaces.drift() == unfolded.drift(), "ccu thickness 0: the same strips as with plates");

    const wood_developable_folding::CurvedCreaseUnfolding leaned(wood_developable_folding::CurvedCreaseUnfoldingGuide::level_crown());
    check(leaned.distortion() < 1e-4 && leaned.get_contacts().size() == 5, "ccu leaning: developable with 5 miter contacts");

    const wood_developable_folding::CreaseQuadrants quadrants(wood_developable_folding::CreaseQuadrantsGuide::arc_crease());
    const std::array<double, 4>& sums = quadrants.sector_sums();
    check(std::abs(sums[0] - 2.0 * Tolerance::PI) < 1e-6 && std::abs(sums[2] - 2.0 * Tolerance::PI) < 1e-6, "quadrants: Q1 and Q3 sum to 2*pi");
    check(sums[1] < 2.0 * Tolerance::PI - 1e-3 && sums[3] > 2.0 * Tolerance::PI + 1e-3, "quadrants: Q2 synclastic (< 2*pi) and Q4 anticlastic (> 2*pi)");

    wood_developable_folding::CurvedCreaseTubeGuide tubes = wood_developable_folding::CurvedCreaseTubeGuide::cellular_vault();
    tubes.tubes = 4;
    check_template(wood_developable_folding::CurvedCreaseTube(tubes), 4);
    wood_developable_folding::CurvedCreaseTubeGuide deployed = tubes;
    deployed.tubes = 2;
    deployed.deployments = {0.45};
    const wood_developable_folding::CurvedCreaseTube tube_check(deployed, "tube_check");
    check(tube_check.distortion() < 1e-3 && tube_check.min_speed() > 0.1, "tachi tube: rigid 1-DOF space-curve deployment stays developable");

    check_template(wood_developable_folding::CurvedFold(), 12);

    // a fold along a space curve: planar quads, flat-foldable vertices, one opening along the crease, its plates put in one by one
    wood_developable_folding::SpaceCurveFoldGuide arc = wood_developable_folding::SpaceCurveFoldGuide::helix_fold();
    arc.curve = wood_developable_folding::SpaceCurveFoldGuide::helix(2000.0, 3000.0, 0.4, 8);
    const wood_developable_folding::SpaceCurveFold fold(arc);
    const wood_developable_folding::FoldMeasures& folded = fold.measures();
    check(folded.planarity < 1e-9 && folded.kawasaki < 1e-9 && folded.developability < 1e-9, "space curve fold: planar quads, developable and flat-foldable vertices");
    check(std::abs(folded.openings[0] - arc.angle) < 1e-9 && std::abs(folded.openings[1] - arc.angle) < 1e-9, "space curve fold: one opening along the crease");
    check_template(fold, 16);
    const wood_developable_folding::SpaceCurveFold zigzag(wood_developable_folding::SpaceCurveFoldGuide::zigzag_fold(), "zigzag");
    check(zigzag.measures().openings[0] < 0.021 * arc.angle && zigzag.measures().crossings == 0 && zigzag.measures().stretch < 1e-6, "space curve fold: the zig-zag folds flat, every edge kept, no quads crossing");

    // the smooth fold: every arm one curved plate, its development exact, both put in one after the other
    wood_developable_folding::SpaceCurveFoldGuide smooth = wood_developable_folding::SpaceCurveFoldGuide::smooth_helix_fold();
    smooth.curve = wood_developable_folding::SpaceCurveFoldGuide::helix(2000.0, 3000.0, 0.4, 48);
    smooth.deployments = {1.0};
    const wood_developable_folding::SpaceCurveFold curved(smooth, "smooth_fold");
    check(curved.measures().distortion < 1e-3 && curved.get_elements<FaceCurvedElement>().size() == 2, "smooth space curve fold: two curved plates that develop flat");
    check_template(curved, 2);
    check(wood_developable_folding::SpaceCurveFold(wood_developable_folding::SpaceCurveFoldGuide::smooth_helix_fold(), "smooth_stages").measures().crossings == 0, "smooth space curve fold: no quads crossing down to 6 degrees");

    // a Miura-ori along the curve: the same checks over four strips, its plates put in one by one
    wood_developable_folding::CurvedMiuraGuide strips = wood_developable_folding::CurvedMiuraGuide::helix_miura();
    strips.curve = arc.curve;
    strips.strips = 4;
    strips.deployments = {1.0, 0.5};
    const wood_developable_folding::CurvedMiura miura(strips);
    const wood_developable_folding::FoldMeasures& tessellated = miura.measures();
    check(tessellated.planarity < 1e-9 && tessellated.kawasaki < 1e-9 && tessellated.developability < 1e-9 && tessellated.stretch < 1e-6, "curved miura: planar quads, flat-foldable vertices, every edge kept");
    check(std::abs(tessellated.openings[0] - 0.5 * strips.angle) < 1e-9 && std::abs(tessellated.openings[1] - strips.angle) < 1e-9, "curved miura: every crease at the stage's opening");
    check_template(miura, 32);

    // the other presets as bare quads: planar, flat-foldable and crossing nowhere
    wood_developable_folding::SpaceCurveFoldGuide wave = wood_developable_folding::SpaceCurveFoldGuide::wave_fold();
    wave.thickness = 0.0;
    wood_developable_folding::CurvedMiuraGuide wavy = wood_developable_folding::CurvedMiuraGuide::wave_miura();
    wavy.thickness = 0.0;
    for (const wood_developable_folding::FoldMeasures& preset : {wood_developable_folding::SpaceCurveFold(wave).measures(), wood_developable_folding::CurvedMiura(wavy).measures(), wood_developable_folding::CurvedMiura(wood_developable_folding::CurvedMiuraGuide::zigzag_miura()).measures()})
        check(preset.planarity < 1e-9 && preset.kawasaki < 1e-9 && preset.crossings == 0, "space curve presets: planar, flat-foldable, no quads crossing");

    // section-active grid systems: haunched cantilevers, Gerber step-hinges, concentric grillage, mushroom & waffle drop-panel slabs, and umbrella slabs
    const wood_grid::Grid haunched_grid(wood_grid::Plan::cantilever_hall(), wood_grid::Framing::haunched_cantilever(), "haunched_cantilever");
    check_template(haunched_grid, haunched_grid.elements().size());
    const wood_grid::Grid gerber_grid(wood_grid::Plan::gerber_hall(), wood_grid::Framing::gerber_hall(), "gerber_hall");
    check_template(gerber_grid, gerber_grid.elements().size());
    const wood_grid::Grid concentric_grid(wood_grid::Plan::concentric_floor(), wood_grid::Framing::concentric_floor(), "concentric_lantern");
    check_template(concentric_grid, concentric_grid.elements().size());
    const wood_grid::Grid mushroom_grid(wood_grid::Plan::mushroom_floor(), wood_grid::Framing::mushroom_drop_slab(), "mushroom_drop_slab");
    check_template(mushroom_grid, mushroom_grid.elements().size());
    const wood_grid::Grid waffle_grid(wood_grid::Plan::waffle_floor(), wood_grid::Framing::waffle_coffered_slab(), "waffle_coffered_slab");
    check_template(waffle_grid, waffle_grid.elements().size());
    const wood_grid::Grid diag_waffle_grid(wood_grid::Plan::diagonal_waffle_floor(), wood_grid::Framing::diagonal_waffle_slab(), "diagonal_waffle_slab");
    check_template(diag_waffle_grid, diag_waffle_grid.elements().size());
    const wood_grid::Grid umbrella_grid(wood_grid::Stack::umbrella_canopies(), "umbrella_canopies");
    check_template(umbrella_grid, umbrella_grid.elements().size());

    // section-active frame systems: gable 3-hinged portal, articulated T-frame, tiered clerestory, concentric Vierendeel, and crosswise box frames
    const wood_frame::Frame gable_frame(wood_frame::FrameGuide(wood_frame::ThreeHingedGable{}), "three_hinged_gable");
    check_template(gable_frame, gable_frame.elements().size());
    const wood_frame::Frame t_frame(wood_frame::FrameGuide(wood_frame::ArticulatedTFrame{}), "cantilever_t_frame");
    check_template(t_frame, t_frame.elements().size());
    const wood_frame::Frame tiered_frame(wood_frame::FrameGuide(wood_frame::TieredClerestoryHall{}), "tiered_clerestory_hall");
    check_template(tiered_frame, tiered_frame.elements().size());
    const wood_frame::Frame vierendeel_frame(wood_frame::FrameGuide(wood_frame::ConcentricVierendeelGrid{}), "concentric_vierendeel_grid");
    check_template(vierendeel_frame, vierendeel_frame.elements().size());
    const wood_frame::Frame box_frame(wood_frame::FrameGuide(wood_frame::CrosswiseStackedBoxes{}), "crosswise_stacked_boxes");
    check_template(box_frame, box_frame.elements().size());

    // NURBS periodic C2 continuity on closed cylindrical/funnel/toroidal vaults and multi-patch sewn funnels
    const NurbsSurface funnel_surf = wood_vault::funnel_vault_surface(3400.0, 920.0, 2700.0);
    check(funnel_surf.is_periodic(0), "funnel vault surface: periodic C2 NURBS in u");
    const std::pair<double, double> f_du = funnel_surf.domain(0);
    const std::pair<double, double> f_dv = funnel_surf.domain(1);
    for (int j = 0; j <= 8; j++) {
        const double v = f_dv.first + (f_dv.second - f_dv.first) * (static_cast<double>(j) / 8.0);
        const Point p0 = funnel_surf.point_at(f_du.first, v);
        const Point p1 = funnel_surf.point_at(f_du.second, v);
        const Vector n0 = funnel_surf.normal_at(f_du.first, v).normalized();
        const Vector n1 = funnel_surf.normal_at(f_du.second, v).normalized();
        check(p0.distance(p1) < 1e-9 && n0.dot(n1) > 1.0 - 1e-12, "funnel vault surface: zero seam kink");
    }
    const NurbsSurface closed_torus = wood_vault::torus_vault_surface(3200.0, 1450.0, 1650.0, 360.0);
    check(closed_torus.is_periodic(0), "closed torus surface: periodic C2 NURBS in u");
    const std::vector<NurbsSurface> petals = wood_vault::sewn_funnel_surfaces(3400.0, 920.0, 2700.0, 480.0, 4);
    for (size_t k = 0; k < petals.size(); k++) {
        const NurbsSurface& sa = petals[k];
        const NurbsSurface& sb = petals[(k + 1) % petals.size()];
        const std::pair<double, double> ua = sa.domain(0);
        const std::pair<double, double> ub = sb.domain(0);
        const std::pair<double, double> va = sa.domain(1);
        for (int j = 0; j <= 8; j++) {
            const double v = va.first + (va.second - va.first) * (static_cast<double>(j) / 8.0);
            const Point pa = sa.point_at(ua.second, v);
            const Point pb = sb.point_at(ub.first, v);
            const Vector na = sa.normal_at(ua.second, v).normalized();
            const Vector nb = sb.normal_at(ub.first, v).normalized();
            check(pa.distance(pb) < 1e-9 && na.dot(nb) > 1.0 - 1e-12, "sewn funnel patches: exact C2 seam continuity");
        }
    }

    // NURBS and sewn multi-surface NGon vaults: planarized side joints, planar caps, shifted and bevelled facets
    wood_vault::SurfaceVaultGuide groin_guide = wood_vault::SurfaceVaultGuide::sewn_groin_vault();
    groin_guide.size_u = 1100.0;
    groin_guide.size_v = 1100.0;
    const wood_vault::SurfaceVault sewn_groin(groin_guide, "sewn_groin");
    check(sewn_groin.side_planarity() < 1e-6 && sewn_groin.cap_planarity() < 1e-6, "sewn groin vault: planar side joints and planar caps");
    check(!sewn_groin.get_contacts().empty(), "sewn groin vault: coplanar side-joint contacts across seams");
    check_template(sewn_groin, sewn_groin.elements().size());

    wood_vault::SurfaceVaultGuide shifted_guide = wood_vault::SurfaceVaultGuide::shifted_hex_vault();
    shifted_guide.size_u = 1100.0;
    shifted_guide.size_v = 800.0;
    const wood_vault::SurfaceVault shifted_hex(shifted_guide, "shifted_hex");
    check(shifted_hex.side_planarity() < 1e-6 && shifted_hex.cap_planarity() < 1e-6, "shifted hex vault: planar side joints and caps under tangential shift");
    check(!shifted_hex.get_contacts().empty(), "shifted hex vault: coplanar side-joint contacts");
    check_template(shifted_hex, shifted_hex.elements().size());

    wood_vault::SurfaceVaultGuide bevel_guide = wood_vault::SurfaceVaultGuide::bevelled_hex_dome();
    bevel_guide.size_u = 1100.0;
    bevel_guide.size_v = 950.0;
    const wood_vault::SurfaceVault bevelled_hex(bevel_guide, "bevelled_hex");
    check(bevelled_hex.side_planarity() < 1e-6 && bevelled_hex.cap_planarity() < 1e-6, "bevelled hex dome: planar bevelled side joints and caps");
    check(!bevelled_hex.get_contacts().empty(), "bevelled hex dome: coplanar bevelled contacts");
    check_template(bevelled_hex, bevelled_hex.elements().size());

    // Popescu (2017) knit-course increase/decrease funnel with half-brick running-bond equalization
    const wood_vault::SurfaceVault full_funnel(wood_vault::SurfaceVaultGuide::courses_funnel_vault(), "courses_funnel_full");
    check(full_funnel.side_planarity() < 1e-6 && full_funnel.cap_planarity() < 1e-6, "courses funnel: planar meridian head and bed joints");
    const std::pair<std::vector<Point>, std::vector<std::vector<size_t>>> funnel_mid =
        wood_vault::surface_vault_mesh(wood_vault::SurfaceVaultGuide::courses_funnel_vault()).to_vertices_and_faces();
    double min_funnel_edge = 1e30;
    for (const std::vector<size_t>& face : funnel_mid.second) {
        for (size_t k = 0; k < face.size(); k++) {
            min_funnel_edge = std::min(min_funnel_edge, funnel_mid.first[face[k]].distance(funnel_mid.first[face[(k + 1) % face.size()]]));
        }
    }
    check(min_funnel_edge > 45.0, fmt::format("courses funnel: zero Vernier T-joint slivers (min edge {:.2f} mm)", min_funnel_edge));
    wood_vault::SurfaceVaultGuide compact_funnel_guide = wood_vault::SurfaceVaultGuide::courses_funnel_vault();
    compact_funnel_guide.size_u = 850.0;
    compact_funnel_guide.size_v = 680.0;
    const wood_vault::SurfaceVault compact_funnel(compact_funnel_guide, "courses_funnel");
    check(compact_funnel.side_planarity() < 1e-6 && compact_funnel.cap_planarity() < 1e-6, "compact courses funnel: planar side and cap joints");
    check_template(compact_funnel, compact_funnel.elements().size());

    // Chalmers (Adiels, Ander & Williams, IASS 2017) constant-bed-height geodesic-coordinate NURBS brick vault
    wood_vault::SurfaceVaultGuide geod_surf_guide = wood_vault::SurfaceVaultGuide::geodesic_dome_vault();
    geod_surf_guide.size_u = 960.0;
    geod_surf_guide.size_v = 480.0;
    const wood_vault::SurfaceVault geodesic_dome(geod_surf_guide, "geodesic_dome");
    check(geodesic_dome.side_planarity() < 1e-6 && geodesic_dome.cap_planarity() < 1e-6, "geodesic dome: planar side and cap joints on orthogonal geodesic courses");
    check(!geodesic_dome.get_contacts().empty(), "geodesic dome: coplanar bed and head joint contacts");
    check_template(geodesic_dome, geodesic_dome.elements().size());

    // Pure-C++ ShapeUp regularization, BIP brick matching (Du et al. 2023), and QuadriFlow field-guided remeshing
    std::vector<Point> grid_pts;
    for (int j = 0; j <= 10; j++) {
        for (int i = 0; i <= 10; i++) {
            const double jx = (i > 0 && i < 10 && j > 0 && j < 10) ? (12.0 * std::sin(static_cast<double>(i * 3 + j * 5))) : 0.0;
            const double jy = (i > 0 && i < 10 && j > 0 && j < 10) ? (12.0 * std::cos(static_cast<double>(i * 7 - j * 2))) : 0.0;
            grid_pts.emplace_back(static_cast<double>(i) * 100.0 + jx, static_cast<double>(j) * 100.0 + jy, 0.0);
        }
    }
    std::vector<std::vector<size_t>> grid_faces;
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < 10; i++) {
            const size_t v00 = static_cast<size_t>(j * 11 + i);
            grid_faces.push_back({v00, v00 + 1, v00 + 12, v00 + 11});
        }
    }
    const Mesh reg_grid = wood_vault::shapeup_regularize(Mesh::from_vertices_and_faces(grid_pts, grid_faces), 100.0, 60, 1.0, 0.0, 0.0);
    double grid_edge_err = 0.0;
    double grid_max_edge_err = 0.0;
    double grid_ang_err = 0.0;
    double grid_max_ang_err = 0.0;
    size_t grid_sing = 0;
    wood_vault::evaluate_quad_distortion(reg_grid, 100.0, grid_edge_err, grid_max_edge_err, grid_ang_err, grid_max_ang_err, grid_sing);
    check(grid_edge_err < 1e-6 && grid_ang_err < 1e-4 && grid_sing == 0, "shapeup: disturbed 10x10 quad grid converges to exact 100 mm squares");
    const wood_vault::BrickMatchingResult match_stag = wood_vault::match_brick_tiles(reg_grid, wood_vault::TileVaultGuide::STAGGERED);
    const wood_vault::BrickMatchingResult match_run = wood_vault::match_brick_tiles(reg_grid, wood_vault::TileVaultGuide::RUNNING_BOND);
    check(std::abs(match_stag.objective - 42.45) < 1e-6 && match_stag.matched_pairs.size() == 50 && match_stag.unmatched_interior == 0, fmt::format("bip matching: exact staggered MILP optimum on 10x10 grid (obj={:.4f}, pairs={})", match_stag.objective, match_stag.matched_pairs.size()));
    check(std::abs(match_run.objective - 61.65) < 1e-6 && match_run.matched_pairs.size() == 50 && match_run.running_pairs >= 60 && match_run.unmatched_interior == 0, fmt::format("bip matching: exact running-bond MILP optimum on 10x10 grid (obj={:.4f}, run={})", match_run.objective, match_run.running_pairs));

    wood_vault::TileVaultGuide tile_guide = wood_vault::TileVaultGuide::funicular_running_blocks();
    tile_guide.tile_width = 300.0;
    const wood_vault::TileVault tile_session(tile_guide, "funicular_running_test");
    check(
        tile_session.metrics().quad_faces > 70 &&
            tile_session.metrics().mean_edge_error < 0.15 &&
            tile_session.metrics().mean_angle_error_deg < 12.0 &&
            tile_session.metrics().mean_edge_error < tile_session.metrics().raw_edge_error &&
            tile_session.metrics().mean_angle_error_deg < tile_session.metrics().raw_angle_error_deg,
        fmt::format(
            "quadriflow + shapeup: low edge and corner-angle distortion on oculus shell (quads={}, edge={:.3f}->{:.3f}, ang={:.2f}->{:.2f} deg)",
            tile_session.metrics().quad_faces,
            tile_session.metrics().raw_edge_error,
            tile_session.metrics().mean_edge_error,
            tile_session.metrics().raw_angle_error_deg,
            tile_session.metrics().mean_angle_error_deg
        )
    );
    check(tile_session.metrics().parallel_pairs <= 2 && tile_session.metrics().running_pairs > 20 && tile_session.metrics().side_planarity < 1e-6, "tile vault: running-bond courses and planarized brick sides");
    check(!tile_session.get_contacts().empty(), "tile vault: coplanar brick contacts");
    check_template(tile_session, tile_session.elements().size());

    // Chalmers polar geodesic-coordinate brick topology on free-form oculus shell
    wood_vault::TileVaultGuide geod_tile_guide = wood_vault::TileVaultGuide::funicular_geodesic_blocks();
    geod_tile_guide.tile_width = 280.0;
    const wood_vault::TileVault geod_tile_session(geod_tile_guide, "funicular_geodesic_test");
    check(geod_tile_session.metrics().full_bricks >= 35 && geod_tile_session.metrics().side_planarity < 1e-6, "geodesic tile vault: polar geodesic courses and planarized brick sides");
    check(!geod_tile_session.get_contacts().empty(), "geodesic tile vault: coplanar bed and head contacts");
    check_template(geod_tile_session, geod_tile_session.elements().size());

    // Calgary 2025 boundary-inclusive trivalent dual & variable-thickness chamfered blocks
    const wood_vault::CalgaryVault calgary_session(wood_vault::CalgaryVaultGuide::chamfered_blocks(), "calgary_chamfered_test");
    check(calgary_session.metrics().dual_faces >= 90 && calgary_session.metrics().support_blocks >= 4, "calgary dual shell: trivalent dual blocks and pier support cuts");
    check(calgary_session.metrics().min_thickness <= 105.0 && calgary_session.metrics().max_thickness >= 295.0, "calgary dual shell: Delaunay XY variable thickness 100..300 mm");
    check(calgary_session.metrics().chamfer_cuts >= 300 && calgary_session.metrics().side_planarity < 1e-6 && calgary_session.metrics().top_cap_planarity < 1e-6, "calgary dual shell: planar side interfaces, flat top caps, and trivalent chamfers");
    check(!calgary_session.get_contacts().empty(), "calgary dual shell: coplanar contacts");
    check_template(calgary_session, calgary_session.elements().size());

    std::cout << "template_assembly: passed" << std::endl;
    return 0;
}
