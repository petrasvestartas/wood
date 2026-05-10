#!/usr/bin/env python3
"""
Second pass fixes for wood_joint_lib.cpp based on actual compiler errors.
"""

FILEPATH = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_joint_lib.cpp'

with open(FILEPATH, 'r', encoding='utf-8') as f:
    lines = f.readlines()

original = ''.join(lines)
content = original

changes = []

def replace_at_line(lineno, old, new):
    """Replace old with new on the specified 1-based line."""
    idx = lineno - 1
    if idx < len(lines) and old in lines[idx]:
        lines[idx] = lines[idx].replace(old, new)
        changes.append(f"L{lineno}: {old[:60]!r} -> {new[:60]!r}")
        return True
    # Search nearby lines
    for delta in range(-3, 4):
        i = idx + delta
        if 0 <= i < len(lines) and old in lines[i]:
            lines[i] = lines[i].replace(old, new)
            changes.append(f"L{i+1}(near {lineno}): {old[:60]!r} -> {new[:60]!r}")
            return True
    print(f"WARNING: Could not find on L{lineno}: {old[:60]!r}")
    return False

def replace_all(old, new):
    """Replace all occurrences in the entire file."""
    count = 0
    for i in range(len(lines)):
        if old in lines[i]:
            lines[i] = lines[i].replace(old, new)
            count += 1
    if count:
        changes.append(f"replace_all({count}x): {old[:60]!r} -> {new[:60]!r}")
    return count

# ============================================================
# Fix: to_cgal_v applied to IK::Vector_3 (wrong - remove the wrapping)
# The script's Phase 6 incorrectly wrapped IK::Vector_3 expressions with to_cgal_v()
# ============================================================

# Fix these 5 occurrences of the broken to_cgal_v() wrapping:
replace_all(
    'IK::Vector_3 move_from_center_to_the_end = to_cgal_v(dir * ((total_length_scaled * 0.5) - (move_length_scaled * 0.5)));',
    'IK::Vector_3 move_from_center_to_the_end = dir * ((total_length_scaled * 0.5) - (move_length_scaled * 0.5));'
)
replace_all(
    'IK::Vector_3 move_length_dir = to_cgal_v( - dir * move_length_scaled);',
    'IK::Vector_3 move_length_dir = -dir * move_length_scaled;'
)

# ============================================================
# Fix: p += IK::Vector_3 (need to wrap right-hand side with to_sc_v)
# Lines 909, 912, 915, 918 (and similar at 1486, 1489, 1492, 1495, 3250, 3253, ...)
# These are in a for-auto loop: for (auto &p : some_polyline) { p += ik_vector; }
# Fix: p += ik_vector -> p += to_sc_v(ik_vector)
# But we need to only wrap when p is session_cpp::Point (from CGAL_Polyline)
# and ik_vector is IK::Vector_3
# ============================================================

# The specific pattern: p += move_from_center_to_the_end + move_length_dir * i
# Need to wrap the RHS with to_sc_v()
replace_all(
    'p += move_from_center_to_the_end + move_length_dir * i;',
    'p += to_sc_v(move_from_center_to_the_end + move_length_dir * i);'
)
replace_all(
    'p += move_from_center_to_the_end - move_length_dir * i;',
    'p += to_sc_v(move_from_center_to_the_end - move_length_dir * i);'
)
# For lines like "p += move_from_center_to_the_end;" without the * i part
replace_all(
    'p += move_from_center_to_the_end;',
    'p += to_sc_v(move_from_center_to_the_end);'
)

# Lines 1802, 1807: -=
# Check what those look like
content_check = ''.join(lines)

# ============================================================
# Fix: CGAL::midpoint with session_cpp::Point args (lines 2317, 2318)
# ============================================================

# Lines 2317, 2318 have CGAL::midpoint(joint.m[0][0][i], ...)
# The script's fix may not have caught multi-line or nested calls
# Let's search and fix remaining CGAL::midpoint with CGAL_Polyline elements

import re

new_lines = []
for i, line in enumerate(lines):
    # Fix CGAL::midpoint that still has session_cpp::Point args without to_cgal_pt
    if 'CGAL::midpoint(' in line:
        # Check for joint.m/f/joint_lines subscripts without to_cgal_pt
        def fix_midpoint(m):
            a = m.group(1).strip()
            b = m.group(2).strip()
            if not a.startswith('to_cgal_pt(') and not a.startswith('IK::') and not a.startswith('CGAL::'):
                a = f'to_cgal_pt({a})'
            if not b.startswith('to_cgal_pt(') and not b.startswith('IK::') and not b.startswith('CGAL::'):
                b = f'to_cgal_pt({b})'
            return f'CGAL::midpoint({a}, {b})'
        new_line = re.sub(r'CGAL::midpoint\(([^,)]+),\s*([^)]+)\)', fix_midpoint, line)
        if new_line != line:
            changes.append(f"L{i+1}: midpoint fix")
            line = new_line
    new_lines.append(line)
lines = new_lines

# ============================================================
# Fix lines 2319, 2330, 2335: binary '-' and '+' on session_cpp::Point
# with IK::Point_3 (or vice versa)
# ============================================================

# Let's look at lines 2310-2340 to understand context
print("Checking lines around 2315-2340:")
for i in range(2314, 2340):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

# ============================================================
# Fix: squared_distance at lines 1332, 3127
# ============================================================
# These are same pattern as 769 - already has to_cgal_pt wrapping from initial script
# Let's check

print("\nChecking lines around 1332:")
for i in range(1328, 1345):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

print("\nChecking lines around 3127:")
for i in range(3123, 3140):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

# ============================================================
# Fix: to_cgal_v on IK::Vector_3 at lines 1358,1359,3153,3154
# ============================================================
print("\nChecking lines around 1358:")
for i in range(1354, 1368):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

print("\nChecking lines around 1802:")
for i in range(1798, 1812):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

print("\nChecking lines around 2863:")
for i in range(2859, 2875):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

print("\nChecking lines around 2976:")
for i in range(2972, 2985):
    if i < len(lines):
        print(f"  L{i+1}: {lines[i].rstrip()}")

print("\nChanges so far:", len(changes))
for c in changes:
    print(" ", c)

# Save what we have
new_content = ''.join(lines)
if new_content != original:
    with open(FILEPATH, 'w', encoding='utf-8') as f:
        f.write(new_content)
    print("File saved with preliminary fixes!")
else:
    print("No changes yet.")
