#!/usr/bin/env python3
"""
Third pass targeted fixes for wood_joint_lib.cpp.
"""

FILEPATH = 'C:/pc/3_code/wood/cmake/src/wood/include/wood_joint_lib.cpp'

with open(FILEPATH, 'r', encoding='utf-8') as f:
    content = f.read()

original = content
changes = 0

def rep(old, new):
    global content, changes
    count = content.count(old)
    if count > 0:
        content = content.replace(old, new)
        changes += count
        print(f"  [{count}x] {old[:70]!r}")
    else:
        print(f"  [0x - NOT FOUND] {old[:70]!r}")

# ============================================================
# Fix 1: to_cgal_v(-dir0) where dir0 is IK::Vector_3
# The result of -dir0 is IK::Vector_3, not session_cpp::Vector
# So to_cgal_v is wrong here - just remove it
# ============================================================
rep(
    'IK::Vector_3 dir1 = to_cgal_v( - dir0);',
    'IK::Vector_3 dir1 = -dir0;'
)

# ============================================================
# Fix 2: mid_rectangle[i] += v (IK::Vector_3) -> to_sc_v(v)
# These are at lines 5895-5905
# ============================================================
# The pattern repeats 5 times:
rep('mid_rectangle[0] += v;', 'mid_rectangle[0] += to_sc_v(v);')
rep('mid_rectangle[1] -= v;', 'mid_rectangle[1] -= to_sc_v(v);')
rep('mid_rectangle[2] -= v;', 'mid_rectangle[2] -= to_sc_v(v);')
rep('mid_rectangle[3] += v;', 'mid_rectangle[3] += to_sc_v(v);')
rep('mid_rectangle[4] += v;', 'mid_rectangle[4] += to_sc_v(v);')

# But after unitize, v is still IK::Vector_3 and we have more += v
# For mid_rectangle[0] += v; the second occurrence (line 5903)
# -- already handled by replace_all above

# Check if there are any remaining "mid_rectangle[...] += v;"
# or "mid_rectangle[...] -= v;" not yet handled
import re
remaining = re.findall(r'mid_rectangle\[\d+\]\s*[+-]=\s*\w+;', content)
for r in remaining:
    if 'to_sc_v' not in r:
        print(f"  REMAINING: {r}")

# ============================================================
# Fix 3: p += to_sc_v(move_from_center_to_the_end - ...)
# These may also appear (with - instead of +)
# ============================================================
# Check if there are "p -= move_from_center_to_the_end" patterns
if 'p -= move_from_center_to_the_end' in content:
    rep('p -= move_from_center_to_the_end', 'p -= to_sc_v(move_from_center_to_the_end')
    # This would be incomplete (no closing paren) - better check
    print("WARNING: p -= move_from_center_to_the_end found - need manual fix")

# ============================================================
# Verify no remaining "arrays[i][j] -= N * v * flip" without to_sc_v
# ============================================================
remaining_arrays = re.findall(r'arrays\[\w+\]\[\w+\]\s*[+-]=\s*[^;]+;', content)
for r in remaining_arrays:
    if 'to_sc_v' not in r and 'to_sc_pt' not in r:
        print(f"  REMAINING arrays op: {r}")

# ============================================================
# Summary
# ============================================================
print(f"\nTotal replacements: {changes}")
print(f"Content changed: {content != original}")

if content != original:
    with open(FILEPATH, 'w', encoding='utf-8') as f:
        f.write(content)
    print("File saved!")
else:
    print("No changes - file unchanged.")
