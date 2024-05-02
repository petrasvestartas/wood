# wood

Timber joinery development: https://petrasvestartas.github.io/wood/

Auhor: Petras Vestartas

ToDo:

- build wood_nano wheel on mac, specially the gmp mpfr libraries, do I need 

- 17_beam_custom_joints - verify if it works on the latest pip install

- Nearest object for 16_beams_nearest_curve.gh :

        -   lines - simple rtree - priority, implement till the end, check what was already existing from cgal impmenentation
        -   polylines - cgal
        -   curves - LNlib
        -   Based on the nearest object implement:
            -   _circles
            -   _length_of_box
            -   _distance_tolerance
            -   _parallel_tolerance
            -   _cross_or_toptoside

- 38 joint crashes on 15_beams_simple_volume_a.gh example

- Solve the joint orientation planarity problem - chevron corner example: 7_assign_directions_and_joint_types.gh


[ 79%] Building CXX object CMakeFiles/wood.dir/src/wood/include/wood_joint_lib.cpp.o
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint_lib.cpp:383:44: warning: array index 2 is past the end of the array (which contains 2 elements) [-Warray-bounds]
            std::swap(jo.joint_volumes[0], jo.joint_lines[2]);
                                           ^              ~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint.h:49:9: note: array 'joint_lines' declared here
        CGAL_Polyline joint_lines[2]; // delete
        ^
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint_lib.cpp:384:44: warning: array index 3 is past the end of the array (which contains 2 elements) [-Warray-bounds]
            std::swap(jo.joint_volumes[1], jo.joint_lines[3]);
                                           ^              ~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint.h:49:9: note: array 'joint_lines' declared here
        CGAL_Polyline joint_lines[2]; // delete
        ^
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint_lib.cpp:2538:44: warning: array index 2 is past the end of the array (which contains 2 elements) [-Warray-bounds]
            std::swap(jo.joint_volumes[0], jo.joint_lines[2]);
                                           ^              ~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint.h:49:9: note: array 'joint_lines' declared here
        CGAL_Polyline joint_lines[2]; // delete
        ^
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint_lib.cpp:2539:44: warning: array index 3 is past the end of the array (which contains 2 elements) [-Warray-bounds]
            std::swap(jo.joint_volumes[1], jo.joint_lines[3]);
                                           ^              ~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_joint.h:49:9: note: array 'joint_lines' declared here
        CGAL_Polyline joint_lines[2]; // delete
        ^
4 warnings generated.
Elapsed time (seconds): 16.3666
[ 82%] Building CXX object CMakeFiles/wood.dir/src/wood/include/wood_test.cpp.o
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2578:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2579:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2580:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2581:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2582:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2583:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2586:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2587:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2588:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2589:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2590:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2591:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2594:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2595:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2596:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2597:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2598:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {10},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2599:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2602:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2603:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2604:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {10},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2605:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2606:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {0},
                    ^~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2607:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2610:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2611:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2612:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2613:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2614:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2615:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2618:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2619:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2620:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2621:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2622:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2623:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {20},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2626:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2627:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2628:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2629:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2630:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {10},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2631:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2634:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2635:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2636:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {10},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2637:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2638:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},
                    ^~~~
/Users/petras/brg/2_code/wood_nano/src/wood/cmake/src/wood/include/wood_test.cpp:2639:21: warning: braces around scalar initializer [-Wbraced-scalar-init]
                    {-1},