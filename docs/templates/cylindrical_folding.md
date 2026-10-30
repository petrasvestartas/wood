# Folding Cylindrical {#templates_cylindrical_folding}

[TOC]

Folded structures of exact cylindrical [Curved Plate](@ref elements_plate_curved) lamellas with parallel straight rulings, zero Gaussian curvature, rational NURBS ellipse creases, and zero flat-unfolding distortion.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#reflection_principle">
    <img src="templates_cylindrical_folding_1_reflection_lamellas.webp" alt="Reflection Principle">
    <span class="wood-card-title">Reflection Principle</span>
  </a>
  <a class="wood-card" href="#curved_folded_plate">
    <img src="templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp" alt="Curved Folded Arch">
    <span class="wood-card-title">Curved Folded Arch</span>
  </a>
  <a class="wood-card" href="#mendrisio_pavilion">
    <img src="templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp" alt="Mendrisio Pavilion">
    <span class="wood-card-title">Mendrisio Pavilion</span>
  </a>
  <a class="wood-card" href="#folded_cylinders">
    <img src="templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp" alt="Cylindrical Halls">
    <span class="wood-card-title">Cylindrical Halls</span>
  </a>
  <a class="wood-card" href="#cylinder_vault">
    <img src="templates_cylindrical_folding_8_groin_vault_lamellas.webp" alt="Cylinder Vault">
    <span class="wood-card-title">Cylinder Vault</span>
  </a>
  <a class="wood-card" href="#curved_folded_beam">
    <img src="templates_cylindrical_folding_13_curved_folded_beam_all.webp" alt="Curved Folded Beam">
    <span class="wood-card-title">Curved Folded Beam</span>
  </a>
  <a class="wood-card" href="#corner_fan">
    <img src="templates_cylindrical_folding_14_corner_fan_strips.webp" alt="Corner Fan">
    <span class="wood-card-title">Corner Fan</span>
  </a>
  <a class="wood-card" href="#cylinder_carving">
    <img src="templates_cylindrical_folding_17_cylinder_carving_rings_all.webp" alt="Cylinder Carving">
    <span class="wood-card-title">Cylinder Carving</span>
  </a>
</div>

## Cylindrical Crease Invariants {#why_cylinders}

- Planar Crease Theorem: Two smooth developable patches meet along a planar curve when one is the reflection or mirror image of the other across the crease plane (rulings make equal angles with the plane).
- Exact Rational NURBS Solids: Projecting a circular NURBS arc along parallel rulings onto an oblique fold plane is an affine map that preserves rational weights, producing exact ellipse arcs and an exact 6-patch `PlateCurved` solid (`distortion = 0`).

---

## Reflection Fold {#reflection_fold}

| `Fold::reflected` | Ruling Update | Curvature / Bulge | Unfolded Development |
| --- | --- | --- | --- |
| `true` (Reflection) | Sheet beyond fold plane `R` is mirrored across `R` (`normal` parallel to `r_prev - r_next`) | Flips: vault ↔ trough | Single continuous flat sheet |
| `false` (Mirror fold) | Next segment is mirrored in the angle bisector (`normal` parallel to `r_prev + r_next`) | Preserved: vault ↔ vault | Separate mirror-image developments per segment |

```cpp
const Plane first = compute_profile_plane();
const std::vector<Segment> segments = compute_segments(first);
const std::vector<std::vector<NurbsCurve>> creases = compute_creases(segments);
const std::array<std::vector<NurbsCurve>, 2> profiles = {
    compute_inner_profile(first, 1.0),
    compute_inner_profile(first, -1.0)
};
const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, profiles);
```

| Step | Construction |
| --- | --- |
| `segments` | Fold planes from station + turn angle; optional `symmetry` plane at ridge/crown |
| `creases` | Affine projection of `profile` curves along segment rulings onto each fold plane |
| `inners` | Normal offsets (`±thickness`) projected along rulings to form shared inner miter curves |
| `lamellas` | `PlateCurved(outer_a, outer_b, inner_a, inner_b)` per segment and profile arc |
| `flat` / `contacts` | Unrolled 2D `Development` cutting patterns + `compute_face_contacts(0)` verification |

### Reflection Principle {#reflection_principle}

Circular arc strip (`120°`, `R = 1000 mm`, `t = 120 mm`) cut by an oblique plane `1500 mm` down and reflected by `120°` (`2` lamellas, `1` contact).

![](templates/templates_cylindrical_folding_1_reflection_lamellas.webp)

```cpp
ReflectionFold fold(
    {Primitives::arc(Point(500.0, -866.0, 3000.0), Point(1000.0, 0.0, 3000.0), Point(500.0, 866.0, 3000.0))},
    Vector(0.0, 0.0, -1.0),
    Vector(0.0, -1.0, 0.0),
    3500.0,
    {{1500.0, 60.0 * Tolerance::TO_RADIANS}}
);
```

### Curved Folded Plate Arch {#curved_folded_plate}

Six half-sine waves (`9 m` wide, `400 mm` amplitude) swept along a 5-segment polygonal arch with four `-30°` reverse folds (`13.5 m` span, `4.9 m` rise, `30` lamellas, `49` contacts).

![](templates/templates_cylindrical_folding_2_curved_folded_plate_lamellas.webp)

```cpp
const double climb = 60.0 * Tolerance::TO_RADIANS;
const Vector along(0.0, std::cos(climb), std::sin(climb));
const Vector up(0.0, -std::sin(climb), std::cos(climb));
ReflectionFold fold(
    ReflectionFold::waves(Point(-4500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0), up, 6, 1500.0, 400.0),
    along, Vector(1.0, 0.0, 0.0), 18100.0,
    {{3620.0, -30.0 * Tolerance::TO_RADIANS}, {7240.0, -30.0 * Tolerance::TO_RADIANS},
     {10860.0, -30.0 * Tolerance::TO_RADIANS}, {14480.0, -30.0 * Tolerance::TO_RADIANS}}
);
```

### Mendrisio Pavilion {#mendrisio_pavilion}

IBOIS Mendrisio pavilion (`13 × 4.5 × 3.4 m`, `77 mm` curved CLT): shallow barrel reflected at the eave (`89.9°`) and ridge (`10°`) across the symmetry plane (`5` plates, `4` contacts).

![](templates/templates_cylindrical_folding_3_mendrisio_pavilion_lamellas.webp)

```cpp
const Vector along(0.1719, 0.0, 0.9851);
ReflectionFold fold(
    {Primitives::arc(Point(6000.0, -2250.0, 0.0), Point(6400.0, 0.0, 0.0), Point(6000.0, 2250.0, 0.0))},
    along, Vector(0.0, -1.0, 0.0), 12000.0,
    {{2923.0, 89.9 * Tolerance::TO_RADIANS, true}, {3623.0, 10.0 * Tolerance::TO_RADIANS, true}},
    Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
    77.0
);
```

### Linear Folded Cylindrical Hall Systems {#folded_cylinders}

Four-barrel rows (`4 × 6000 mm` width, `120 mm` thick) folded into halls and arches:

| System | Climb | Folds `(station, angle, reflected)` | Symmetry `x` | Barrel Rise | Lamellas / Contacts | Example |
| --- | --- | --- | --- | --- | --- | --- |
| Two-hinged frame | `90°` | `(6000, -76°, true)` | `20000` | `600 mm` | `16` / `24` | `templates_cylindrical_folding_4_two_hinged_frame` |
| Two-hinged arch | `105°` | `(11000, -50°, false)`, `(22000, -50°, false)` | `11000` | `2400 mm` | `24` / `38` | `templates_cylindrical_folding_5_two_hinged_arch` |
| Arch with top hinge | `90°` | `(300, -15°, true)`, `(18300, -40°, true)`, `(25300, -35°, false)` | `13000` | `1600 mm` | `28` / `45` | `templates_cylindrical_folding_6_arch_with_top_hinge` |
| Two-hinged A-frame | `110°` | `(5600, -60°, true)` | `12500` | `1400 mm` | `16` / `24` | `templates_cylindrical_folding_7_a_frame` |

#### Two-Hinged Frame {#two_hinged_frame}

![](templates/templates_cylindrical_folding_4_two_hinged_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{6000.0, -76.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(20000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

#### Two-Hinged Arch {#two_hinged_arch}

![](templates/templates_cylindrical_folding_5_two_hinged_arch_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 2400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{11000.0, -50.0 * Tolerance::TO_RADIANS, false}, {22000.0, -50.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(11000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

#### Arch with Top Hinge {#arch_with_top_hinge}

![](templates/templates_cylindrical_folding_6_arch_with_top_hinge_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1600.0), along, Vector(0.0, -1.0, 0.0), 30000.0, {{300.0, -15.0 * Tolerance::TO_RADIANS, true}, {18300.0, -40.0 * Tolerance::TO_RADIANS, true}, {25300.0, -35.0 * Tolerance::TO_RADIANS, false}}, Plane::from_point_normal(Point(13000.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

#### Two-Hinged A-Frame {#a_frame}

![](templates/templates_cylindrical_folding_7_a_frame_lamellas.webp)

```cpp
ReflectionFold fold(ReflectionFold::arcs(foot, Vector(0.0, -1.0, 0.0), up, 4, 6000.0, 1400.0), along, Vector(0.0, -1.0, 0.0), 40000.0, {{5600.0, -60.0 * Tolerance::TO_RADIANS, true}}, Plane::from_point_normal(Point(12500.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0)), 120.0);
```

---

## Cylinder Vault {#cylinder_vault}

`CylinderVault(sides, edge, radius, kind, wall, thickness)`: `sides` equal cylinder sectors over a regular polygon meeting on vertical corner-to-centre planes.

| `Kind` | Generatrices | Profile | Creases |
| --- | --- | --- | --- |
| `Groin` | Radial (side → centre) | Arch over polygon side, springing at corners | Half-ellipses from corners to crown |
| `Cloister` | Tangential (parallel to polygon side, on `wall`) | Arc from wall top to central peak | Quarter-ellipses from corners to peak |

### Groin Vault (4 Sides) {#groin_vault}

![](templates/templates_cylindrical_folding_8_groin_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Groin);
```

### Pavilion Cloister Vault on Walls (4 Sides) {#pavilion_vault}

![](templates/templates_cylindrical_folding_9_pavilion_vault_lamellas.webp)

```cpp
CylinderVault vault(4, 12000.0, 6000.0, CylinderVault::Kind::Cloister, 4000.0);
```

### Pentagon Vault (5 Sides) {#pentagon_vault}

![](templates/templates_cylindrical_folding_10_pentagon_vault_lamellas.webp)

```cpp
CylinderVault vault(5, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

### Hexagon Vault (6 Sides) {#hexagon_vault}

![](templates/templates_cylindrical_folding_11_hexagon_vault_lamellas.webp)

```cpp
CylinderVault vault(6, 8000.0, 4000.0, CylinderVault::Kind::Groin);
```

### Octagon Vault (8 Sides) {#octagon_vault}

![](templates/templates_cylindrical_folding_12_octagon_vault_lamellas.webp)

```cpp
CylinderVault vault(8, 6000.0, 3000.0, CylinderVault::Kind::Groin);
```

---

## Curved Folded Beam {#curved_folded_beam}

`CurvedFoldedBeam(span, width, rise, sag, thickness, webs, count)`: closed box girder of `4` cylindrical `PlateCurved` flanges and webs (`top` and `bottom` arched outward by `rise`, `sides` dished inward by `sag`) stiffened by `webs` planar cross-diaphragms (`54` plates, `144` contacts across `6` beams).

![](templates/templates_cylindrical_folding_13_curved_folded_beam_all.webp)

```cpp
CurvedFoldedBeam beams;
```

---

## Corner Fan {#corner_fan}

`CornerFan(size, height, strips, lift, drop, foot, thickness, corner)`: `strips` cylindrical `PlateCurved` bands radiating from one corner of a `size × size` square to its two far sides along 3-point parabolic creases (`lift` mid-chord rise, alternating `drop` height modulation).

### Six-Strip Corner Fan

![](templates/templates_cylindrical_folding_14_corner_fan_strips.webp)

![](templates/templates_cylindrical_folding_14_corner_fan_plan.webp)

```cpp
CornerFan fan(6000.0, 4000.0, 6, 1200.0, 800.0, 0.15, 40.0);
```

### Corner Fan Subdivisions (2, 4, 8, 12 Strips) {#corner_fan_subdivisions}

![](templates/templates_cylindrical_folding_15_corner_fan_subdivisions_all.webp)

```cpp
#include "wood_session.h"
#include "src/templates/cylindrical_folding/corner_fan.h"

using namespace session_cpp;
using namespace wood_session;

/// The corner fan divided into 2, 4, 8 and 12 strips, side by side, the same square, heights and lift.
int main() {

    WoodSession fans("corner_fan_subdivisions");
    const std::array<size_t, 4> counts = {2, 4, 8, 12};

    for (size_t i = 0; i < counts.size(); i++) {
        const Point corner(9000.0 * static_cast<double>(i), 0.0, 0.0);
        CornerFan fan(
            6000.0,
            4000.0,
            counts[i],
            1200.0,
            800.0,
            0.15,
            40.0,
            corner,
            fmt::format("corner_fan_{}", counts[i])
        );
        std::cout << fmt::format("corner fan of {} strips: {} contacts, development distortion {:.2e}\n", counts[i], fan.get_contacts().size(), fan.distortion());
        fans.graft(fan, fans.add_group(fmt::format("fan_{}", counts[i])));
    }

    fans.pb_dump(pb_path("live"));
    return 0;
}
```

---

## Cylinder Carving {#cylinder_carving}

`CylinderCarving(stock, carvers, length, thickness, min_angle, closed)`: sequential boolean subtraction `((a - b) - c) - d` of cylindrical `carvers` from a `stock` cylinder, converting each concave carved patch into a `PlateCurved` shell of `thickness`.

### Sequential Three-Carver Column

![](templates/templates_cylindrical_folding_16_cylinder_carving_all.webp)

Progressive carving stages `a - b`, `(a - b) - c`, and `((a - b) - c) - d` on a `R = 2000 mm`, `12 m` column.

```cpp
#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_carving.h"

using namespace session_cpp;
using namespace wood_session;

/// A cylinder carved by tilted cylinders in sequence: a - b, then - c, then - d, side by side, only the carved patches.
int main() {

    WoodSession stages("cylinder_carving");
    const std::array<double, 3> radii = {1500.0, 1800.0, 1300.0};
    const std::array<double, 3> tilts = {0.06, -0.05, 0.04};

    for (size_t count = 1; count <= 3; count++) {
        // every stage moved along x; the stock a standing on z, every carver 40 degrees apart round one side, cutting 700 mm deep, turned off its axis
        const Vector shift(9000.0 * static_cast<double>(count - 1), 0.0, 0.0);
        const CylinderCarving::Cylinder stock = {Point(0.0, 0.0, 0.0) + shift, Vector(0.0, 0.0, 1.0), 2000.0};
        std::vector<CylinderCarving::Cylinder> carvers;

        for (size_t i = 0; i < count; i++) {
            const double around = (130.0 - 40.0 * static_cast<double>(i)) * Tolerance::TO_RADIANS;
            const Vector out(std::cos(around), std::sin(around), 0.0);
            const Vector across(-std::sin(around), std::cos(around), 0.0);
            const Point axis = Point(0.0, 0.0, 6000.0) + shift + out * (stock.radius + radii[i] - 700.0);
            carvers.push_back({axis, Vector(0.0, 0.0, 1.0) + across * tilts[i], radii[i]});
        }

        CylinderCarving carving(
            stock,
            carvers,
            12000.0,
            60.0,
            10.0 * Tolerance::TO_RADIANS,
            false,
            fmt::format("carving_{}", count)
        );
        const size_t plates = carving.get_elements_placed<PlateCurved>("plate").size();
        std::cout << fmt::format("{} carvers: {} plates, {} contacts, development distortion {:.2e}\n", count, plates, carving.get_contacts().size(), carving.distortion());
        stages.graft(carving, stages.add_group(fmt::format("stage_{}", count)));
    }

    stages.pb_dump(pb_path("live"));
    return 0;
}
```

### Closed Fluted Rings (4, 6, 8 Carvers) {#cylinder_carving_rings}

![](templates/templates_cylindrical_folding_17_cylinder_carving_rings_all.webp)

Closed fluted columns (`closed = true`) carved all round by `4`, `6`, and `8` alternating-tilt cylinders.

```cpp
#include "wood_session.h"
#include "src/templates/cylindrical_folding/cylinder_carving.h"

using namespace session_cpp;
using namespace wood_session;

/// Closed rings of carved patches: a cylinder carved all round by 4, 6 and 8 cylinders, their axes tilted in turn, side by side.
int main() {

    WoodSession rings("cylinder_carving_rings");
    const std::array<size_t, 3> counts = {4, 6, 8};
    const std::array<double, 3> radii = {2000.0, 1600.0, 1200.0};
    const std::array<double, 3> depths = {1600.0, 1000.0, 800.0};

    for (size_t r = 0; r < counts.size(); r++) {
        // the stock standing on z, every carver at an equal step round it, cutting depth deep, turned off its axis one way and the other in turn
        const Vector shift(9000.0 * static_cast<double>(r), 0.0, 0.0);
        const CylinderCarving::Cylinder stock = {Point(0.0, 0.0, 0.0) + shift, Vector(0.0, 0.0, 1.0), 2000.0};
        std::vector<CylinderCarving::Cylinder> carvers;

        for (size_t i = 0; i < counts[r]; i++) {
            const double around = 2.0 * Tolerance::PI * static_cast<double>(i) / static_cast<double>(counts[r]);
            const Vector out(std::cos(around), std::sin(around), 0.0);
            const Vector across(-std::sin(around), std::cos(around), 0.0);
            const double tilt = i % 2 == 0 ? 0.05 : -0.05;
            const Point axis = Point(0.0, 0.0, 6000.0) + shift + out * (stock.radius + radii[r] - depths[r]);
            carvers.push_back({axis, Vector(0.0, 0.0, 1.0) + across * tilt, radii[r]});
        }

        CylinderCarving carving(
            stock,
            carvers,
            12000.0,
            60.0,
            10.0 * Tolerance::TO_RADIANS,
            true,
            fmt::format("ring_{}", counts[r])
        );
        const size_t plates = carving.get_elements_placed<PlateCurved>("plate").size();
        std::cout << fmt::format("ring of {}: {} plates, {} contacts, development distortion {:.2e}\n", counts[r], plates, carving.get_contacts().size(), carving.distortion());
        rings.graft(carving, rings.add_group(fmt::format("ring_{}", counts[r])));
    }

    rings.pb_dump(pb_path("live"));
    return 0;
}
```
