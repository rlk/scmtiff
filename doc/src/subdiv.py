#!/usr/bin/python

from math3d import *
from svg    import *

#-------------------------------------------------------------------------------

# Transform and project a 3D vector into a viewport with width w and height h.

s = 0.25
w = 512
h = 512
M = mcompose([
        mtranslate((w/2, h/2, 0.0, 1.0)),
        mscale    ((w/2, h/2, 1.0, 1.0)),
        mperspective(-s, s, -s, s, 1.0, 10.0),
        mtranslate((0.0, 0.0, -5.0)),
        mrotatex(radians(-10)),
        mrotatey(radians( 25)),
    ])

def view(v):
    return vtransform(M, v)

#-------------------------------------------------------------------------------

def sface(v, f):
    if   f == 0:
        return ( v[2],  v[1], -v[0])
    elif f == 1:
        return (-v[2],  v[1],  v[0])
    elif f == 2:
        return ( v[0],  v[2], -v[1])
    elif f == 3:
        return ( v[0], -v[2],  v[1])
    elif f == 4:
        return ( v[0],  v[1],  v[2])
    else:
        return (-v[0],  v[1], -v[2])

def scube2(y, x):
    a = (math.pi / 2) * x - (math.pi / 4)
    b = (math.pi / 2) * y - (math.pi / 4)

    return vnormalize((math.sin(a) * math.cos(b),
                      -math.cos(a) * math.sin(b),
                       math.cos(a) * math.cos(b)))

def scube3(r, c, n):
    return scube2(float(c) / float(n),
                  float(r) / float(n))

#-------------------------------------------------------------------------------

#  a  b
#  d  c

def poly(g, a, b, c, d):
    if (vcross(vsub(c, a), vsub(b, a))[2] > 0 and
        vcross(vsub(d, a), vsub(c, a))[2] > 0):
       svglineloop(g, [a, b, c, d])

def face(g, t, b, l, r, f, n):
    if n == 0:
        poly(g, view(sface(scube2(t, l), f)),
                view(sface(scube2(t, r), f)),
                view(sface(scube2(b, r), f)),
                view(sface(scube2(b, l), f)))
    else:
        y = (t + b) / 2
        x = (l + r) / 2

        face(g, t, y, l, x, f, n - 1)
        face(g, t, y, x, r, f, n - 1)
        face(g, y, b, l, x, f, n - 1)
        face(g, y, b, x, r, f, n - 1)

# def face(g, f, n):
#     for r in range(0, n + 1):
#         svglinelist(g, [view(sface(scub3(r, c, n), f)) for c in range(0, n + 1)])
#     for c in range(0, n + 1):
#         svglinelist(g, [view(sface(scub3(r, c, n), f)) for r in range(0, n + 1)])

def write_cube(name, n):
    svg = svgroot(w, h)

    g = svggroup(svg)
    g.set("fill",   "none")
    g.set("stroke", "black")

    face(g, 0.0, 1.0, 0.0, 1.0, 0, n)
    face(g, 0.0, 1.0, 0.0, 1.0, 1, n)
    face(g, 0.0, 1.0, 0.0, 1.0, 2, n)
    face(g, 0.0, 1.0, 0.0, 1.0, 3, n)
    face(g, 0.0, 1.0, 0.0, 1.0, 4, n)
    face(g, 0.0, 1.0, 0.0, 1.0, 5, n)

    # face(g, 0, 2**n)
    # face(g, 1, 2**n)
    # face(g, 2, 2**n)
    # face(g, 3, 2**n)
    # face(g, 4, 2**n)
    # face(g, 5, 2**n)

    svgwrite(svg, name)

#-------------------------------------------------------------------------------

write_cube("cube0.svg", 0)
write_cube("cube1.svg", 1)
write_cube("cube2.svg", 2)
write_cube("cube3.svg", 3)
write_cube("cube4.svg", 4)
