"""
Migration script for wood_test.cpp
"""
import re

fp = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_test.cpp'
with open(fp, encoding='utf-8') as f:
    content = f.read()
original = content

# ============================================================
# 1. IK types -> session_cpp types
# ============================================================
count = content.count('IK::Vector_3')
content = content.replace('IK::Vector_3', 'session_cpp::Vector')
print(f'IK::Vector_3: {count}')

count = content.count('IK::Point_3')
content = content.replace('IK::Point_3', 'session_cpp::Point')
print(f'IK::Point_3: {count}')

# ============================================================
# 2. to_sc_pt(IK::Point_3(x,y,z)) -> session_cpp::Point(x,y,z)
# ============================================================
def replace_to_sc_pt_point_literal(content):
    # to_sc_pt(session_cpp::Point(x,y,z)) -> session_cpp::Point(x,y,z)  (after IK::Point_3 was already replaced)
    pattern = r'to_sc_pt\s*\(\s*session_cpp::Point\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point(\1)'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(Point literal): {n}')
    return new
content = replace_to_sc_pt_point_literal(content)

# ============================================================
# 3. to_sc_pt(to_cgal_pt(x)) -> x
# ============================================================
def replace_roundtrip(content):
    pattern = r'to_sc_pt\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(to_cgal_pt(x)): {n}')
    return new
content = replace_roundtrip(content)

# ============================================================
# 4. to_cgal_pt(x) -> x
# ============================================================
def replace_to_cgal_pt(content):
    pattern = r'to_cgal_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_pt(x): {n}')
    return new
content = replace_to_cgal_pt(content)

# ============================================================
# 5. to_sc_pt(x) -> x
# ============================================================
def replace_to_sc_pt(content):
    pattern = r'to_sc_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(x): {n}')
    return new
content = replace_to_sc_pt(content)

# ============================================================
# 6. to_sc_v(x) -> x, to_cgal_v(x) -> x
# ============================================================
def replace_to_sc_v(content):
    pattern = r'to_sc_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_v(x): {n}')
    return new
content = replace_to_sc_v(content)

def replace_to_cgal_v(content):
    pattern = r'to_cgal_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_v(x): {n}')
    return new
content = replace_to_cgal_v(content)

# ============================================================
# 7. CGAL::squared_distance with to_cgal_pt
# ============================================================
def replace_sq_dist(content):
    pattern = r'CGAL::squared_distance\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'sq_dist with wrappers: {n}')
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
# 8. Fix function signatures
# ============================================================
content = content.replace(
    'set_file_path_for_input_xml_and_screenshot (std::vector<CGAL_Polyline> &input_polyline_pairs, std::vector<std::vector<IK::Vector_3> > &input_insertion_vectors,',
    'set_file_path_for_input_xml_and_screenshot (std::vector<CGAL_Polyline> &input_polyline_pairs, std::vector<std::vector<session_cpp::Vector> > &input_insertion_vectors,'
)
content = content.replace(
    'std::vector<std::vector<IK::Vector_3> > &polylines_segment_direction;',
    'std::vector<std::vector<session_cpp::Vector> > &polylines_segment_direction;'
)
content = content.replace(
    'std::vector<std::vector<IK::Point_3> > point_pairs;',
    'std::vector<std::vector<session_cpp::Point> > point_pairs;'
)
print('Fixed signatures')

# ============================================================
# 9. Fix remaining vector/point constructors
# ============================================================
# After the replacements, session_cpp::Vector(0,0,0) is correct
# session_cpp::Point(x,y,z) is correct

# ============================================================
# 10. tmp vector<vector<Point_3>> -> vector<vector<Point>>
# ============================================================
content = content.replace(
    'std::vector<std::vector<IK::Point_3> > tmp;',
    'std::vector<std::vector<session_cpp::Point> > tmp;'
)
content = content.replace(
    'std::vector<IK::Point_3> points;',
    'std::vector<session_cpp::Point> points;'
)
content = content.replace(
    'std::vector<IK::Point_3> polygon_ik;',
    'std::vector<session_cpp::Point> polygon_ik;'
)
print('Fixed tmp vectors')

with open(fp, 'w', encoding='utf-8') as f:
    f.write(content)

print(f'\nDone. Changed: {content != original}')
