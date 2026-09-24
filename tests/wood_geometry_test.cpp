#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, std::string_view name) {
    if (!condition) {
        std::cerr << "FAIL " << name << "\n";
        ++failures;
    }
}

template <class T>
static std::array<std::string, 4> cache_guids(const T& element) {
    return std::array<std::string, 4>{element.element_geometry_mesh().guid(),
                                      element.element_geometry_brep().guid(),
                                      element.model_geometry_mesh().guid(),
                                      element.model_geometry_brep().guid()};
}

/// Empties the parameters a plate lofts from.
static void clear_plate(Plate& value) {
    value.polylines.clear();
}

/// Empties the parameters a beam lofts from.
static void clear_beam(Beam& value) {
    value.radii.clear();
}

/// Empties the parameters a column lofts from.
static void clear_column(Column& value) {
    value.section = Polyline();
}

/// Empties the parameters a block lofts from.
static void clear_block(Block& value) {
    value.loops.clear();
}

template <class T>
static void cache_test(const T& original, void (*clear_parameters)(T&)) {
    static_assert(std::is_same_v<decltype(original.element_geometry_mesh()), const Mesh&>);
    static_assert(std::is_same_v<decltype(original.element_geometry_brep()), const BRep&>);
    static_assert(std::is_same_v<decltype(original.model_geometry_mesh()), const Mesh&>);
    static_assert(std::is_same_v<decltype(original.model_geometry_brep()), const BRep&>);

    for (bool model : {false, true}) {
        T mesh_first = original;
        const Mesh& mesh = model ? mesh_first.model_geometry_mesh() : mesh_first.element_geometry_mesh();
        check(mesh.number_of_faces() > 0, "Mesh exists");
        const std::string mesh_guid = mesh.guid();
        clear_parameters(mesh_first);
        check((model ? mesh_first.model_geometry_brep() : mesh_first.element_geometry_brep()).face_count() == 0,
              "Mesh access does not precompute BRep");
        check((model ? mesh_first.model_geometry_mesh() : mesh_first.element_geometry_mesh()).guid() == mesh_guid,
              "BRep access preserves cached mesh");

        T brep_first = original;
        const BRep& brep = model ? brep_first.model_geometry_brep() : brep_first.element_geometry_brep();
        check(brep.face_count() > 0, "BRep exists");
        const std::string brep_guid = brep.guid();
        clear_parameters(brep_first);
        check((model ? brep_first.model_geometry_mesh() : brep_first.element_geometry_mesh()).number_of_faces() == 0,
              "BRep access does not precompute mesh");
        check((model ? brep_first.model_geometry_brep() : brep_first.element_geometry_brep()).guid() == brep_guid,
              "Mesh access preserves cached BRep");
    }

    T element = original;
    const std::array<std::string, 4> guids = cache_guids(element);
    const Element& base = element;
    check(cache_guids(base) == guids, "Base Element dispatches all four methods to the derived caches");
    check(!element.geometry_synced(), "Typed access does not populate the session slot");
    check(cache_guids(element) == guids, "Repeated reads reuse every cache");
    element.compute_geometry_mesh();
    element.compute_geometry_brep();
    element.pb_dumps();
    check(cache_guids(element) == guids, "Session synchronization and serialization retain every cache");

    element.place(Xform::translation(10, 20, 30));
    const std::array<std::string, 4> moved_guids = cache_guids(element);
    for (size_t i = 0; i < guids.size(); ++i)
        check(moved_guids[i] != guids[i], "Placement invalidates every cache");

    clear_parameters(element);
    element.invalidate_geometry();
    check(!element.geometry_synced(), "Invalidation marks session slot stale");
    check(element.element_geometry_mesh().number_of_faces() == 0 && element.element_geometry_brep().face_count() == 0
          && element.model_geometry_mesh().number_of_faces() == 0 && element.model_geometry_brep().face_count() == 0,
          "Invalidation rebuilds every form from edited parameters");
}

template <class T>
static void cut_test(T element) {
    const double mesh_volume = std::abs(element.element_geometry_mesh().volume());
    const double brep_volume = std::abs(element.element_geometry_brep().volume());
    element.cuts.push_back(Plane(Point(50, 0, 0), Vector::y_axis(), Vector::z_axis()));
    element.invalidate_geometry();
    const Element& base = element;
    check(std::abs(std::abs(base.model_geometry_mesh().volume()) - mesh_volume / 2) < 1e-4,
          "Model mesh applies cuts");
    check(std::abs(std::abs(base.model_geometry_brep().volume()) - brep_volume / 2) < 1e-4,
          "Model BRep applies cuts");
    check(std::abs(std::abs(base.element_geometry_mesh().volume()) - mesh_volume) < 1e-4
          && std::abs(std::abs(base.element_geometry_brep().volume()) - brep_volume) < 1e-4,
          "Element geometry stays uncut");
}

int main() {
    const Polyline bottom = Polyline::rectangle(Point(0, 0, 0), Vector::x_axis(), Vector::y_axis(), 100, 100);
    const Polyline top = bottom.translated(Vector(0, 0, 100));
    const Plate plate(bottom, top);
    const Beam beam(Polyline({Point(0, 50, 50), Point(100, 50, 50)}), 50.0);
    const Column column(Line::from_points(Point(50, 50, 0), Point(50, 50, 100)), bottom);
    const Block block({bottom, top});

    cache_test(plate, clear_plate);
    cache_test(beam, clear_beam);
    cache_test(column, clear_column);
    cache_test(block, clear_block);
    cut_test(beam);
    cut_test(column);
    cut_test(block);

    Plate jointed = plate;
    const Polyline hole = Polyline::rectangle(Point(25, 25, 0), Vector::x_axis(), Vector::y_axis(), 50, 50);
    jointed.features.bottom = {bottom, hole};
    jointed.features.top = {top, hole.translated(Vector(0, 0, 100))};
    jointed.invalidate_geometry();
    const Mesh expected_mesh = Mesh::loft(jointed.features.bottom, jointed.features.top);
    check(jointed.model_geometry_mesh() == expected_mesh
          && jointed.model_geometry_mesh().number_of_vertices() > jointed.element_geometry_mesh().number_of_vertices(),
          "Plate model mesh lofts the merged outlines and holes");
    check(std::abs(std::abs(jointed.model_geometry_brep().volume()) - 750000) < 1e-4, "Plate model BRep includes holes");
    check(std::abs(std::abs(jointed.element_geometry_mesh().volume()) - 1000000) < 1e-4
          && std::abs(std::abs(jointed.element_geometry_brep().volume()) - 1000000) < 1e-4,
          "Plate element geometry stays uncut");

    return failures ? 1 : 0;
}
