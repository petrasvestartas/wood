"""
pb_to_3dm.py — load a session .pb file and export a Rhino .3dm for visualization.
Uses session_py for parsing (proper high-level API) + rhino3dm for writing.
Usage: python pb_to_3dm.py <input.pb> [output.3dm]
"""
import sys
sys.path.insert(0, r"C:\pc\3_code\code_rust\session\session_py\src")

from session_py.session import Session
import rhino3dm


def session_to_3dm(sess, out_path):
    model = rhino3dm.File3dm()
    objs  = sess.objects

    # ── Polylines ─────────────────────────────────────────────────
    for pl in objs.polylines:
        raw_pts = pl.get_points()
        if len(raw_pts) < 2:
            continue
        pts = [rhino3dm.Point3d(float(p[0]), float(p[1]), float(p[2]))
               for p in raw_pts]
        curve = rhino3dm.PolylineCurve(pts)
        attr = rhino3dm.ObjectAttributes()
        attr.Name = pl.name
        model.Objects.AddCurve(curve, attr)

    # ── Meshes ────────────────────────────────────────────────────
    for m in objs.meshes:
        rm = rhino3dm.Mesh()

        # Vertices in sorted key order
        vk_sorted = sorted(m.vertex.keys())
        vk_to_idx = {vk: i for i, vk in enumerate(vk_sorted)}
        for vk in vk_sorted:
            vd = m.vertex[vk]
            rm.Vertices.Add(float(vd.x), float(vd.y), float(vd.z))

        # Faces: m.face[fk] is list of vertex keys
        for fk in sorted(m.face.keys()):
            vl = [vk_to_idx[v] for v in m.face[fk]]
            if len(vl) == 3:
                rm.Faces.AddFace(vl[0], vl[1], vl[2])
            elif len(vl) == 4:
                rm.Faces.AddFace(vl[0], vl[1], vl[2], vl[3])

        rm.Normals.ComputeNormals()
        rm.Compact()
        attr = rhino3dm.ObjectAttributes()
        attr.Name = m.name
        model.Objects.AddMesh(rm, attr)

    # ── NURBS surfaces ────────────────────────────────────────────
    for ns in objs.nurbssurfaces:
        order_u = ns.order(0)   # degree + 1
        order_v = ns.order(1)
        n_u     = ns.cv_count(0)
        n_v     = ns.cv_count(1)
        is_rat  = ns.is_rational()

        rns = rhino3dm.NurbsSurface.Create(3, is_rat, order_u, order_v, n_u, n_v)
        if rns is None:
            continue

        for i, k in enumerate(ns.get_nurbsknots(0)):
            rns.KnotsU[i] = k
        for j, k in enumerate(ns.get_nurbsknots(1)):
            rns.KnotsV[j] = k

        for i in range(n_u):
            for j in range(n_v):
                if is_rat:
                    ok, x, y, z, w = ns.get_cv_4d(i, j)
                    rns.Points[i, j] = rhino3dm.Point4d(x, y, z, w)
                else:
                    cv = ns.get_cv(i, j)
                    rns.Points[i, j] = rhino3dm.Point4d(cv[0], cv[1], cv[2], 1.0)

        attr = rhino3dm.ObjectAttributes()
        attr.Name = ns.name
        model.Objects.AddSurface(rns, attr)

    model.Write(out_path, 8)
    print(f"Written: {out_path}")
    print(f"  polylines:      {len(objs.polylines)}")
    print(f"  meshes:         {len(objs.meshes)}")
    print(f"  nurbs surfaces: {len(objs.nurbssurfaces)}")


if __name__ == "__main__":
    pb_path  = sys.argv[1] if len(sys.argv) > 1 \
               else r"C:\pc\3_code\code_cpp\wood\data\output\annen_chevron.pb"
    out_path = sys.argv[2] if len(sys.argv) > 2 \
               else pb_path.replace(".pb", ".3dm")
    sess = Session.pb_load(pb_path)
    print(f"Session: {sess.name}")
    session_to_3dm(sess, out_path)
