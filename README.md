# wood

## Documentation

- Download Doxygen: [http://www.doxygen.nl/download.html](http://www.doxygen.nl/download.html)
- Check install version: `doxygen -v`
- Install VSCode Doxygen package
- Run: `git submodule update --init --recursive`

## ToDo:

- Nearest object for `16_beams_nearest_curve.gh`:
  - Parallel tolerance
  - Issue with multiple segments

- 38 joint crashes on `15_beams_simple_volume_a.gh` example

- Solve the joint orientation planarity problem - chevron corner example: `7_assign_directions_and_joint_types.gh`

## Ubuntu

```sh
sudo rm -rf /home/petras/brg/2_code/wood/cmake/build