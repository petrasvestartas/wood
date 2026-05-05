// main_annen_chevron.cpp — load 23 Annen building NURBS surfaces,
// generate chevron mesh + plates + joinery data on each surface,
// dump to data/output/annen_chevron.pb and joinery .txt files.
#include <filesystem>
#include <fstream>
#include <iostream>
#include "session.h"
#include "wood_chevron.h"
using namespace session_cpp;

// Write joinery txt files consumed by get_connection_zones().
// Appends all surfaces into a single set of files (one row per plate-pair,
// across all surfaces in order).
static void write_joinery_txt(
    const std::filesystem::path&            out_dir,
    const std::string&                      dataset_name,
    const std::vector<wood_chevron::ChevronResult>& results)
{
    // ── insertion_vectors.txt ─────────────────────────────────────────────
    {
        std::ofstream f((out_dir / (dataset_name + "_insertion_vectors.txt")).string());
        for (auto& r : results)
            for (auto& iv : r.insertion_vectors) {
                for (int k = 0; k < 18; k++)
                    f << (k ? " " : "") << iv[k];
                f << "\n";
            }
    }
    // ── joints_types.txt ──────────────────────────────────────────────────
    {
        std::ofstream f((out_dir / (dataset_name + "_joints_types.txt")).string());
        for (auto& r : results)
            for (auto& jp : r.joints_per_face) {
                for (int k = 0; k < 6; k++)
                    f << (k ? " " : "") << jp[k];
                f << "\n";
            }
    }
    // ── three_valence.txt ─────────────────────────────────────────────────
    // Type 0 (Annen alignment).  Each surface's plate-pair indices must be
    // offset by the cumulative plate-pair count of all previous surfaces.
    {
        std::ofstream f((out_dir / (dataset_name + "_three_valence.txt")).string());
        f << "0\n";  // type 0 = Annen joint-line alignment
        int base = 0;
        for (auto& r : results) {
            for (auto& tv : r.three_valence)
                f << (tv[0]+base) << " " << (tv[1]+base) << " "
                  << (tv[2]+base) << " " << (tv[3]+base) << "\n";
            base += (int)r.joints_per_face.size();
        }
    }
    // ── adjacency.txt ─────────────────────────────────────────────────────
    {
        std::ofstream f((out_dir / (dataset_name + "_adjacency.txt")).string());
        int base = 0;
        for (auto& r : results) {
            for (auto& [a, b] : r.adjacency)
                f << (a+base) << " " << (b+base) << "\n";
            base += (int)r.joints_per_face.size();
        }
    }
}

int main() {
    auto root   = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    auto json   = (root / "data" / "annen_surfaces.json").string();
    auto outdir = root / "data" / "output";
    std::filesystem::create_directories(outdir);

    auto surfaces = wood_chevron::annen_surfaces(json);
    std::cout << "Loaded " << surfaces.size() << " Annen surfaces\n";

    Session session("annen_chevron");
    std::vector<wood_chevron::ChevronResult> all_results;
    all_results.reserve(surfaces.size());

    for (size_t i = 0; i < surfaces.size(); i++) {
        surfaces[i].name = "annen_" + std::to_string(i);
        Mesh m = wood_chevron::chevron_mesh(surfaces[i], 4, 900.0, 0.5, 0.05799);
        m.name = "chevron_" + std::to_string(i);

        auto res = wood_chevron::chevron_plates(m, 1.0, 0.5, 760.0, 80.0, 40.0, true);

        std::cout << "  [" << i << "] v=" << m.number_of_vertices()
                  << " f=" << m.number_of_faces()
                  << " plate-pairs=" << (res.plines.size() / 2)
                  << " adjacency=" << res.adjacency.size()
                  << " tv=" << res.three_valence.size() << "\n";

        session.add_nurbssurface(std::make_shared<NurbsSurface>(surfaces[i]));
        session.add_mesh(std::make_shared<Mesh>(m));

        for (size_t j = 0; j < res.plines.size(); j++) {
            res.plines[j].name = "plate_" + std::to_string(i) + "_" + std::to_string(j);
            session.add_polyline(std::make_shared<Polyline>(res.plines[j]));
        }
        for (auto& bl : res.box_insertion_lines)
            session.add_polyline(std::make_shared<Polyline>(bl));

        all_results.push_back(std::move(res));
    }

    session.pb_dump((outdir / "annen_chevron.pb").string());
    std::cout << "Written: " << (outdir / "annen_chevron.pb").string() << "\n";

    write_joinery_txt(outdir, "annen_chevron", all_results);
    std::cout << "Written joinery txt files to " << outdir.string() << "\n";
    return 0;
}
