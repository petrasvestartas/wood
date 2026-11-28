#include "src/templates/delorme/delorme.h"

using namespace laika_cpp;
using namespace wood_session;
using namespace wood_delorme;

namespace {

DelormeRibCurve make_catenary_rib(
    const std::string& name,
    double y_pos,
    double half_span,
    double rise,
    int samples = 80
) {
    DelormeRibCurve rib;
    rib.name = name;
    rib.origin = Point(0.0, y_pos, 0.0);
    rib.u_axis = Vector::x_axis();
    rib.normal = rib.u_axis.cross(Vector::z_axis()).normalized();

    const double k = 1.65;
    const double denom = std::cosh(k) - 1.0;

    for (int i = 0; i <= samples; i++) {
        const double xi = -1.0 + 2.0 * static_cast<double>(i) / samples;
        const double u = half_span * xi;
        const double z = rise * (std::cosh(k) - std::cosh(k * xi)) / denom;
        const double dz_du = -rise * (k / half_span) * std::sinh(k * xi) / denom;
        const Vector ut = Vector(1.0, 0.0, dz_du).normalized();
        const Vector ur = rib.normal.cross(ut).normalized();
        rib.pts.emplace_back(u, y_pos, z);
        rib.tangents.push_back(ut);
        rib.radials.push_back(ur);
    }
    rib.arc_s.assign(rib.pts.size(), 0.0);
    for (size_t i = 1; i < rib.pts.size(); i++)
        rib.arc_s[i] = rib.arc_s[i - 1] + Point::distance(rib.pts[i - 1], rib.pts[i]);
    return rib;
}

} // namespace

/// Intersecting Delorme groin pavilion and custom catenary funicular vault.
int main() {

    WoodSession scene("templates_delorme_4_crossed_vault");

    const DelormeGuide groin_guide =
        DelormeGuide::crossed_vault(4600.0, 2050.0, 2, 2, 1150.0, 200.0, 32.0);
    const DelormeSystem groin(groin_guide, "groin_vault");
    groin.append_to(scene, Vector(0.0, 0.0, 0.0), "groin_vault");

    std::vector<DelormeRibCurve> catenary_ribs;
    for (int r = 0; r < 4; r++) {
        const double y = -1350.0 + 900.0 * r;
        catenary_ribs.push_back(
            make_catenary_rib(fmt::format("catenary_rib_{}", r), y, 2200.0, 2350.0)
        );
    }
    DelormeParams cat_params;
    cat_params.n_plies = 2;
    cat_params.plank_length = 1120.0;
    cat_params.rib_depth = 210.0;
    cat_params.plank_thickness = 34.0;
    cat_params.lierne_spacing = 960.0;

    const DelormeSystem catenary(DelormeGuide(cat_params, catenary_ribs), "funicular_catenary");
    catenary.append_to(scene, Vector(6200.0, 0.0, 0.0), "funicular_catenary");

    std::cout << fmt::format(
        "Groin pavilion: {} planks, {} liernes | Catenary vault: {} planks, {} liernes\n",
        groin.planks().size(),
        groin.liernes().size(),
        catenary.planks().size(),
        catenary.liernes().size()
    );
    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Spatial and free-form extensions of Philibert de l'Orme's 1561 flat-plank system: a crossed square groin pavilion with four facade portal arches and four diagonal groin ribs meeting at an octagonal crown ring tied by haunch liernes (left), beside a custom catenary funicular vault constructed from user-supplied planar rib curves (right).

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_delorme_4_crossed_vault --parallel 6 && ./build/templates_delorme_4_crossed_vault && ../bash/publish-scene.sh --target templates_delorme_4_crossed_vault

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/laika/
*/
