#include "pch.h"
#include "src/templates/floor/floor_geometry.h"

using namespace session_cpp;

namespace wood_floor {

using namespace wood_floor::geometry;

const std::array<std::string, 6> FAMILY_NAMES = {"outer_ribs", "inner_ribs", "inner_beams", "wedges_inner_beams", "tsections", "beds"}; // the group each quarter family is named after, in Family order

/// A ring, column or support reference.
static MemberRef shared_member(Family family, size_t index) {
    return MemberRef{-1, family, index, -1};
}

// ═══════════════════════════════════════════════════════════════════════════
// References
// ═══════════════════════════════════════════════════════════════════════════

std::string MemberRef::name() const {

    if (family == Family::ring)
        return fmt::format("oculus_{}", index);

    if (family == Family::column)
        return fmt::format("column_{}", index);

    if (family == Family::support)
        return fmt::format("support_{}", index);

    if (family == Family::beds)
        return fmt::format("beds_{}_{}_{}", row, index, quarter);

    return fmt::format("{}_{}_{}", FAMILY_NAMES[static_cast<size_t>(family)], index, quarter);
}

double Relationship::area() const {
    return polygon_area(contact);
}

std::string relation_name(Relation kind) {

    const std::array<std::string, 12> kinds = {"support", "column_plate", "cross_lap", "seam_tie", "seam_wedge", "oculus_wedge", "block_dowels", "screw_rib_beam", "screw_beam_mitre", "screw_rib_corner", "screw_ring", "screw_oculus"};

    return kinds[static_cast<size_t>(kind)];
}

std::string Relationship::text() const {
    return fmt::format("{} {} - {}", relation_name(kind), a.name(), b.name());
}

Place Relationship::place() const {

    if (kind == Relation::column_plate || kind == Relation::cross_lap || kind == Relation::support)
        return Place::column;

    if (kind == Relation::seam_wedge || kind == Relation::seam_tie)
        return Place::seam;

    if (kind == Relation::oculus_wedge || kind == Relation::screw_ring || kind == Relation::screw_oculus)
        return Place::oculus;

    return Place::quarter;
}

// ═══════════════════════════════════════════════════════════════════════════
// Relationships
// ═══════════════════════════════════════════════════════════════════════════

/// The seam wedge of seam q: inner beam 0 of q and inner beam 2 of q + 1 on the seam plane, the contact beam 0's loop on it; run on to the bay's outer face when the beams run through the rib band.
static Relationship seam_wedge(const Floor& floor, size_t q) {

    const double lift = floor.sizes.bay_height;
    Relationship row;
    row.kind = Relation::seam_wedge;
    row.a = quarter_member(q, Family::inner_beams, 0);
    row.b = quarter_member((q + 1) % 4, Family::inner_beams, 2);
    row.plane = lifted(floor.seams[q].plane_into(q), lift);
    row.contact = lifted(open_points(floor.quarter(q).inner_beams()[0].bottom), lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    if (floor.sizes.seam_through_ribs)
        row.end = lifted(floor.edges[q].band[0], lift);

    return row;
}

/// The oculus wedge of quarter q: inner beam 1 of q and ring beam q on the tilted plane, the contact beam 1's loop on it.
static Relationship oculus_wedge(const Floor& floor, size_t q) {

    const double lift = floor.sizes.bay_height;
    Relationship row;
    row.kind = Relation::oculus_wedge;
    row.a = quarter_member(q, Family::inner_beams, 1);
    row.b = shared_member(Family::ring, q);
    row.plane = lifted(floor.oculus_edges[q].tilted, lift);
    row.contact = lifted(open_points(floor.quarter(q).inner_beams()[1].bottom), lift);
    row.type = wood_session::ContactType::side_side;
    row.seam_or_corner = q;

    return row;
}

/// The column plate of corner q on outer rib k: the rib's column end face on the fan side plane, clipped at the middle cutter level where the carved face ends.
static Relationship column_plate(const Floor& floor, size_t q, size_t k) {

    const double lift = floor.sizes.bay_height;
    const Outline rib = floor.quarter(q).outer_ribs()[k];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    Relationship row;
    row.kind = Relation::column_plate;
    row.a = shared_member(Family::column, q);
    row.b = quarter_member(q, Family::outer_ribs, k);
    row.plane = lifted(floor.columns[q].wedge_fan[k == 0 ? 0 : 2][0], lift);
    row.contact = lifted(above({top[1], top[2], bottom[2], bottom[1]}, floor.columns[q].levels[1]), lift);
    row.seam_or_corner = q;

    return row;
}

/// The cross lap of corner q: its two column plates, on outer ribs 0 and 1, crossing inside the column.
static Relationship cross_lap(size_t q) {

    Relationship row;
    row.kind = Relation::cross_lap;
    row.a = quarter_member(q, Family::outer_ribs, 0);
    row.b = quarter_member(q, Family::outer_ribs, 1);
    row.seam_or_corner = q;

    return row;
}

/// The tie of seam q: outer rib 0 of q and outer rib 1 of q + 1 meeting end to end on the seam plane, the contact rib 0's seam end face.
static Relationship seam_tie(const Floor& floor, size_t q) {

    const double lift = floor.sizes.bay_height;
    const Outline rib = floor.quarter(q).outer_ribs()[0];
    const std::vector<Point> top = rib.top.get_points();
    const std::vector<Point> bottom = rib.bottom.get_points();
    const size_t n = top.size();
    Relationship row;
    row.kind = Relation::seam_tie;
    row.a = quarter_member(q, Family::outer_ribs, 0);
    row.b = quarter_member((q + 1) % 4, Family::outer_ribs, 1);
    row.plane = lifted(floor.seams[q].plane_into(q), lift);
    row.contact = lifted({top[0], top[n - 2], bottom[n - 2], bottom[0]}, lift);
    row.type = wood_session::ContactType::end_end;
    row.seam_or_corner = q;

    return row;
}

/// The dowels of block k of quarter q on one of its two ribs: the block's face on that rib's plane, corners 3 and 0 of its loops on the first rib plane, 1 and 2 on the second.
static Relationship block_dowels(const Floor& floor, size_t q, size_t k, size_t side, const MemberRef& rib) {

    const double lift = floor.sizes.bay_height;
    const Outline block = floor.quarter(q).wedges_inner_beams()[k];
    const std::vector<Point> top = block.top.get_points();
    const std::vector<Point> bottom = block.bottom.get_points();
    const ConstructionPlanes& cp = floor.geometry[q].planes;
    const std::array<std::array<Plane, 2>, 3> ribs = {{{cp.outer_ribs[0][1], cp.inner_ribs[0][0]}, {cp.inner_ribs[0][1], cp.inner_ribs[1][1]}, {cp.inner_ribs[1][0], cp.outer_ribs[1][1]}}};
    Relationship row;
    row.kind = Relation::block_dowels;
    row.a = rib;
    row.b = quarter_member(q, Family::wedges_inner_beams, k);
    row.plane = lifted(ribs[k][side], lift);
    row.contact = side == 0 ? lifted({bottom[3], bottom[0], top[0], top[3]}, lift) : lifted({bottom[1], bottom[2], top[2], top[1]}, lift);
    row.seam_or_corner = q;

    return row;
}

/// The support of corner q under its column.
static Relationship support(const Floor& floor, size_t q) {

    Relationship row;
    row.kind = Relation::support;
    row.a = shared_member(Family::support, q);
    row.b = shared_member(Family::column, q);
    row.plane = floor.columns[q].support_plane;
    row.seam_or_corner = q;

    return row;
}

std::vector<Relationship> relationships(const Floor& floor) {

    std::vector<Relationship> rows;

    for (size_t q = 0; q < 4; q++)
        rows.push_back(seam_wedge(floor, q));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(oculus_wedge(floor, q));

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            rows.push_back(column_plate(floor, q, k));

    for (size_t q = 0; q < 4; q++)
        rows.push_back(cross_lap(q));

    for (size_t q = 0; q < 4 && !floor.sizes.seam_through_ribs; q++)
        rows.push_back(seam_tie(floor, q));

    for (size_t q = 0; q < 4; q++) {
        rows.push_back(block_dowels(floor, q, 0, 0, quarter_member(q, Family::outer_ribs, 0)));
        rows.push_back(block_dowels(floor, q, 2, 1, quarter_member(q, Family::outer_ribs, 1)));
        rows.push_back(block_dowels(floor, q, 0, 1, quarter_member(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(floor, q, 1, 0, quarter_member(q, Family::inner_ribs, 0)));
        rows.push_back(block_dowels(floor, q, 1, 1, quarter_member(q, Family::inner_ribs, 1)));
        rows.push_back(block_dowels(floor, q, 2, 0, quarter_member(q, Family::inner_ribs, 1)));
    }

    for (size_t q = 0; q < 4; q++)
        rows.push_back(support(floor, q));

    for (const Relationship& row : screw_relationships(floor))
        rows.push_back(row);

    return rows;
}

std::vector<Relationship> relationships(const Floor& floor, Relation kind) {

    std::vector<Relationship> rows;

    for (const Relationship& row : relationships(floor))
        if (row.kind == kind)
            rows.push_back(row);

    return rows;
}

}
