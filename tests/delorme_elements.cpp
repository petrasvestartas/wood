#include "src/templates/delorme/delorme.h"
#include "wood_module_sequence.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_delorme;

namespace {

void check(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error("delorme_elements failure: " + message);
}

void verify_delorme_system(DelormeSystem sys, const std::string& label) {

    check(!sys.planks().empty(), label + ": no planks generated");
    check(!sys.liernes().empty(), label + ": no liernes generated");
    check(
        sys.clavettes().size() >= sys.liernes().size() * 2,
        label + ": expected at least two opposing clavettes per lierne"
    );
    check(sys.assemblies.size() == 1, label + ": expected 1 root AssemblyModule");

    size_t solid_count = 0;
    for (const std::shared_ptr<Element>& elem : sys.elements()) {
        if (!elem->is_visible)
            continue;
        if (std::dynamic_pointer_cast<JointElement>(elem)
            && !std::dynamic_pointer_cast<JointPinElement>(elem))
            continue;
        const BRep& brep = elem->model_geometry_brep();
        check(brep.is_solid(), label + ": non-solid BRep on " + elem->name);
        check(brep.volume() > 100.0, label + ": degenerate volume on " + elem->name);
        solid_count++;
    }
    check(solid_count > 10, label + ": too few visible solids");

    const std::string bytes = sys.pb_dumps();
    const WoodSession reloaded = WoodSession::pb_loads(bytes);
    check(
        reloaded.elements().size() == sys.elements().size(),
        label + ": pb_loads element count mismatch"
    );

    const AssemblyModule& root = sys.assemblies.front();
    const ModuleSequence seq(sys, root, 20.0, root.play);
    check(seq.open_solids().empty(), label + ": ModuleSequence reported open solids");
    if (!seq.locked().empty()) {
        std::string msg = label + ": locked pieces:";
        for (const std::string& s : seq.locked())
            msg += " " + s;
        check(false, msg);
    }
    std::cout << fmt::format(
        "[PASS] {:24s} | solids={:3d} planks={:3d} liernes={:2d} clavettes={:2d} chevilles={:2d} | locked=0\n",
        label,
        solid_count,
        sys.planks().size(),
        sys.liernes().size(),
        sys.clavettes().size(),
        sys.chevilles().size()
    );
}

} // namespace

int main() {

    verify_delorme_system(
        DelormeSystem(DelormeGuide::barrel(3600.0, 1650.0, 1800.0, 3, 2, 1150.0), "bay_1561"),
        "bay_1561_2ply"
    );

    verify_delorme_system(
        DelormeSystem(DelormeGuide::barrel(4400.0, 2200.0, 2400.0, 3, 3, 1250.0), "barrel_3ply"),
        "barrel_3ply"
    );

    verify_delorme_system(
        DelormeSystem(
            DelormeGuide::pointed_imperiale(4200.0, 2700.0, 2200.0, 3, 2, 1150.0), "imperiale"
        ),
        "pointed_imperiale"
    );

    verify_delorme_system(
        DelormeSystem(DelormeGuide::dome(5600.0, 1200.0, 8, 2, 1150.0), "dome_cupola"),
        "halle_au_ble_dome"
    );

    verify_delorme_system(
        DelormeSystem(DelormeGuide::crossed_vault(4400.0, 1950.0, 2, 2, 1150.0), "groin_vault"),
        "crossed_groin_vault"
    );

    return 0;
}
