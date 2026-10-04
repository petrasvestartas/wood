#pragma once
#include "wood_session.h"
#include "wood_profile.h"

/// Masonry vault templates after compas_dem and compas_tna: every voussoir a Block lofted from its intrados to its extrados, millimetres, the vault centred on the origin in plan with its springing at z 0.
namespace wood_vault {

// ═══════════════════════════════════════════════════════════════════════════
// Arches and barrels
// ═══════════════════════════════════════════════════════════════════════════

/// An arch of voussoirs over span, a semicircle when rise is half the span and a segment when lower: the intrados crown at rise, thickness out from the intrados, depth along y from 0.
std::vector<std::shared_ptr<session_cpp::Element>> arch(double span, double rise, double thickness, double depth, int voussoirs);

/// A barrel vault of the arch section along y from 0 to length: courses across the span, rings along the length, every odd course shifted by stagger of a ring (0 aligned joints, 0.5 running bond) with the cut ends as part voussoirs, and a closing ring at both ends when closed.
std::vector<std::shared_ptr<session_cpp::Element>> barrel(double span, double rise, double thickness, double length, int courses, int rings, double stagger = 0.5, bool closed = false);

// ═══════════════════════════════════════════════════════════════════════════
// Domes and walls
// ═══════════════════════════════════════════════════════════════════════════

/// A spherical dome of intrados radius: hoops of voussoirs between the oculus and the springing, both in degrees from the zenith, meridians around, every odd hoop shifted half a voussoir and every voussoir six-cornered, its edges bending where the hoops above and below joint, so no gap opens; the thickness tapers from bottom at the springing to top at the oculus.
std::vector<std::shared_ptr<session_cpp::Element>> dome(double radius, double bottom, double top, int meridians, int hoops, double oculus, double springing);

/// A wall of bricks along x from 0 to length, up to height, thickness along y from 0: every odd course shifted by stagger of a brick, the cut ends as part bricks.
std::vector<std::shared_ptr<session_cpp::Element>> wall(double length, double height, double thickness, double brick, double course, double stagger = 0.5);

// ═══════════════════════════════════════════════════════════════════════════
// Vaults over a square bay
// ═══════════════════════════════════════════════════════════════════════════

/// A cross (groin) vault over a square bay after compas_model's cross vault: two barrels of the arch section crossing, courses per half arch across every web in running bond along its barrel, rings over the bay side, the course at the crown shifted half a ring; in every course one groin stone bent along the diagonal takes both webs from the groin out to the first joint past it, the lowest a springer.
std::vector<std::shared_ptr<session_cpp::Element>> cross_vault(double span, double rise, double thickness, int courses, int rings);

/// A cloister (pavilion) vault over a square bay of span: four webs rising from the four walls, each the lower barrel, its voussoirs courses from the wall up to the crown and rings along the wall, cut on the bay diagonals where the webs meet.
std::vector<std::shared_ptr<session_cpp::Element>> cloister_vault(double span, double rise, double thickness, int courses, int rings);

/// A star (stellar) vault over a square bay of span on the sail surface through its corners, crown at rise: ribs along the diagonals, the tiercerons from every corner to the star points at star of the half span on the axes, the liernes from the star points to the crown and the wall arches; a web of thickness between every three ribs, a BRep solid whose intrados is one cubic NURBS sail surface trimmed by its ribs, extrados that surface moved out, sides ruled; subdivisions sets the sail's control net and the samples per rib.
std::vector<std::shared_ptr<session_cpp::Element>> star_vault(double span, double rise, double thickness, double star, double rib, int subdivisions);

} // namespace wood_vault
