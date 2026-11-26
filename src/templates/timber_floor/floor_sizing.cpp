#include "src/templates/timber_floor/floor_sizing.h"

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Section
// ═══════════════════════════════════════════════════════════════════════════
SizingSection SizingRule::rectangle(double width, double depth) const {

    SizingSection section;
    section.width = width;
    section.depth = depth;
    section.stiffness = elastic * width * std::pow(depth, 3.0) / 12.0;
    section.shear = sliding * width * depth / 1.2;
    section.weight = density * 1e-6 * width * depth;

    const double design = modification / partial;
    double size = exponent > 0.0 ? std::pow(reference / depth, exponent) : 1.0;

    if (cap > 0.0)
        size = std::min(size, cap);

    section.moment = bending * size * design * width * depth * depth / 6.0;
    section.force = shearing * std::pow(reference / depth, exponent_shear) * design * width * depth / 1.5;
    section.bearing = pressing * design * width * support;

    return section;
}

SizingSection SizingRule::stacked(const std::vector<SizingLayer>& layers) const {

    SizingSection section;
    double area = 0.0;
    double turned = 0.0;

    for (const SizingLayer& layer : layers) {
        const double stretch = layer.elastic * layer.width * layer.depth;
        area += stretch;
        turned += stretch * (section.depth + 0.5 * layer.depth);
        section.depth += layer.depth;
        section.width = std::max(section.width, layer.width);
        section.weight += layer.density * 1e-6 * layer.depth * (layer.real > 0.0 ? layer.real : layer.width);
    }

    const double axis = turned / area;
    double low = 0.0;

    for (const SizingLayer& layer : layers) {
        const double arm = low + 0.5 * layer.depth - axis;
        section.stiffness += layer.elastic * layer.width * layer.depth * (layer.depth * layer.depth / 12.0 + arm * arm);
        low += layer.depth;
    }

    return section;
}

SizingSection
SizingRule::ribs(const SizingSection& rib, double spacing, const SizingLayer& top, const SizingLayer& bottom, double span) const {

    // a panel works with its rib over the rib's width and a tenth of the span
    const double works = rib.width + 0.1 * span;
    std::vector<SizingLayer> layers;

    if (bottom.width > 0.0)
        layers.push_back({std::min(bottom.width, works), bottom.depth, bottom.elastic, bottom.density, bottom.width});

    layers.push_back({rib.width, rib.depth, elastic, density, rib.width});
    layers.push_back({std::min(spacing, works), top.depth, top.elastic, top.density, spacing});
    SizingSection section = stacked(layers);
    section.shear = rib.shear;

    return section;
}

// ═══════════════════════════════════════════════════════════════════════════
// Span
// ═══════════════════════════════════════════════════════════════════════════
std::vector<std::pair<double, std::string>>
SizingRule::spans(const SizingSection& section, double spacing, double g, double q) const {

    // loads in N/mm on one beam, its own weight with the permanent one
    const double permanent = g * 1e-3 * spacing + section.weight;
    const double imposed = q * 1e-3 * spacing;
    const double whole = permanent + imposed;
    const double design = factor_permanent * permanent + factor_imposed * imposed;
    const double sheared = creep_shear > 0.0 ? creep_shear : creep;
    const double lasting = permanent * (1.0 + creep) + imposed * (1.0 + quasi * creep);
    const double lasting_shear = permanent * (1.0 + sheared) + imposed * (1.0 + quasi * sheared);
    std::vector<std::pair<double, std::string>> found;

    // the longest span that holds, by halving
    const auto halved = [](const auto& holds) {
        double low = 0.0;
        double high = 1e5;

        for (size_t k = 0; k < 80; k++) {
            const double span = 0.5 * (low + high);
            (holds(span) ? low : high) = span;
        }

        return low;
    };

    // deflection: bending and shear together
    const auto deflected = [&](double load, double load_shear, double limit) {
        const double longest = halved([&](double span) {
            double deflection = deflection_factor * load * std::pow(span, 4.0) / section.stiffness;

            if (section.shear > 0.0)
                deflection += load_shear * span * span / (8.0 * section.shear);

            return deflection * limit <= span;
        });

        return step > 0.0 ? std::floor(longest / step + 1e-9) * step : longest;
    };

    if (deflection_start > 0.0)
        found.push_back({deflected(whole, whole, deflection_start), "deflection"});

    if (deflection_end > 0.0)
        found.push_back({deflected(lasting, lasting_shear, deflection_end), "end deflection"});

    if (deflection_net > 0.0) {
        const double net = (permanent + quasi * imposed) * (1.0 + creep);
        const double net_shear = (permanent + quasi * imposed) * (1.0 + sheared);
        found.push_back({deflected(net, net_shear, deflection_net), "net deflection"});
    }

    if (section.moment > 0.0)
        found.push_back({std::sqrt(bending_factor * section.moment / design), "bending"});

    if (section.force > 0.0)
        found.push_back({section.force / (shear_factor * design), "shear"});

    if (section.bearing > 0.0)
        found.push_back({section.bearing / (0.5 * design), "bearing"});

    // frequency: pi / 2 over the span squared, the root of stiffness over mass
    if (frequency > 0.0) {
        const double moved = mass_permanent * permanent + mass_imposed * imposed;
        const double mass = moved / (1e3 * gravity) + mass_added * 1e-9 * spacing;
        const double swing = section.stiffness / mass;
        found.push_back({std::sqrt(0.5 * Tolerance::PI * std::sqrt(swing) / frequency), "frequency"});
    }

    if (point > 0.0 && cross == 0.0)
        found.push_back({std::cbrt(48.0 * section.stiffness * point / 1000.0), "point"});

    // 1 kN shared across the beams: the limit grows for spans under 6 m
    if (point > 0.0 && cross > 0.0) {
        const double along = section.stiffness / spacing;
        const double shared = std::sqrt(std::sqrt(cross / along));
        const double longest = halved([&](double span) {
            const double alone = 1000.0 * std::pow(span, 3.0) / (48.0 * section.stiffness);
            const double plate = 1000.0 * span * span / (42.0 * shared * along);
            const double grown = std::max(1.0, 1.0 / (0.318 + 0.114e-3 * span));

            return std::min(alone, plate) <= point * grown;
        });
        found.push_back({longest, "point"});
    }

    return found;
}

double SizingRule::span(const SizingSection& section, double spacing, double g, double q) const {

    double least = 1e9;

    for (const std::pair<double, std::string>& check : spans(section, spacing, g, q))
        least = std::min(least, check.first);

    return least;
}

double SizingRule::span_ribs(
    const SizingSection& rib, double spacing, const SizingLayer& top, const SizingLayer& bottom, double g, double q) const {

    double length = 1000.0;

    for (size_t k = 0; k < 40; k++)
        length = span(ribs(rib, spacing, top, bottom, length), spacing, g, q);

    return length;
}

std::string SizingRule::governing(const SizingSection& section, double spacing, double g, double q) const {

    std::pair<double, std::string> least = {1e9, ""};

    for (const std::pair<double, std::string>& check : spans(section, spacing, g, q))
        if (check.first < least.first)
            least = check;

    return least.second;
}

SizingSection SizingRule::pick(const std::vector<SizingSection>& sections, double length, double spacing, double g, double q) const {

    for (const SizingSection& section : sections)
        if (span(section, spacing, g, q) >= length)
            return section;

    throw std::invalid_argument("no section in stock spans that far");
}

// ═══════════════════════════════════════════════════════════════════════════
// Producers
// ═══════════════════════════════════════════════════════════════════════════
SizingRule SizingRule::baubuche_beam(size_t spans, double limit) {

    SizingRule rule;
    rule.name = "BauBuche GL75";
    rule.bending = 75.0;
    rule.shearing = 4.5;
    rule.elastic = 16800.0;
    rule.reference = 600.0;
    rule.exponent = 0.10;
    rule.exponent_shear = 0.13;
    rule.partial = 1.3;
    rule.factor_permanent = 1.4;
    rule.factor_imposed = 1.4;
    rule.deflection_start = limit;
    rule.deflection_factor = spans == 1 ? 5.0 / 384.0 : 2.08 / 384.0;
    rule.shear_factor = spans == 1 ? 0.5 : 0.625;

    return rule;
}

SizingRule SizingRule::baubuche_board(size_t spans, double limit) {

    SizingRule rule = baubuche_beam(spans, limit);
    rule.name = "BauBuche S";
    rule.bending = 80.0;
    rule.shearing = 8.0;
    rule.reference = 300.0;
    rule.exponent = 0.12;
    rule.exponent_shear = 0.0;
    rule.cap = 1.0;

    return rule;
}

SizingRule SizingRule::baubuche_joist() {

    SizingRule rule;
    rule.name = "BauBuche GL75 joists";
    rule.elastic = 16800.0;
    rule.density = 8.5;
    rule.frequency = 8.0;

    return rule;
}

SizingRule SizingRule::kerto_beam() {

    SizingRule rule;
    rule.name = "Kerto-S";
    rule.bending = 44.0;
    rule.shearing = 4.1;
    rule.elastic = 13800.0;
    rule.sliding = 600.0;
    rule.density = 5.1;
    rule.exponent = 0.12;
    rule.cap = 1.2;
    rule.factor_permanent = 1.15;
    rule.creep = 0.8;
    rule.quasi = 0.3;
    rule.deflection_start = 400.0;
    rule.deflection_end = 300.0;

    return rule;
}

SizingRule SizingRule::kerto_panel(double thickness) {

    SizingRule rule;
    rule.name = "Kerto-Q";
    rule.elastic = thickness < 27.0 ? 10000.0 : 10500.0;
    rule.sliding = thickness < 27.0 ? 80.0 : 120.0;
    rule.density = 5.1;
    rule.creep = 0.8;
    rule.quasi = 0.3;
    rule.deflection_start = 400.0;
    rule.deflection_end = 300.0;

    return rule;
}

SizingRule SizingRule::lvl_joist() {

    SizingRule rule = kerto_beam();
    rule.name = "LVL 48 P joists";
    rule.shearing = 4.2;
    rule.cap = 1.1;
    rule.exponent = 0.15;
    rule.creep = 0.6;
    rule.frequency = 8.0;
    rule.mass_added = 30.0;
    rule.point = 0.5;
    rule.cross = 3.11e6;

    return rule;
}

SizingRule SizingRule::kerto_ripa() {

    SizingRule rule = lvl_joist();
    rule.name = "LVL ribs";
    rule.point = 0.0;
    rule.cross = 0.0;

    return rule;
}

SizingRule SizingRule::vmg_beam(size_t grade) {

    SizingRule rule;
    rule.name = fmt::format("VMG Lignum {}", grade);
    rule.elastic = grade == 48 ? 13800.0 : grade == 35 ? 12000.0 : grade == 32 ? 9600.0 : 10500.0;
    rule.sliding = grade == 48 || grade == 36 ? 600.0 : 500.0;
    rule.pressing = grade == 36 ? 9.0 : grade == 32 ? 4.0 : 6.0;
    rule.support = 60.0;
    rule.factor_permanent = 1.35 * 0.85;
    rule.creep = 0.6;
    rule.quasi = 0.3;
    rule.deflection_end = 250.0;
    rule.frequency = 8.0;
    rule.mass_added = 30.0;
    rule.gravity = 10.0;
    rule.point = 1.5;
    rule.step = 100.0;

    return rule;
}

SizingRule SizingRule::i_joist() {

    SizingRule rule;
    rule.name = "VMG Lignum joist";
    rule.creep = 0.6;
    rule.creep_shear = 2.25;
    rule.quasi = 0.3;
    rule.deflection_end = 250.0;
    rule.frequency = 8.0;
    rule.mass_imposed = 0.3;
    rule.gravity = 10.0;
    rule.step = 100.0;

    return rule;
}

SizingSection SizingRule::i_section(double width, double depth, double spacing) {

    // the producer's tables leave the joist's own weight out
    const double flange = 39.0;
    std::vector<SizingLayer> layers = {{width, flange, 13800.0}, {10.0, depth - 2.0 * flange, 2550.0}, {width, flange, 13800.0}};

    if (spacing > 0.0)
        layers.push_back({spacing, 22.0, 2500.0});

    SizingSection section = SizingRule().stacked(layers);
    section.width = width;
    section.depth = depth;
    section.shear = 960.0 * 10.0 * (depth - 2.0 * flange);

    return section;
}

SizingRule SizingRule::lignatur(double limit) {

    SizingRule rule;
    rule.name = "Lignatur";
    rule.elastic = 11000.0;
    rule.density = 4.7;
    rule.deflection_start = limit;

    return rule;
}

SizingSection SizingRule::lignatur_section(double depth) const {

    const double skin = depth > 320.0 ? 40.0 : 31.0;
    const double webs = 5.0 * 31.0;
    const SizingLayer flange = {1000.0, skin, elastic, density};

    return stacked({flange, {webs, depth - 2.0 * skin, elastic, density}, flange});
}

SizingRule SizingRule::clt_box() {

    SizingRule rule;
    rule.name = "CLT BOX";
    rule.elastic = 11550.0;
    rule.density = 5.0;
    rule.creep = 0.6;
    rule.quasi = 0.3;
    rule.partial = 1.3;
    rule.deflection_start = 300.0;
    rule.deflection_end = 200.0;
    rule.deflection_net = 300.0;

    return rule;
}

SizingSection SizingRule::clt_box_section(double depth, double rib, double span) const {

    const double width = 1200.0;
    const double layer = 20.0;
    const double rolling = 50.0;
    const double high = depth - 120.0;

    // the outer layers slip over their cross layers, the inner ones are one with the ribs
    const double stretch = elastic * width * layer;
    const double slip = 1.0 / (1.0 + std::pow(Tolerance::PI / span, 2.0) * stretch * layer / (width * rolling));
    const double outer = 0.5 * (depth - layer);
    const double inner = 0.5 * high + 0.5 * layer;
    SizingSection section;
    section.width = width;
    section.depth = depth;
    section.stiffness = 11500.0 * 3.0 * rib * std::pow(high, 3.0) / 12.0 + 4.0 * stretch * layer * layer / 12.0
        + 2.0 * stretch * (inner * inner + slip * outer * outer);
    section.weight = density * 1e-6 * (120.0 * width + 3.0 * rib * high);

    // shear: the middle rib with the flanges next to it, at its axis
    const double reach = 560.0 / width;
    const double middle = reach * (stretch * inner + slip * stretch * outer) + 11500.0 * rib * high * high / 8.0;
    const double stiff = reach * (section.stiffness - 11500.0 * 3.0 * rib * std::pow(high, 3.0) / 12.0)
        + 11500.0 * rib * std::pow(high, 3.0) / 12.0;
    section.force = 3.5 * 0.71 * modification / partial * rib * stiff / (middle * reach);

    return section;
}

SizingRule SizingRule::novatop() {

    SizingRule rule;
    rule.name = "Novatop Element";
    rule.elastic = 7800.0;
    rule.sliding = 600.0;
    rule.density = 4.9;
    rule.deflection_start = 300.0;

    return rule;
}

SizingSection SizingRule::novatop_section(double depth) const {

    const double webs = 1000.0 * 27.0 / 340.0;
    const double high = depth - 54.0;
    const SizingLayer flange = {1000.0, 27.0, elastic, density};
    SizingSection section = stacked({flange, {webs, high, 5300.0, density}, flange});
    section.shear = sliding * webs * high;

    return section;
}

SizingRule SizingRule::clt_rib() {

    SizingRule rule;
    rule.name = "CLT ribs";
    rule.elastic = 11500.0;
    rule.density = 5.0;
    rule.frequency = 8.0;

    return rule;
}

SizingSection SizingRule::clt_rib_section(double width, double depth) const {

    const double spacing = 625.0;

    return stacked({
        {width, depth, elastic, density},
        {spacing, 40.0, 11600.0, density},
        {spacing, 40.0, 0.0, density},
        {spacing, 40.0, 11600.0, density},
    });
}

SizingRule SizingRule::book_joist(size_t use) {

    SizingRule rule;
    rule.name = "C24 / GL24h";
    rule.elastic = 11000.0;
    rule.density = 4.5;
    rule.creep = 0.6;
    rule.quasi = use == 2 ? 0.6 : 0.3;
    rule.deflection_end = 500.0;

    return rule;
}

// ═══════════════════════════════════════════════════════════════════════════
// Stock
// ═══════════════════════════════════════════════════════════════════════════
std::vector<SizingSection> SizingStock::sections(const SizingRule& rule) const {

    std::vector<SizingSection> found;

    for (const double depth : depths)
        for (const double width : widths)
            found.push_back(rule.rectangle(width, depth));

    std::stable_sort(found.begin(), found.end(), [](const SizingSection& a, const SizingSection& b) {
        return a.width * a.depth < b.width * b.depth;
    });

    return found;
}

/// The numbers from first to last by step.
static std::vector<double> steps(double first, double last, double step) {

    std::vector<double> numbers;

    for (double number = first; number <= last + 1e-6; number += step)
        numbers.push_back(number);

    return numbers;
}

SizingStock SizingStock::baubuche() {
    return {{100.0, 120.0, 160.0, 200.0, 240.0, 280.0, 300.0}, steps(120.0, 1360.0, 40.0)};
}

SizingStock SizingStock::kerto() {
    return {{45.0, 51.0, 57.0, 63.0, 75.0}, {200.0, 220.0, 240.0, 260.0, 300.0, 360.0, 400.0, 450.0, 600.0}};
}

SizingStock SizingStock::vmg() {
    return {{36.0, 39.0, 45.0, 63.0, 75.0}, {90.0, 95.0, 120.0, 140.0, 150.0, 180.0, 190.0, 200.0, 240.0, 290.0, 300.0, 360.0, 400.0}};
}

SizingStock SizingStock::solid() {
    return {steps(60.0, 140.0, 20.0), steps(100.0, 240.0, 20.0)};
}

SizingStock SizingStock::glulam() {
    return {steps(80.0, 240.0, 20.0), steps(120.0, 600.0, 40.0)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Producers' floors
// ═══════════════════════════════════════════════════════════════════════════
TimberFloorGuide TimberFloorGuide::baubuche(double span, double g, double q) {

    const SizingRule rule = SizingRule::baubuche_joist();
    SizingStock stock = SizingStock::baubuche();
    stock.widths = {120.0};
    const SizingSection joist = rule.pick(stock.sections(rule), span, 850.0, g, q);
    TimberFloorGuide guide;
    guide.span = span;
    guide.rib_width = joist.width;
    guide.rib_depth = joist.depth;
    guide.rib_spacing = 850.0;
    guide.top = 40.0;

    return guide;
}

TimberFloorGuide TimberFloorGuide::kerto_ripa(double span, bool boxed, double g, double q) {

    const SizingRule rule = SizingRule::kerto_ripa();
    const double spacing = 612.0;
    const SizingLayer top = {spacing, 25.0, 10500.0, 5.1};
    const SizingLayer strip = boxed ? SizingLayer{300.0, 43.0, 13800.0, 5.1} : SizingLayer{};

    for (const double depth : SizingStock::kerto().depths) {
        const SizingSection rib = rule.rectangle(boxed ? 45.0 : 51.0, depth);

        if (rule.span_ribs(rib, spacing, top, strip, g, q) < span)
            continue;

        TimberFloorGuide guide;
        guide.span = span;
        guide.rib_width = rib.width;
        guide.rib_depth = depth;
        guide.rib_spacing = spacing;
        guide.top = top.depth;
        guide.bottom = strip.depth;
        guide.bottom_width = strip.width;

        return guide;
    }

    throw std::invalid_argument("no rib in stock spans that far");
}

TimberFloorGuide TimberFloorGuide::vmg_joists(double span, double spacing, double g, double q) {

    // two checks: the joist alone bends, the deck swings with it
    SizingRule bends = SizingRule::i_joist();
    SizingRule swings = bends;
    bends.frequency = 0.0;
    swings.deflection_end = 0.0;

    for (const double depth : {200.0, 220.0, 240.0, 250.0, 300.0, 360.0, 400.0})
        for (const double width : {45.0, 60.0, 90.0}) {
            if (bends.span(SizingRule::i_section(width, depth), spacing, g, q) < span
                || swings.span(SizingRule::i_section(width, depth, spacing), spacing, g, q) < span)
                continue;

            TimberFloorGuide guide;
            guide.span = span;
            guide.rib_width = 10.0;
            guide.rib_depth = depth;
            guide.rib_spacing = spacing;
            guide.flange_width = width;
            guide.flange_depth = 39.0;
            guide.top = 22.0;

            return guide;
        }

    throw std::invalid_argument("no joist in stock spans that far");
}

TimberFloorGuide TimberFloorGuide::lignatur(double span, double g, double q) {

    const SizingRule rule = SizingRule::lignatur();

    for (const double depth : {120.0, 140.0, 160.0, 180.0, 200.0, 220.0, 240.0, 280.0, 320.0, 360.0}) {
        if (rule.span(rule.lignatur_section(depth), 1000.0, g, q) < span)
            continue;

        TimberFloorGuide guide;
        guide.span = span;
        guide.width = 2000.0;
        guide.panel = depth;
        guide.panel_width = 1000.0;
        guide.skin = depth > 320.0 ? 40.0 : 31.0;
        guide.web_spacing = 240.0;

        return guide;
    }

    throw std::invalid_argument("no element spans that far");
}

TimberFloorGuide TimberFloorGuide::clt_box(double span, double g, double q) {

    const SizingRule rule = SizingRule::clt_box();

    for (double depth = 220.0; depth <= 480.0; depth += 20.0)
        for (const double rib : {80.0, 100.0, 120.0}) {
            if (rule.span(rule.clt_box_section(depth, rib, span), 1200.0, g, q) < span)
                continue;

            TimberFloorGuide guide;
            guide.span = span;
            guide.rib_width = rib;
            guide.rib_depth = depth - 120.0;
            guide.rib_spacing = 560.0;
            guide.top = 60.0;
            guide.bottom = 60.0;

            return guide;
        }

    throw std::invalid_argument("no element spans that far");
}

TimberFloorGuide TimberFloorGuide::novatop(double span, double g, double q) {

    const SizingRule rule = SizingRule::novatop();

    for (double depth = 160.0; depth <= 400.0; depth += 20.0) {
        if (rule.span(rule.novatop_section(depth), 1000.0, g, q) < span)
            continue;

        TimberFloorGuide guide;
        guide.span = span;
        guide.rib_width = 27.0;
        guide.rib_depth = depth - 54.0;
        guide.rib_spacing = 340.0;
        guide.top = 27.0;
        guide.bottom = 27.0;

        return guide;
    }

    throw std::invalid_argument("no element spans that far");
}

TimberFloorGuide TimberFloorGuide::clt_ribs(double span, double g, double q) {

    const SizingRule rule = SizingRule::clt_rib();

    for (double depth = 160.0; depth <= 640.0; depth += 40.0) {
        if (rule.span(rule.clt_rib_section(160.0, depth), 625.0, g, q) < span)
            continue;

        TimberFloorGuide guide;
        guide.span = span;
        guide.width = 2500.0;
        guide.rib_width = 160.0;
        guide.rib_depth = depth;
        guide.rib_spacing = 625.0;
        guide.top = 120.0;

        return guide;
    }

    throw std::invalid_argument("no rib spans that far");
}

}
