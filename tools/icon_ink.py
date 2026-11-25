"""Small drawings of solids in one hand: a sun and its shadows in flat tones, wood grain, brown ink lines, on no background. Used by
make_icons.py; needs a GPU (moderngl over EGL).

A piece is a dict: v its points, f its faces, c its colour (0..1), ax the direction of its grain (its longest one when None)."""

import math

import moderngl
import numpy as np
from PIL import Image, ImageFilter

VERTICES = """#version 330
in vec3 p; in vec3 n; in vec3 c; in vec3 g; in float id; uniform mat4 M; out vec3 P,N,C,G; flat out float I;
void main(){P=p;N=n;C=c;G=g;I=id;gl_Position=M*vec4(p,1);}"""

# the tones of a face: in the sun, in the shade or in a shadow, its grain where it is no end
FRAGMENTS = """#version 330
in vec3 P,N,C,G; flat in float I; uniform vec3 eye; uniform vec3 sun; uniform mat4 SM; uniform sampler2D sh; uniform float unit; uniform vec3 amb; uniform vec3 key; uniform float grain; uniform float zlo; uniform float zhi;
layout(location=0) out vec4 col; layout(location=1) out vec4 gb;
float h(float x){return fract(sin(x*91.345)*47453.5);} 
float shadow(vec3 w,vec3 n){vec4 s=SM*vec4(w+n*unit*.004,1); vec3 q=s.xyz*.5+.5; if(q.x<0||q.y<0||q.x>1||q.y>1) return 1.; float r=0; vec2 px=1./vec2(textureSize(sh,0)); for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++) r+= texture(sh,q.xy+vec2(i,j)*px*.8).r+.0012>q.z?1.:0.; return r/9.;}
void main(){ vec3 n=normalize(N); vec3 v=normalize(eye-P); if(dot(n,v)<0) n=-n;
 float l=dot(n,sun); float s=l>0?shadow(P,n):0.; float lit=smoothstep(.02,.07,l)*smoothstep(.35,.65,s);
 float half_=smoothstep(.42,.5,l)*lit;
 // grain: lines along the piece
 vec3 a=normalize(G); vec3 u=abs(a.z)<.9?normalize(cross(a,vec3(0,0,1))):vec3(1,0,0); vec3 w2=cross(a,u);
 float t=dot(P,u)+.73*dot(P,w2); float al=dot(P,a); float endg=abs(dot(n,a));
 float f=t/unit*95.+.8*sin(al/unit*7.+h(I)*9.)+h(I)*20.; float grn=smoothstep(.86,.97,abs(fract(f)*2.-1.))*step(endg,.6)*grain;
 vec3 base=C*(.93+.14*h(I+3.)); vec3 shade=base*amb; vec3 c=mix(shade,base*key,lit); c=mix(c,min(base*key*1.13+.03,vec3(1)),half_);
 c*=1.-.13*grn; c=mix(c,c*vec3(1.0,.93,.86),clamp(1.-(P.z-zlo)/(zhi-zlo),0,1)*.35);
 col=vec4(c,1); gb=vec4(n*.5+.5,I+1.); }"""

DEPTHS = """#version 330
in vec3 p; uniform mat4 M; void main(){gl_Position=M*vec4(p,1);}"""


def look(eye: np.ndarray, at: np.ndarray) -> np.ndarray:
    """The view from eye to at, z up."""
    forward = at - eye
    forward = forward / np.linalg.norm(forward)
    right = np.cross(forward, (0.0, 0.0, 1.0))
    right = right / np.linalg.norm(right)
    view = np.eye(4)
    view[0, :3], view[1, :3], view[2, :3] = right, np.cross(right, forward), -forward
    view[:3, 3] = -view[:3, :3] @ eye
    return view


def perspective(angle: float, aspect: float, near: float, far: float) -> np.ndarray:
    """A perspective of a vertical angle, degrees."""
    scale = 1.0 / math.tan(math.radians(angle) / 2.0)
    matrix = np.zeros((4, 4))
    matrix[0, 0], matrix[1, 1] = scale / aspect, scale
    matrix[2, 2], matrix[2, 3], matrix[3, 2] = (far + near) / (near - far), 2.0 * far * near / (near - far), -1.0
    return matrix


def parallel(radius: float, far: float) -> np.ndarray:
    """A parallel projection of a square, for the shadows."""
    matrix = np.eye(4)
    matrix[0, 0] = matrix[1, 1] = 1.0 / radius
    matrix[2, 2], matrix[2, 3] = -2.0 / far, -1.0
    return matrix


def triangles(pieces: list) -> tuple:
    """Every triangle's points, normals, colours, grain directions and piece numbers."""
    points, normals, colours, grains, numbers = [], [], [], [], []
    for number, piece in enumerate(pieces):
        vertices = np.array(piece["v"], float)
        grain = piece.get("ax")
        if grain is None:
            grain = np.linalg.svd(vertices - vertices.mean(0), full_matrices=False)[2][0]
        for loop in piece["f"]:
            corners = vertices[loop]
            normal = sum(np.cross(a, b) for a, b in zip(corners, np.roll(corners, -1, axis=0)))
            length = np.linalg.norm(normal)
            if length < 1e-9:
                continue
            for i in range(1, len(loop) - 1):
                for k in (0, i, i + 1):
                    points.append(corners[k])
                    normals.append(normal / length)
                    colours.append(piece["c"])
                    grains.append(grain)
                    numbers.append(number)
    return tuple(np.array(values, dtype="f4") for values in (points, normals, colours, grains, numbers))


def framed(pieces: list, azimuth: float, elevation: float, angle: float = 14.0) -> tuple:
    """The eye and the point it looks at so the pieces fill a square, seen from an azimuth and an elevation, degrees."""
    points = np.concatenate([np.array(piece["v"], float) for piece in pieces])
    centre = (points.min(0) + points.max(0)) / 2.0
    a, e = math.radians(azimuth), math.radians(elevation)
    way = np.array([math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e)])
    right = np.cross(-way, (0.0, 0.0, 1.0))
    right = right / np.linalg.norm(right)
    above = np.cross(right, -way)
    distance = np.linalg.norm(points - centre, axis=1).max() / math.sin(math.radians(angle) / 2.0)
    for _ in range(40):
        seen = np.c_[points, np.ones(len(points))] @ (perspective(angle, 1.0, distance * 0.01, distance * 4.0) @ look(centre + way * distance, centre)).T
        seen = seen[:, :2] / seen[:, 3:4]
        distance *= np.abs(seen).max() / 0.926
        middle = (seen.min(0) + seen.max(0)) / 2.0
        centre = centre + (right * middle[0] + above * middle[1]) * distance * math.tan(math.radians(angle) / 2.0) * 0.8
    return centre + way * distance, centre


def thickened(mask: np.ndarray, pixels: float) -> np.ndarray:
    """A mask grown to lines of a width."""
    size = int(round(pixels))
    size += 1 - size % 2
    image = Image.fromarray((mask * 255).astype("u1"))
    return np.asarray(image.filter(ImageFilter.MaxFilter(size)) if size > 1 else image).astype("f4") / 255.0


def draw(pieces: list, size: int, azimuth: float, elevation: float, line: float, sun: tuple, angle: float = 14.0, fine: int = 4) -> Image.Image:
    """The pieces as a square picture, drawn fine times larger and averaged; line is the width of the inner lines."""
    ink, shade, light = (0.36, 0.22, 0.12), (0.88, 0.78, 0.68), (1.0, 0.985, 0.965)
    eye, at = framed(pieces, azimuth, elevation, angle)
    points, normals, colours, grains, numbers = triangles(pieces)
    low, high = points.min(0), points.max(0)
    centre, radius = (low + high) / 2.0, np.linalg.norm(high - low) / 2.0
    sun = np.array(sun, float) / np.linalg.norm(sun)
    gl = moderngl.create_standalone_context(backend="egl")
    gl.enable(gl.DEPTH_TEST)
    drawing = gl.program(VERTICES, FRAGMENTS)
    casting = gl.program(DEPTHS, "#version 330\nvoid main(){}")
    faces = gl.vertex_array(drawing, [(gl.buffer(np.c_[points, normals, colours, grains, numbers].astype("f4").tobytes()), "3f 3f 3f 3f 1f", "p", "n", "c", "g", "id")])
    casters = gl.vertex_array(casting, [(gl.buffer(points.tobytes()), "3f", "p")])

    # what the sun sees
    from_sun = parallel(radius * 1.05, radius * 4.0) @ look(centre + sun * radius * 2.0, centre)
    shadows = gl.depth_texture((4096, 4096))
    shadows.compare_func = ""
    gl.framebuffer(depth_attachment=shadows).use()
    gl.viewport = (0, 0, 4096, 4096)
    gl.clear()
    casting["M"].write(from_sun.T.astype("f4").tobytes())
    casters.render()

    # the tones, and what the lines are found from: the normals, the piece and the depth of every pixel
    pixels = size * fine
    distance = np.linalg.norm(eye - centre)
    near, far = max(distance - radius * 1.6, distance * 0.02), distance + radius * 3.0
    tones, marks, depths = gl.texture((pixels, pixels), 4, dtype="f1"), gl.texture((pixels, pixels), 4, dtype="f4"), gl.depth_texture((pixels, pixels))
    target = gl.framebuffer([tones, marks], depths)
    target.use()
    gl.viewport = (0, 0, pixels, pixels)
    target.clear(0.0, 0.0, 0.0, 0.0)
    shadows.use(0)
    drawing["M"].write((perspective(angle, 1.0, near, far) @ look(eye, at)).T.astype("f4").tobytes())
    drawing["SM"].write(from_sun.T.astype("f4").tobytes())
    values = {"sh": 0, "eye": tuple(eye), "sun": tuple(sun), "unit": float(radius), "amb": shade, "key": light, "grain": 0.8, "zlo": float(low[2]), "zhi": float(high[2])}
    for name, value in values.items():
        if name in drawing:
            drawing[name].value = value
    faces.render()
    tone = np.frombuffer(tones.read(), "u1").reshape(pixels, pixels, 4)[::-1].astype("f4") / 255.0
    mark = np.frombuffer(marks.read(), "f4").reshape(pixels, pixels, 4)[::-1]
    depth = np.frombuffer(depths.read(), "f4").reshape(pixels, pixels)[::-1]
    solid = tone[..., 3]
    depth = np.where(solid > 0.0, 2.0 * far * near / (far + near - (2.0 * depth - 1.0) * (far - near)), far)
    gl.release()

    # the lines: where the faces turn or the piece changes, wider round what stands in front of something farther
    creases, outlines = np.zeros_like(depth), np.zeros_like(depth)
    for across, down in ((1, 0), (0, 1), (1, 1), (-1, 1)):
        moved = lambda values: np.roll(np.roll(values, down, 0), across, 1)
        turned = 1.0 - np.sum((mark[..., :3] * 2.0 - 1.0) * (moved(mark)[..., :3] * 2.0 - 1.0), -1) > 0.12
        creases = np.maximum(creases, (turned | (mark[..., 3] != moved(mark)[..., 3])).astype("f4"))
        outlines = np.maximum(outlines, ((np.abs(depth - moved(depth)) > radius * 0.035) | (moved(solid) != solid)).astype("f4"))
    outlines = thickened(outlines, line * 1.35 * fine)
    lines = np.maximum(thickened(creases, line * 0.62 * fine) * 0.92, outlines)
    cover = np.maximum(solid, outlines)
    colour = tone[..., :3] * (1.0 - lines[..., None]) + np.array(ink) * lines[..., None]
    picture = np.dstack([colour * cover[..., None], cover]).reshape(size, fine, size, fine, 4).mean((1, 3))
    cover = picture[..., 3:]
    colour = np.where(cover > 1e-4, picture[..., :3] / np.maximum(cover, 1e-4), 0.0)
    return Image.fromarray((np.dstack([np.clip(colour, 0.0, 1.0), cover[..., 0]]) * 255.0 + 0.5).astype("u1"), "RGBA")
