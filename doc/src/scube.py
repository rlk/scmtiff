#!/usr/bin/python

from math3d import *
from svg    import *

# Return the vector at the lower-left corner of the the pixel at row r and
# column c of an n-by-n linear cube map face.

def cube(r, c, n):
    return vnormalize((2.0 * float(c) / float(n) - 1.0,
                       2.0 * float(r) / float(n) - 1.0, 1.0))

# Return the vector at the lower-left corner of the the pixel at row r and
# column c of an n-by-n spherical cube map face.

def scube(r, c, n):
    a = (math.pi / 2) * float(c) / float(n) - (math.pi / 4)
    b = (math.pi / 2) * float(r) / float(n) - (math.pi / 4)

    return vnormalize((math.sin(a) * math.cos(b),
                      -math.cos(a) * math.sin(b),
                       math.cos(a) * math.cos(b)))

#-------------------------------------------------------------------------------

def solid_angle3(a, b, c):
    aa = vlength(a)
    bb = vlength(b)
    cc = vlength(c)
    return vdot(a, vcross(b, c)) / (aa * bb * cc + vdot(b, c) * aa
                                                 + vdot(a, c) * bb
                                                 + vdot(a, b) * cc)

def solid_angle4(a, b, c, d):
    return solid_angle3(a, b, c) + solid_angle3(c, d, a)

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

def view(v):
    return vtransform(M, v)

#-------------------------------------------------------------------------------

def draw_cube_face(svg, n):
    for c in range(0, n + 1):
        svglinelist(svg, map(lambda r: view(cube(r, c, n)), range(0, n + 1)))
    for r in range(0, n + 1):
        svglinelist(svg, map(lambda c: view(cube(r, c, n)), range(0, n + 1)))

def draw_scube_face(svg, n):
    for c in range(0, n + 1):
        svglinelist(svg, map(lambda r: view(scube(r, c, n)), range(0, n + 1)))
    for r in range(0, n + 1):
        svglinelist(svg, map(lambda c: view(scube(r, c, n)), range(0, n + 1)))

def cube_solid_angle(r, c, n):
    return solid_angle4(cube(r,     c,     n),
                        cube(r + 1, c,     n),
                        cube(r + 1, c + 1, n),
                        cube(r,     c + 1, n))

def scube_solid_angle(r, c, n):
    return solid_angle4(scube(r,     c,     n),
                        scube(r + 1, c,     n),
                        scube(r + 1, c + 1, n),
                        scube(r,     c + 1, n))

def cube_edge_length(r, c, n):
    return vdistance(cube(r, c, n), cube(r, c + 1, n))

def scube_edge_length(r, c, n):
    return vdistance(scube(r, c, n), scube(r, c + 1, n))

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

#-------------------------------------------------------------------------------
# Compute the ratios of largest to smallest pixel areas

def report(a, b):
    print(a, b, a / b)

n = 1024

report(reduce(min, map(lambda c:  cube_edge_length(0, c, n), range(0, n+1))),
       reduce(max, map(lambda c:  cube_edge_length(0, c, n), range(0, n+1))))

report(reduce(min, map(lambda c:  cube_edge_length(n/2, c, n), range(0, n+1))),
       reduce(max, map(lambda c:  cube_edge_length(n/2, c, n), range(0, n+1))))

report(reduce(min, map(lambda c:  cube_edge_length(0,   c, n), range(0, n+1))),
       reduce(max, map(lambda c:  cube_edge_length(n/2, c, n), range(0, n+1))))

report(reduce(min, map(lambda c: scube_edge_length(0, c, n), range(0, n+1))),
       reduce(max, map(lambda c: scube_edge_length(0, c, n), range(0, n+1))))

report(reduce(min, map(lambda c: scube_edge_length(n/2, c, n), range(0, n+1))),
       reduce(max, map(lambda c: scube_edge_length(n/2, c, n), range(0, n+1))))

report(reduce(min, map(lambda c: scube_edge_length(0,   c, n), range(0, n+1))),
       reduce(max, map(lambda c: scube_edge_length(n/2, c, n), range(0, n+1))))

#-------------------------------------------------------------------------------
# Compute the variation in edge length

n = 1024

print(vdistance(cube(0,   0,   n), cube(0,   1,     n)))
print(vdistance(cube(0,   n/2, n), cube(0,   n/2+1, n)))
print(vdistance(cube(n/2, n/2, n), cube(n/2, n/2+1, n)))

print(vdistance(scube(0,   0,   n), scube(0,   1,     n)))
print(vdistance(scube(0,   n/2, n), scube(0,   n/2+1, n)))
print(vdistance(scube(n/2, n/2, n), scube(n/2, n/2+1, n)))

#-------------------------------------------------------------------------------
# Draw figures of the cube and scube faces

write_cube_face("cube.svg", 16)
write_scube_face("scube.svg", 16)
write_both_face("both.svg", 16)
