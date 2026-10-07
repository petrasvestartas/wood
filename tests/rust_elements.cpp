#include "wood_session.h"
#include "wood_session.pb.h"
#include "element_beam.pb.h"
#include "element_beam_variable.pb.h"
#include "element_block.pb.h"
#include "element_column.pb.h"
#include "element_plate.pb.h"
#include "element_support.pb.h"

using namespace session_cpp;
using namespace wood_session;

/// The session wood/rust writes, one element per class: Plate, Beam, two Columns, Block, BeamVariable, Support.
static const std::string GOLDEN = std::string(WOOD_SOURCE_DIR) + "/rust/tests/data/rust_elements.pb";

static void check(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

/// The payload as protobuf writes it after a parse, so a field order or an encoding choice compares equal.
template <typename Payload> static std::string canonical(const std::string& bytes) {
    Payload payload;
    check(payload.ParseFromString(bytes), "payload does not parse");
    return payload.SerializeAsString();
}

/// True when the two BReps hold as many faces and enclose the same volume within the relative tolerance.
static bool same_brep(const BRep& a, const BRep& b, double tolerance) {
    return a.face_count() == b.face_count() && std::abs(a.volume() - b.volume()) <= tolerance * std::max(1.0, std::abs(b.volume()));
}

/// Element i of the file comes back as T, its payload unchanged by C++ and its Rust BRep the C++ BRep.
template <typename T, typename Payload> static void reads_back(const WoodSession& session, const wood_proto::WoodSession& file, size_t i) {
    const session_proto::Element& stored = file.objects().elements(static_cast<int>(i));
    const std::shared_ptr<T> typed = session.get_element<T>(stored.guid());
    check(typed != nullptr, stored.name() + " does not load as its class");
    check(canonical<Payload>(typed->element_data_dumps()) == canonical<Payload>(stored.element_data()), stored.name() + ": C++ rewrites the Rust payload differently");
    check(stored.geometry_type() == "BRep", stored.name() + ": Rust wrote no BRep");
    const BRep rust = BRep::pb_loads(stored.geometry_data());
    const BRep& cpp = typed->element_geometry_brep();

    // C++ drills the support's anchor holes exactly, Rust writes them polygonal: the volume within the chord error
    if constexpr (std::is_same_v<T, Support>)
        check(rust.face_count() > 0 && std::abs(rust.volume() - cpp.volume()) <= 2e-3 * std::abs(cpp.volume()), stored.name() + ": the Rust BRep differs from the C++ one");
    else
        check(same_brep(rust, cpp, 1e-9), stored.name() + ": the Rust BRep differs from the C++ one");
}

int main() {
    std::ifstream in(GOLDEN, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    check(!bytes.empty(), "missing " + GOLDEN);

    wood_proto::WoodSession file;
    check(file.ParseFromString(bytes) && file.objects().elements_size() == 7, "the Rust session holds seven elements");
    const WoodSession session = WoodSession::pb_loads(bytes);

    reads_back<Plate, wood_proto::Plate>(session, file, 0);
    reads_back<Beam, wood_proto::Beam>(session, file, 1);
    reads_back<Column, wood_proto::Column>(session, file, 2);
    reads_back<Column, wood_proto::Column>(session, file, 3);
    reads_back<Block, wood_proto::Block>(session, file, 4);
    reads_back<BeamVariable, wood_proto::BeamVariable>(session, file, 5);
    reads_back<Support, wood_proto::Support>(session, file, 6);
    check(session.consistent(), "the Rust session loads consistent");

    std::cout << "rust_elements: 7 elements typed, payloads and BReps equal\n";
    return 0;
}
