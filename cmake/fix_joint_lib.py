#!/usr/bin/env python3
"""
Fix CGAL_Polyline usage in wood_joint_lib.cpp.
CGAL_Polyline changed from std::vector<IK::Point_3> to std::vector<session_cpp::Point>.
We apply bridge helpers:
  to_cgal_pt(session_cpp::Point) -> IK::Point_3
  to_sc_pt(IK::Point_3)          -> session_cpp::Point
  to_cgal_v(session_cpp::Vector) -> IK::Vector_3
  to_sc_v(IK::Vector_3)          -> session_cpp::Vector
"""

import re

FILEPATH = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_joint_lib.cpp'

with open(FILEPATH, 'r', encoding='utf-8') as f:
    content = f.read()

original = content

# ============================================================
# Helper: apply a regex substitution and report count
# ============================================================
def sub(pattern, repl, text, flags=0):
    result, n = re.subn(pattern, repl, text, flags=flags)
    if n:
        print(f"  {n} replacements: {pattern[:60]!r}")
    return result

# ============================================================
# Phase 1: IK::Point_3 literals in CGAL_Polyline initializers
# Pattern: IK::Point_3(expr) that appears as an element of a
# CGAL_Polyline (push_back, emplace_back, initializer lists, assignments)
# We wrap them: to_sc_pt(IK::Point_3(...))
# But ONLY if not already wrapped.
# ============================================================

# 1a. push_back(IK::Point_3(...))  ->  push_back(to_sc_pt(IK::Point_3(...)))
#     but NOT if already to_sc_pt(...)
content = sub(
    r'\.push_back\(IK::Point_3\(',
    r'.push_back(to_sc_pt(IK::Point_3(',
    content
)
# We need to close the extra paren - but regex can't match balanced parens easily.
# Instead let's use a different approach: process the specific patterns.

# Revert and do it properly - we need balanced paren matching.
# Let's use a line-by-line approach for complex cases.

# Actually, let's restart with a clean approach using line-by-line processing
# for the push_back and emplace_back cases.

# Revert the push_back change (we'll redo it properly below)
content = content.replace(
    '.push_back(to_sc_pt(IK::Point_3(',
    '.push_back(IK::Point_3('
)

print("Starting line-by-line processing...")

lines = content.split('\n')
new_lines = []

def wrap_ik_point3(s):
    """
    In string s, find occurrences of IK::Point_3(...) not already wrapped with to_sc_pt(...)
    and wrap them. Returns modified string.
    Uses a simple state machine to handle balanced parens.
    """
    result = []
    i = 0
    while i < len(s):
        # Check for to_sc_pt(IK::Point_3 - already wrapped, skip
        if s[i:i+22] == 'to_sc_pt(IK::Point_3(':
            # Find the matching close paren for to_sc_pt(
            depth = 1
            j = i + 9  # after 'to_sc_pt('
            while j < len(s) and depth > 0:
                if s[j] == '(':
                    depth += 1
                elif s[j] == ')':
                    depth -= 1
                j += 1
            result.append(s[i:j])
            i = j
        # Check for IK::Point_3(
        elif s[i:i+12] == 'IK::Point_3(':
            # Find the matching close paren
            start = i
            depth = 1
            j = i + 12  # after 'IK::Point_3('
            while j < len(s) and depth > 0:
                if s[j] == '(':
                    depth += 1
                elif s[j] == ')':
                    depth -= 1
                j += 1
            inner = s[start:j]  # IK::Point_3(...)
            result.append('to_sc_pt(' + inner + ')')
            i = j
        else:
            result.append(s[i])
            i += 1
    return ''.join(result)


def needs_ik_pt3_wrap(line):
    """Return True if line has IK::Point_3( not already inside to_sc_pt("""
    # Quick check
    if 'IK::Point_3(' not in line:
        return False
    # Check contexts where IK::Point_3 is being used as a CGAL_Polyline element
    # i.e., in push_back, emplace_back, initializer lists, assignments to CGAL_Polyline
    # For safety, we'll check: if IK::Point_3 is used as a value (not as a type declaration)
    stripped = line.strip()
    # Skip lines that are type declarations (e.g., "IK::Point_3 varname = ...")
    # Skip lines like "IK::Point_3 p[16] = {"
    # These are local variable declarations - the IK::Point_3 here is a type name
    if re.match(r'\s*IK::Point_3\s+\w', line):
        return False
    # Skip lines with IK::Point_3 as a cast or type parameter
    if 'typedef' in line or 'template' in line:
        return False
    return True

# We'll process push_back and CGAL_Polyline initializer lists
# Context tracking: are we in a CGAL_Polyline variable's initializer?
# This is complex. Let's do targeted line-by-line fixes.

# Pattern groups to fix:
# 1. .push_back(IK::Point_3(...))  - wrap IK::Point_3(...) with to_sc_pt
# 2. .emplace_back(IK::Point_3(...)) - same
# 3. CGAL_Polyline var = { IK::Point_3(...), ... } - wrap each IK::Point_3
# 4. { IK::Point_3(...), IK::Point_3(...) } in assignment to CGAL_Polyline var
# 5. = IK::Point_3(...) when assigning to a CGAL_Polyline element: poly[i] = IK::Point_3(...)

push_back_ik_count = 0
emplace_back_ik_count = 0
assignment_ik_count = 0
initializer_ik_count = 0

new_content_lines = []
for lineno, line in enumerate(lines, 1):
    original_line = line

    # Fix .push_back(IK::Point_3(...)) - wrap IK::Point_3(...) with to_sc_pt(...)
    if re.search(r'\.push_back\(IK::Point_3\(', line) and 'to_sc_pt' not in line:
        line = re.sub(r'\.push_back\(IK::Point_3\(', '.push_back(to_sc_pt(IK::Point_3(', line)
        # Need to add closing ) - but we need to find where IK::Point_3(...) ends
        # The push_back will now have an extra unclosed paren at the to_sc_pt level
        # Let's use wrap_ik_point3 on just the push_back argument
        # Actually, let's redo: process the whole line with wrap_ik_point3
        line = original_line
        # Find all push_back(IK::Point_3 occurrences and fix
        new_line = []
        i = 0
        while i < len(line):
            if line[i:i+22] == '.push_back(IK::Point_3' and 'to_sc_pt' not in line[max(0,i-10):i+30]:
                # Find the ( after IK::Point_3
                j = i + 11  # points to IK::Point_3
                # Find opening paren of IK::Point_3
                k = line.index('(', j)
                # Find matching close paren
                depth = 1
                m = k + 1
                while m < len(line) and depth > 0:
                    if line[m] == '(':
                        depth += 1
                    elif line[m] == ')':
                        depth -= 1
                    m += 1
                # line[j:m] is "IK::Point_3(...)"
                # line[i:j] is ".push_back("  -> wait, i+11 skips ".push_back("
                # Actually i points to '.', i+11 = 'IK::Point_3'
                # .push_back( is 11 chars: .push_back(
                new_line.append('.push_back(to_sc_pt(')
                new_line.append(line[j:m])  # IK::Point_3(...)
                new_line.append(')')
                push_back_ik_count += 1
                i = m
            else:
                new_line.append(line[i])
                i += 1
        line = ''.join(new_line)

    # Fix .emplace_back(IK::Point_3(...)) - wrap
    if re.search(r'\.emplace_back\(IK::Point_3\(', line) and 'to_sc_pt' not in line:
        new_line = []
        i = 0
        while i < len(line):
            if line[i:i+14] == '.emplace_back(' and line[i+14:i+25] == 'IK::Point_3':
                j = i + 14  # points to IK::Point_3
                k = line.index('(', j)
                depth = 1
                m = k + 1
                while m < len(line) and depth > 0:
                    if line[m] == '(':
                        depth += 1
                    elif line[m] == ')':
                        depth -= 1
                    m += 1
                new_line.append('.emplace_back(to_sc_pt(')
                new_line.append(line[j:m])
                new_line.append(')')
                emplace_back_ik_count += 1
                i = m
            else:
                new_line.append(line[i])
                i += 1
        line = ''.join(new_line)

    # Fix: poly[i] = IK::Point_3(...) or poly.back() = IK::Point_3(...)
    # Pattern: [indexer] = IK::Point_3(  or  .back() = IK::Point_3(
    if re.search(r'(\[.*?\]|\.back\(\)|\.front\(\))\s*=\s*IK::Point_3\(', line) and 'to_sc_pt' not in line:
        new_line = []
        i = 0
        while i < len(line):
            # Look for "= IK::Point_3("
            if line[i:i+3] == '= I' and line[i:i+15] == '= IK::Point_3(':
                j = i + 2  # points to IK::Point_3
                k = line.index('(', j)
                depth = 1
                m = k + 1
                while m < len(line) and depth > 0:
                    if line[m] == '(':
                        depth += 1
                    elif line[m] == ')':
                        depth -= 1
                    m += 1
                new_line.append('= to_sc_pt(')
                new_line.append(line[j:m])
                new_line.append(')')
                assignment_ik_count += 1
                i = m
            else:
                new_line.append(line[i])
                i += 1
        line = ''.join(new_line)

    new_content_lines.append(line)

print(f"push_back IK::Point_3 fixes: {push_back_ik_count}")
print(f"emplace_back IK::Point_3 fixes: {emplace_back_ik_count}")
print(f"assignment IK::Point_3 fixes: {assignment_ik_count}")

content = '\n'.join(new_content_lines)
lines = new_content_lines  # update for further processing

# ============================================================
# Phase 2: CGAL_Polyline initializer lists with IK::Point_3
# These are lines like:
#   joint.m[0][0] = { IK::Point_3(...), IK::Point_3(...), ... };
#   CGAL_Polyline line0 = { IK::Point_3(...), IK::Point_3(...) };
#   joint.f[1][0] = { IK::Point_3(...), ... };
# We need to wrap each IK::Point_3(...) in to_sc_pt(...)
#
# Strategy: find lines with { IK::Point_3 and process them
# ============================================================
print("\nPhase 2: CGAL_Polyline initializer lists...")

new_lines2 = []
init_list_ik_count = 0
for lineno, line in enumerate(lines, 1):
    if 'IK::Point_3(' in line and '{' in line and 'to_sc_pt' not in line:
        # Check it's not a type declaration or IK::Point_3 p[] = {
        stripped = line.strip()
        if re.match(r'IK::Point_3\s+\w', stripped):
            new_lines2.append(line)
            continue
        # Apply wrap_ik_point3 to wrap all IK::Point_3(...) with to_sc_pt(...)
        new_line = wrap_ik_point3(line)
        if new_line != line:
            init_list_ik_count += 1
        new_lines2.append(new_line)
    else:
        new_lines2.append(line)

print(f"init list IK::Point_3 fixes: {init_list_ik_count}")
content = '\n'.join(new_lines2)
lines = new_lines2

# ============================================================
# Phase 3: Remaining IK::Point_3 used as VALUES (not types)
# Check for: = IK::Point_3( without to_sc_pt wrapping
# (outside of initializer lists - e.g., direct assignments)
# ============================================================
print("\nPhase 3: remaining IK::Point_3 value uses...")

# Handle std::initializer_list<IK::Point_3>{ ... } -> just remove the type annotation
# making them CGAL_Polyline{ ... }
# Pattern: std::initializer_list<IK::Point_3>{
std_init_count = 0
new_content = '\n'.join(lines)
if 'std::initializer_list<IK::Point_3>' in new_content:
    new_content2 = new_content.replace(
        'std::initializer_list<IK::Point_3>',
        ''
    )
    std_init_count = new_content.count('std::initializer_list<IK::Point_3>')
    print(f"  std::initializer_list<IK::Point_3> removed: {std_init_count}")
    new_content = new_content2

content = new_content
lines = content.split('\n')

# ============================================================
# Phase 4: CGAL function calls with CGAL_Polyline elements
# ============================================================
print("\nPhase 4: CGAL function calls...")

# CGAL::midpoint(poly[i], poly[j]) -> CGAL::midpoint(to_cgal_pt(poly[i]), to_cgal_pt(poly[j]))
# But only for CGAL_Polyline elements (poly[i] where poly is a CGAL_Polyline)
# We look for patterns where the arguments are subscript expressions or named polyline elements

# General pattern: any CGAL function that takes Point_3 args when given session_cpp::Point args
# The safest approach: wrap arguments that are "poly[...]" or "joint.xxx[.][.][.]"

# midpoint fix
def fix_cgal_midpoint(line):
    """Fix CGAL::midpoint(a, b) where a,b are session_cpp::Point"""
    # Match CGAL::midpoint( ... , ... )
    pattern = r'CGAL::midpoint\(([^,)]+),\s*([^)]+)\)'
    def replacer(m):
        a = m.group(1).strip()
        b = m.group(2).strip()
        # Only wrap if not already wrapped
        if not a.startswith('to_cgal_pt('):
            a = f'to_cgal_pt({a})'
        if not b.startswith('to_cgal_pt('):
            b = f'to_cgal_pt({b})'
        return f'CGAL::midpoint({a}, {b})'
    return re.sub(pattern, replacer, line)

# squared_distance fix
def fix_cgal_sq_dist(line):
    pattern = r'CGAL::squared_distance\(([^,)]+),\s*([^)]+)\)'
    def replacer(m):
        a = m.group(1).strip()
        b = m.group(2).strip()
        if not a.startswith('to_cgal_pt(') and not a.startswith('to_cgal_'):
            a = f'to_cgal_pt({a})'
        if not b.startswith('to_cgal_pt(') and not b.startswith('to_cgal_'):
            b = f'to_cgal_pt({b})'
        return f'CGAL::squared_distance({a}, {b})'
    return re.sub(pattern, replacer, line)

# has_smaller_distance_to_point fix
def fix_has_smaller_dist(line):
    pattern = r'CGAL::has_smaller_distance_to_point\(([^,)]+),\s*([^,)]+),\s*([^)]+)\)'
    def replacer(m):
        a = m.group(1).strip()
        b = m.group(2).strip()
        c = m.group(3).strip()
        if not a.startswith('to_cgal_pt('):
            a = f'to_cgal_pt({a})'
        if not b.startswith('to_cgal_pt('):
            b = f'to_cgal_pt({b})'
        if not c.startswith('to_cgal_pt('):
            c = f'to_cgal_pt({c})'
        return f'CGAL::has_smaller_distance_to_point({a}, {b}, {c})'
    return re.sub(pattern, replacer, line)

midpoint_count = 0
sq_dist_count = 0
has_smaller_count = 0

new_lines3 = []
for line in lines:
    orig = line
    if 'CGAL::midpoint(' in line:
        line = fix_cgal_midpoint(line)
        if line != orig:
            midpoint_count += 1
    if 'CGAL::squared_distance(' in line:
        line = fix_cgal_sq_dist(line)
        if line != orig:
            sq_dist_count += 1
    if 'CGAL::has_smaller_distance_to_point(' in line:
        line = fix_has_smaller_dist(line)
        if line != orig:
            has_smaller_count += 1
    new_lines3.append(line)

print(f"CGAL::midpoint fixes: {midpoint_count}")
print(f"CGAL::squared_distance fixes: {sq_dist_count}")
print(f"CGAL::has_smaller_distance_to_point fixes: {has_smaller_count}")
lines = new_lines3

# ============================================================
# Phase 5: IK::Segment_3 with CGAL_Polyline elements
# IK::Segment_3(poly[i], poly[j]) -> IK::Segment_3(to_cgal_pt(poly[i]), to_cgal_pt(poly[j]))
# ============================================================
print("\nPhase 5: IK::Segment_3 with CGAL_Polyline elements...")

seg3_count = 0
new_lines4 = []
for line in lines:
    orig = line
    if 'IK::Segment_3(' in line:
        # Pattern: IK::Segment_3(a, b) where a,b may be session_cpp::Point
        def fix_segment3(m):
            a = m.group(1).strip()
            b = m.group(2).strip()
            # Check if args look like CGAL_Polyline elements or session_cpp::Point vars
            # Heuristic: if they contain [ or are known polyline variable names
            # and not already to_cgal_pt wrapped
            # For safety wrap all that are not IK:: types
            if (not a.startswith('to_cgal_pt(') and not a.startswith('IK::') and
                    not a.startswith('CGAL::') and not 'IK::' in a):
                a = f'to_cgal_pt({a})'
            if (not b.startswith('to_cgal_pt(') and not b.startswith('IK::') and
                    not b.startswith('CGAL::') and not 'IK::' in b):
                b = f'to_cgal_pt({b})'
            return f'IK::Segment_3({a}, {b})'
        line = re.sub(r'IK::Segment_3\(([^,)]+),\s*([^)]+)\)', fix_segment3, line)
        if line != orig:
            seg3_count += 1
    new_lines4.append(line)

print(f"IK::Segment_3 fixes: {seg3_count}")
lines = new_lines4

# ============================================================
# Phase 6: IK::Vector_3 from CGAL_Polyline subtraction
# IK::Vector_3 var = poly[a] - poly[b]
# poly[a] - poly[b] now gives session_cpp::Vector, need to_cgal_v()
# Pattern: IK::Vector_3 \w+ = <expr> - <expr>  where exprs are CGAL_Polyline elements
# ============================================================
print("\nPhase 6: IK::Vector_3 from CGAL_Polyline subtraction...")

vec3_assign_count = 0
new_lines5 = []
for lineno, line in enumerate(lines, 1):
    orig = line
    # Pattern: IK::Vector_3 varname = something - something;
    # Or: IK::Vector_3 varname(something - something, ...);
    # We look for lines that assign or initialize IK::Vector_3 from subtraction of CGAL_Polyline elems
    # Simple heuristic: IK::Vector_3 \w+ =
    # and the RHS involves polyline subscripts like [0], [1], joint.f[..][..][..], etc.
    m = re.match(r'^(\s*IK::Vector_3\s+\w+\s*=\s*)(.+)(;.*)$', line)
    if m:
        prefix = m.group(1)
        rhs = m.group(2)
        suffix = m.group(3)
        # Check if rhs contains subtraction of session_cpp::Point expressions
        # (i.e., has [ ] indexing but no explicit to_cgal conversion)
        if '-' in rhs and 'to_cgal' not in rhs and 'IK::' not in rhs:
            # Looks like it needs wrapping
            # Check if it's a simple a - b
            parts = rhs.split('-', 1)
            if len(parts) == 2:
                a = parts[0].strip()
                b = parts[1].strip()
                line = f'{prefix}to_cgal_v({a} - {b}){suffix}'
                vec3_assign_count += 1
    new_lines5.append(line)

print(f"IK::Vector_3 = subtraction fixes: {vec3_assign_count}")
lines = new_lines5

# ============================================================
# Phase 7: CGAL_Polyline element += IK::Vector_3 (needs to_sc_v conversion)
# For (auto &p : polyline) p += vector;
# But vector is IK::Vector_3, need to convert:
#   p += vector  ->  p += to_sc_v(vector)
# ============================================================
print("\nPhase 7: CGAL_Polyline element += IK::Vector_3...")

# Actually the for loop patterns are:
# for (auto &p : polyline) p += some_ik_vector;
# We need to check each += in for loop body

# This requires context - let's look for the specific patterns we found:
# p += move_from_center_to_the_end + move_length_dir * i
# These variables are IK::Vector_3 types being added to session_cpp::Point p
# session_cpp::Point::operator+= takes session_cpp::Vector, not IK::Vector_3

# First let's check what += patterns exist after for loops
# We'll look for lines like: "p += " in for loop bodies
# where p is auto &p from a CGAL_Polyline

# The pattern is hard to do automatically without full type info.
# Let's look at what was found in the analysis:
# Lines ~909, 912, 915, 918, 1486, 1489, 1492, 1495, 3250, 3253, 3256, 3259, etc.
# Pattern: p += [expr involving IK::Vector_3 operations]

# ============================================================
# Phase 8: *it = it->transform(xform)
# This was for CGAL_Polyline iterators calling .transform()
# session_cpp::Point has no .transform() method
# Fix: *it = to_sc_pt(to_cgal_pt(*it).transform(xform))
# ============================================================
print("\nPhase 8: *it = it->transform(xform)...")

transform_count = 0
new_lines6 = []
for line in lines:
    orig = line
    # Pattern: *it = it->transform(...)
    if '*it = it->transform(' in line:
        # Find the transform argument
        m = re.search(r'\*it\s*=\s*it->transform\((\w+)\)', line)
        if m:
            xf = m.group(1)
            line = line.replace(
                f'*it = it->transform({xf})',
                f'*it = to_sc_pt(to_cgal_pt(*it).transform({xf}))'
            )
            transform_count += 1
    new_lines6.append(line)

print(f"*it = it->transform() fixes: {transform_count}")
lines = new_lines6

# ============================================================
# Phase 9: IK::Plane_3(poly[i], normal) -> IK::Plane_3(to_cgal_pt(poly[i]), normal)
# ============================================================
print("\nPhase 9: IK::Plane_3 with CGAL_Polyline elements...")

plane_count = 0
new_lines7 = []
for line in lines:
    orig = line
    if 'IK::Plane_3(' in line and 'to_cgal_pt' not in line:
        # Pattern: IK::Plane_3(poly_element, normal)
        # poly_element looks like: joint.joint_lines[..][..], poly[..], etc.
        def fix_plane3(m):
            a = m.group(1).strip()
            b = m.group(2).strip()
            # Only wrap first arg if it looks like a session_cpp::Point (has brackets)
            if '[' in a and not a.startswith('to_cgal_pt(') and 'IK::' not in a:
                a = f'to_cgal_pt({a})'
            return f'IK::Plane_3({a}, {b})'
        line = re.sub(r'IK::Plane_3\(([^,)]+),\s*([^)]+)\)', fix_plane3, line)
        if line != orig:
            plane_count += 1
    new_lines7.append(line)

print(f"IK::Plane_3 fixes: {plane_count}")
lines = new_lines7

# ============================================================
# Phase 10: IK::Point_3 variable = CGAL_Polyline element
# IK::Point_3 p = poly[i] -> IK::Point_3 p = to_cgal_pt(poly[i])
# ============================================================
print("\nPhase 10: IK::Point_3 var = CGAL_Polyline element...")

ik_var_count = 0
new_lines8 = []
for line in lines:
    orig = line
    # Pattern: IK::Point_3 varname = something[something]
    # But not IK::Point_3 p[] = { ... }
    m = re.match(r'^(\s*IK::Point_3\s+\w+\s*=\s*)([^{;]+)(;.*)?$', line)
    if m and '[' in m.group(2) and 'to_cgal_pt' not in m.group(2):
        prefix = m.group(1)
        rhs = m.group(2).strip()
        suffix = m.group(3) or ''
        # Check it's a simple element access (no IK:: in rhs)
        if 'IK::' not in rhs and 'CGAL::' not in rhs:
            line = f'{prefix}to_cgal_pt({rhs}){suffix}'
            ik_var_count += 1
    new_lines8.append(line)

print(f"IK::Point_3 var = CGAL_Polyline element fixes: {ik_var_count}")
lines = new_lines8

# ============================================================
# Phase 11: CGAL::cross_product with CGAL_Polyline subtractions
# CGAL::cross_product(a - b, c - d) where a,b,c,d are session_cpp::Points
# The subtraction gives session_cpp::Vector, need to_cgal_v()
# ============================================================
print("\nPhase 11: CGAL::cross_product with CGAL_Polyline subtractions...")

cross_count = 0
new_lines9 = []
for line in lines:
    orig = line
    if 'CGAL::cross_product(' in line and 'to_cgal_v' not in line and 'to_cgal_pt' not in line:
        # Pattern: CGAL::cross_product(a, b)
        # where a and b are Vector expressions
        def fix_cross_product(m):
            a = m.group(1).strip()
            b = m.group(2).strip()
            # If a contains session_cpp::Point subtraction (has [ but no IK::)
            if not a.startswith('to_cgal_v(') and ('joint.' in a or '[' in a) and 'IK::' not in a:
                a = f'to_cgal_v({a})'
            if not b.startswith('to_cgal_v(') and ('joint.' in b or '[' in b) and 'IK::' not in b:
                b = f'to_cgal_v({b})'
            return f'CGAL::cross_product({a}, {b})'
        line = re.sub(r'CGAL::cross_product\(([^,)]+(?:\([^)]*\)[^,)]*)?),\s*([^)]+)\)', fix_cross_product, line)
        if line != orig:
            cross_count += 1
    new_lines9.append(line)

print(f"CGAL::cross_product fixes: {cross_count}")
lines = new_lines9

# ============================================================
# Save the result
# ============================================================
content = '\n'.join(lines)

print(f"\nOriginal size: {len(original)} chars")
print(f"New size: {len(content)} chars")
print(f"Changed: {content != original}")

if content != original:
    with open(FILEPATH, 'w', encoding='utf-8') as f:
        f.write(content)
    print("File saved!")
else:
    print("No changes made.")
