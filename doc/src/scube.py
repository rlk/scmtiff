#!/usr/bin/python

from math3d import *
from svg    import *

# Return the vector at the lower-left corner of the the pixel at row r and
# column c of an n-by-n linear cube map face.

def cube(r, c, n):
    return vnormalize((2.0 * float(c) / float(n) - 1.0,
                       2.0 * float(r) / float(n) - 1.0, 1.0, 1.0))

# Return the vector at the lower-left corner of the the pixel at row r and
# column c of an n-by-n spherical cube map face.

def scube(r, c, n):
    a = (math.pi / 2) * float(c) / float(n) - (math.pi / 4)
    b = (math.pi / 2) * float(r) / float(n) - (math.pi / 4)

    return vnormalize((math.sin(a) * math.cos(b),
                      -math.cos(a) * math.sin(b),
                       math.cos(a) * math.cos(b)))

#-------------------------------------------------------------------------------

# Transform and project a 3D vector into a viewport with width w and height h.

s = 0.75
w = 512
h = 512
M = mcompose([
        mtranslate((w/2, h/2, 0.0, 1.0)),
        mscale    ((w/2, h/2, 1.0, 1.0)),
        morthogonal(-s, s, -s, s, 1.0, 10.0)
    ])

def project(v):
    return vproject(vtransform(M, v))

#-------------------------------------------------------------------------------

def draw_cube_face(svg, n):
    for c in range(0, n + 1):
        svglinelist(svg, map(lambda r: project(cube(r, c, n)), range(0, n + 1)))
    for r in range(0, n + 1):
        svglinelist(svg, map(lambda c: project(cube(r, c, n)), range(0, n + 1)))

def draw_scube_face(svg, n):
    for c in range(0, n + 1):
        svglinelist(svg, map(lambda r: project(scube(r, c, n)), range(0, n + 1)))
    for r in range(0, n + 1):
        svglinelist(svg, map(lambda c: project(scube(r, c, n)), range(0, n + 1)))

#-------------------------------------------------------------------------------

def write_cube_face(name, n):
    svg = svgroot(w, h)

    a = svggroup(svg)
    a.set("fill",   "none")
    a.set("stroke", "red")
    draw_cube_face(a, n)

    svgwrite(svg, name)

def write_scube_face(name, n):
    svg = svgroot(w, h)

    b = svggroup(svg)
    b.set("fill",   "none")
    b.set("stroke", "blue")
    draw_scube_face(b, n)

    svgwrite(svg, name)

def write_both_face(name, n):
    svg = svgroot(w, h)

    a   = svggroup(svg)
    a.set("fill",   "none")
    a.set("stroke", "red")
    draw_cube_face(a, n)

    b   = svggroup(svg)
    b.set("fill",   "none")
    b.set("stroke", "blue")
    draw_scube_face(b, n)

    svgwrite(svg, name)

write_cube_face("cube.svg", 16)
write_scube_face("scube.svg", 16)
write_both_face("both.svg", 16)
