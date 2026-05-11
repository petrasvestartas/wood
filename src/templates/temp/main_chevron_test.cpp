/// main_chevron_test.cpp
/// Runs chevron_plates() on the same synthetic 2-strip×4-row flat mesh
/// as chevron_ref.py and dumps plate coordinates to JSON for comparison.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "session.h"
#include "wood_chevron.h"
using namespace session_cpp;

int main() {
    auto root = std::filesystem::path(__FILE__)
                    .parent_path().parent_path().parent_path();
    auto out_path = (root / "src" / "templates" / "chevron_cpp.json").string();

    // ── Build the same synthetic mesh as make_test_mesh() in chevron_ref.py ──
    // 3 columns (u=0,50,100 → scaled ×100 → 0,100,200)
    // 5 rows (v=0..4 → scaled ×200 → 0,200,400,600,800)
    std::vector<std::vector<Point>> polygons;

    auto vi = [](int r, int c) -> std::array<double,3> {
        return { c * 100.0, r * 200.0, 0.0 };
    };

    for (int row = 0; row < 4; row++) {
        auto r0 = row, r1 = row + 1;
        // left strip  (col 0-1)
        polygons.push_back({
            Point(vi(r0,0)[0], vi(r0,0)[1], vi(r0,0)[2]),
            Point(vi(r1,0)[0], vi(r1,0)[1], vi(r1,0)[2]),
            Point(vi(r1,1)[0], vi(r1,1)[1], vi(r1,1)[2]),
            Point(vi(r0,1)[0], vi(r0,1)[1], vi(r0,1)[2])
        });
        // right strip (col 1-2)
        polygons.push_back({
            Point(vi(r0,1)[0], vi(r0,1)[1], vi(r0,1)[2]),
            Point(vi(r1,1)[0], vi(r1,1)[1], vi(r1,1)[2]),
            Point(vi(r1,2)[0], vi(r1,2)[1], vi(r1,2)[2]),
            Point(vi(r0,2)[0], vi(r0,2)[1], vi(r0,2)[2])
        });
    }

    Mesh mesh = Mesh::from_polylines(polygons, 0.01);
    std::cout << "Mesh: v=" << mesh.number_of_vertices()
              << " f=" << mesh.number_of_faces() << "\n";

    // ── Run full plate algorithm ──────────────────────────────────────────
    auto plates = wood_chevron::chevron_plates(mesh,
        1.0,   // edge_rotation
        0.5,   // edge_offset
        760.0, // box_height
        80.0,  // top_plate_inlet
        40.0,  // plate_thickness
        true); // ortho

    std::cout << "Plates: " << plates.plines.size() << "\n";

    // ── Dump to JSON ──────────────────────────────────────────────────────
    std::ofstream f(out_path);
    f << std::fixed << std::setprecision(6);
    f << "{\n  \"n_plines\": " << plates.plines.size() << ",\n  \"plines\": [\n";
    for (size_t pi = 0; pi < plates.plines.size(); pi++) {
        f << "    [";
        const auto& pl = plates.plines[pi];
        auto pts = pl.get_points();
        for (size_t i = 0; i < pts.size(); i++) {
            f << "\n      [" << pts[i][0] << ", " << pts[i][1] << ", " << pts[i][2] << "]";
            if (i + 1 < pts.size()) {
                f << ",";
            }
        }
        f << "\n    ]";
        if (pi + 1 < plates.plines.size()) {
            f << ",";
        }
        f << "\n";
    }
    f << "  ]\n}\n";

    std::cout << "Written: " << out_path << "\n";
    return 0;
}
