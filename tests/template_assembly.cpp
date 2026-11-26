#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_module_sequence.h"
#include "src/templates/cross/vda_mesh.h"
#include "src/templates/developable_folding/crease_quadrants.h"
#include "src/templates/developable_folding/curved_crease_tube.h"
#include "src/templates/developable_folding/curved_crease_unfolding.h"
#include "src/templates/developable_folding/curved_fold.h"
#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/folded_frame.h"
#include "src/templates/module/cassette_shell.h"
#include "src/templates/plaited/plaited.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
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

    std::cout << "template_assembly: passed" << std::endl;
    return 0;
}
