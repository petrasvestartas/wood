#pragma once
#include "wood_session.h"
#include "src/templates/floor/floor.h"
#include "src/templates/floor/floor_geometry.h"

/// The documentation film of the floor: every chapter writes its frames, one scene and its notes each, that docs/floor/render.py renders into docs/templates/floor.
namespace movie {

using namespace session_cpp;
using namespace wood_session;
using namespace wood_floor;
using namespace wood_floor::geometry;

// ═══════════════════════════════════════════════════════════════════════════
// Look
// ═══════════════════════════════════════════════════════════════════════════

const Color GREY(0.74f, 0.74f, 0.74f, 1.0f, "context"); // What earlier steps built.
const Color INK(0.10f, 0.10f, 0.10f, 1.0f, "ink"); // The construction of this step.
const Color MARK(0.86f, 0.12f, 0.12f, 1.0f, "mark"); // The variable this step introduces.
const Color RING(0.78f, 0.22f, 0.48f, 1.0f, "oculus"); // The oculus ring and plate.
const Color STEEL(0.42f, 0.42f, 0.46f, 1.0f, "column"); // The column and its support.

/// The quarter families in Family order, the colour of each in FAMILY_COLORS.
const std::array<Family, 6> FAMILIES = {Family::outer_ribs, Family::inner_ribs, Family::inner_beams, Family::wedges, Family::tsections, Family::beds};

// ═══════════════════════════════════════════════════════════════════════════
// Camera boxes
// ═══════════════════════════════════════════════════════════════════════════

/// A box the camera frames, mm: x0, y0, z0, x1, y1, z1.
using Box = std::array<double, 6>;

const double H = FloorParameters().bay_height; // The guide is drawn where the floor stands, its datum lifted to this.
const Xform LIFT = Xform::translation(0.0, 0.0, H);
const Box BAY = {-3300.0, -3300.0, H - 800.0, 3300.0, 3300.0, H + 50.0}; // The whole bay in plan.
const Box QUARTER = {-3100.0, -3100.0, H - 700.0, 100.0, 100.0, H + 20.0}; // Quarter 0 in 3D.
const Box PLANES = {-3100.0, -3100.0, H - 160.0, 100.0, 100.0, H + 160.0}; // Quarter 0's planes near the datum.
const Box HEAD = {-3040.0, -3040.0, H - 10.0, -2740.0, -2740.0, H + 10.0}; // Column head 0 in plan.
const Box FAN = {-3080.0, -3080.0, H - 800.0, -2350.0, -2350.0, H + 40.0}; // Column 0's fan and head in 3D.
const Box OCULUS = {-1300.0, -1300.0, H - 320.0, 1300.0, 1300.0, H + 40.0}; // The oculus in 3D.
const Box FLOOR = {-3300.0, -3300.0, 0.0, 3300.0, 3300.0, H + 100.0}; // The floor on its columns.
const Box RIB = {-3100.0, -3050.0, H - 700.0, 100.0, -2850.0, H + 60.0}; // Outer rib 0 in elevation, with the front view.
const Box PANEL = {-2900.0, -2900.0, H - 650.0, -300.0, -300.0, H + 30.0}; // The central panel between the inner ribs.

/// The geometry lifted from the guide's datum to the floor.
template <typename T>
T up(const T& geometry) {
    return geometry.transformed(LIFT);
}

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

/// One frame of the film: a scene, its caption, the labels pinned to its points and the camera that shows it. Its name is its number in the film and a slug, so the frames sort into the film's order.
struct Frame {
    std::string chapter; // The chapter page the frame belongs to, e.g. "01_bay".
    std::string name; // File stem: the three-digit number in the film and a slug.
    std::string caption; // The line written over the picture.
    std::string view; // top, iso, front (looking along +y), right (looking along -x).
    Box box; // What the camera frames.
    double plane_size = 120.0; // Half side of every drawn plane's square, mm.
    bool features = false; // Draw the elements' features: drills and cutters.
    bool key = false; // Also in the overview film on the floor page.
    std::string orbit; // Mouse pixels dx,dy the camera turns from the view, empty for none.
    double distance = 1.0; // The fitted camera distance times this: below 1 comes closer.
    WoodSession scene;
    nlohmann::json labels = nlohmann::json::array();

    Frame(const std::string& frame_chapter, size_t number, const std::string& slug, const std::string& frame_caption, const std::string& frame_view, const Box& frame_box);

    /// A name plate for a scene point; the renderer places it clear of the others with a leader back to the point, or on the point when centred.
    void label(const std::string& text, const Point& at, bool centred = false);

    void polyline(Polyline polyline, const Color& color, double width = 2.0);

    /// A line, dashed for a construction helper, headed for a direction.
    void line(Line line, const Color& color, double width = 2.0, bool dashed = false, bool headed = false);

    void point(Point point, const Color& color, double width = 12.0);

    /// A plane, drawn by the viewer as its square and a headed normal.
    void plane(Plane plane, const Color& color);

    /// A copy of the element in the colour.
    void element(const std::shared_ptr<Element>& element, const Color& color);

    /// The scene and its notes and camera beside it, in dir.
    void write(const std::filesystem::path& dir);
};

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The middle of a member outline: the mean of its two loops' area centroids.
Point middle(const Outline& outline);

/// The quarter's outlines of one family in member order, the bed rows one after another.
std::vector<Outline> outlines(const Quarter& quarter, Family family);

/// The quarter's placed members of one family in member order, the bed rows one after another.
std::vector<Member> members(const QuarterMembers& quarter, Family family);

/// The element name of member index of a family in quarter q, as MemberRef::name() gives it.
std::string member_name(Family family, size_t index, size_t q);

/// The bay edges and, with quarters, the four quarter polygons, grey: where the plan steps leave the drawing.
void plan_context(Frame& frame, const FloorGuide& guide, bool quarters);

/// Every placed member of quarter q, its ring beam and its column in one colour.
void quarter_members(Frame& frame, const Floor& floor, size_t q, const Color& color);

// ═══════════════════════════════════════════════════════════════════════════
// Chapters
// ═══════════════════════════════════════════════════════════════════════════

/// What every chapter draws from, built once: the default square guide, the tied 3000 x 2400 guide, a floor with its members alone, a floor with every connector and screw, and the folder the frames go to.
struct Context {
    const FloorGuide& guide;
    const FloorGuide& tied;
    const Floor& members;
    const Floor& connected;
    std::filesystem::path dir;
};

void chapter_01_bay(const Context& context);
void chapter_02_quarter_planes(const Context& context);
void chapter_03_parabolas(const Context& context);
void chapter_04_central_panel(const Context& context);
void chapter_05_rib_outlines(const Context& context);
void chapter_06_outlines(const Context& context);
void chapter_07_elements(const Context& context);
void chapter_08_relationships(const Context& context);
void chapter_09_connectors(const Context& context);
void chapter_10_screws(const Context& context);
void chapter_11_checks(const Context& context);

}
