"""
Migration script for wood_joint_lib.cpp
"""
import re

fp = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_joint_lib.cpp'
with open(fp, encoding='utf-8') as f:
    content = f.read()
original = content

# ============================================================
# 1. unitize function - find actual text in file
# ============================================================
# Find the actual unitize block
idx = content.find('unitize (IK::Vector_3')
if idx >= 0:
    # Find the function start
    start = content.rfind('\nbool\n', 0, idx)
    if start >= 0:
        end = content.find('\n}', idx) + 2
        old_block = content[start:end]
        new_block = '''
bool
unitize (session_cpp::Vector &vector)
{
    double sq = vector.magnitude_squared ();
    if (sq > 1e-20)
        {
            vector.normalize_self ();
            return true;
        }
    return false;
}'''
        content = content[:start] + new_block + content[end:]
        print('unitize: replaced')
    else:
        print('unitize: start not found')
else:
    print('unitize: pattern not found')

# ============================================================
# 2. rotation_in_xy_plane function
# ============================================================
idx = content.find('rotation_in_xy_plane (const IK::Vector_3')
if idx >= 0:
    # find CGAL::Aff_transformation_3<IK> on preceding line
    start = content.rfind('\nCGAL::Aff_transformation_3<IK>\n', 0, idx)
    if start >= 0:
        end = content.find('\n}\n', idx) + 3
        old_block = content[start:end]
        new_block = '''
session_cpp::Xform
rotation_in_xy_plane (const session_cpp::Vector &x_axis, const session_cpp::Vector &y_axis, const session_cpp::Vector &z_axis)
{
    return session_cpp::Xform ({x_axis[0], x_axis[1], x_axis[2], 0, y_axis[0], y_axis[1], y_axis[2], 0, z_axis[0], z_axis[1], z_axis[2], 0, 0, 0, 0, 1});
}
'''
        content = content[:start] + new_block + content[end:]
        print('rotation_in_xy_plane: replaced')
    else:
        print('rotation_in_xy_plane: start not found')
else:
    print('rotation_in_xy_plane: pattern not found')

# ============================================================
# 3. planes[x].orthogonal_vector() -> planes[x].z_axis()
# ============================================================
count = content.count('.orthogonal_vector ()')
content = content.replace('.orthogonal_vector ()', '.z_axis ()')
print(f'orthogonal_vector: {count} replacements')

# ============================================================
# 4. CGAL::midpoint(CGAL::midpoint(to_cgal_pt(a), to_cgal_pt(b)), CGAL::midpoint(to_cgal_pt(c), to_cgal_pt(d)))
# -> session_cpp::Point::mid_point(session_cpp::Point::mid_point(a, b), session_cpp::Point::mid_point(c, d))
# ============================================================
# Replace nested midpoints
def replace_nested_midpoints(content):
    pattern = r'CGAL::midpoint\s*\(\s*CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)\s*,\s*CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)\s*\)'
    replacement = r'session_cpp::Point::mid_point(session_cpp::Point::mid_point(\1, \2), session_cpp::Point::mid_point(\3, \4))'
    new, n = re.subn(pattern, replacement, content)
    print(f'nested midpoints: {n} replacements')
    return new

content = replace_nested_midpoints(content)

# ============================================================
# 5. Simple midpoints: CGAL::midpoint(to_cgal_pt(a), to_cgal_pt(b)) -> session_cpp::Point::mid_point(a, b)
# ============================================================
def replace_simple_midpoints(content):
    pattern = r'CGAL::midpoint\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point::mid_point(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'simple midpoints: {n} replacements')
    return new

content = replace_simple_midpoints(content)

# ============================================================
# 5b. Simple midpoints: CGAL::midpoint(a, b) where a and b are already session_cpp::Point vars
# ============================================================
def replace_direct_midpoints(content):
    # These are already IK::Point_3 vars (after previous replacements, these are session_cpp vars)
    pattern = r'CGAL::midpoint\s*\(([^()]+),\s*([^()]+)\)'
    replacement = r'session_cpp::Point::mid_point(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'direct midpoints: {n} replacements')
    return new

content = replace_direct_midpoints(content)

# ============================================================
# 6. CGAL::squared_distance(to_cgal_pt(a), to_cgal_pt(b)) -> session_cpp::Point::squared_distance(a, b)
# ============================================================
def replace_sq_dist(content):
    pattern = r'CGAL::squared_distance\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*,\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'sq_dist with to_cgal_pt: {n} replacements')
    return new
content = replace_sq_dist(content)

def replace_sq_dist2(content):
    pattern = r'CGAL::squared_distance\s*\(([^,()]+),\s*([^()]+)\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'sq_dist direct: {n} replacements')
    return new
content = replace_sq_dist2(content)

# ============================================================
# 7. CGAL::cross_product(to_cgal_v(a), to_cgal_v(b)) -> a.cross(b)
# ============================================================
def replace_cross(content):
    pattern = r'CGAL::cross_product\s*\(\s*to_cgal_v\s*\(([^)]+)\)\s*,\s*to_cgal_v\s*\(([^)]+)\)\s*\)'
    replacement = r'(\1).cross(\2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'cross_product with to_cgal_v: {n} replacements')
    return new
content = replace_cross(content)

def replace_cross2(content):
    # for CGAL::cross_product(v1, v2) where v1/v2 are already IK::Vector_3 vars
    pattern = r'CGAL::cross_product\s*\(([^()]+),\s*([^()]+)\)'
    replacement = r'(\1).cross(\2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'cross_product direct: {n} replacements')
    return new
content = replace_cross2(content)

# ============================================================
# 8. CGAL::has_smaller_distance_to_point(a, b, ref) -> session_cpp::Point::squared_distance(b,ref) < session_cpp::Point::squared_distance(a,ref)
# but actual form is has_smaller_distance_to_point(ref, b, c) -> squared_distance(b,ref) < squared_distance(c,ref)
# CGAL: has_smaller_distance_to_point(p, q, r) = distance(p,q) < distance(p,r)
# ============================================================
def replace_has_smaller(content):
    pattern = r'CGAL::has_smaller_distance_to_point\s*\(([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^,()]+(?:\([^()]*\))?[^,()]*),\s*([^()]+(?:\([^()]*\))?[^()]*)\)'
    replacement = r'session_cpp::Point::squared_distance(\1, \2) < session_cpp::Point::squared_distance(\1, \3)'
    new, n = re.subn(pattern, replacement, content)
    print(f'has_smaller_distance: {n} replacements')
    return new
content = replace_has_smaller(content)

# ============================================================
# 9. IK::Vector_3 declarations -> session_cpp::Vector
# ============================================================
count = content.count('IK::Vector_3')
content = content.replace('IK::Vector_3', 'session_cpp::Vector')
print(f'IK::Vector_3: {count} replacements')

# ============================================================
# 10. IK::Point_3 declarations -> session_cpp::Point
# ============================================================
count = content.count('IK::Point_3')
content = content.replace('IK::Point_3', 'session_cpp::Point')
print(f'IK::Point_3: {count} replacements')

# ============================================================
# 11. IK::Plane_3 -> session_cpp::Plane
# ============================================================
count = content.count('IK::Plane_3')
content = content.replace('IK::Plane_3', 'session_cpp::Plane')
print(f'IK::Plane_3: {count} replacements')

# ============================================================
# 12. IK::Segment_3 -> session_cpp::Line
# ============================================================
count = content.count('IK::Segment_3')
content = content.replace('IK::Segment_3', 'session_cpp::Line')
print(f'IK::Segment_3: {count} replacements')

# ============================================================
# 13. to_sc_pt(to_cgal_pt(x) + v) -> x + session_cpp::Vector{v}
# More specifically: to_sc_pt(to_cgal_pt(x) OP expr) -> ...
# This is complex. Let's handle specific simple patterns first.
# ============================================================
# to_sc_pt(to_cgal_pt(X)) -> X
def replace_to_sc_pt_to_cgal_pt(content):
    pattern = r'to_sc_pt\s*\(\s*to_cgal_pt\s*\(([^)]+)\)\s*\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(to_cgal_pt(x)): {n} replacements')
    return new
content = replace_to_sc_pt_to_cgal_pt(content)

# ============================================================
# 14. to_sc_v(to_cgal_v(x)) -> x
# ============================================================
def replace_to_sc_v_to_cgal_v(content):
    pattern = r'to_sc_v\s*\(\s*to_cgal_v\s*\(([^)]+)\)\s*\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_v(to_cgal_v(x)): {n} replacements')
    return new
content = replace_to_sc_v_to_cgal_v(content)

# ============================================================
# 15. to_cgal_pt(X) -> X (remaining ones - just strip the wrapper)
# ============================================================
def replace_to_cgal_pt(content):
    pattern = r'to_cgal_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_pt(x): {n} replacements')
    return new
content = replace_to_cgal_pt(content)

# ============================================================
# 16. to_sc_pt(X) -> X (remaining wrappers)
# ============================================================
def replace_to_sc_pt(content):
    pattern = r'to_sc_pt\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_pt(x): {n} replacements')
    return new
content = replace_to_sc_pt(content)

# ============================================================
# 17. to_cgal_v(X) -> X
# ============================================================
def replace_to_cgal_v(content):
    pattern = r'to_cgal_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_cgal_v(x): {n} replacements')
    return new
content = replace_to_cgal_v(content)

# ============================================================
# 18. to_sc_v(X) -> X
# ============================================================
def replace_to_sc_v(content):
    pattern = r'to_sc_v\s*\(([^)]+)\)'
    replacement = r'\1'
    new, n = re.subn(pattern, replacement, content)
    print(f'to_sc_v(x): {n} replacements')
    return new
content = replace_to_sc_v(content)

# ============================================================
# 19. Fix .x() .y() .z() on session_cpp::Point/Vector vars -> [0][1][2]
# These are CGAL-style accessors. In session_cpp we use [0][1][2]
# But be careful: .x() inside CGAL headers/template code should stay
# Focus on patterns like: variable.x() where variable is session_cpp type
# ============================================================
# Handle x_axis.x(), z_axis.x() etc. from rotation_in_xy_plane usage
# After replacement, these should use [0],[1],[2]
# e.g.: x_axis.x () -> x_axis[0] , x_axis.y () -> x_axis[1] , x_axis.z () -> x_axis[2]
# But we have IK::Point_3 p; p.x() - now session_cpp::Point p; p[0]
# These patterns are like: varname.x() varname.y() varname.z()
# We need to be careful not to break things like sc_project_to_plane(p, plane) where n.x() is used
# (that's in stdafx.h, not this file)

# IK::Point_3/Vector_3 literal constructors
# After replacement, these are now session_cpp::Vector(x,y,z) or session_cpp::Point(x,y,z)
# which should work fine since both have (x,y,z) constructors

# ============================================================
# 20. Fix rotation_in_xy_plane calls that now pass IK::Vector_3 args
# After step 9, IK::Vector_3(x,y,z) -> session_cpp::Vector(x,y,z) - OK
# ============================================================

# ============================================================
# 21. Fix IK::Plane_3(point, normal) constructor usage
# After replacement: session_cpp::Plane(point, normal) - need from_point_normal
# ============================================================
def replace_plane_constructor(content):
    # session_cpp::Plane(point, normal) -> session_cpp::Plane::from_point_normal(point, normal)
    pattern = r'session_cpp::Plane\s*\(([^,()]+(?:\([^()]*\)[^,()]*)?),\s*([^()]+(?:\([^()]*\)[^()]*)?)\)'
    replacement = r'session_cpp::Plane::from_point_normal(\1, \2)'
    new, n = re.subn(pattern, replacement, content)
    print(f'Plane constructor: {n} replacements')
    return new
content = replace_plane_constructor(content)

# ============================================================
# 22. Fix segment[0] / segment[1] -> segment.start() / segment.end()
# ============================================================
# These come from IK::Segment_3 s; s[0] = start, s[1] = end
# After type replace: session_cpp::Line s; s[0] -> s.start()
# But session_cpp::Line doesn't have operator[]
# This is risky with regex - skip and handle manually

# ============================================================
# 23. Fix vector.x() -> vector[0] etc. on known session_cpp vars
# These appear after the rotation_in_xy_plane fix - not needed since
# we rewrote that function to use [0][1][2] directly
# ============================================================

# Save
with open(fp, 'w', encoding='utf-8') as f:
    f.write(content)

print(f'\nDone. Changed: {content != original}')
