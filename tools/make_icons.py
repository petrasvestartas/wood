"""The icons of the docs, docs/doxygen/icons.webp: small models of what each page is about, drawn by icon_ink.py, the parts that join
green. The Rhino plugin's icons (wood_nano/plugin_rhino/plugin/icons) are among them, drawn again as models; a template whose own
model reads at this size is that model, written by its example. One picture of all, in the order it writes into app.js.

    uv run --with moderngl --with numpy --with pillow --with manifold3d tools/make_icons.py [--sheet file.png]"""

import json
import math
import pathlib
import re
import subprocess
import sys

import icon_ink
import manifold3d as m3
import numpy as np
from PIL import Image, ImageDraw

WOOD = pathlib.Path(__file__).resolve().parents[1]
PYTHON = WOOD.parent / "session/session_py/.venv/bin/python"
WORK = WOOD / "build/icons"
CELL, COLUMNS = 160, 8
THIN = {"floor": 0.42, "vidy": 0.42, "developable_folding": 0.42, "reflex_fold": 0.62, "cylindrical_folding": 0.62, "vault": 0.6, "grid": 0.75, "plaited": 0.8, "truss": 0.85}

# The pieces of a scene: names, groups, points and faces where they stand.
PIECES = r"""
import json, sys
from session_py.session import Session
scene = Session.pb_load(sys.argv[1])
paths = {}
def walk(node, path):
    for child in node.children:
        paths[child.name] = path
        walk(child, path + [child.name])
walk(scene.tree.root, [])
pieces = []
for element in scene.objects.elements:
    mesh = element.geometry.mesh() if type(element.geometry).__name__ == "BRep" else element.geometry
    if mesh is None or mesh.number_of_faces() == 0:
        continue
    world = scene.world_xform(element.guid)
    points, faces = mesh.to_vertices_and_faces()
    points = [point.transformed(world) for point in points] if world is not None else points
    pieces.append({"name": element.name, "path": paths.get(element.guid, []), "v": [[p[0], p[1], p[2]] for p in points], "f": [list(face) for face in faces]})
json.dump(pieces, open(sys.argv[2], "w"))
"""


def model(example: str) -> list:
    """The pieces an example writes, kept in build/icons."""
    WORK.mkdir(parents=True, exist_ok=True)
    kept = WORK / f"{example}.json"
    if not kept.exists():
        live = WOOD / "data/output/pb/live.pb"
        before = live.read_bytes() if live.exists() else None
        subprocess.run([str(WOOD / "build" / example)], cwd=WOOD, check=True, capture_output=True)
        subprocess.run([str(PYTHON), "-c", PIECES, str(live), str(kept)], check=True)
        if before is not None:
            live.write_bytes(before)
    return json.loads(kept.read_text())


# ------------------------------------------------------------------ the solids the models are made of


def T(colour: str) -> list:
    """A hex colour as 0..1."""
    return [int(colour[i : i + 2], 16) / 255 for i in (1, 3, 5)]


A, B, C_, G, G2, P_ = T("#efc48d"), T("#e4ae72"), T("#f7dcb2"), T("#4db389"), T("#86d0ad"), T("#fdf0d8")  # three woods, two greens, paper


def box(x0: float, x1: float, y0: float, y1: float, z0: float, z1: float) -> m3.Manifold:
    """A box by its bounds."""
    return m3.Manifold.cube((x1 - x0, y1 - y0, z1 - z0)).translate((x0, y0, z0))


def cyl(a: tuple, b: tuple, r: float, n: int = 20) -> m3.Manifold:
    """A cylinder from a to b."""
    a, b = np.array(a, float), np.array(b, float)
    length = np.linalg.norm(b - a)
    along = (b - a) / length
    across = np.cross((0.0, 0.0, 1.0) if abs(along[2]) < 0.9 else (1.0, 0.0, 0.0), along)
    across /= np.linalg.norm(across)
    return m3.Manifold.cylinder(length, r, r, n).transform(np.c_[across, np.cross(along, across), along, a])


def ext(loop: list, y0: float, y1: float) -> m3.Manifold:
    """A loop in (x, z) extruded along y."""
    solid = m3.CrossSection([np.array(loop, float)], m3.FillRule.EvenOdd).extrude(y1 - y0)
    return solid.transform(np.array([[1, 0, 0, 0], [0, 0, -1, y1], [0, 1, 0, 0]], float))


def flat(loop: list, z0: float, z1: float) -> m3.Manifold:
    """A loop in (x, y) extruded along z."""
    return m3.CrossSection([np.array(loop, float)], m3.FillRule.EvenOdd).extrude(z1 - z0).translate((0, 0, z0))


def hull(points: list) -> m3.Manifold:
    """The convex solid of points."""
    return m3.Manifold.hull_points(np.array(points, float))


def plate(corners: list, thickness: float) -> m3.Manifold:
    """A plate on four corners, along their normal."""
    corners = np.array(corners, float)
    normal = np.cross(corners[1] - corners[0], corners[2] - corners[0])
    return hull(np.r_[corners, corners + normal / np.linalg.norm(normal) * thickness])


def rot(solid: m3.Manifold, x: float = 0, y: float = 0, z: float = 0, at: tuple = (0, 0, 0)) -> m3.Manifold:
    """A solid turned about a point, degrees."""
    return solid.translate(tuple(-np.array(at, float))).rotate((x, y, z)).translate(tuple(at))


def piece(solid: m3.Manifold, colour: list, grain: tuple = None) -> dict:
    """A solid as a piece of icon_ink."""
    mesh = solid.to_mesh()
    return {"v": np.array(mesh.vert_properties)[:, :3].tolist(), "f": np.array(mesh.tri_verts).tolist(), "c": colour, "ax": grain}


# ------------------------------------------------------------------ the models: solids, each with its colour and its grain, and the view


def icons() -> dict:
    """Every icon: its pieces, and the azimuth and elevation it is seen from."""
    I={}
    def add(n,*S,az=-52,el=27): I[n]=([piece(m,c,ax) for m,c,ax in [(s+(None,))[:3] for s in S]],az,el)
    X,Y,Z=(1,0,0),(0,1,0),(0,0,1)
    # chapters
    r=lambda a,b:(cyl(a,b,.45),G)
    add('architecture',r((5,2,9),(5,2,4)),r((1,2,4),(9,2,4)),r((1,2,4),(1,2,3)),r((9,2,4),(9,2,3)),(box(3,7,0,4,8,12),A),(box(-1.6,2.6,0,4,-1,3),B),(box(7.4,11.6,0,4,-1,3),C_),az=-65,el=20)
    add('elements',(box(0,12,0,12,0,1.6),A,X),(box(1.5,4.7,7,10.2,1.6,11.5),B,Z),(box(6.5,10.5,1.5,5.5,1.6,6),C_,X),(cyl((8.5,9,1.6),(8.5,9,5.5),1.3),G))
    d=[(0,0),(12,0),(12,12),(0,12)]; z=[0,0,0,0]
    q=lambda x,y,zz:(x,y,zz); m=(6,6,5)
    add('templates',*[(plate([a,b,m],.55),c) for a,b,c in((q(0,0,0),q(6,0,2.6),A),(q(6,0,2.6),q(12,0,0),B),(q(12,0,0),q(12,6,2.6),C_),(q(12,6,2.6),q(12,12,0),A),(q(12,12,0),q(6,12,2.6),B),(q(6,12,2.6),q(0,12,0),C_),(q(0,12,0),q(0,6,2.6),A),(q(0,6,2.6),q(0,0,0),B))],el=34)
    add('examples',(box(-1,13,0,7,0,1.4)+box(-1,.6,0,7,1.4,5)+box(11.4,13,0,7,1.4,5),A,X),(ext([(4.7,13),(7.3,13),(7.3,7.4),(10,7.4),(6,2.4),(2,7.4),(4.7,7.4)],2.2,4.8),G),az=-68,el=18)
    br=[(4,12),(1.6,12),(.6,11),(.6,7.4),(-.8,6),(.6,4.6),(.6,1),(1.6,0),(4,0),(4,1.8),(2.6,1.8),(2.4,2.1),(2.4,5),(1.5,6),(2.4,7),(2.4,9.9),(2.6,10.2),(4,10.2)]
    add('classes',(ext(br,0,2.2),A,Z),(ext([(14-x,zz) for x,zz in br][::-1],0,2.2),A,Z),(ext([(7,3.6),(9.4,6),(7,8.4),(4.6,6)],0,2.2),G),az=-76,el=12)
    # elements
    add('block',(hull([(0,0,0),(10,0,0),(10,10,0),(0,10,0),(1.2,1.2,9),(8.8,1.2,9),(8.8,8.8,9),(1.2,8.8,9)]),A,X))
    add('support',(box(0,10,0,10,0,1.3),B),(cyl((5,5,1.3),(5,5,7),1.5),C_),(box(1.2,8.8,1.2,8.8,7,8.3),A),*[(cyl((x,y,1.3),(x,y,2),.7),G) for x in(1.6,8.4) for y in(1.6,8.4)])
    add('edge_straight',(box(0,17,0,4,0,5),A,X),az=-58)
    arc=lambda r,a0,a1,n:[(r*math.cos(math.radians(a0+(a1-a0)*k/n)),r*math.sin(math.radians(a0+(a1-a0)*k/n))) for k in range(n+1)]
    add('edge_curved',*[(ext(arc(r1,155,25,24)+arc(r0,25,155,24),0,4),c) for r0,r1,c in((8,9.4,A),(9.4,10.8,B),(10.8,12.2,C_))],az=-62,el=20)
    add('edge_variable',(ext([(0,0),(17,3.6),(17,7),(0,10)],0,3.4),A,X),az=-58,el=22)
    add('face_planar',(box(0,14,0,14,0,1.6),A,X),el=30)
    add('face_curved',*[(ext(arc(12,160-k*30,130-k*30,6)+arc(10.9,130-k*30,160-k*30,6),0,12),c,Y) for k,c in enumerate((A,C_,B,A))],az=-58,el=26)
    add('cut_plane',(box(0,9,0,4,0,5).trim_by_plane((-1,0,-.55),-9.2),A,X),(box(11.5,17,0,4,0,5).trim_by_plane((1,0,.55),12.2),B,X),(plate([(8.1,-3,9),(8.1,7,9),(14.2,7,-2.2),(14.2,-3,-2.2)],.35),G2),az=-60,el=22)
    add('pin',(cyl((0,0,0),(13,0,0),2,28),A,X),(cyl((-.6,0,0),(0,0,0),1.7,28),C_,X),(cyl((13,0,0),(13.6,0,0),1.7,28),C_,X),az=-52,el=26)
    # joints of plates
    base=box(-2,14,-3,7,0,2.2)-box(1.9,5.1,.9,3.1,-1,3)-box(6.9,10.1,.9,3.1,-1,3)
    add('plate_top_side',(base,A,X),(box(0,12,1,3,6.6,14),C_,X),(box(2,5,1,3,4.3,6.6),G),(box(7,10,1,3,4.3,6.6),G),az=-56,el=26)
    tail=lambda y:[(5.8,y-1.05),(9.4,y-1.9),(9.4,y+1.9),(5.8,y+1.05)]
    lft=flat([(0,0),(6,0),(6,13),(0,13)],0,1.8); tl=[flat(tail(y),0,1.8) for y in(3.2,9.8)]; rgt=flat([(6,0),(14,0),(14,13),(6,13)],0,1.8)
    for t_ in tl: rgt=rgt-t_
    add('plate_side_side_inplane',(lft,A,X),*[(t_,G) for t_ in tl],(rgt.translate((4.2,0,0)),C_,X),az=-60,el=38)
    fl=box(0,10,0,1.6,0,12); fr=box(0,1.6,0,11,0,12)
    pl=box(0,1.6,0,10,0,12); 
    s1=box(1.6,11,0,1.6,0,12); s2=box(0,1.6,1.6,11,0,12); fA=[box(0,1.6,0,1.6,k*2.4,(k+1)*2.4) for k in(0,2,4)]; fB=[box(0,1.6,0,1.6,k*2.4,(k+1)*2.4) for k in(1,3)]
    add('plate_side_side_outplane',(s1.translate((2.6,0,0)),A,Z),*[(f.translate((2.6,0,0)),G) for f in fA],(s2,C_,Z),*[(f,G) for f in fB],az=-132,el=24)
    a=box(0,4,0,1.6,0,12); 
    add('plate_side_side_rotated',(box(-9,0,0,1.8,0,12),A,X),(rot(box(0,9,0,1.8,0,12),y=0,z=0).translate((1.2,0,0)).rotate((0,0,38)),C_,X),(box(-2.2,3.2,-.8,2.6,4.4,7.6),G),az=-82,el=22)
    v1=box(0,12,5.2,6.8,0,11)-box(5.1,6.9,5,7,5.5,12); v2=box(5.2,6.8,0,12,0,11)-box(5,7,5.1,6.9,-1,5.5)
    add('plate_cross',(v1,A,X),(v2.translate((0,0,7.2)),C_,Y),az=-40,el=30)
    add('plate_top_top',(box(0,12,0,12,0,1.8),A,X),*[(cyl((x,y,1.8),(x,y,6),.9),G) for x in(3,9) for y in(3,9)],(box(0,12,0,12,7.4,9.2)-m3.Manifold.batch_boolean([cyl((x,y,7),(x,y,10),.9) for x in(3,9) for y in(3,9)],m3.OpType.Add),C_,X))
    add('plate_border',(box(2.4,11.6,2.4,11.6,0,1.6),A,X),*[(flat(P,0,2.6),G) for P in([(0,0),(14,0),(11.6,2.4),(2.4,2.4)],[(14,0),(14,14),(11.6,11.6),(11.6,2.4)],[(14,14),(0,14),(2.4,11.6),(11.6,11.6)],[(0,14),(0,0),(2.4,2.4),(2.4,11.6)])],el=36)
    # joints of beams
    low=box(0,16,0,4,0,4)-box(6,10,-1,5,2,5); upb=(box(6,10,-6,10,0,4)-box(5,11,0,4,-1,2)).translate((0,0,5.2))
    add('beam_cross',(low,A,X),(upb,C_,Y),az=-52,el=25)
    add('beam',(box(0,4,0,4,-9,0),A,Z),(box(-5,12,0,4,0,4),C_,X),(cyl((2,-.6,2),(2,0,2),.9),G),az=-58,el=18)
    add('beam_top_side',(box(0,16,0,4.4,0,4.4)-box(6.4,9.6,1.2,3.2,1,5),A,X),(box(5.8,10.2,0,4.4,8.6,15),C_,Z),(box(6.5,9.5,1.3,3.1,5.4,8.6),G),az=-58,el=22)
    add('beam_bridle',(box(0,16,0,4.4,0,4.4)-box(6,10,-1,1.3,-1,5)-box(6,10,3.1,5,-1,5),A,X),(box(6,10,0,4.4,9,15),C_,Z),(box(6,10,0,1.3,5,9),G),(box(6,10,3.1,4.4,5,9),G),az=-58,el=22)
    add('beam_column_head',(box(4,8,4,8,-12,0),A,Z),(box(-3,15,4.4,7.6,0,3.2),C_,X),(box(4.4,7.6,-3,4.4,0,3.2),B,Y),(box(4.4,7.6,7.6,15,0,3.2),B,Y),*[(hull([(6+sx*2,5,0),(6+sx*2,7,0),(6+sx*6,5,0),(6+sx*6,7,0),(6+sx*2,5,-4.5),(6+sx*2,7,-4.5)]),G) for sx in(1,-1)],az=-58,el=14)
    pg=lambda s:hull([(0,0,0),(s*7.2,0,1.3),(s*7.2,10,1.3),(0,10,0),(0,0,1.2),(s*7.2,0,3),(s*7.2,10,3),(0,10,1.2)])
    add('beam_compendium',(box(-7.8,7.8,-.6,10.6,-.9,-.2),G),(pg(-1),C_,Y),(pg(1),P_,Y),*[(hull([(x,y,1.62+abs(x)*.25),(x+2.6,y,1.62+abs(x+2.6)*.25),(x+2.6,y+.55,1.62+abs(x+2.6)*.25),(x,y+.55,1.62+abs(x)*.25),(x,y,1.2),(x+2.6,y+.55,1.2)]),B) for x in(-5.6,) for y in(2.4,4.6,6.8)],az=-90,el=44)
    add('beam_todo',(box(0,10,0,1.4,0,13),A,Z),(box(3,7,-.5,1,12.2,14),G),*[(box(4.2,8.4,-.35,0,z_,z_+.9),B) for z_ in(9.2,6.2,3.2)],*[(box(1.4,2.9,-.4,0,z_-.2,z_+1.2),G) for z_ in(9.2,6.2,3.2)],az=-74,el=10)
    add('beam_keyed_box',(box(0,2.4,-4,9,-4,9)-box(-1,3,.9,4.1,.9,4.1),A,Z),(box(-8,0,0,5,0,5),C_,X),(box(0,7.4,1,4,1,4)-box(3,4.6,1.8,3.2,-1,6),B,X),(hull([(3,1.8,-2),(4.4,1.8,-2),(4.8,1.8,6.6),(3,1.8,6.6),(3,3.2,-2),(4.4,3.2,-2),(4.8,3.2,6.6),(3,3.2,6.6)]),G),az=-40,el=22)
    add('beam_end_to_end',(ext([(0,0),(5.4,0),(5.4,1.4),(12.2,3.4),(12.2,5),(0,5)],0,4),A,X),(ext([(5.4,0),(17.6,0),(17.6,5),(12.2,5),(12.2,3.4),(5.4,1.4)],0,4).translate((3,0,-2.4)),C_,X),(box(8.6,10.2,-2.4,0,1.3,2.9).translate((.9,0,-.6)),G),az=-60,el=20)
    add('beam_butt',(box(0,4,-7,11,0,5),A,Y),(box(4,15,0,4,0,5),C_,X),(cyl((-3.4,2,1.4),(0,2,1.4),.55),G),(cyl((-3.4,2,3.6),(0,2,3.6),.55),G),az=-128,el=26)
    add('connectors',(cyl((-6,0,0),(16,0,0),.7),G),(plate([(0,-5,-5),(0,5,-5),(0,5,5),(0,-5,5)],1.5),A,Z),(plate([(9,-5,-5),(9,5,-5),(9,5,5),(9,-5,5)],1.5),C_,Z),az=-38,el=22)
    add('beam_pins',(box(0,16,0,4.6,0,5),A,X),*[(cyl((x,-3.4,2.5),(x,0,2.5),.8),G) for x in(3,8,13)],az=-60,el=24)
    # templates
    add('translation_shell',*[(plate([(x*4,y*4,f(x,y)),((x+1)*4,y*4,f(x+1,y)),((x+1)*4,(y+1)*4,f(x+1,y+1)),(x*4,(y+1)*4,f(x,y+1))],.6),(A,C_,B)[(x+2*y)%3]) for f in[lambda x,y:6-.95*(x-2)**2-.75*(y-1.5)**2] for x in range(4) for y in range(3)],el=26)
    zz=[(0,0),(2.4,3.4)]
    def folded(prof,n,w,t=.5,amp=1.2):
        S=[]
        for k in range(n):
            for i in range(len(prof)-1):
                o=lambda j,yy,hi:(prof[j][0]+(prof[j][2]*amp if hi else 0),yy,prof[j][1]+(prof[j][3]*amp if hi else 0))
                S.append((plate([o(i,k*w,0),o(i+1,k*w,0),o(i+1,(k+.5)*w,1),o(i,(k+.5)*w,1)],t),A)); S.append((plate([o(i,(k+.5)*w,1),o(i+1,(k+.5)*w,1),o(i+1,(k+1)*w,0),o(i,(k+1)*w,0)],t),C_))
        return S
    add('reflex_fold',*folded([(0,0,-.9,0),(2,8,-.7,.7),(12,8,.7,.7),(14,0,.9,0)],4,2.6),az=-66,el=24)
    pr=[(7-9*math.cos(math.radians(a_)),9*math.sin(math.radians(a_)),-math.cos(math.radians(a_)),math.sin(math.radians(a_))) for a_ in(0,36,72,108,144,180)]
    add('cylindrical_folding',*folded(pr,3,3.8,amp=1.3),az=-64,el=22)
    F=model('templates_developable_folding_12_corrugated_funnel')
    I['developable_folding']=([{**p,'c':(A,C_)[k%2],'ax':Z} for k,p in enumerate(F)],-60,8)
    ch=[]; n=5
    for k in range(n):
        a0,a1,a2=[math.radians(175-170*(k+j*.5)/n) for j in(0,1,2)]; R_=9
        pt=lambda a_,y:(R_*math.cos(a_),y,R_*math.sin(a_)); dl=math.radians(170*.32/n)
        for y0,y1,s0,s1,c in((0,4,0,1,A),(4,8,1,0,C_)):
            ch.append((plate([pt(a0-dl*s0,y0),pt(a2-dl*s0,y0),pt(a2-dl*s1,y1),pt(a0-dl*s1,y1)],-.9),c))
    add('chevron',*ch,az=-60,el=24)
    add('cross',(plate([(0,0,0),(9,0,0),(9,11,0),(0,11,0)],1.4),A,X),(rot(plate([(9,0,0),(18,0,0),(18,11,0),(9,11,0)],1.4),y=34,at=(9,0,0)),C_,X),*[(rot(box(7,11,y,y+1,-2.2,3.6),y=17,at=(9,0,.7)),G) for y in(2.5,7.5)],az=-64,el=30)
    house=ext([(0,0),(12,0),(12,7),(6,12.2),(0,7)],0,10)-box(4.2,7.8,-1,1.2,-1,5.6)-box(11,13,3.4,6.8,2.4,5.4)
    add('module',(house,A,Z),(plate([(-1,-.8,6.4),(6,-.8,12.5),(6,10.8,12.5),(-1,10.8,6.4)],-.9),C_,Y),(plate([(6,-.8,12.5),(13,-.8,6.4),(13,10.8,6.4),(6,10.8,12.5)],-.9),B,Y),(box(4.2,7.8,.4,1.2,0,5.6),G),az=-56,el=18)
    pw=[]
    for k in range(4): pw.append((rot(box(-3,10,2.2,4.2,0,4.4),z=90*k),(A,C_)[k%2],(X,Y)[k%2]))
    add('reciprocal_rotation',*pw,el=30)
    add('reciprocal_move',(box(-2,14,-5,-3,0,4.6),A,X),(box(-2,14,7,9,0,4.6),A,X),(box(1,3,-3,11,.0,4.6).translate((0,-4,0))-box(0,4,-9,-3,-1,6),C_,Y),(box(8.6,10.6,-3,7,0,4.6),C_,Y),(box(2.4,4.4,-3,7,0,4.6),C_,Y),el=32)
    gr=[]
    for x in(0,7,14):
        for y in(0,7): gr.append((box(x,x+1.5,y,y+1.5,0,12),A,Z))
    for zc in(5,12):
        for y in(0,7): gr.append((box(-.4,15.9,y,y+1.5,zc,zc+1.6),C_,X))
        for x in(0,7,14): gr.append((box(x,x+1.5,1.5,7,zc,zc+1.6),B,Y))
    add('grid',*gr,(box(0,15.5,0,8.5,6.6,7.1),C_,X),az=-58,el=18)
    H=[p for p in model('templates_floor_7_contacts_cantilevers') if not re.search('_head_|pin|cutters', p['name']) and p['name'] != 'support']
    I['floor']=([{**p,'c':G if p['name'].startswith(('connector','column_plate')) else (C_ if p['name'].startswith('beds') else A)} for p in H],-60,24)
    bm=lambda a_,b_,w,c,yy=0:(hull([(a_[0]+s*nx*w+0,y,a_[1]+s*nz*w) for nx,nz in[(-(b_[1]-a_[1])/math.hypot(b_[0]-a_[0],b_[1]-a_[1]),(b_[0]-a_[0])/math.hypot(b_[0]-a_[0],b_[1]-a_[1]))] for s in(-1,1) for y in(yy,yy+1.8)]+[(b_[0]+s*nx*w,y,b_[1]+s*nz*w) for nx,nz in[(-(b_[1]-a_[1])/math.hypot(b_[0]-a_[0],b_[1]-a_[1]),(b_[0]-a_[0])/math.hypot(b_[0]-a_[0],b_[1]-a_[1]))] for s in(-1,1) for y in(yy,yy+1.8)]),c)
    add('truss',bm((0,0),(20,0),.7,A),bm((0,0),(10,8.4),.7,A),bm((20,0),(10,8.4),.7,A),bm((10,0),(10,8.4),.6,C_),bm((5,4.2),(10,0),.55,C_),bm((15,4.2),(10,0),.55,C_),*[(cyl((x,-.5,z_),(x,2.3,z_),.95),G) for x,z_ in((1,.2),(19,.2),(10,8),(10,0),(5,4.2),(15,4.2))],az=-72,el=12)
    V_=model('templates_vault_9_gothic_ribbed_vault'); I['vault']=([{**p,'c':(A,C_,B)[k%3]} for k,p in enumerate([p for p in V_ if p['path'] and p['path'][0]=='star_stellar_burgos'])],-58,18)
    pl_=[]
    for k,y in enumerate((0,5,10)): pl_.append((box(-2,14,y,y+2.6,0,2),A,X))
    for i,x in enumerate((0,5,10)):
        for k,y in enumerate((0,5,10)):
            up=(i+k)%2==0; pl_.append((box(x,x+2.6,y-1.2,y+3.8,2 if up else -2,4 if up else 0),C_,Y))
    add('plaited',*pl_,el=40)
    VD=model('templates_vidy_4_section_joints'); sel=[p for p in VD if p['path'] and p['path'][0] in('outer_walls','outer_roof','inner_walls','inner_roof')]
    cy=[np.array(p['v'])[:,:].mean(0) for p in sel]; ax_=int(np.argmax(np.ptp(np.array(cy),axis=0))); lo_=min(c[ax_] for c in cy); ran=np.ptp(np.array(cy)[:,ax_])
    I['vidy']=([{**p,'c':(A,C_,B)[k%3]} for k,(p,c) in enumerate(zip(sel,cy)) if c[ax_]<lo_+ran*.36],-118 if ax_==1 else -58,16)
    # what the solver does
    add('joint_type',(rot(box(-6,6,-6,6,0,1.8),z=45),A,X),*[(cyl((x,y,1.8),(x,y,2.9),1.5),G) for x,y in((4.3,4.3),(-4.3,4.3),(4.3,-4.3),(-4.3,-4.3))],az=-90,el=42)
    arw=lambda:flat([(0,-.7),(5,-.7),(5,-2),(8,0),(5,2),(5,.7),(0,.7)],0,1.2)
    add('insertion_direction',(box(0,11,0,11,0,1.8),A,X),(arw().rotate((0,0,180)).translate((20,3,.3)),G),(arw().translate((-9,8,.3)),G),el=40)
    zig=[(0,6.6),(6,12.7),(12,6.8),(18,12.7),(24,6.6),(24,0),(0,0)]
    up_=[(x,24-zz_) for x,zz_ in zig]; dn=[(x,24-zz_) for x,zz_ in [(0,12.7),(6,18.7),(12,12.8),(18,18.7),(24,12.7),(24,24),(0,24)]]
    add('solver',(ext(up_,0,5),C_,X),(ext(dn,0,5),A,X),az=-74,el=14)
    return I


def picture(name: str, pieces: list, azimuth: float, elevation: float) -> Image.Image:
    """An icon drawn, the sun always over the left shoulder."""
    side = math.radians(azimuth - 48.0)
    return icon_ink.draw(pieces, CELL, azimuth, elevation, CELL / 74.0 * THIN.get(name, 1.0), (math.cos(side), math.sin(side), 1.25))


def main(arguments: list) -> None:
    """Draws every icon into one picture and writes their order into app.js."""
    drawn = {name: picture(name, *spec) for name, spec in icons().items()}
    names = sorted(drawn)
    atlas = Image.new("RGBA", (COLUMNS * CELL, math.ceil(len(names) / COLUMNS) * CELL), (0, 0, 0, 0))
    for k, name in enumerate(names):
        atlas.paste(drawn[name], (k % COLUMNS * CELL, k // COLUMNS * CELL))
    atlas.save(WOOD / "docs/doxygen/icons.webp", quality=88, method=6, exact=False)
    script = WOOD / "docs/doxygen/app.js"
    order = f'  const ORDER = "{" ".join(names)}".split(" "); // of icons.webp, {COLUMNS} a row, written by make_icons.py'
    script.write_text(re.sub(r"  const ORDER = .*", lambda _: order, script.read_text()))
    print(len(names), "icons", (WOOD / "docs/doxygen/icons.webp").stat().st_size)
    if "--sheet" in arguments:
        sheet = Image.new("RGB", (9 * 190, math.ceil(len(names) / 9) * 200), "white")
        for k, name in enumerate(names):
            x, y = k % 9 * 190, k // 9 * 200
            sheet.paste(drawn[name], (x + 15, y + 8), drawn[name])
            ImageDraw.Draw(sheet).text((x + 15, y + 176), name, fill=(70, 70, 70))
        sheet.save(arguments[arguments.index("--sheet") + 1])


if __name__ == "__main__":
    main(sys.argv[1:])
