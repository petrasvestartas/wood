#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// SizingSection
// ═══════════════════════════════════════════════════════════════════════════

/// What a beam, or a strip of a panel, resists with: N and mm.
struct SizingSection {
    double width = 0.0; // Width of the beam, or of the strip.
    double depth = 0.0;
    double stiffness = 0.0; // E I, N mm2.
    double shear = 0.0; // G A over the shear factor, N; 0 leaves the shear deflection out.
    double moment = 0.0; // Moment resistance, N mm; 0 when it is not checked.
    double force = 0.0; // Shear resistance, N; 0 when it is not checked.
    double bearing = 0.0; // Resistance of a support, N; 0 when it is not checked.
    double weight = 0.0; // Own weight, N/mm.
};

/// One layer of a glued section: its width, its depth and its modulus along the span, counted from below.
struct SizingLayer {
    double width = 0.0;
    double depth = 0.0;
    double elastic = 0.0;
    double density = 0.0; // kN/m3.
    double real = 0.0; // Width its weight counts with, 0 for the width.
};

/// A Kielsteg type, its stiffness read back from the producer's spans.
struct KielstegType {
    double depth = 0.0;
    double chord_depth = 0.0;
    double chord_width = 0.0;
    double stiffness = 0.0; // MN m2 per metre of width.
    double weight = 0.0; // kg/m2.

    /// The sixteen types, the shallow first.
    static const std::vector<KielstegType>& all();
};

// ═══════════════════════════════════════════════════════════════════════════
// SizingRule
// ═══════════════════════════════════════════════════════════════════════════

/// How a producer or a book turns a load into a span: its material, its factors and its limits, each 0 when it is not used.
struct SizingRule {
    std::string name;

    double bending = 0.0; // Characteristic bending strength, N/mm2.
    double shearing = 0.0; // Characteristic shear strength.
    double pressing = 0.0; // Characteristic strength across the grain at a support.
    double elastic = 0.0; // Mean modulus of elasticity.
    double sliding = 0.0; // Mean shear modulus.
    double density = 0.0; // Own weight, kN/m3.
    double reference = 300.0; // Depth the bending strength is declared at.
    double exponent = 0.0; // Size effect of bending: (reference / depth) to this.
    double exponent_shear = 0.0; // Size effect of shear.
    double cap = 0.0; // Largest size factor, 0 for none.

    double modification = 0.8; // kmod.
    double partial = 1.2; // gamma M.
    double factor_permanent = 1.35; // gamma G.
    double factor_imposed = 1.5; // gamma Q.
    double creep = 0.0; // kdef.
    double creep_shear = 0.0; // kdef of the shear deflection, 0 for the same.
    double quasi = 0.0; // psi 2.

    double support = 0.0; // Length of a support, the spread along the grain counted.
    double deflection_start = 0.0; // Span over the deflection under the whole load, 300 or 400.
    double deflection_end = 0.0; // Span over the deflection with creep.
    double deflection_net = 0.0; // Span over the lasting deflection of the quasi-permanent load.
    double frequency = 0.0; // Least first frequency, Hz.
    double mass_permanent = 1.0; // Share of the permanent load in the swinging mass.
    double mass_imposed = 0.0; // Share of the imposed load in it.
    double mass_added = 0.0; // kg/m2 added to it.
    double gravity = 9.81; // m/s2, 10 where a kN is taken as 100 kg.
    double point = 0.0; // Largest deflection under 1 kN in the middle, mm.
    double cross = 0.0; // Stiffness across the beams sharing that 1 kN, N mm2/mm, 0 for one beam alone.

    double bending_factor = 8.0; // M = q l2 / this.
    double shear_factor = 0.5; // V = this q l.
    double deflection_factor = 5.0 / 384.0; // w = this q l4 / EI.
    double step = 0.0; // The deflection's span is cut down to this, mm.

    /// A rectangle width x depth, its resistances by this rule.
    SizingSection rectangle(double width, double depth) const;

    /// Layers glued on each other: one stiffness about their common axis, no resistances.
    SizingSection stacked(const std::vector<SizingLayer>& layers) const;

    /// Ribs of one spacing under a glued panel, a strip or a panel under them when it has a width, as they work on a span.
    SizingSection
    ribs(const SizingSection& rib, double spacing, const SizingLayer& top, const SizingLayer& bottom, double span) const;

    /// The longest span of ribs and panels, whose working widths grow with it.
    double
    span_ribs(const SizingSection& rib, double spacing, const SizingLayer& top, const SizingLayer& bottom, double g, double q) const;

    /// The longest single span of a section every spacing under g and q, kN/m2, in mm.
    double span(const SizingSection& section, double spacing, double g, double q) const;

    /// What limits that span: deflection, end deflection, bending, shear, bearing, frequency or point.
    std::string governing(const SizingSection& section, double spacing, double g, double q) const;

    /// The least of the sections, lightest first, that carries g and q over a span; throws when none does.
    SizingSection pick(const std::vector<SizingSection>& sections, double span, double spacing, double g, double q) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Producers
    // ═══════════════════════════════════════════════════════════════════════════

    /// Pollmeier BauBuche GL75 on spans 1 or 2, deflection up to span / limit: the dimensioning tables of brochure 04.
    static SizingRule baubuche_beam(size_t spans = 1, double limit = 300.0);

    /// Pollmeier BauBuche S on edge, the same tables.
    static SizingRule baubuche_board(size_t spans = 1, double limit = 300.0);

    /// Pollmeier BauBuche GL75 joists of a floor between flats: 8 Hz under the permanent load alone.
    static SizingRule baubuche_joist();

    /// Metsa Kerto-S main beams, a fifth of the load permanent: the Finnish charts of the Kerto manual.
    static SizingRule kerto_beam();

    /// Metsa Kerto-Q panels spanning along their face veneers: the tables of the Kerto manual.
    static SizingRule kerto_panel(double thickness = 27.0);

    /// LVL 48 P joists of homes, no bracing and the deck unglued: the charts of the LVL Handbook Europe.
    static SizingRule lvl_joist();

    /// Ribs glued to LVL panels: the rib and open box charts of the handbook.
    static SizingRule kerto_ripa();

    /// VMG Lignum beams of grade 48, 35, 32 (P) or 36 (C), a metre of floor each: the tables of the construction guide.
    static SizingRule vmg_beam(size_t grade = 48);

    /// VMG Lignum joists: end deflection span / 250 of flanges and web, 8 Hz with the deck glued on.
    static SizingRule i_joist();

    /// Flanges width x 39 and a web 10 of particle board; the deck 22 over a spacing works with it when it is given.
    static SizingSection i_section(double width, double depth, double spacing = 0.0);

    /// Lignatur: C24, whole and rigid, span / 475 under the whole load, 600 for a light floor: the producer's calculator.
    static SizingRule lignatur(double limit = 475.0);

    /// Lignatur surface element LFE, a metre of it: skins 31 or 40, five webs 31.
    SizingSection lignatur_section(double depth) const;

    /// Best wood Schneider CLT BOX, closed: span / 300 of the lasting deflection, the catalogue's tables.
    static SizingRule clt_box();

    /// The CLT BOX 1200 wide, three ribs: flanges 20 20 20 whose cross layers slip, on a span.
    SizingSection clt_box_section(double depth, double rib, double span) const;

    /// Novatop Element: span / 300 under the whole load, bending and the ribs' shear.
    static SizingRule novatop();

    /// Novatop Element, a metre of it.
    SizingSection novatop_section(double depth) const;

    /// Kielsteg floors: 6 or 8 Hz with three fifths of the imposed load in the mass.
    static SizingRule kielsteg(double hertz = 8.0);

    /// A metre of a Kielsteg type.
    static SizingSection kielsteg_section(const KielstegType& type);

    /// Hasslacher ribs under cross-laminated 120: 8 Hz under the permanent load.
    static SizingRule clt_rib();

    /// A rib every 625 with the cross-laminated plate on it.
    SizingSection clt_rib_section(double width, double depth) const;

    /// Joists C24 of the book's tables: span / 500 with creep under their own weight too.
    static SizingRule book_joist(size_t use = 0);

private:
    /// Every check's longest span and its name, mm.
    std::vector<std::pair<double, std::string>> spans(const SizingSection& section, double spacing, double g, double q) const;
};

/// The sections a producer keeps, width x depth, the shallow and narrow first.
struct SizingStock {
    std::vector<double> widths;
    std::vector<double> depths;

    /// Every section by rule, the lightest first.
    std::vector<SizingSection> sections(const SizingRule& rule) const;

    /// BauBuche GL75: widths 100 to 300, depths 80 to 1360 by 40.
    static SizingStock baubuche();

    /// Kerto-S: 45, 51, 57, 63, 75 x 200 to 600.
    static SizingStock kerto();

    /// VMG Lignum: 36, 39, 45, 63, 75 x 90 to 400.
    static SizingStock vmg();

    /// Finger-jointed timber: 60 to 140 x 100 to 240 by 20.
    static SizingStock solid();

    /// Glulam: 80 to 240 by 20 x 120 to 600 by 40.
    static SizingStock glulam();
};

}
