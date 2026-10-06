#pragma once
#include "wood_session.h"
#include "src/templates/floor/floor.h"

/// The documentation film of the floor: every chapter writes its frames, one scene and its notes each, that docs/floor/render.py renders into docs/templates/floor.
namespace movie {

using namespace session_cpp;
using namespace wood_session;
using namespace wood_floor;

// ═══════════════════════════════════════════════════════════════════════════
// Look
// ═══════════════════════════════════════════════════════════════════════════

// The role a colour plays in a frame, from the Block Research Group palette (brg.ethz.ch): the same colour names the same role in the picture and in its page's text.
const Color BUILT(33.0f / 255.0f, 150.0f / 255.0f, 234.0f / 255.0f, 1.0f, "built"); // #2196EA, BRG primary: what this step builds.
const Color VARIABLE(232.0f / 255.0f, 71.0f / 255.0f, 139.0f / 255.0f, 1.0f, "variable"); // #E8478B, pink: the variable or value this step introduces.
const Color RESULT(242.0f / 255.0f, 204.0f / 255.0f, 12.0f / 255.0f, 1.0f, "result"); // #F2CC0C, yellow: a second thing this step builds, set apart from the first.
const Color INPUT(115.0f / 255.0f, 115.0f / 255.0f, 115.0f / 255.0f, 1.0f, "input"); // #737373, neutral grey: what this step reads from earlier steps; dashed, a construction helper.
const Color GREY(218.0f / 255.0f, 218.0f / 255.0f, 218.0f / 255.0f, 1.0f, "context"); // #DADADA, light neutral grey: everything else, for context, solid with light edges.
const Color EDGE_GREY(184.0f / 255.0f, 184.0f / 255.0f, 184.0f / 255.0f, 1.0f, "context_edge"); // #B8B8B8: the edges of context geometry.
const Color RING(244.0f / 255.0f, 166.0f / 255.0f, 200.0f / 255.0f, 1.0f, "oculus"); // #F4A6C8, pink tint: the oculus ring, where families are told apart.
const double PEN = 2.0; // Every line's width, px: the colour alone tells the roles apart.
const Color INK(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f, "dimension"); // #1A1A1A: dimension lines, one pen for all.
const Color STEEL(110.0f / 255.0f, 110.0f / 255.0f, 110.0f / 255.0f, 1.0f, "column"); // #6E6E6E, dark neutral grey: the column and its support, where families are told apart.

/// The six member families of a quarter, in the order the Floor adds them.
enum class Family {
    outer_ribs, // Variable beams under the two outer parabolas.
    inner_ribs, // Variable beams under the two inner parabolas.
    inner_beams, // Variable beams on the seams and the oculus edge.
    wedges, // The three column blocks at the column head, plates.
    tsections, // T-section plates beside the ribs.
    beds, // Bed plates in three rows.
};

const std::array<Family, 6> FAMILIES = {Family::outer_ribs, Family::inner_ribs, Family::inner_beams, Family::wedges, Family::tsections, Family::beds};

const std::array<std::string, 6> FAMILY_NAMES = {"outer_ribs", "inner_ribs", "inner_beams", "wedges", "tsections", "beds"}; // The name of each family, in Family order: the FloorGuide method and the element name prefix.

/// The colour of each family, in Family order: pink, yellow, two neutral greys and the yellow and blue tints, the same as FloorGuide::draw.
const std::array<Color, 6> FAMILY_COLORS = {
    Color(232.0f / 255.0f, 71.0f / 255.0f, 139.0f / 255.0f, 1.0f, "outer_ribs"),
    Color(242.0f / 255.0f, 204.0f / 255.0f, 12.0f / 255.0f, 1.0f, "inner_ribs"),
    Color(124.0f / 255.0f, 124.0f / 255.0f, 124.0f / 255.0f, 1.0f, "inner_beams"),
    Color(168.0f / 255.0f, 168.0f / 255.0f, 168.0f / 255.0f, 1.0f, "wedges"),
    Color(245.0f / 255.0f, 216.0f / 255.0f, 144.0f / 255.0f, 1.0f, "tsections"),
    Color(166.0f / 255.0f, 211.0f / 255.0f, 246.0f / 255.0f, 1.0f, "beds"),
};

// ═══════════════════════════════════════════════════════════════════════════
// Camera boxes
// ═══════════════════════════════════════════════════════════════════════════

// Every camera box is std::array<double, 6>, mm: x0, y0, z0, x1, y1, z1.

const double H = 3500.0; // The default FloorGuide::bay_height: the guide is drawn where the floor stands, its datum lifted to this.
const Xform LIFT = Xform::translation(0.0, 0.0, H);
const std::array<double, 6> BAY = {-3300.0, -3300.0, H - 800.0, 3300.0, 3300.0, H + 50.0}; // The whole bay in plan.
const std::array<double, 6> QUARTER = {-3100.0, -3100.0, H - 700.0, 100.0, 100.0, H + 20.0}; // Quarter 0 in 3D.
const std::array<double, 6> PLANES = {-3100.0, -3100.0, H - 160.0, 100.0, 100.0, H + 160.0}; // Quarter 0's planes near the datum.
const std::array<double, 6> HEAD = {-3040.0, -3040.0, H - 10.0, -2740.0, -2740.0, H + 10.0}; // Column head 0 in plan.
const std::array<double, 6> FAN = {-3080.0, -3080.0, H - 800.0, -2350.0, -2350.0, H + 40.0}; // Column 0's fan and head in 3D.
const std::array<double, 6> OCULUS = {-1300.0, -1300.0, H - 320.0, 1300.0, 1300.0, H + 40.0}; // The oculus in 3D.
const std::array<double, 6> FLOOR = {-3300.0, -3300.0, 0.0, 3300.0, 3300.0, H + 100.0}; // The floor on its columns.
const std::array<double, 6> RIB = {-3100.0, -3050.0, H - 700.0, 100.0, -2850.0, H + 60.0}; // Outer rib 0 in elevation, with the front view.
const std::array<double, 6> PANEL = {-2900.0, -2900.0, H - 650.0, -300.0, -300.0, H + 30.0}; // The central panel between the inner ribs.

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
    std::array<double, 6> box; // What the camera frames.
    double plane_size = 120.0; // Half side of every drawn plane's square, mm.
    bool features = false; // Draw the elements' features: drills and cutters.
    bool key = false; // Also in the overview film on the floor page.
    std::string orbit; // Mouse pixels dx,dy the camera turns from the view, empty for none.
    double distance = 1.0; // The fitted camera distance times this: below 1 comes closer.
    WoodSession scene; // Everything the frame shows.
    WoodSession bare; // The same without the highlighted surfaces: the render blends the two so only those are see-through.
    bool translucent = false; // Whether any highlighted surface was drawn.
    nlohmann::json labels = nlohmann::json::array();
    std::vector<std::pair<Line, int>> covers; // Every drawn segment with the rank of its colour, for cutting lower ones away under it.
    std::vector<std::pair<Line, int>> pending; // Context and input segments, drawn at write minus where a higher rank covers them.

    Frame(const std::string& frame_chapter, size_t number, const std::string& slug, const std::string& frame_caption, const std::string& frame_view, const std::array<double, 6>& frame_box);

    /// A name plate for a scene point; the renderer places it clear of the others with a leader back to the point, or on the point when centred.
    void label(const std::string& text, const Point& at, bool centred = false);

    void polyline(Polyline polyline, const Color& color, double width = 2.5);

    /// One segment in the pen, drawn now or, below the top rank, held back for write.
    void stroke(Line line, const Color& color, bool headed);

    /// A line, dashed for a construction helper, headed for a direction.
    void line(Line line, const Color& color, double width = 2.5, bool dashed = false, bool headed = false);

    void point(Point point, const Color& color, double width = 12.0);

    /// A plane: its square filled with a pale tint of the colour, edged in the colour, and its normal headed.
    void plane(Plane plane, const Color& color);

    /// A planar closed loop filled with a pale tint of the colour and edged in the colour.
    void face(const Polyline& loop, const Color& color, double width = 2.5);

    /// A planar closed loop filled with a pale tint of the colour, no edge: an area between lines drawn on their own.
    void fill(const Polyline& loop, const Color& color);

    /// A dimension between two scene points moved off them by offset, in INK: extension lines, the dimension line, a 45 degree tick at each end, the value as a label on its middle.
    void dimension(const Point& a, const Point& b, const Vector& offset, const std::string& text);

    /// A member outline from the guide drawn as the solid between its two loops, lifted from the datum to the floor.
    void solid(const std::array<Polyline, 2>& loops, const Color& color);

    /// A copy of the element in the colour: in GREY a solid light grey mesh with light edges, in INPUT the same with INPUT edges, in any other colour highlighted, see-through in the render.
    void element(const std::shared_ptr<Element>& element, const Color& color);

    /// A mesh in the colour, highlighted unless GREY or INPUT, which draw it solid light grey.
    void mesh(Mesh mesh, const Color& color);

    /// A boundary representation in the colour, highlighted.
    void brep(const BRep& brep, const Color& color);

    /// The scene and its notes and camera beside it, in dir.
    void write(const std::filesystem::path& dir);
};

// ═══════════════════════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════════════════════

/// The middle of a member: the mean of its two loops' area centroids.
Point middle(const std::array<Polyline, 2>& loops);

/// Quarter q's loops of one family in member order, the bed rows one after another.
std::vector<std::array<Polyline, 2>> outlines(const FloorGuide& guide, size_t q, Family family);

/// The placed elements of one family of a quarter in member order, the bed rows one after another.
std::vector<std::shared_ptr<Element>> members(const QuarterMembers& quarter, Family family);

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

void chapter_00_vocabulary(const Context& context);

}
