#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("assignment");

    // In-plane plates: seam at x = 300 keeps default 90-degree fingers; seam at x = 600 is angled by line
    wood_session.add(
        Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(300, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(600, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );

    // Top-to-side T-joint: horizontal base (50 mm) and vertical wall with angled tenons and mortises
    wood_session.add(
        Plate::from_rectangle(Point(1040, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 420, 420, 50)
    );
    wood_session.add(
        Plate::from_rectangle(Point(1230, 0, 50), Vector(0, 1, 0), Vector(0, 0, 1), 420, 280, 40)
    );

    wood_session.settings.joint_parameters[0 * 3 + 0] = 60;
    wood_session.settings.joint_parameters[2 * 3 + 0] = 100;
    wood_session.settings.joint_volume_extension = {0, 0, -50};

    const Point straight_seam(300, 210, 40.5);
    const Point angled_seam(600, 210, 40.5);
    const Point tenon_edge(1229.5, 230, 50.5);

    const Line finger_dir = Line::from_points(angled_seam, angled_seam + Vector(-125, 145, 0));
    const Line tenon_dir = Line::from_points(tenon_edge, tenon_edge + Vector(0, -145, 160));

    const int finger_id = JointPlate::library_id("side_side_inplane_dovetails");
    const int tenon_id = JointPlate::library_id("top_side_tenons");
    wood_session.assign_joint_types_by_points(
        {straight_seam, angled_seam, tenon_edge}, {finger_id, finger_id, tenon_id}, 30.0
    );
    wood_session.assign_insertion_vectors_by_lines({finger_dir, tenon_dir}, 30.0);

    wood_session.compute_features(face_to_face);
    std::cout << wood_session.get_plate_features().size() << " assigned joints solved\n";

    const Color pink(0.807f, 0.063f, 0.258f);
    for (const Point& pt : {straight_seam, angled_seam, tenon_edge}) {
        auto marker = std::make_shared<Point>(pt);
        marker->pointcolor = pink;
        wood_session.add_point(marker);
    }
    for (const Line& dir_line : {finger_dir, tenon_dir}) {
        const Vector u = dir_line.to_vector().normalized();
        const Vector n = std::abs(u[2]) < 0.1 ? Vector(-u[1], u[0], 0) : Vector(0, -u[2], u[1]);
        const Point tip = dir_line.end();
        auto arrow = std::make_shared<Polyline>(std::vector<Point>{
            dir_line.start(), tip, tip - u * 28 + n * 14, tip, tip - u * 28 - n * 14
        });
        arrow->linecolor = pink;
        arrow->width = 5.0;
        wood_session.add_polyline(arrow);
    }

    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
assigns side_side_inplane_dovetails and top_side_tenons from edge points and alters both finger-joint and tenon-mortise directions from lines placed on the shared plate edges.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target 7_assignment --parallel 4 && ./build/7_assignment && ../bash/publish-scene.sh --target 7_assignment

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
