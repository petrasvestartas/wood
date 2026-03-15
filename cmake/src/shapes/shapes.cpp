#include "shapes.h"
#include "nurbssurface.h"
#include "mesh.h"
#include "polyline.h"
#include "line.h"
#include "plane.h"
#include "point.h"
#include "vector.h"
#include "xform.h"
#include "knot.h"
#include "tolerance.h"
#include <cmath>
#include <fstream>
#include <algorithm>
#include <map>

namespace session_cpp {

Mesh chevron_mesh(const NurbsSurface& surface,
                              int u_divisions, double v_division_dist,
                              double shift, double scale) {
    NurbsSurface srf = surface;

    // Measure arc lengths to determine long direction
    auto du0 = srf.domain(0);
    auto dv0 = srf.domain(1);
    int nsamp = 50;
    double u_arc = 0, v_arc = 0;
    {
        double v_mid = (dv0.first + dv0.second) / 2.0;
        Point prev = srf.point_at(du0.first, v_mid);
        for (int i = 1; i <= nsamp; i++) {
            Point curr = srf.point_at(du0.first + (du0.second - du0.first) * i / nsamp, v_mid);
            u_arc += prev.distance(curr);
            prev = curr;
        }
    }
    {
        double u_mid = (du0.first + du0.second) / 2.0;
        Point prev = srf.point_at(u_mid, dv0.first);
        for (int i = 1; i <= nsamp; i++) {
            Point curr = srf.point_at(u_mid, dv0.first + (dv0.second - dv0.first) * i / nsamp);
            v_arc += prev.distance(curr);
            prev = curr;
        }
    }

    // Transpose so v is the long direction (march direction)
    if (u_arc > v_arc) {
        srf.transpose();
        std::swap(u_arc, v_arc);
    }

    auto du = srf.domain(0);
    auto dv = srf.domain(1);

    // Scale v_division_dist from physical to parametric units
    double param_v = dv.second - dv.first;
    double half_v = dv.second * 0.5;
    double StepU = (du.second - du.first) / u_divisions;
    double totalV = param_v;
    double baseStepV = (v_arc > 1e-10) ? v_division_dist * param_v / v_arc : v_division_dist;

    std::vector<std::vector<Point>> polygons;

    double ctU = 0;
    for (int j = 0; j < u_divisions; j++) {
        double ctV = 0;
        double thresh = totalV / 2.0;
        double StepV1 = baseStepV;
        bool running = true;
        std::vector<double> ListV;

        Point p0, p1, p2, p6, p7, p8;
        Point savept6, savept7, savept8;
        int iterations = 0;

        while (running && iterations < 1000) {
            iterations++;
            ListV.push_back(StepV1);

            if (iterations == 1) {
                p0 = srf.point_at(ctU, ctV);
                p1 = srf.point_at(ctU + StepU * 0.5, ctV);
                p2 = srf.point_at(ctU + StepU, ctV);
                p6 = srf.point_at(ctU, ctV + StepV1 * (1.0 - shift / 2.0));
                p7 = srf.point_at(ctU + StepU * 0.5, ctV + StepV1 * (1.0 + shift / 2.0));
                p8 = srf.point_at(ctU + StepU, ctV + StepV1 * (1.0 - shift / 2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            } else {
                p0 = savept6; p1 = savept7; p2 = savept8;
                p6 = srf.point_at(ctU, ctV + StepV1 * (1.0 - shift / 2.0));
                p7 = srf.point_at(ctU + StepU * 0.5, ctV + StepV1 * (1.0 + shift / 2.0));
                p8 = srf.point_at(ctU + StepU, ctV + StepV1 * (1.0 - shift / 2.0));
                savept6 = p6; savept7 = p7; savept8 = p8;
            }

            polygons.push_back({p0, p6, p7, p1});
            polygons.push_back({p1, p7, p8, p2});

            ctV += StepV1;
            thresh -= StepV1;
            StepV1 += StepV1 * scale;

            if (ctV + StepV1 > half_v) {
                ListV.push_back(thresh);
                std::reverse(ListV.begin(), ListV.end());
                double revCt = totalV / 2.0;

                for (size_t i = 0; i < ListV.size() - 1; i++) {
                    revCt += ListV[i];

                    if (i == 0) {
                        p0 = srf.point_at(ctU, revCt - ListV[i + 1] * shift / 2.0);
                        p1 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * shift / 2.0);
                        p2 = srf.point_at(ctU + StepU, revCt - ListV[i + 1] * shift / 2.0);

                        polygons.push_back({p6, p0, p1, p7});
                        polygons.push_back({p7, p1, p2, p8});

                        p6 = srf.point_at(ctU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * (1.0 + shift / 2.0));
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        savept6 = p6; savept7 = p7; savept8 = p8;
                    } else if (i == ListV.size() - 2) {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU, revCt + ListV[i + 1]);
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1]);
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1]);
                    } else {
                        p0 = savept6; p1 = savept7; p2 = savept8;
                        p6 = srf.point_at(ctU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        p7 = srf.point_at(ctU + StepU * 0.5, revCt + ListV[i + 1] * (1.0 + shift / 2.0));
                        p8 = srf.point_at(ctU + StepU, revCt + ListV[i + 1] * (1.0 - shift / 2.0));
                        savept6 = p6; savept7 = p7; savept8 = p8;
                    }

                    polygons.push_back({p1, p7, p8, p2});
                    polygons.push_back({p0, p6, p7, p1});
                }

                running = false;
            }
        }

        ctU += StepU;
    }

    return Mesh::from_polylines(polygons, 0.01);
}

std::vector<NurbsSurface> annen_surfaces() {
    std::vector<NurbsSurface> surfaces;
    std::vector<std::string> prefixes = {
        "data/annen_surfaces/",
        "../data/annen_surfaces/"
    };
    for (int i = 0; i < 23; i++) {
        std::string fname = "surface_" + std::to_string(i) + ".json";
        for (auto& prefix : prefixes) {
            std::string path = prefix + fname;
            if (!std::filesystem::exists(path)) continue;
            NurbsSurface srf = NurbsSurface::json_load(path);
            if (srf.is_valid()) { surfaces.push_back(std::move(srf)); break; }
        }
    }
    return surfaces;
}

} // namespace session_cpp
