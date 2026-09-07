// Face-to-face detection on a real dataset, drawn as two layers.
//
//   Contacts — every face pair that touches, from wood_session::face_contacts.
//              Coloured by ContactType: the topology class a contact can work
//              out from its own face indices (side_side / side_top / top_top).
//              N per element pair, and pure: nothing is mutated.
//
//   Joints   — what get_connection_zones made of those contacts. Coloured by
//              joint_type, the refined solver code (11/12/13/20/30/40). At most
//              ONE per element pair, because the detector returns at the first
//              face pair that survives every downstream gate.
//
// Putting both in one scene answers the question the two functions raise: the
// Joints groups are a subset of the Contacts groups, and the difference is the
// contacts the solver looked at and rejected.
//
//   main_face_to_face [dataset]     # default: hexboxes
//
// `dataset` takes either the short OBJ name (hexboxes, hexbox_and_corner,
// annen_box) or the full wood test name (type_plates_name_hexbox_and_corner).
// A name with no plate dataset is tried as a block .pb, which is how the
// floor_model blocks still reach this example - and the only way to see
// ContactType::unknown, since a block has no top/bottom convention.
//
// Writes data/output/pb/live.pb, which is what bash/publish-scene.sh uploads.
#include "wood_session.h"
#include "wood_face_to_face.h"
#include "wood_element.h"
#include "../src/polyline.h"
#include "../src/session.h"

#include <fmt/core.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace {

// hexboxes: measured, not guessed. Of the 41 runnable datasets it is the only
// one that produces all three contact classes, and it produces the most joint
// types (4), in 26 plates. Runners-up if you want something smaller:
// cross_and_sides_corner (9 plates, 2 classes, 3 types) and hexbox_and_corner
// (11 plates, 2 classes, 3 types).
constexpr const char* DEFAULT_DATASET = "hexboxes";

// wood/src/config, reached via data/ because that is the one path the library
// exposes. config_dir() itself is internal to wood_globals.cpp.
std::filesystem::path config_dir() {
    return internal::session_data_dir().parent_path() / "src" / "config";
}

// Short name -> full wood test name, by looking for the config that belongs to
// it. Datasets are named after what they contain, and the full names are long
// ("type_plates_name_side_to_side_edge_outofplane_inplane_and_top_to_top_hexboxes"),
// so an example that made you type one would not get used.
std::string resolve_dataset(const std::string& arg) {
    const std::filesystem::path dir = config_dir();
    if (std::filesystem::exists(dir / (arg + ".yml"))) { return arg; }

    const std::string prefixed = "type_plates_name_" + arg;
    if (std::filesystem::exists(dir / (prefixed + ".yml"))) { return prefixed; }

    // Otherwise the short name is the tail of a longer one - "hexboxes" inside
    // "..._inplane_and_top_to_top_hexboxes".
    const std::string suffix = "_" + arg + ".yml";
    if (std::filesystem::exists(dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            const std::string file = entry.path().filename().string();
            if (file.size() > suffix.size() &&
                file.compare(file.size() - suffix.size(), suffix.size(), suffix) == 0) {
                return entry.path().stem().string();
            }
        }
    }
    return arg;
}

/// Counts keyed by group label, printed so the run says what it found rather
/// than leaving you to infer it from the dataset's name.
void report(const std::string& heading, const std::map<std::string, int>& tally, size_t total) {
    fmt::print("{} ({}):\n", heading, total);
    if (tally.empty()) { fmt::print("  none\n"); return; }
    for (const auto& [label, n] : tally) { fmt::print("  {:<12} {}\n", label, n); }
}

int run_pb(const std::string& name) {
    const std::filesystem::path pb = wood_session::dataset_pb(name);
    if (!std::filesystem::exists(pb)) {
        fmt::print(stderr,
                   "no dataset '{}': neither a plate config in {} nor a .pb at {}\n",
                   name, config_dir().string(), pb.string());
        return 1;
    }

    wood_session::globals::reset_defaults();

    // Split by element_type, so a model that mixes plates with columns and loose solids
    // goes through one detection pass and each part is treated as what it is.
    wood_session::WoodSession elements = wood_session::WoodSession::from_session(session_cpp::Session::pb_load(pb.string()));
    const std::vector<wood_session::ContactElement> view = wood_session::contact_view(elements);
    const std::vector<wood_session::ContactPair> contacts = wood_session::face_contacts(view);

    std::map<std::string, int> by_type;
    size_t n_contacts = 0;
    for (const wood_session::ContactPair& pair : contacts)
        for (const wood_session::FaceContact& c : pair.faces) {
            by_type[wood_session::contact_type_name(c.type)]++;
            n_contacts++;
        }
    fmt::print("\n=== {} — {} plates, {} columns, {} solids ===\n",
               name, elements.plates.size(), elements.columns.size(), elements.solids.size());
    report("Contacts", by_type, n_contacts);

    // Joints need the plate convention, so only the plates go to the solver - and they go
    // in ONE call. Its type-12 geometry cache lets the first joint of a given key fix the
    // tooth positions for every later joint with that key, so solving in batches would
    // silently change the geometry.
    std::map<std::string, int> joints_by_type;
    size_t n_joints = 0;
    if (!elements.plates.empty()) {
        std::vector<wood_session::WoodElement> solver_elements = elements.plates;
        const std::vector<wood_session::WoodJoint> joints =
            get_connection_zones(solver_elements, face_to_face);
        for (const wood_session::WoodJoint& j : joints) {
            joints_by_type[wood_session::joint_type_name(j.joint_type)]++;
        }
        n_joints = joints.size();
        report("Joints", joints_by_type, n_joints);
    } else {
        fmt::print("Joints: none — nothing in this file carries the plate convention.\n");
    }

    session_cpp::Session session(fmt::format("wood - contacts - {}", name));
    auto inputs = session.add_group("Inputs");
    wood_session::add_faces(session, inputs, elements.plates);
    wood_session::add_faces(session, inputs, elements.solids);
    wood_session::add_contacts_by_type(session, contacts);
    fmt::print("wrote {}\n", wood_session::pb_dump(session, "live").string());
    return 0;
}

int run_plates(const std::string& short_name, const std::string& dataset) {
    wood_session::globals::globals_yaml(dataset);
    std::vector<wood_session::WoodElement> elements = internal::load_plates(dataset);

    // Contacts first, on the plates as loaded. get_connection_zones mutates the
    // elements it is given - it installs insertion vectors, and swaps faces 0
    // and 1 of a plate whose partner asks for it - so the solver gets a copy and
    // the contact pass and the Inputs group both see the untouched geometry.
    const std::vector<wood_session::ContactPair> contacts = wood_session::face_contacts(elements);

    std::vector<wood_session::WoodElement> solver_elements = elements;
    const std::vector<wood_session::WoodJoint> joints =
        get_connection_zones(solver_elements, face_to_face);

    std::map<std::string, int> contacts_by_type;
    size_t n_contacts = 0;
    for (const wood_session::ContactPair& pair : contacts)
        for (const wood_session::FaceContact& c : pair.faces) {
            contacts_by_type[wood_session::contact_type_name(c.type)]++;
            n_contacts++;
        }
    std::map<std::string, int> joints_by_type;
    for (const wood_session::WoodJoint& j : joints) {
        joints_by_type[wood_session::joint_type_name(j.joint_type)]++;
    }

    fmt::print("\n=== {} — {} plates ===\n", short_name, elements.size());
    report("Contacts", contacts_by_type, n_contacts);
    report("Joints", joints_by_type, joints.size());

    session_cpp::Session session(fmt::format("wood - contacts and joints - {}", short_name));
    wood_session::add_faces(session, session.add_group("Inputs"), elements);
    wood_session::add_contacts_by_type(session, contacts);
    wood_session::add_joints_by_type(session, joints);
    fmt::print("wrote {}\n", wood_session::pb_dump(session, "live").string());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const std::string requested = (argc > 1) ? argv[1] : DEFAULT_DATASET;
    const std::string dataset = resolve_dataset(requested);

    if (!internal::plates_exist(dataset)) { return run_pb(requested); }
    return run_plates(requested, dataset);
}
