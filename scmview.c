// SCMVIEW Copyright (C) 2012 Robert Kooima
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.


//------------------------------------------------------------------------------
// SCMVIEW is a light-weight OpenGL-based viewer for SCMTIFF format image files.
// It enables panning, zooming, and false-color rendering of individual pages,
// as well as side-by-side comparison of multiple SCMTIFF data files. SCMVIEW is
// a debugging tool, and does NOT produce seamless spherical renderings.

#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef __APPLE__
#include <GL/glew.h>
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#include "scm.h"
#include "scmdef.h"
#include "err.h"
#include "util.h"

//------------------------------------------------------------------------------
// SCM TIFF file and page state

struct file
{
    scm   *s;           // SCM file
    GLuint texture;     // On-screen texture cache
};

struct file     *filev;
static int       filec;
static long long pagei;
static long long pagem;

//------------------------------------------------------------------------------
// Viewing and interaction state

static GLfloat pos_x = -0.5f;
static GLfloat pos_y = -0.5f;
static GLfloat scale =  1.0f;

static int     drag_modifier = 0;
static int     drag_button   = 0;
static int     drag_x;
static int     drag_y;
static GLfloat drag_pos_x;
static GLfloat drag_pos_y;
static GLfloat drag_scale;

static GLuint  prog_color;
static GLuint  cmap_color;

//------------------------------------------------------------------------------

static GLfloat *buf;

// Initialize the float buffer to a checkerboard pattern. This provides an
// unambiguous representation for missing pages, where a black page might be
// a page that's actually black.

static void data_null(int n, int c)
{
    int l = 0;

    for         (int i = 0; i < n; ++i)
        for     (int j = 0; j < n; ++j)
            for (int k = 0; k < c; ++k, ++l)
                buf[l] = ((((i - 1) >> 3) & 1) ==
                          (((j - 1) >> 3) & 1)) ? 0.4f : 0.6f;
}

// Load page x from all files. Upload the data to the on-screen texture cache.

static int data_load(long long x)
{
    const GLenum e[] = { 0, GL_LUMINANCE,
                            GL_LUMINANCE_ALPHA,
                            GL_RGB,
                            GL_RGBA };
    const GLint  f[] = { 0, GL_LUMINANCE32F_ARB,
                            GL_LUMINANCE_ALPHA32F_ARB,
                            GL_RGB32F_ARB,
                            GL_RGBA32F_ARB };

    for (int i = 0; i < filec; ++i)
    {
        int       n = scm_get_n (filev[i].s) + 2;
        int       c = scm_get_c (filev[i].s);
        long long j = scm_search(filev[i].s, x);

        if (j < 0)
            data_null(n, c);
        else
            scm_read_page(filev[i].s, scm_get_offset(filev[i].s, j), buf);

        glBindTexture(GL_TEXTURE_2D, filev[i].texture);
        glTexImage2D (GL_TEXTURE_2D, 0, f[c], n, n, 0, e[c], GL_FLOAT, buf);
    }
    return 0;
}

// Determine whether any of the loaded files has a page at index x.

static bool data_test(long long x)
{
    for (int i = 0; i < filec; ++i)
        if (scm_search(filev[i].s, x) >= 0)
            return true;

    return false;
}

// Iterate the list of files given on the command line and initialize a data
// file structure for each. Note the size and channel extreme, and allocate
// working buffers to accommodate the largest file.

static int data_init(int argc, char **argv)
{
    static const GLenum T = GL_TEXTURE_2D;

    int N = 0;
    int C = 0;
    int i = 0;

    pagem = 0;

    if ((filev = (struct file *) calloc((size_t) argc, sizeof (struct file))))

        for (int argi = 1; argi < argc; ++argi)
            if ((filev[i].s = scm_ifile(argv[argi])))
            {
                const int n = scm_get_n(filev[i].s) + 2;
                const int c = scm_get_c(filev[i].s);

                N = max(N, n);
                C = max(C, c);

                glGenTextures  (1, &filev[i].texture);
                glBindTexture  (T,  filev[i].texture);
                glTexParameteri(T, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(T, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                scm_scan_catalog(filev[i].s);

                pagem = max(pagem, scm_get_index(filev[i].s, scm_get_length(filev[i].s) - 1));

                i++;
            }

    filec = i;

    if (i && N && C)
    {
        glutReshapeWindow((argc - 1) * N, N);

        if ((buf = (GLfloat *) malloc((size_t) N *
                                      (size_t) N *
                                      (size_t) C * sizeof (GLfloat))))
            return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------

static const char *vert_color =
    "void main()\n"
    "{\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "    gl_Position = ftransform();\n"
    "}\n";

static const char *frag_color =
    "uniform sampler2D image;"
    "uniform sampler1D color;"

    "void main()\n"
    "{\n"
    "    vec4 i = texture2D(image, gl_TexCoord[0].xy);\n"
    "    gl_FragColor = texture1D(color, i.r);\n"
    "}\n";

// Compile and link the given shader source files to a program object.  Scan
// the loaded files for minimum and maximum sample values and apply these to
// the program uniform.

static GLuint prog_init(const char *vertsrc, const char *fragsrc)
{
    GLuint prog = glCreateProgram();
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vert, 1, (const GLchar **) &vertsrc, NULL);
    glCompileShader(vert);
    glAttachShader(prog, vert);

    glShaderSource(frag, 1, (const GLchar **) &fragsrc, NULL);
    glCompileShader(frag);
    glAttachShader(prog, frag);

    glLinkProgram(prog);
    glUseProgram(prog);

    return prog;
}

// Initialize a 1D texture object with a rainbow color color map.

static GLuint cmap_init(GLuint prog)
{
    static const GLubyte c[6][4] = {
        { 0xFF, 0x00, 0xFF, 0xFF },
        { 0x00, 0x00, 0xFF, 0xFF },
        { 0x00, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0x00, 0x00, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    };

    GLuint texture;

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 6, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);

    glUniform1i(glGetUniformLocation(prog, "color"), 1);

    return texture;
}

// Set the current page. Load the appropriate data from each file and set the
// window title accordingly.

static void jump(long long x)
{
    char str[256];

    data_load(x);

    pagei = x;

    sprintf(str, "page %lld face %lld level %lld row %lld col %lld\n",
        x,
        scm_page_root(x),
        scm_page_level(x),
        scm_page_row(x),
        scm_page_col(x));

    glutSetWindowTitle(str);
    glutPostRedisplay();
}


static void page(long long d, bool s)
{
    long long x = pagei + d;

    if (s && d > 0)
        while (x < pagem && !data_test(x))
            x += d;
    if (s && d < 0)
        while (x > 0     && !data_test(x))
            x += d;

    jump(x);
}

//------------------------------------------------------------------------------

// Application startup.

static int start(int argc, char **argv)
{
    if (data_init(argc, argv))
    {
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        // glEnable(GL_BLEND);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        prog_color = prog_init(vert_color, frag_color);
        cmap_color = cmap_init(prog_color);

        glUseProgram(0);

        jump(0);

        return 1;
    }
    return 0;
}

// GLUT keyboard callback.

static void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(EXIT_SUCCESS);

    else if (key == 13)
    {
        pos_x = -0.5f;
        pos_y = -0.5f;
        scale =  1.0f;
        glutPostRedisplay();
    }

    else if (key == ')') jump(0);
    else if (key == '!') jump(scm_page_count(0));
    else if (key == '@') jump(scm_page_count(1));
    else if (key == '#') jump(scm_page_count(2));
    else if (key == '$') jump(scm_page_count(3));
    else if (key == '%') jump(scm_page_count(4));
    else if (key == '^') jump(scm_page_count(5));
    else if (key == '&') jump(scm_page_count(6));
    else if (key == '*') jump(scm_page_count(7));
    else if (key == '(') jump(scm_page_count(8));

    else if (key == '0') jump(scm_page_root (pagei));
    else if (key == '1') jump(scm_page_child(pagei, 2));
    else if (key == '2') jump(scm_page_south(pagei));
    else if (key == '3') jump(scm_page_child(pagei, 3));
    else if (key == '4') jump(scm_page_west (pagei));
    else if (key == '5') jump((pagei < 6) ? pagei : scm_page_parent(pagei));
    else if (key == '6') jump(scm_page_east (pagei));
    else if (key == '7') jump(scm_page_child(pagei, 0));
    else if (key == '8') jump(scm_page_north(pagei));
    else if (key == '9') jump(scm_page_child(pagei, 1));
}

// GLUT special keyboard callback.

static void special(int key, int x, int y)
{
    int s = (glutGetModifiers() & GLUT_ACTIVE_SHIFT)  ?  1 : 0;

    if      (key == GLUT_KEY_PAGE_UP)   page(+1, s);
    else if (key == GLUT_KEY_PAGE_DOWN) page(-1, s);
    else if (key == GLUT_KEY_F1)
    {
        glUseProgram(0);
        glutPostRedisplay();
    }
    else if (key == GLUT_KEY_F2)
    {
        glUseProgram(prog_color);
        glutPostRedisplay();
    }
}

// GLUT display callback.

static void display(void)
{
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glViewport(0, 0, w, h);

    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.5f, 0.5f, 0.0f);
    glScalef(1.0f / scale, 1.0f / scale, 1.0f);
    glTranslatef(pos_x, pos_y, 0);

    for (int i = 0; i < filec; ++i)
    {
        glViewport(i * w / filec, 0, w / filec, h);
        glBindTexture(GL_TEXTURE_2D, filev[i].texture);

        glBegin(GL_QUADS);
        {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord2f(0.f, 1.f);
            glVertex2f(0.f, 0.f);
            glTexCoord2f(1.f, 1.f);
            glVertex2f(1.f, 0.f);
            glTexCoord2f(1.f, 0.f);
            glVertex2f(1.f, 1.f);
            glTexCoord2f(0.f, 0.f);
            glVertex2f(0.f, 1.f);
        }
        glEnd();
    }
    glutSwapBuffers();
}

// GLUT active motion callback.

static void motion(int x, int y)
{
    const int w = glutGet(GLUT_WINDOW_WIDTH) / filec;
    const int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (drag_button == GLUT_LEFT_BUTTON)
    {
        pos_x = drag_pos_x + (GLfloat) (x - drag_x) / (GLfloat) w;
        pos_y = drag_pos_y - (GLfloat) (y - drag_y) / (GLfloat) h;

        glutPostRedisplay();
    }
    if (drag_button == GLUT_RIGHT_BUTTON)
    {
        scale = drag_scale - (GLfloat) (y - drag_y) / (GLfloat) h;

        glutPostRedisplay();
    }
}

// GLUT mouse button click callback.

static void mouse(int button, int state, int x, int y)
{
    drag_modifier = glutGetModifiers();
    drag_button   = button;
    drag_x        = x;
    drag_y        = y;
    drag_pos_x    = pos_x;
    drag_pos_y    = pos_y;
    drag_scale    = scale;
    glutPostRedisplay();
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    setexe(argv[0]);

    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(128, 128);
    glutInit(&argc, argv);

    glutCreateWindow(argv[0]);

    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutDisplayFunc(display);
    glutMotionFunc(motion);
    glutMouseFunc(mouse);

    if (glewInit() == GLEW_OK)
    {
        if (start(argc, argv))
            glutMainLoop();
    }

    return 0;
}
