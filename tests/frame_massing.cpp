#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/frame/frame.h"

using namespace session_cpp;
using namespace wood_session;
using namespace wood_frame;

namespace {

constexpr double CLEARANCE = 1.0;

void check(bool ok, const std::string& message) {
    if (!ok)
        throw std::runtime_error(message);
}

std::vector<Plane> outward_planes(const Mesh& mesh) {
    const Point centre = mesh.centroid();
    std::vector<Plane> planes;
    for (const size_t face : mesh.faces()) {
        const std::vector<Point> pts = mesh.face_polygon(face)->get_points();
        const Vector n = compute_newell(pts).normalized();
        const Point orig = Point::centroid(pts);
        planes.push_back(Plane::from_point_normal(orig, (orig - centre).dot(n) < 0.0 ? -n : n));
    }
    return planes;
}

bool is_convex_mesh(const Mesh& mesh, const std::vector<Plane>& planes) {
    for (const Plane& p : planes)
        for (const size_t v : mesh.vertices())
            if ((*mesh.vertex_point(v) - p.origin()).dot(p.z_axis()) > CLEARANCE)
                return false;
    return true;
}

double convex_overlap(const Mesh& convex, const Mesh& other) {
    Mesh part = other;
    for (const Plane& p : outward_planes(convex)) {
        part = part.cut_by_plane(
            Plane::from_point_normal(p.origin() - p.z_axis() * CLEARANCE, -p.z_axis())
        );
        if (part.faces().empty())
            return 0.0;
    }
    return std::abs(part.volume());
}

std::pair<Point, Point> mesh_box(const Mesh& mesh) {
    std::pair<Point, Point> b(Point(1e18, 1e18, 1e18), Point(-1e18, -1e18, -1e18));
    for (const size_t v : mesh.vertices()) {
        const Point p = *mesh.vertex_point(v);
        for (int a = 0; a < 3; ++a) {
            b.first[a] = std::min(b.first[a], p[a]);
            b.second[a] = std::max(b.second[a], p[a]);
        }
    }
    return b;
}

bool boxes_near(const Mesh& a, const Mesh& b) {
    const std::pair<Point, Point> ba = mesh_box(a);
    const std::pair<Point, Point> bb = mesh_box(b);
    for (int axis = 0; axis < 3; ++axis)
        if (ba.second[axis] <= bb.first[axis] + CLEARANCE ||
            bb.second[axis] <= ba.first[axis] + CLEARANCE)
            return false;
    return true;
}

double largest_clash(const WoodSession& session, std::string& pair_out) {
    double worst = 0.0;
    const std::vector<std::shared_ptr<Element>>& elems = session.elements();
    for (size_t i = 0; i < elems.size(); ++i) {
        const Mesh& a = elems[i]->model_geometry_mesh();
        const std::vector<Plane> pa = outward_planes(a);
        const bool ca = is_convex_mesh(a, pa);
        for (size_t j = i + 1; j < elems.size(); ++j) {
            const Mesh& b = elems[j]->model_geometry_mesh();
            if (!boxes_near(a, b))
                continue;
            const std::vector<Plane> pb = outward_planes(b);
            const bool cb = is_convex_mesh(b, pb);
            double ov = 0.0;
            if (!ca && !cb) {
                const Mesh common = solid_boolean(a, b, SolidOperation::intersect);
                const double vol = common.number_of_faces() ? compute_volume(common) : 0.0;
                ov = vol > CLEARANCE * 5.0 ? vol : 0.0;
            } else {
                ov = ca ? convex_overlap(a, b) : convex_overlap(b, a);
            }
            if (ov > worst) {
                worst = ov;
                pair_out = fmt::format("{}#{} vs {}#{}", elems[i]->name, i, elems[j]->name, j);
            }
        }
    }
    return worst;
}

void verify_frame(const std::string& label, const Frame& frame) {
    check(!frame.elements().empty(), label + ": produced zero elements");
    check(frame.graph.number_of_edges() > 0, label + ": produced zero face contacts");
    std::string pair_info;
    const double clash = largest_clash(frame, pair_info);
    std::cout << fmt::format(
        "{}: {} elements, {} contacts, largest clash {:.3f} mm3\n",
        label,
        frame.elements().size(),
        frame.graph.number_of_edges(),
        clash
    );
    check(
        clash <= CLEARANCE,
        fmt::format("{}: overlap {:.3f} mm3 ({}) exceeds tolerance", label, clash, pair_info)
    );
}

} // namespace

int main() {
    try {
        // 1. Portal, gable, articulated T-frame, and doubled-column cantilever halls.
        verify_frame("two_hinged_portal", Frame(FrameGuide(TwoHingedPortal{}), "two_hinged_portal"));
        verify_frame("three_hinged_gable", Frame(FrameGuide(ThreeHingedGable{}), "three_hinged_gable"));
        verify_frame("cantilever_t_frame", Frame(FrameGuide(ArticulatedTFrame{}), "cantilever_t_frame"));
        verify_frame(
            "doubled_column_cantilever",
            Frame(FrameGuide(DoubledColumnCantilever{}), "doubled_column_cantilever")
        );

        // 2. Multi-storey coupled and tiered hinged portal frames.
        verify_frame("coupled_two_hinged", Frame(FrameGuide(CoupledTwoHinged{}), "coupled_two_hinged"));
        verify_frame("coupled_three_hinged", Frame(FrameGuide(CoupledThreeHinged{}), "coupled_three_hinged"));
        verify_frame(
            "tiered_clerestory_hall", Frame(FrameGuide(TieredClerestoryHall{}), "tiered_clerestory_hall")
        );
        verify_frame("tiered_outward_hall", Frame(FrameGuide(TieredOutwardHall{}), "tiered_outward_hall"));

        // 3. Shear-gradated, concentric, and crosswise-stacked Vierendeel frames.
        verify_frame(
            "column_gradated_vierendeel",
            Frame(FrameGuide(ColumnGradatedVierendeel{}), "column_gradated_vierendeel")
        );
        verify_frame(
            "bay_gradated_vierendeel",
            Frame(FrameGuide(BayGradatedVierendeel{}), "bay_gradated_vierendeel")
        );
        verify_frame(
            "concentric_vierendeel_grid",
            Frame(FrameGuide(ConcentricVierendeelGrid{}), "concentric_vierendeel_grid")
        );
        verify_frame(
            "crosswise_stacked_vierendeel",
            Frame(FrameGuide(CrosswiseStackedVierendeel{}), "crosswise_stacked_vierendeel")
        );

        // 4. Cellular CLT slab box frames (`Plattenrahmen`).
        verify_frame("open_box_frames", Frame(FrameGuide(OpenBoxFrames{}), "open_box_frames"));
        verify_frame("closed_pilotis_box", Frame(FrameGuide(ClosedPilotisBox{}), "closed_pilotis_box"));
        verify_frame(
            "cantilevered_multi_box", Frame(FrameGuide(CantileveredMultiBox{}), "cantilevered_multi_box")
        );
        verify_frame(
            "crosswise_stacked_boxes", Frame(FrameGuide(CrosswiseStackedBoxes{}), "crosswise_stacked_boxes")
        );

        std::cout << "frame checks passed" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "FAILED: " << e.what() << std::endl;
        return 1;
    }
}
