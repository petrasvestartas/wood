#include "vda_mesh.h"

#include "json.h"   // nlohmann 3.11.3, vendored in session_cpp as json.h (not .hpp)
#include <fstream>
#include <iostream>

using json = nlohmann::json;

int main()
{
    // Default 2-quad mesh (matches Python default)
    VdaMesh vda(
        VdaMesh::default_mesh(),
        /*face_thickness*/  3.0,
        /*face_positions*/  {0.0},
        /*edge_divisions*/  {2},
        /*edge_division_len*/ {},
        /*insertion_lines*/ {},
        /*rect_width*/      10.0,
        /*rect_height*/     10.0,
        /*rect_thickness*/  5.0
    );

    std::cout << "faces : " << vda.f_polylines.size() << "\n";
    for (size_t i = 0; i < vda.f_polylines.size(); ++i) {
        std::cout << "  face " << i << ": " << vda.f_polylines[i].size()
                  << " polylines\n";
        for (size_t j = 0; j < vda.f_polylines[i].size(); ++j) {
            auto pts = vda.f_polylines[i][j].get_points();
            std::cout << "    pline " << j << ": " << pts.size() << " pts\n";
        }
    }
    std::cout << "edges : " << vda.e_polylines.size() << "\n";
    for (size_t i = 0; i < vda.e_polylines.size(); ++i) {
        std::cout << "  edge " << i << ": " << vda.e_polylines[i].size()
                  << " polylines";
        if (!vda.e_polylines_index[i].empty() && !vda.e_polylines_index[i][0].empty()) {
            std::cout << "  [" << vda.e_polylines_index[i][0] << "]";
        }
        std::cout << "\n";
    }

    // ── JSON export ──────────────────────────────────────────────────────────
    json out;

    // Face polylines
    json jf = json::array();
    for (const auto& face_plines : vda.f_polylines) {
        json jface = json::array();
        for (const auto& pl : face_plines) {
            json jpline = json::array();
            for (const auto& pt : pl.get_points()) {
                jpline.push_back({pt[0], pt[1], pt[2]});
            }
            jface.push_back(jpline);
        }
        jf.push_back(jface);
    }
    out["f_polylines"] = jf;

    // Face plane origins
    json jfp = json::array();
    for (const auto& face_planes : vda.f_polylines_planes) {
        json jrow = json::array();
        for (const auto& pl : face_planes) {
            if (pl.is_valid()) {
                jrow.push_back({pl.origin()[0], pl.origin()[1], pl.origin()[2]});
            }
        }
        jfp.push_back(jrow);
    }
    out["f_polylines_planes"] = jfp;

    // Edge polylines
    json je = json::array();
    for (const auto& edge_plines : vda.e_polylines) {
        json jedge = json::array();
        for (const auto& pl : edge_plines) {
            json jpline = json::array();
            for (const auto& pt : pl.get_points()) {
                jpline.push_back({pt[0], pt[1], pt[2]});
            }
            jedge.push_back(jpline);
        }
        je.push_back(jedge);
    }
    out["e_polylines"] = je;

    // Edge indices
    json jei = json::array();
    for (const auto& idx_row : vda.e_polylines_index) {
        json jrow = json::array();
        for (const auto& s : idx_row) {
            jrow.push_back(s);
        }
        jei.push_back(jrow);
    }
    out["e_polylines_index"] = jei;

    std::ofstream ofs("src/templates/vda_mesh_cpp.json");
    ofs << out.dump(2);
    std::cout << "wrote src/templates/vda_mesh_cpp.json\n";
    return 0;
}
