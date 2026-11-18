#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_grid;

struct FacadeCase {
    std::string name;
    Plan plan;
    Framing framing;
    double width;
};

int main() {
    const Vector X(1.0, 0.0, 0.0);
    const Vector Y(0.0, 1.0, 0.0);
    const double GAP = 5000.0;

    const Profiles mullion_profiles{
        .girder = profile_rectangle(160.0, 280.0),
        .beam = profile_rectangle(140.0, 280.0),
        .brace = profile_rectangle(140.0, 180.0),
    };

    // 1. Regular vertical mullion-and-transom timber curtain wall (`span = 1` -> continuous vertical mullions).
    const Plan curtain_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(16000.0, 0.0, 0.0)),
        10800.0,
        {4000.0, 4000.0, 4000.0, 4000.0},
        {3600.0, 3600.0, 3600.0}
    );

    // 2. Punched timber facade with window/door opening voids and flush CLT spandrel panels.
    const std::vector<Polyline> window_openings = {
        Polyline::rectangle(Point(3000.0, 0.0, 0.0), X, Y, 3000.0, 3200.0),
        Polyline::rectangle(Point(9000.0, 900.0, 0.0), X, Y, 3000.0, 2300.0),
        Polyline::rectangle(Point(3000.0, 4500.0, 0.0), X, Y, 3000.0, 2300.0),
        Polyline::rectangle(Point(9000.0, 4500.0, 0.0), X, Y, 3000.0, 2300.0),
    };
    const Plan punched_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(15000.0, 0.0, 0.0)),
        8000.0,
        {3000.0, 3000.0, 3000.0, 3000.0, 3000.0},
        {900.0, 2300.0, 1300.0, 2300.0, 1200.0},
        window_openings
    );

    // 3. Wind-braced multi-storey timber facade (`Brace::Cross` in outer bays).
    const Plan braced_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(18000.0, 0.0, 0.0)),
        11400.0,
        {4500.0, 4500.0, 4500.0, 4500.0},
        {3800.0, 3800.0, 3800.0}
    );
    std::vector<size_t> outer_bays;
    for (size_t ci = 0; ci < braced_plan.cells.size(); ++ci) {
        std::vector<Point> pts;
        for (size_t vi : braced_plan.cells[ci].outer)
            pts.push_back(braced_plan.nodes[vi]);
        const Point c = Point::centroid(pts);
        if (c[0] < 4500.0 || c[0] > 13500.0)
            outer_bays.push_back(ci);
    }

    // 4. Pitched pentagonal gable timber facade (`silhouette_uv`).
    const Polyline gable_silhouette({
        Point(0.0, 0.0, 0.0),
        Point(16000.0, 0.0, 0.0),
        Point(16000.0, 6400.0, 0.0),
        Point(8000.0, 10800.0, 0.0),
        Point(0.0, 6400.0, 0.0),
        Point(0.0, 0.0, 0.0),
    });
    const Plan gable_plan = Plan::facade(
        Line::from_points(Point(0.0, 0.0, 0.0), Point(16000.0, 0.0, 0.0)),
        10800.0,
        {3200.0, 3200.0, 3200.0, 3200.0, 3200.0},
        {3200.0, 3200.0, 2200.0, 2200.0},
        {},
        gable_silhouette
    );

    const std::vector<FacadeCase> cases = {
        {"curtain_mullion",
         curtain_plan,
         Framing{
             .system = System::PostAndBeam,
             .span = 1,
             .height = 0.0,
             .profiles = mullion_profiles,
         },
         16000.0},
        {"punched_openings",
         punched_plan,
         Framing{
             .system = System::PostAndBeam,
             .span = 1,
             .height = 0.0,
             .deck = 120.0,
             .deck_flush = true,
             .profiles = mullion_profiles,
         },
         15000.0},
        {"braced_facade",
         braced_plan,
         Framing{
             .system = System::PostAndBeam,
             .brace = Brace::Cross,
             .braced_bays = outer_bays,
             .span = 1,
             .height = 0.0,
             .profiles =
                 {.girder = profile_rectangle(240.0, 360.0),
                  .beam = profile_rectangle(220.0, 360.0),
                  .brace = profile_rectangle(160.0, 240.0)},
         },
         18000.0},
        {"gable_facade",
         gable_plan,
         Framing{
             .system = System::PostAndBeam,
             .brace = Brace::Chevron,
             .span = 1,
             .height = 0.0,
             .profiles = mullion_profiles,
         },
         16000.0},
    };

    WoodSession session("templates_grid_3_facades");
    double offset = 0.0;
    for (const FacadeCase& item : cases) {
        const Plan placed = item.plan.transformed(Xform::translation(offset, 0.0, 0.0));
        const Grid grid(placed, item.framing, item.name);
        session.graft(grid, session.add_group(item.name));
        offset += item.width + GAP;
    }

    std::cout << session << std::endl;
    session.pb_dump(pb_path("live"));
    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Vertical timber facade grids built with the same planar Grid engine (height = 0): a 4x3 mullion-and-transom curtain wall with continuous vertical mullions and T-butted rails; a punched facade with window and door voids and flush CLT spandrel panels; a 4x3 heavy-timber facade with co-planar X-bracing in the outer bays; and a pitched pentagonal gable wall with chevron bracing clipped cleanly to the roof rake.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_3_facades --parallel 6 && ./build/templates_grid_3_facades && ../bash/publish-scene.sh --target templates_grid_3_facades

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
