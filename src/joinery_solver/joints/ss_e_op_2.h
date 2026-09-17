/// ss_e_op_2: like ss_e_op_1 with a non-uniform shift - the central pairs move 4v, the outer 2v, sign flipped past the middle.
static void ss_e_op_2(WoodJoint& joint) {

    joint.name = "ss_e_op_2";

    int div = std::max(4, std::min(20, joint.divisions));
    div += div % 2;

    std::vector<Point> arr0 = Point::interpolate(Point( 0.5,-0.5,-0.5), Point( 0.5,-0.5, 0.5), div, 0);
    std::vector<Point> arr1 = Point::interpolate(Point(-0.5,-0.5,-0.5), Point(-0.5,-0.5, 0.5), div, 0);
    std::vector<Point> arr2 = Point::interpolate(Point(-0.5, 0.5,-0.5), Point(-0.5, 0.5, 0.5), div, 0);
    std::vector<Point> arr3 = Point::interpolate(Point( 0.5, 0.5,-0.5), Point( 0.5, 0.5, 0.5), div, 0);
    std::vector<Point>* arrays[4] = {&arr0, &arr1, &arr2, &arr3};

    const double vz = (joint.shift == 0)
        ? 0.0
        : Intersection::remap(joint.shift, 0, 1.0, -0.5, 0.5) / (div + 1);
    const Vector v(0, 0, vz);
    for (int i = 0; i < 4; i++) {
        std::vector<Point>& a = *arrays[i];
        const int mid = (int)(a.size() * 0.5);
        for (int j = 0; j < (int)a.size(); j++) {
            const int flip = (j < mid) ? 1 : -1;
            if (i == 1) {
                if (j < mid - 1 || j > mid)
                    a[j] = a[j] - 4 * v * flip;
            } else if (i == 0 || i == 2) {
                if (j < mid - 1 || j > mid)
                    a[j] = a[j] - 2 * v * flip;
            }
        }
    }

    for (int i = 0; i < 4; i += 2) {
        std::vector<Point> pts;
        pts.reserve(arr0.size() * 2);
        const std::vector<Point>& aA = *arrays[i];
        const std::vector<Point>& aB = *arrays[i + 1];
        for (int j = 0; j < (int)aA.size(); j++) {
            bool flip = (j % 2 == 0);
            if (i >= 2)
                flip = !flip;
            pts.push_back(flip ? aA[j] : aB[j]);
            pts.push_back(flip ? aB[j] : aA[j]);
        }

        const Polyline outline(pts);
        const Polyline endpoints({pts.front(), pts.back()});
        const int idx = (i < 2) ? 1 : 0;
        joint.male_outlines[idx] = {outline, endpoints};
    }

    for (int i = 1; i < 4; i += 2) {
        std::vector<Point> pts;
        pts.reserve(arr0.size() * 2);
        const std::vector<Point>& aA = *arrays[i];
        const std::vector<Point>& aB = *arrays[(i + 1) % 4];
        for (int j = 0; j < (int)aA.size(); j++) {
            bool flip = (j % 2 == 0);
            if (i >= 2)
                flip = !flip;
            pts.push_back(flip ? aA[j] : aB[j]);
            pts.push_back(flip ? aB[j] : aA[j]);
        }

        const Polyline outline(pts);
        const Polyline endpoints({pts.front(), pts.back()});
        const int idx = (i < 2) ? 0 : 1;
        joint.female_outlines[idx] = {outline, endpoints};
    }

    joint.female_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.female_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
