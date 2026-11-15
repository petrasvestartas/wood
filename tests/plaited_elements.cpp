#include "wood_session.h"
#include "src/templates/plaited/plaited.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Fails the test executable with a diagnostic message when ok is false.
void check(bool ok, const std::string& message) {

    if (!ok) {
        std::cerr << "plaited_elements FAILED: " << message << std::endl;
        throw std::runtime_error(message);
    }
}

/// Counts interior seams (owned by two cells) in a PlaitedGuide and verifies opposite bevel signs and normals.
size_t check_guide_seams(const wood_plaited::PlaitedGuide& guide, const std::string& label) {

    size_t interior = 0;
    for (const wood_plaited::PlaitedSeam& seam : guide.seams()) {
        if (seam.cell_b < 0)
            continue;
        interior++;
        check(seam.sign_a == -seam.sign_b, label + ": interior seam signs must oppose");
        const double anti_dot = seam.plane_for_a.z_axis().dot(seam.plane_for_b.z_axis());
        check(anti_dot < -1.0 + 1e-11, label + ": shared bevel normals must be exact opposites");
        check(
            seam.plane_for_a.origin().distance(seam.plane_for_b.origin()) < 1e-11,
            label + ": shared bevel origins must coincide"
        );
    }
    return interior;
}

/// Verifies every stereotomic block has positive volume, a valid BRep, and exact contact count matching interior seams.
void check_stereotomy(
    const wood_plaited::PlaitedStereotomy& assembly,
    size_t expected_blocks,
    const std::string& label
) {

    check(
        assembly.blocks().size() == expected_blocks,
        fmt::format("{}: expected {} blocks, got {}", label, expected_blocks, assembly.blocks().size())
    );
    const size_t interior_seams = check_guide_seams(assembly.guide, label);

    for (const std::shared_ptr<NodeBlockElement>& block : assembly.blocks()) {
        const Mesh& mesh = block->model_geometry_mesh();
        const BRep& brep = block->model_geometry_brep();
        check(mesh.number_of_vertices() >= 6, label + ": block mesh vertex count");
        check(std::abs(mesh.volume()) > 1000.0, label + ": block volume must be positive");
        check(brep.is_valid(), label + ": block BRep must be valid");

        if (assembly.guide.base_plane().has_value()) {
            const double z_base = assembly.guide.base_plane()->origin()[2];
            for (const size_t fk : mesh.faces()) {
                for (const size_t vk : mesh.face_vertices(fk).value()) {
                    const Point pt = mesh.vertex_point(vk).value();
                    check(pt[2] >= z_base - 1e-1, label + ": vertex dipped below flat_base");
                }
            }
        }
    }

    check(
        assembly.graph.number_of_edges() == interior_seams,
        fmt::format(
            "{}: expected {} face contacts, got {}",
            label,
            interior_seams,
            assembly.graph.number_of_edges()
        )
    );
}

/// Counts rational (cylindrical bore) surfaces on a BRep.
size_t count_bores(const BRep& brep) {

    size_t bores = 0;
    for (const NurbsSurface& surface : brep.m_surfaces)
        bores += surface.is_rational() ? 1 : 0;
    return bores;
}

} // namespace

int main() {

    // 1. Flat floor Abeille vault (volta piana): 4x4 = 16 blocks, 24 interior seams.
    {
        const Mesh flat = wood_plaited::flat_quad_mesh(2400.0, 2400.0, 4, 4, 0.0, 200.0);
        wood_plaited::PlaitedStereotomy volta(
            flat,
            wood_plaited::PlaitedPattern::Abeille,
            200.0,
            0.46
        );
        check_stereotomy(volta, 16, "flat_abeille");

        // Protobuf round-trip preserves blocks and contact graph.
        const WoodSession reloaded = WoodSession::pb_loads(volta.pb_dumps());
        check(
            reloaded.world_elements<NodeBlockElement>().size() == 16,
            "flat_abeille pb_loads block count"
        );
        check(
            reloaded.graph.number_of_edges() == 24,
            "flat_abeille pb_loads contact edge count"
        );
    }

    // 2. Single-curved arch with flat horizontal springing and full bevel contact, plus relief mode check.
    {
        const Mesh arch_mesh = wood_plaited::arch_quad_mesh(5200.0, 1650.0, 2400.0, 10, 4);
        const wood_plaited::PlaitedStereotomy full_arch(
            arch_mesh,
            wood_plaited::PlaitedPattern::Abeille,
            220.0,
            0.45,
            0.0,
            false,
            false,
            true,
            0.0,
            true
        );
        check_stereotomy(full_arch, 40, "full_contact_arch");

        const Mesh skewed_arch = wood_plaited::arch_quad_mesh(4800.0, 1600.0, 1800.0, 8, 3, 0.25, 0.15);
        const wood_plaited::PlaitedStereotomy stepped_arch(
            skewed_arch,
            wood_plaited::PlaitedPattern::Abeille,
            220.0,
            0.44,
            70.0
        );
        check_stereotomy(stepped_arch, 24, "skewed_stepped_arch");
    }

    // 3. Doubly curved Sphera hexagonal dome with boundary fang trimming and flat springing.
    {
        const Mesh cap = wood_plaited::spherical_cap_hex_mesh(5000.0, 1400.0, 1);
        const wood_plaited::PlaitedStereotomy sphera(
            cap,
            wood_plaited::PlaitedPattern::Sphera,
            170.0,
            0.40,
            0.0,
            false,
            false,
            true,
            0.0,
            true
        );
        check_stereotomy(sphera, 21, "sphera_dome");
    }

    // 4. Doubly curved barrel hex vault (Sphera) and hypar saddle (Abeille).
    {
        const Mesh barrel = wood_plaited::barrel_hex_mesh(4600.0, 1400.0, 2400.0, 5, 3);
        const wood_plaited::PlaitedStereotomy barrel_sphera(
            barrel,
            wood_plaited::PlaitedPattern::Sphera,
            160.0,
            0.38
        );
        check_stereotomy(barrel_sphera, 45, "barrel_hex_sphera");

        const Mesh hypar = wood_plaited::doubly_curved_quad_mesh(3200.0, 3200.0, 800.0, 4, 4, 1.0);
        const wood_plaited::PlaitedStereotomy hypar_vault(
            hypar,
            wood_plaited::PlaitedPattern::Abeille,
            180.0,
            0.42
        );
        check_stereotomy(hypar_vault, 16, "hypar_abeille");
    }

    // 5. Level-rimmed tangent-plane Goldberg geodesic dome with LeonardoStar ortho-kite voussoirs.
    {
        const Mesh goldberg = wood_plaited::geodesic_dome_mesh(3000.0, 2, 0.40);
        const wood_plaited::PlaitedStereotomy star(
            goldberg,
            wood_plaited::PlaitedPattern::LeonardoStar,
            180.0,
            0.36,
            0.0,
            false,
            false,
            true,
            0.0,
            true
        );
        check(!star.blocks().empty(), "geodesic_star non-empty");
        check_stereotomy(star, star.guide.cells().size(), "geodesic_star");
    }

    // 6. Leonardo reciprocal barrel arch: merged continuous sills, radial ribs, and BRep cylindrical pins.
    {
        const wood_plaited::PlaitedGuide arch_guide(
            wood_plaited::arch_quad_mesh(4800.0, 1500.0, 2400.0, 12, 4),
            wood_plaited::PlaitedPattern::Abeille,
            150.0
        );
        const wood_plaited::PlaitedReciprocal pinned(
            arch_guide,
            75.0,
            150.0,
            true,
            12.0
        );
        check(pinned.beams().size() == 48, "pinned arch beam count");
        // 2 merged straight springing sills + 2 * 12 curved arch rib segments = 26 frame members.
        check(pinned.frame_beams().size() == 26, "pinned arch merged frame beam count");
        check(!pinned.pins().empty(), "pinned arch created pin connectors");

        size_t total_bores = 0;
        for (const std::shared_ptr<FacePlanarElement>& beam : pinned.beams()) {
            const BRep& brep = beam->model_geometry_brep();
            check(brep.is_valid(), "pinned beam valid BRep");
            total_bores += count_bores(brep);
        }
        check(total_bores > 0, "pinned arch bored exact cylindrical holes into BReps");
    }

    // 7. Open hot-wire reciprocal block vaults (opening > 0): flush perimeter rims and exact T-junction contacts.
    {
        const Mesh curved_quad =
            wood_plaited::doubly_curved_quad_mesh(4000.0, 4000.0, 850.0, 6, 6, 0.0);
        const wood_plaited::PlaitedStereotomy hotwire_abeille(
            curved_quad,
            wood_plaited::PlaitedPattern::Abeille,
            190.0,
            0.46,
            0.0,
            false,
            false,
            true,
            0.45,
            true
        );
        check_stereotomy(hotwire_abeille, 36, "hotwire_abeille");

        // Verify flush square outer boundary at +/-2000 mm in plan.
        double x_max = 0.0;
        double y_max = 0.0;
        for (const std::shared_ptr<NodeBlockElement>& block : hotwire_abeille.blocks()) {
            const Mesh& mesh = block->model_geometry_mesh();
            for (const size_t fk : mesh.faces()) {
                for (const size_t vk : mesh.face_vertices(fk).value()) {
                    const Point pt = mesh.vertex_point(vk).value();
                    x_max = std::max(x_max, std::abs(pt[0]));
                    y_max = std::max(y_max, std::abs(pt[1]));
                }
            }
        }
        check(std::abs(x_max - 2000.0) < 1.0 && std::abs(y_max - 2000.0) < 1.0, "hotwire_abeille flush square rim");

        const Mesh cap = wood_plaited::spherical_cap_hex_mesh(4800.0, 1300.0, 1);
        const wood_plaited::PlaitedStereotomy hotwire_sphera(
            cap,
            wood_plaited::PlaitedPattern::Sphera,
            180.0,
            0.42,
            0.0,
            false,
            false,
            true,
            0.44,
            true
        );
        check_stereotomy(hotwire_sphera, 21, "hotwire_sphera");
    }

    std::cout << "plaited_elements: all checks passed\n";
    return 0;
}
