#include "pch.h"
#include "src/templates/vault/surface_vault.h"

namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceVaultGuide Presets
// ═══════════════════════════════════════════════════════════════════════════

SurfaceVaultGuide SurfaceVaultGuide::hex_saddle_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = {saddle_vault_surface(6400.0, 6000.0, 2300.0, 520.0)};
    guide.subdivision = HEXAGONS;
    guide.size_u = 420.0;
    guide.size_v = 270.0;
    guide.parameter = 0.25;
    guide.thickness = 240.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::diamond_torus_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = {torus_vault_surface(3200.0, 1450.0, 1650.0, 270.0)};
    guide.subdivision = DIAMONDS;
    guide.size_u = 520.0;
    guide.size_v = 380.0;
    guide.thickness = 220.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::sewn_groin_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = groin_vault_surfaces(6600.0, 2700.0, 2000.0);
    guide.subdivision = DIAMONDS;
    guide.size_u = 450.0;
    guide.size_v = 450.0;
    guide.thickness = 240.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::sewn_funnel_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = sewn_funnel_surfaces(3400.0, 920.0, 2700.0, 480.0, 4);
    guide.subdivision = DUAL_HEXAGONS;
    guide.size_u = 480.0;
    guide.size_v = 430.0;
    guide.thickness = 190.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::courses_funnel_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = {funnel_vault_surface(3400.0, 920.0, 2700.0)};
    guide.subdivision = BRICKS;
    guide.size_u = 540.0;
    guide.size_v = 290.0;
    guide.parameter = 0.5;
    guide.thickness = 200.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::chebyshev_dome_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = {pillow_dome_surface(6400.0, 6400.0, 2400.0)};
    guide.subdivision = CHEBYSHEV;
    guide.size_u = 540.0;
    guide.size_v = 540.0;
    guide.parameter = 45.0;
    guide.thickness = 230.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::geodesic_dome_vault() {

    SurfaceVaultGuide guide;
    guide.surfaces = {pillow_dome_surface(6400.0, 6400.0, 2400.0)};
    guide.subdivision = GEODESIC_BRICKS;
    guide.size_u = 620.0;
    guide.size_v = 310.0;
    guide.parameter = 35.0;
    guide.thickness = 220.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::shifted_hex_vault() {

    // Thick saddle-arch hexagonal shell with large oblique tangential extrados-intrados shift.
    SurfaceVaultGuide guide;
    guide.surfaces = {saddle_vault_surface(6400.0, 5600.0, 2400.0, 460.0)};
    guide.subdivision = HEXAGONS;
    guide.size_u = 540.0;
    guide.size_v = 360.0;
    guide.parameter = 0.28;
    guide.thickness = 380.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.facet_shift = Vector(280.0, 180.0, 0.0);
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::bevelled_hex_dome() {

    // Pendentive dome of running-bond hexagons with alternating 16.5-degree side-plane bevels (d4.jpg A).
    SurfaceVaultGuide guide;
    guide.surfaces = {pillow_dome_surface(6400.0, 6400.0, 2400.0)};
    guide.subdivision = HEXAGONS;
    guide.size_u = 600.0;
    guide.size_v = 520.0;
    guide.parameter = 1.0 / 3.0;
    guide.thickness = 280.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.bevel_degrees = 16.5;
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::shifted_diamond_groin() {

    // Four-patch sewn groin vault with diagonal tangential extrados-intrados shift and planarized side joints.
    SurfaceVaultGuide guide;
    guide.surfaces = groin_vault_surfaces(6600.0, 2700.0, 2000.0);
    guide.subdivision = DIAMONDS;
    guide.size_u = 520.0;
    guide.size_v = 520.0;
    guide.thickness = 340.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.facet_shift = Vector(260.0, 260.0, 0.0);
    guide.trim_base = true;

    return guide;
}

SurfaceVaultGuide SurfaceVaultGuide::shifted_chebyshev_dome() {

    // Chebyshev equal-edge pendentive dome with tangential extrados-intrados facet shift and planarized joints.
    SurfaceVaultGuide guide;
    guide.surfaces = {pillow_dome_surface(6400.0, 6400.0, 2400.0)};
    guide.subdivision = CHEBYSHEV;
    guide.size_u = 580.0;
    guide.size_v = 580.0;
    guide.parameter = 45.0;
    guide.thickness = 320.0;
    guide.planar_sides = true;
    guide.flatten_caps = 1.0;
    guide.facet_shift = Vector(260.0, 130.0, 0.0);
    guide.trim_base = true;

    return guide;
}

// ═══════════════════════════════════════════════════════════════════════════
// Free Functions
// ═══════════════════════════════════════════════════════════════════════════

Mesh surface_vault_mesh(const SurfaceVaultGuide& guide) {

    const SurfaceDivision division(guide.surfaces, guide.size_u, guide.size_v);
    if (guide.subdivision == SurfaceVaultGuide::QUADS)
        return division.quads(guide.parameter);
    if (guide.subdivision == SurfaceVaultGuide::DIAMONDS)
        return division.diamonds();
    if (guide.subdivision == SurfaceVaultGuide::HEXAGONS)
        return division.hexagons(guide.parameter);
    if (guide.subdivision == SurfaceVaultGuide::DUAL_HEXAGONS)
        return division.dual_hexagons();
    if (guide.subdivision == SurfaceVaultGuide::BRICKS)
        return division.bricks(guide.size_u, guide.parameter);
    if (guide.subdivision == SurfaceVaultGuide::GEODESIC_BRICKS)
        return division.geodesic_bricks(guide.size_u, guide.size_v, guide.parameter);

    return division.chebyshev(guide.size_u, guide.parameter);
}

std::vector<std::shared_ptr<Element>> surface_vault(
    const SurfaceVaultGuide& guide,
    NgonSolidMetrics* metrics
) {

    const Mesh mesh = surface_vault_mesh(guide);
    const std::vector<std::shared_ptr<NodeBlockElement>> solids = ngon_solids(
        mesh,
        guide.thickness,
        guide.planar_sides,
        guide.flatten_caps,
        guide.facet_shift,
        guide.bevel_degrees,
        guide.trim_base,
        guide.base_z,
        metrics
    );

    std::vector<std::shared_ptr<Element>> out;
    out.reserve(solids.size());
    for (const std::shared_ptr<NodeBlockElement>& block : solids)
        out.push_back(block);

    return out;
}

// ═══════════════════════════════════════════════════════════════════════════
// SurfaceVault Constructor & Accessors
// ═══════════════════════════════════════════════════════════════════════════

SurfaceVault::SurfaceVault(const SurfaceVaultGuide& guide, const std::string& name)
    : WoodSession(name),
      guide(guide) {

    // mesh: sewn NURBS surface subdivision
    _mesh = surface_vault_mesh(guide);

    // blocks: volumetric extrusion, side planarization, and top/bottom facet shift
    const std::vector<std::shared_ptr<NodeBlockElement>> blocks = ngon_solids(
        _mesh,
        guide.thickness,
        guide.planar_sides,
        guide.flatten_caps,
        guide.facet_shift,
        guide.bevel_degrees,
        guide.trim_base,
        guide.base_z,
        &_metrics
    );
    for (const std::shared_ptr<NodeBlockElement>& block : blocks)
        add(block);

    // contacts: coplanar side-joint overlaps between touching blocks
    compute_face_contacts(0);

    // assembly: blocks raised from springings to crown
    assemblies = {AssemblyModule::raise(name, AssemblyModule::parts_of(get_elements<Element>()))};
}

const Mesh& SurfaceVault::mesh() const {
    return _mesh;
}

double SurfaceVault::side_planarity() const {
    return _metrics.side_planarity;
}

double SurfaceVault::cap_planarity() const {
    return _metrics.cap_planarity;
}

}
