#pragma once
#include "session.h"
#include "wood_session.h"

#include <stdexcept>

using namespace session_cpp;
using namespace wood_session;

/// A translation shell from two polylines with thickness.
///
/// Sweeps a cross_section curve along a profile by accumulating the
/// displacement steps of the profile, producing a quad mesh. Each quad face
/// is then offset into a top/bottom plate pair (WoodElement) using miter chamfers.
///
/// Usage:
///   TranslationShell ts;
///   TranslationShell ts(my_cross_section, my_profile, 15.0, 2.0, 8.0);
class TranslationShell {
public:
    Mesh mesh;
    std::vector<WoodElement> elements;

    TranslationShell(const Polyline& cross_section = default_cross_section(),
                     const Polyline& profile        = default_profile(),
                     double thickness   = 10.0,
                     double chamfer_bot = 1.0,
                     double chamfer_top = 6.0)
    {
        if (cross_section.point_count() < 2)
            throw std::invalid_argument("TranslationShell: cross_section must have at least 2 points");
        if (profile.point_count() < 2)
            throw std::invalid_argument("TranslationShell: profile must have at least 2 points");
        if (thickness <= 0.0)
            throw std::invalid_argument("TranslationShell: thickness must be positive");

        mesh = sweep(cross_section, profile);

        for (const auto& plate :
                Mesh::miter_contours(mesh, thickness, chamfer_bot, chamfer_top, false)) {
            auto close = [](std::vector<Point> pts) {
                if (!pts.empty()) pts.push_back(pts[0]);
                return pts;
            };
            // get<0>=top_chamfered, get<1>=bot_chamfered (use chamfered outlines for correct plate shape)
            elements.emplace_back(
                Polyline(close(std::get<1>(plate))),   // bot_chamfered
                Polyline(close(std::get<0>(plate)))    // top_chamfered
            );
        }
    }

private:
    struct empty_tag {};
    explicit TranslationShell(empty_tag) {}

    static Mesh sweep(const Polyline& cross_section, const Polyline& profile) {
        size_t nC = cross_section.point_count();
        size_t nP = profile.point_count();

        std::vector<Point> all_pts;
        all_pts.reserve(nC * nP);
        for (size_t j = 0; j < nC; ++j)
            all_pts.push_back(cross_section[j]);

        std::vector<std::vector<size_t>> faces;
        for (size_t i = 1; i < nP; ++i) {
            Point  pi  = profile[i];
            Point  pip = profile[i - 1];
            Vector step(pi[0]-pip[0], pi[1]-pip[1], pi[2]-pip[2]);

            size_t row = all_pts.size();
            for (size_t j = 0; j < nC; ++j) {
                const Point& prev = all_pts[row - nC + j];
                all_pts.push_back(Point(prev[0]+step[0], prev[1]+step[1], prev[2]+step[2]));
            }
            for (size_t j = 0; j + 1 < nC; ++j) {
                size_t new_j  = row + j;
                size_t old_j  = row - nC + j;
                faces.push_back({new_j, old_j, old_j+1, new_j+1});
            }
        }
        return Mesh::from_vertices_and_faces(all_pts, faces);
    }

    static Polyline default_cross_section() {
        return Polyline(std::vector<Point>{
            Point(4668.324796,  -1744.868541,   4.784134),
            Point(4562.643541,  -1744.868541, 126.748409),
            Point(4442.550969,  -1744.868541, 234.470158),
            Point(4306.753536,  -1744.868541, 321.402901),
            Point(4156.323594,  -1744.868541, 379.131783),
            Point(3996.588453,  -1744.868541, 399.552715),
            Point(3836.853312,  -1744.868541, 379.131783),
            Point(3686.423369,  -1744.868541, 321.402901),
            Point(3550.625939,  -1744.868541, 234.47016 ),
            Point(3430.533365,  -1744.868541, 126.748409),
            Point(3324.85211,   -1744.868541,   4.784134),
        });
    }

    static Polyline default_profile() {
        return Polyline(std::vector<Point>{
            Point(3324.85211, -1744.868541,   4.784134),
            Point(3324.85211, -1899.208521, 126.657309),
            Point(3324.85211, -2066.485875, 230.012521),
            Point(3324.85211, -2245.51187,  311.280689),
            Point(3324.85211, -2433.939031, 367.343471),
            Point(3324.85211, -2628.426542, 395.949972),
            Point(3324.85211, -2825.003177, 395.949972),
            Point(3324.85211, -3019.490677, 367.343473),
            Point(3324.85211, -3207.917847, 311.280689),
            Point(3324.85211, -3386.94384,  230.012522),
            Point(3324.85211, -3554.221194, 126.657311),
            Point(3324.85211, -3708.561177,   4.784134),
        });
    }
};
