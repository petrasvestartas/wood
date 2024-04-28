# wood

Timber joinery development: https://petrasvestartas.github.io/wood/

Auhor: Petras Vestartas

ToDo:

- Nearest object for 16_beams_nearest_curve.gh :
        -   lines - simple rtree
        -   polylines - cgal
        -   curves - LNlib

- Based on the nearest object implement:
        -   _circles
        -   _length_of_box
        -   _distance_tolerance
        -   _parallel_tolerance
        -   _cross_or_toptoside

- 38 joint crashes on 15_beams_simple_volume_a.gh example

- Solve the joint orientation planarity problem - chevron corner example: 7_assign_directions_and_joint_types.gh

- 17_beam_custom_joints