#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_module_sequence.h"
#include "src/templates/cross/vda_mesh.h"
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
    for (const std::shared_ptr<Element>& element : session.elements())
        meshes[element->guid()] = element->model_geometry_mesh();
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

    // a pavilion bay: two walls of plates and a roof lowered on them
    const wood_vidy::VidyPavilion pavilion(wood_vidy::VidyGuide(2));
    check_template(pavilion, 2);
    const ModuleSequence bays(pavilion, pavilion.assemblies[0]);
    for (const ModuleSteps& steps : bays.modules())
        if (steps.name == "bay_0") {
            check(steps.moves.size() == 3 && steps.moves[2].piece == "roof_0", "vidy: the roof goes on last");
            check(steps.moves[2].insertion.dot(Vector::z_axis()) < -0.999, "vidy: the roof is lowered on the walls");
        }

    check_template(CounterFold(CounterFold::elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0), Folding::ridge_to_ridge, 120.0), 4);
    check_template(FoldedFrame(), 4);
    check_template(wood_module::CassetteShell(), 15);
    check_template(wood_plaited::PlaitedStereotomy(wood_plaited::flat_quad_mesh(3600.0, 3600.0, 6, 6, 0.0, 220.0), wood_plaited::PlaitedPattern::Abeille, 220.0, 0.48), 36);

    // the connectors turned along the slides: every plate onto all of its own at once
    const Mesh mesh = wood_cross::VdaMesh::default_mesh();
    check_template(wood_cross::VdaMesh(mesh, 40.0, {0.0}, {2}, {}, wood_cross::VdaMesh::assembly_lines(mesh), 300.0, 300.0, 40.0), 73);

    std::cout << "template_assembly: passed" << std::endl;
    return 0;
}
