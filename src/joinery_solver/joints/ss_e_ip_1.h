/// ss_e_ip_1: parametric in-plane zigzag along z at y=-0.5, the other face offset by (0,1,0); symmetric male/female.
static void ss_e_ip_1(WoodJoint& joint) {

    joint.name = "ss_e_ip_1";

    int div = std::max(2, std::min(100, joint.divisions));
    div += div % 2;

    const std::vector<Point> pts0 = Point::interpolate(Point(0, -0.5, 0.5), Point(0, -0.5, -0.5), div, 0);
    const Vector v(0.5, 0, 0);
    const double shift_ = Intersection::remap(joint.shift, 0, 1.0, -0.5, 0.5);
    const Vector v_d(0, 0, -(1.0 / ((div + 1) * 2)) * shift_);

    std::vector<Point> pline0;
    pline0.reserve(pts0.size() * 2);
    pline0.push_back(pts0[0]);
    pline0.push_back(pts0[0] - v - v_d);
    for (int i = 1; i + 1 < (int)pts0.size(); i++) {
        if (i % 2 == 1) {
            pline0.push_back(pts0[i] - v + v_d);
            pline0.push_back(pts0[i] + v - v_d);
        } else {
            pline0.push_back(pts0[i] + v + v_d);
            pline0.push_back(pts0[i] - v - v_d);
        }
    }
    const Point last = pts0.back();
    pline0.push_back(last - v + v_d);
    pline0.push_back(last);

    const Vector v_o(0, 1, 0);
    std::vector<Point> pline1;
    pline1.reserve(pline0.size());
    for (const Point& p : pline0)
        pline1.push_back(p + v_o);

    const Polyline outline0(pline0);
    const Polyline outline1(pline1);
    const Polyline endpoints0({pline0.front(), pline0.back()});
    const Polyline endpoints1({pline1.front(), pline1.back()});

    joint.female_outlines[0] = { outline0, endpoints0 };
    joint.female_outlines[1] = { outline1, endpoints1 };

    joint.male_outlines[0] = { outline0, endpoints0 };
    joint.male_outlines[1] = { outline1, endpoints1 };

    joint.female_cut_types[0] = { CutType::edge_insertion, CutType::edge_insertion };
    joint.female_cut_types[1] = { CutType::edge_insertion, CutType::edge_insertion };
    joint.male_cut_types[0] = { CutType::edge_insertion, CutType::edge_insertion };
    joint.male_cut_types[1] = { CutType::edge_insertion, CutType::edge_insertion };
}
