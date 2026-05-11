"""
Migration script for wood_main.cpp
"""
import re

fp = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_main.cpp'
with open(fp, encoding='utf-8') as f:
    content = f.read()
original = content

# Helper functions
def count_and_replace(content, old, new, label):
    n = content.count(old)
    result = content.replace(old, new)
    print(f'{label}: {n} replacements')
    return result

# ============================================================
# 0. Fix get_elements signature and planes init
# ============================================================
content = content.replace(
    'get_elements (std::vector<CGAL_Polyline> &pp, std::vector<std::vector<IK::Vector_3> > &insertion_vectors,',
    'get_elements (std::vector<CGAL_Polyline> &pp, std::vector<std::vector<session_cpp::Vector> > &insertion_vectors,'
)
content = content.replace(
    'get_connection_zones (std::vector<CGAL_Polyline> &input_polyline_pairs, std::vector<std::vector<IK::Vector_3> > &input_insertion_vectors,',
    'get_connection_zones (std::vector<CGAL_Polyline> &input_polyline_pairs, std::vector<std::vector<session_cpp::Vector> > &input_insertion_vectors,'
)
content = content.replace(
    'beam_volumes (std::vector<CGAL_Polyline> &polylines, std::vector<std::vector<double> > &polylines_segment_radii, std::vector<std::vector<IK::Vector_3> > &polylines_segment_direction,',
    'beam_volumes (std::vector<CGAL_Polyline> &polylines, std::vector<std::vector<double> > &polylines_segment_radii, std::vector<std::vector<session_cpp::Vector> > &polylines_segment_direction,'
)
content = content.replace(
    'std::vector<std::vector<double> > &distance, std::vector<std::vector<IK::Point_3> > &point_pairs,',
    'std::vector<std::vector<double> > &distance, std::vector<std::vector<session_cpp::Point> > &point_pairs,'
)
content = content.replace(
    'std::vector<std::vector<IK::Vector_3> > input_insertion_vectors;',
    'std::vector<std::vector<session_cpp::Vector> > input_insertion_vectors;'
)
content = content.replace(
    'elements[count].planes = std::vector<IK::Plane_3> (1 + pp[i].size ());',
    'elements[count].planes = std::vector<session_cpp::Plane> (1 + pp[i].size ());'
)
print('Fixed signatures')

# ============================================================
# 1. plane function signatures
# ============================================================
content = content.replace(
    'border_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<IK::Plane_3> &Plane0, std::vector<IK::Vector_3> &insertion_vectors0,',
    'border_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Vector> &insertion_vectors0,'
)
content = content.replace(
    'plane_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<CGAL_Polyline> &Polyline1, std::vector<IK::Plane_3> &Plane0, std::vector<IK::Plane_3> &Plane1,\n               std::vector<IK::Vector_3> &insertion_vectors0, std::vector<IK::Vector_3> &insertion_vectors1,',
    'plane_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<CGAL_Polyline> &Polyline1, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Plane> &Plane1,\n               std::vector<session_cpp::Vector> &insertion_vectors0, std::vector<session_cpp::Vector> &insertion_vectors1,'
)
content = content.replace(
    'face_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<CGAL_Polyline> &Polyline1, std::vector<IK::Plane_3> &Plane0, std::vector<IK::Plane_3> &Plane1,\n               std::vector<IK::Vector_3> &insertion_vectors0, std::vector<IK::Vector_3> &insertion_vectors1,',
    'face_to_face (const size_t &joint_id, std::vector<CGAL_Polyline> &Polyline0, std::vector<CGAL_Polyline> &Polyline1, std::vector<session_cpp::Plane> &Plane0, std::vector<session_cpp::Plane> &Plane1,\n               std::vector<session_cpp::Vector> &insertion_vectors0, std::vector<session_cpp::Vector> &insertion_vectors1,'
)
print('Fixed plane function signatures')

# ============================================================
# 2. Nested midpoints
# ============================================================
def replace_nested_midpoints(content):
    pattern = r'CGAL::midpoint\s*\(\s*CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)\s*,\s*CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)\s*\)'
    replacement = r'session_cpp::Point::mid_point(session_cpp::Point::mid_point(\1, \2), session_cpp::Point::mid_point(\3, \4))'
    new, n = re.subn(pattern, replacement, content)
    print(f'nested midpoints: {n}')
    return new
content = replace_nested_midpoints(content)

# ============================================================
# 3. Simple midpoints with to_cgal_pt
# ============================================================
def replace_simple_midpoints(content):
    pattern = r'CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point::mid_point(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'simple midpoints: {n}')
    return new
content = replace_simple_midpoints(content)

# ============================================================
# 4. Direct midpoints
# ============================================================
def replace_direct_midpoints(content):
    pattern = r'CGAL::midpoint\s*\(([^()]+),\s*([^()]+)\)'
    replacement = r'session_cpp::Point::mid_point(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'direct midpoints: {n}')
    return new
content = replace_direct_midpoints(content)

# ============================================================
# 5. squared_distance with to_cgal_pt
# ============================================================
def replace_sq_dist(content):
    pattern = r'CGAL::squared_distance\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'sq_dist with to_cgal_pt: {n}')
    return new
content = replace_sq_dist(content)

def replace_sq_dist2(content):
    pattern = r'CGAL::squared_distance\s*\(([^,()]+(?:\([^()]*\)[^,()]*)?),\s*([^()]+(?:\([^()]*\)[^()]*)?)\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'sq_dist direct: {n}')
    return new
content = replace_sq_dist2(content)

# ============================================================
# 6. cross_product with to_cgal_v
# ============================================================
def replace_cross_v(content):
    pattern = r'CGAL::cross_product\s*\(\s*to_cgal_v\s*\(([^)]+)\)\s*,\s*to_cgal_v\s*\(([^)]+)\)\s*\)'
    replacement = r'(\1).cross(\2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'cross_product with to_cgal_v: {n}')
    return new
content = replace_cross_v(content)

def replace_cross_direct(content):
    pattern = r'CGAL::cross_product\s*\(([^()]+),\s*([^()]+)\)'
    replacement = r'(\1).cross(\2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'cross_product direct: {n}')
    return new
content = replace_cross_direct(content)

# ============================================================
# 7. approximate_angle
# ============================================================
def replace_approx_angle(content):
    pattern = r'CGAL::approximate_angle\s*\(([^,()]+(?:\(\))?[^,()]*),\s*([^()]+(?:\(\))?[^()]*)\)'
    replacement = r'(\1).angle(\2, false, true)'
    new, n = re.subn(pattern, replacement, content)
    print(f'approximate_angle: {n}')
    return new
content = replace_approx_angle(content)

# ============================================================
# 8. has_smaller_distance_to_point
# ============================================================
def replace_has_smaller(content):
    pattern = r'CGAL::has_smaller_distance_to_point\s*\(([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^()]+(?:\([^()]*\))?[^()]*)\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2) < session_cpp::Point::squared_distance(\1, \3)'
    new, n = re.subn(pattern, replacement, content)
    print(f'has_smaller_distance: {n}')
    return new
content = replace_has_smaller(content)

# ============================================================
# 9. has_larger_distance_to_point
# ============================================================
def replace_has_larger(content):
    pattern = r'CGAL::has_larger_distance_to_point\s*\(([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^()]+(?:\([^()]*\))?[^()]*)\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2) > session_cpp::Point::squared_distance(\1, \3)'
    new, n = re.subn(pattern, replacement, content)
    print(f'has_larger_distance: {n}')
    return new
content = replace_has_larger(content)

# ============================================================
# 10. IK types -> session_cpp types
# ============================================================
content = count_and_replace(content, 'IK::Vector_3', 'session_cpp::Vector', 'IK::Vector_3')
content = count_and_replace(content, 'IK::Point_3', 'session_cpp::Point', 'IK::Point_3')
content = count_and_replace(content, 'IK::Plane_3', 'session_cpp::Plane', 'IK::Plane_3')
content = count_and_replace(content, 'IK::Segment_3', 'session_cpp::Line', 'IK::Segment_3')
content = count_and_replace(content, 'IK::Line_3', 'session_cpp::Line', 'IK::Line_3')
content = count_and_replace(content, 'CGAL::Aff_transformation_3<IK>', 'session_cpp::Xform', 'Aff_transformation')

# ============================================================
# 11. to_sc_pt(to_cgal_pt(x)) -> x
# ============================================================
def replace_roundtrip(content):
    pattern = r'to_sc_pt\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(to_cgal_pt(x)): {n}')
    return new
content = replace_roundtrip(content)

# ============================================================
# 12. to_cgal_pt(X) -> X
# ============================================================
def replace_to_cgal_pt(content):
    pattern = r'to_cgal_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_pt(x): {n}')
    return new
content = replace_to_cgal_pt(content)

# ============================================================
# 13. to_sc_pt(X) -> X
# ============================================================
def replace_to_sc_pt(content):
    pattern = r'to_sc_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(x): {n}')
    return new
content = replace_to_sc_pt(content)

# ============================================================
# 14. to_cgal_v(X) -> X, to_sc_v(X) -> X
# ============================================================
def replace_to_cgal_v(content):
    pattern = r'to_cgal_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_v(x): {n}')
    return new
content = replace_to_cgal_v(content)

def replace_to_sc_v(content):
    pattern = r'to_sc_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_v(x): {n}')
    return new
content = replace_to_sc_v(content)

# ============================================================
# 15. orthogonal_vector() -> z_axis()
# ============================================================
content = count_and_replace(content, '.orthogonal_vector ()', '.z_axis ()', 'orthogonal_vector')

# ============================================================
# 16. CGAL::Bbox_3 with cgal_bbox3_polyline - replace with session_cpp::BoundingBox
# ============================================================
content = content.replace(
    'CGAL::Bbox_3 AABB = cgal_bbox3_polyline (twoPolylines);',
    'session_cpp::BoundingBox AABB = session_cpp::BoundingBox::from_points (twoPolylines);'
)
content = content.replace(
    'CGAL::Bbox_3 AABBXY = cgal_bbox3_polyline (twoPolylines);',
    'session_cpp::BoundingBox AABBXY = session_cpp::BoundingBox::from_points (twoPolylines);'
)

# Fix AABB.xmin(), xmax(), etc.
for ax, idx in [('xmin', '[0][0]'), ('ymin', '[0][1]'), ('zmin', '[0][2]'),
                ('xmax', '[1][0]'), ('ymax', '[1][1]'), ('zmax', '[1][2]')]:
    content = content.replace(f'AABB.{ax} ()', f'AABB.min_point(){idx if "min" in ax else "".join(["[", ax[-1] if "max" in ax else ""])}')

# More precise AABB replacements
for old, new in [
    ('AABB.xmin ()', 'AABB.min_point()[0]'),
    ('AABB.ymin ()', 'AABB.min_point()[1]'),
    ('AABB.zmin ()', 'AABB.min_point()[2]'),
    ('AABB.xmax ()', 'AABB.max_point()[0]'),
    ('AABB.ymax ()', 'AABB.max_point()[1]'),
    ('AABB.zmax ()', 'AABB.max_point()[2]'),
]:
    content = count_and_replace(content, old, new, old)

# AABBXY
for old, new in [
    ('AABBXY.xmin ()', 'AABBXY.min_point()[0]'),
    ('AABBXY.ymin ()', 'AABBXY.min_point()[1]'),
    ('AABBXY.zmin ()', 'AABBXY.min_point()[2]'),
    ('AABBXY.xmax ()', 'AABBXY.max_point()[0]'),
    ('AABBXY.ymax ()', 'AABBXY.max_point()[1]'),
    ('AABBXY.zmax ()', 'AABBXY.max_point()[2]'),
]:
    content = count_and_replace(content, old, new, old)

print('Fixed BoundingBox')

with open(fp, 'w', encoding='utf-8') as f:
    f.write(content)

print(f'\nDone. Changed: {content != original}')
