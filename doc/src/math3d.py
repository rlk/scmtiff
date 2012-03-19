import math

# Convert the given angle in degrees to radians.

def radians(a):
    return a * math.pi / 180.0

# Convert the given angle in radians to degrees.

def degrees(a):
    return a * 180.0 / math.pi

# Calculate the dot product of 3-vectors a and b.

def vdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

# Calculate the dot product of 4-vectors a and b.

def wdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]

# Calculate the length of vector v.

def vlength(v):
    return math.sqrt(vdot(v, v))

# Give the normalization of vector v.

def vnormalize(v):
    k = vlength(v)
    return (v[0] / k, v[1] / k, v[2] / k)

# Compute the sub of vectors a and b.

def vadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])

# Compute the difference of vectors a and b.

def vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])

# Compute the cross product of vectors a and b.

def vcross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])

# Compute the distance from a to b.

def vdistance(a, b):
    return vlength(vsub(a, b))

# Compute the coefficients of the plane passing through points a, b, and c.

def vplane(a, b, c):
    n = vnormalize(vcross(vsub(b, a), vsub(c, a)))
    return (n[0], n[1], n[2], vdot(n, a))

# Transform vector v by matrix M and project the result to a 3-vector.

def vtransform(M, v):
    u = (v[0], v[1], v[2], 1.0)

    x = wdot(M[0], u)
    y = wdot(M[1], u)
    z = wdot(M[2], u)
    w = wdot(M[3], u)

    return (x / w, y / w, z / w)

# Give the identity matrix.

def midentity():
    return (( 1.0,  0.0,  0.0,  0.0),
            ( 0.0,  1.0,  0.0,  0.0),
            ( 0.0,  0.0,  1.0,  0.0),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the matrix translating along vector v.

def mtranslate(v):
    return (( 1.0,  0.0,  0.0, v[0]),
            ( 0.0,  1.0,  0.0, v[1]),
            ( 0.0,  0.0,  1.0, v[2]),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the matrix scaling by vector v.

def mscale(v):
    return ((v[0],  0.0,  0.0,  0.0),
            ( 0.0, v[1],  0.0,  0.0),
            ( 0.0,  0.0, v[2],  0.0),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the matrix rotating about the X axis through angle a.

def mrotatex(a):
    s = math.sin(a)
    c = math.cos(a)

    return (( 1.0,  0.0,  0.0,  0.0),
            ( 0.0,    c,   -s,  0.0),
            ( 0.0,    s,    c,  0.0),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the matrix rotating about the Y axis through angle a.

def mrotatey(a):
    s = math.sin(a)
    c = math.cos(a)

    return ((   c,  0.0,    s,  0.0),
            ( 0.0,  1.0,  0.0,  0.0),
            (  -s,  0.0,    c,  0.0),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the matrix rotating about the Z axis through angle a.

def mrotatez(a):
    s = math.sin(a)
    c = math.cos(a)

    return ((   c,   -s,  0.0,  0.0),
            (   s,    c,  0.0,  0.0),
            ( 0.0,  0.0,  1.0,  0.0),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the perspective projection matrix with left, right, bottom, top, near,
# and far clipping plane distances.

def mperspective(l, r, b, t, n, f):
    m00 = (n + n) / (r - l)
    m11 = (n + n) / (t - b)
    m02 = (r + l) / (r - l)
    m12 = (t + b) / (t - b)
    m22 = (n + f) / (n - f)
    m32 = -1.0
    m23 = -2.0 * f * n / (f - n)

    return (( m00,  0.0,  m02,  0.0),
            ( 0.0,  m11,  m12,  0.0),
            ( 0.0,  0.0,  m22,  m23),
            ( 0.0,  0.0,  m32,  0.0))

# Give the orthogonal projection matrix with left, right, bottom, top, near,
# and far clipping plane distances.

def morthogonal(l, r, b, t, n, f):
    m00 =  2.0 / (r - l)
    m11 =  2.0 / (t - b)
    m22 = -2.0 / (f - n)
    m03 = -(r + l) / (r - l)
    m13 = -(t + b) / (t - b)
    m23 = -(f + n) / (f - n)

    return (( m00,  0.0,  0.0,  m03),
            ( 0.0,  m11,  0.0,  m13),
            ( 0.0,  0.0,  m22,  m23),
            ( 0.0,  0.0,  0.0,  1.0))

# Give the transpose of matrix M.

def mtranspose(M):
    return ((M[0][0], M[1][0], M[2][0], M[3][0]),
            (M[0][1], M[1][1], M[2][1], M[3][1]),
            (M[0][2], M[1][2], M[2][2], M[3][2]),
            (M[0][3], M[1][3], M[2][3], M[3][3]))

# Multiply matrices A and B.

def mmultiply(A, B):
    T = mtranspose(B)

    x = (wdot(A[0], T[0]), wdot(A[0], T[1]), wdot(A[0], T[2]), wdot(A[0], T[3]))
    y = (wdot(A[1], T[0]), wdot(A[1], T[1]), wdot(A[1], T[2]), wdot(A[1], T[3]))
    z = (wdot(A[2], T[0]), wdot(A[2], T[1]), wdot(A[2], T[2]), wdot(A[2], T[3]))
    w = (wdot(A[3], T[0]), wdot(A[3], T[1]), wdot(A[3], T[2]), wdot(A[3], T[3]))

    return (x, y, z, w)

# Give the composition of a list of matrices.

def mcompose(L):
    M = midentity()
    for N in L:
        M = mmultiply(M, N)
    return M

