// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

#include "scm.h"
#include "err.h"

//------------------------------------------------------------------------------

static scm    *s;

static GLuint  texture = 0;
static GLfloat posx    = 0.f;
static GLfloat posy    = 0.f;
static GLfloat scale   = 1.f;

static int     pagei;
static int     pagec;
static int    *pagexv;
static off_t  *pageov;

static double *dbuf;
static void   *bbuf;

//------------------------------------------------------------------------------

static int data_load(int i)
{
	const GLenum f[] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	const GLenum t[] = { GL_UNSIGNED_BYTE, GL_BYTE };

	if (scm_read_page(s, pageov[i], dbuf))
	{
		const int N = scm_get_n(s) + 2;
		const int S = scm_get_s(s);
		const int C = scm_get_c(s);

		int i;

		if (scm_get_s(s))
			for (i = 0; i < N * N * C; ++i)
				((         char *) bbuf)[i] = (         char) (dbuf[i] * 127);
		else
			for (i = 0; i < N * N * C; ++i)
				((unsigned char *) bbuf)[i] = (unsigned char) (dbuf[i] * 255);

		glTexImage2D(GL_TEXTURE_2D, 0, C, N, N, 0, f[C], t[S], bbuf);

		return 1;
	}
	return 0;
}

static void set_page(int i)
{
	if (i >= pagec) i = 0;
	if (i <      0) i = pagec - 1;

	if (data_load(i))
	{
		char str[256];

		pagei = i;

		sprintf(str, "Page %d\n", i);

		glutSetWindowTitle(str);
		glutPostRedisplay();
	}
}

static int data_init(int argc, char **argv)
{
	if (argc > 1)
	{
		if ((s = scm_ifile(argv[1])))
		{
			const int N = scm_get_n(s) + 2;
			const int c = scm_get_c(s);

			if ((pagec = scm_catalog(s, &pagexv, &pageov)))
			{
				if ((dbuf = (double *) malloc(N * N * c * sizeof (double))))
				{
					if ((bbuf = malloc(N * N * c)))
					{
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

//------------------------------------------------------------------------------

static int start(int argc, char **argv)
{
	if (data_init(argc, argv))
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		set_page(0);

		return 1;
	}
	return 0;
}

static void keyboard(unsigned char key, int x, int y)
{
	if (key == 27) exit(EXIT_SUCCESS);
}

static void special(int key, int x, int y)
{
	if      (key == GLUT_KEY_PAGE_UP)   set_page(pagei + 1);
	else if (key == GLUT_KEY_PAGE_DOWN) set_page(pagei - 1);
}

static void display(void)
{
	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(scale, scale, scale);
	glTranslatef(posx, posy, 0);

	glBegin(GL_QUADS);
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(0.f, 0.f);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(1.f, 0.f);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(1.f, 1.f);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(0.f, 1.f);
	}
	glEnd();

	glutSwapBuffers();
}

static void motion(int x, int y)
{
}

static void mouse(int button, int state, int x, int y)
{
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	setexe(argv[0]);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(512, 512);
	glutInit(&argc, argv);

	glutCreateWindow(argv[0]);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutDisplayFunc(display);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);

	if (start(argc, argv))
		glutMainLoop();

    return 0;
}
