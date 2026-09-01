#include <filesystem>
#include <chrono>
#include "session.h"
#include "element.h"
#include "file_obj.h"
#include "pair_polylines.h"
#include "wood_element.h"
#include "wood_session.h"
using namespace session_cpp;

int main() {
    auto base = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();

    // 1. Import + pair polylines
    auto polylines = file_obj::read_file_obj_polylines((base / "data" / "annen_polylines.obj").string());
    auto pairs = wood::pair_polylines(polylines);
    auto t1 = Clock::now();

    // 2. Build WoodElements from the (bottom, top) outline pairs.
    //
    // This used to make a session_cpp::ElementPlate per pair and then call
    // Session::compute_face_to_face. Both were deleted from session_cpp in
    // 89da090c ("refactoring"), so the contact search now runs through wood's
    // own pipeline, which is what that session-side helper was standing in for.
    std::vector<wood_session::WoodElement> elements;
    elements.reserve(pairs.size());
    for (auto [a, b] : pairs) { elements.emplace_back(polylines[a], polylines[b]); }
    auto t2 = Clock::now();

    // 3. Face-to-face contacts: broad-phase adjacency + joint classification.
    std::vector<wood_session::WoodJoint> joints =
        get_connection_zones(elements, face_to_face);
    auto t3 = Clock::now();

    // 4. Save
    Session session("WoodComplete");
    fill_session(session, elements, joints, true);
    session.pb_dump((base / "data" / "output" / "WoodComplete.pb").string());
    auto t4 = Clock::now();

    auto ms = [](auto a, auto b) { return std::chrono::duration<double,std::milli>(b-a).count(); };
    fmt::print("{} polylines -> {} elements -> {} joints\n",
        polylines.size(), elements.size(), joints.size());
    fmt::print("  import+pair: {:.0f}ms  elements: {:.0f}ms  contacts: {:.0f}ms  save: {:.0f}ms  total: {:.0f}ms\n",
        ms(t0,t1), ms(t1,t2), ms(t2,t3), ms(t3,t4), ms(t0,t4));
    return 0;
}
