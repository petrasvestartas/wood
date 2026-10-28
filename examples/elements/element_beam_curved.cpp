#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// Single-curved and double-curved glulam beams (after Tom Svilans' GluLamb).
int main() {

    WoodSession scene("element_beam_curved");

    // 1. Single-curved planar glulam arch (140 x 240 mm, 6 lamellae of 40 mm)
    std::vector<Point> arch_pts;
    for (int i = 0; i <= 8; i++) {
        const double u = static_cast<double>(i) / 8.0;
        const double x = 2400.0 * u;
        const double z = 700.0 * 4.0 * u * (1.0 - u);
        arch_pts.emplace_back(x, 0.0, z);
    }
    const std::shared_ptr<BeamCurved> single_arch = BeamCurved::single_curved(
        NurbsCurve::create_interpolated(arch_pts), 140.0, 240.0, 6, 32, "single_curved_glulam"
    );
    single_arch->extension = 120.0;
    single_arch->cuts = {
        Plane::from_point_normal({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}),
        Plane::from_point_normal({2400.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}),
    };
    scene.add(single_arch);

    // 2. Double-curved 3D spatial glulam beam with RMF twist interpolation (2 x 6 lamella stack)
    std::vector<Point> spatial_pts;
    for (int i = 0; i <= 8; i++) {
        const double u = static_cast<double>(i) / 8.0;
        const double x = 1200.0 + 2400.0 * u;
        const double y = -1100.0 - 280.0 * std::sin(Tolerance::TWO_PI * u);
        const double z = 650.0 * std::sin(Tolerance::PI * u);
        spatial_pts.emplace_back(x, y, z);
    }
    const std::vector<Vector> guides = {
        Vector(0.0, -0.35, 1.0).normalized(),
        Vector(0.0, 0.45, 1.0).normalized(),
        Vector(0.0, -0.35, 1.0).normalized(),
    };
    const std::shared_ptr<BeamCurved> double_arch = BeamCurved::double_curved(
        NurbsCurve::create_interpolated(spatial_pts),
        140.0,
        240.0,
        guides,
        {},
        2,
        6,
        32,
        "double_curved_glulam"
    );
    scene.add(double_arch);

    double kw_s = 0.0, kh_s = 0.0, kw_d = 0.0, kh_d = 0.0;
    single_arch->max_curvature(kw_s, kh_s);
    double_arch->max_curvature(kw_d, kh_d);

    std::cout << scene << std::endl;
    std::cout << fmt::format(
        "single_curved: planar={}, kw={:.6f}, kh={:.6f}, lamellae={}\n"
        "double_curved: double={}, kw={:.6f}, kh={:.6f}, lamellae={}\n",
        single_arch->is_single_curved(),
        kw_s,
        kh_s,
        single_arch->lamella_curves().size(),
        double_arch->is_double_curved(),
        kw_d,
        kh_d,
        double_arch->lamella_curves().size()
    );
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Single-curved and double-curved glulam beams (BeamCurved, after Tom Svilans' GluLamb): a planar parabolic arch of 140 x 240 mm section (6 lamella layers of 40 mm) trimmed by vertical plumb end cuts, paired with a 3D spatial S-curved glulam beam (2 x 6 lamella stack) whose cross-section orientation interpolates guide vectors over a Rotation Minimizing Frame (RMF) and exports an exact 6-patch NURBS BRep.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target element_beam_curved --parallel 6 && ./build/element_beam_curved && ../bash/publish-scene.sh --target element_beam_curved

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
