# Truss joinery: the beam library applied to trusses

Goal: the trusses are an application of the beam joint library (`data/joints/*.yaml`). A jointed truss has no butt anywhere:
every meeting of two members is a joint of the library, chosen for what the node is and for whether the members push or
pull, and the whole truss goes together in an `AssemblySequence` with nothing locked and no collision.

## 1. The three steps of `Truss`

1. **Line graph**: the guide's chords and webs, axes meeting concentrically at every node (required structurally; no
   eccentric seating).
2. **Members meet**: the members on those axes; today the butts. They stay the first draft of every node, so the geometry of
   who stops on whom is known.
3. **Joinery**: every meeting gets a library joint in place of its butt (section 3), each member's joints share one way in
   (section 5), and the result is checked (section 6).

## 2. What the template must know first

- **Axial forces.** The joint depends on whether a member pushes or pulls (research: carpentry joints carry compression by
  bearing; tension only through dovetail shoulders or pegs, at low capacity). A pin-jointed solve of the line graph under
  a unit load at every top chord node, supports at the two heels, gives each member's sign. Wind can reverse webs: a web
  whose sign flips under a one-sided load is treated as tension (it needs a joint that holds both ways).
- **Node type** of every node, read from the graph: heel, apex (with or without a post), post foot, chord node with one web,
  with two webs, with three or more, strut on post, chord kink, web on web, web crossing.

## 3. The joint for every node (book pages)

| Node | Joint | Why | Way in |
|---|---|---|---|
| Heel, top chord on tie | oblique thrust with notched heel (590); double notched (592) under ~30° pitch | most loaded joint; bearing notch leaves the tie its section; the tie runs on past the notch | face |
| Apex with king post | notched oblique thrust (584) on two opposite sides of the post, post running on above | rafters bear, post hangs | face |
| Apex without post | mitred bridle / bridle (corner chapter) | both in compression, the joint only holds | face |
| King post foot (tie, post, struts) | post into tie: wedged dovetail half lap (570) or pegged through mortise and tenon (504); struts on the post: 584 / 586 | post in tension, struts bear on its sloped shoulder | 570 face; tenon along the post |
| Chord node, one compression web | notched oblique thrust (584), double (592), with tenon (600) | standard Versatz | face (600 axial) |
| Chord node, one tension web | diagonal: pegged angled half-dovetail laps (616–622) or lapped mortise and tenon (612); vertical: dovetail half lap (440 / 442 / 448) or wedged dovetail half lap (570) | dovetail shoulders carry the pull | face |
| Chord node, two webs | the tension web one face lap (440 / 570 / 618); the compression web an edge notch (584) into the chord or bearing on the other web's side (586) | one face lap and one edge notch fit one concentric node; two face laps would cut the chord through | vertical first, diagonal after |
| Chord node, three or more webs | one lap at most, the others bear on each other (586) | the only all-wood way | ordered |
| Strut on post | 584 / 586 on the post; strut head into the rafter 596 or 584 | bearing face square to the thrust | face |
| Queen post head, straining beam | beam into posts: mortise and tenon (492–506, haunched 516–524); post into rafter: sloped mortise and tenon (598) | straining beam in compression | axial |
| Chord kink, compression | mitred bridle, or a post running through the kink with both runs bearing on it (584) | | face |
| Chord kink, tension | the chord runs on through the kink (no joint), or a splice from the splicing chapter | | |
| Web on web (K) | compression: 586 on the vertical; tension: 618 | | face |
| Web crossing | oblique cross lap (cross lap chapter, pp. 646–707) | Town lattice: interlocking notches | face, alternating |

Variety is part of the goal: where two joints fit a node equally, the template takes the next one of the family each time,
so a truss shows the family (584, 586, 588, 592 ...), and a joint is never repeated by accident where the book offers more.

## 4. Which trusses get joinery

From the research (TFEC 4-2020 timber truss guide, FHWA covered bridge truss types, Building Construction, Baunetz):

- **All-wood, joinery throughout**: king post with struts, queen post (with braces in the middle bay), Howe (verticals as
  dovetail or tusk joints), multiple king post, scissors with a king post, arched or cambered chords with a post through
  every kink.
- **With care**: Fink, Belgian (half the webs pull), bowstring (hangers pull), Warren with verticals, Town lattice (a cross
  lap at every crossing).
- **Not sensible in all-wood** (tension diagonals): Pratt, Parker, Pennsylvania, Baltimore, K-truss, Warren without
  verticals. Historically wood and iron. Decision (2026-11-23): jointed anyway, no butts, tension webs in dovetail laps
  (616–622, 440 / 570), the docs marking their joints as under-strength.

Decision (2026-11-23): the king post truss is done first, end to end.

## 5. Assembly: one way in per member

The order of a real truss (assembled flat, then raised): the tie flat; the posts into the tie; the top chords dropped in
from the face onto the heel and post notches; the struts and diagonals dropped in from the face, tension laps last; pegs.

Rule for the template: all joints of one member share a way in. Face-open joints (584–592, 602–622, laps, cross laps) all go
in from the same face; a tenon forces the member's axis, so only one end of a member may be tenoned, and its other joints
must accept that axis. The joint choice per node is checked against this before any joint is built.

## 6. Fit and checks

- Each joint is cut with a fit clearance (a library parameter, e.g. 0.5 mm per bearing face), so the assembly sweep (10 mm3
  clearance today) does not read a tight joint as a collision: today every web overlaps its chord by ~12000 mm3 at rest.
- `joint_butt` (no member overlaps another, none loses wood no member fills) for every jointed truss, and an
  `AssemblySequence` with no member locked; the order compared with section 5.
- Every jointed truss pictured with its assembly GIF (`tools/render_family_gifs.py`), nodes close up on the truss page.

## 7. Order of work

1. Axial force signs from the line graph (a small pin-jointed solve), node types read from the graph.
2. The king post truss first (it already has `CarpentryTruss`): heel 590, apex 584 on the post, post foot 570, struts 586.
   No butt; assembly passes.
3. Queen post, Howe, scissors, multiple king post: the remaining node types of section 3.
4. Arched chords (posts through kinks), Fink, Belgian, bowstring, Warren with verticals, Town lattice (cross laps).
5. The library fit clearance; the assembly test for every jointed truss in CI.
6. Docs: a truss joinery page, one row per truss (GIF, picture, the joints it uses with their pages).

## 8. As built (2026-11-24)

The user's decisions after the first rounds: crowded nodes take a slotted-in plate with pins (their reference: a Schlitzblech
node), a truss is one system, plates at every node with members split at every node, the plate in the middle of the thickness;
the book's joints alone stay in the carpentry king post (`CarpentryTruss`). `Truss::add_joinery` keeps the book path
(`add_book_joinery`) for a truss whose every node is one member ending on another, which none of the library has; every library
truss takes `add_plated_joinery`:

- members flush faced and split at every node on them, short pieces hanging past a node removed;
- chords mitered (`butt_side_side`), a heel (under 35 degrees) the rafter running on, bearing on the tie, one plane cutting both;
- webs flush on the chords, mitered with each other, bearing under 35 degrees;
- one plate per node (`JointBeamElement::node_plate`, convex hull of the member strips, middle of the thickness), slotted along
  its way in the plane (square to the chord, else the clear way farthest off the node's members), from the face at a node too
  dense for 45 degrees; one row of two pins per member (`slotted_plate`);
- `tests/joint_butt.cpp` checks every truss whole and assembled; six are listed unfinished (crowded plates overlapping, chords
  whose two plates leave no common way in, a short piece losing wood).

## 9. Sources

- TFEC 4-2020, Design Guide for Timber Roof Trusses: https://www.carolinatimberworks.com/wp-content/uploads/2020/08/Design-Guide-for-Timber-Roof-Trusses.pdf
- FHWA, covered bridge truss types: https://www.fhwa.dot.gov/publications/research/infrastructure/structures/04098/04.cfm
- Building Construction (chestofbooks): parts of a king post roof, oblique tenons, strut and beam joints
- Baunetz, zimmermannsmäßige Verbindungen: https://www.baunetzwissen.de/holz/fachwissen/verbindungen/zimmermannsmaessige-verbindungen-7105522
- timberframehq.com king post assembly; Small Farmer's Journal raising day; fonerbooks king post
- Graphic statics of framed structures, ch. 2–3 (the user's library)
