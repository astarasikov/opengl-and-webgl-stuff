#define _USE_MATH_DEFINES
#include <math.h>

#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <stdio.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#ifndef M_PI
	#define M_PI acos(-1.0)
#endif

#if 0
GLSL TODO:
projection
scale, rotate, translate
color, lighting
quaternions
mipmap
#endif

/* Constants and global variables */
#define gl_width 1.0
#define gl_height 1.0

static GLint draw_mode = GL_TRIANGLES;
static GLfloat cam_x = 0, cam_y = 0, cam_z = 1.0;

/* OpenGL-specific functions */
static void initGL(void);
static void displayGL(void);
static void idleGL(void);
static void light_setup(void);
static void move_cam(double, double, double);

/* GLUT functions */
static void glut_reshape(int, int);
static void glut_mouse(int, int, int, int);
static void glut_keyboard(unsigned char, int, int);
static void initTextureList(void);

int main(int argc, char *argv[])
{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
		glutInitWindowSize(400, 400);
		glutInitWindowPosition(100, 100);
		glutCreateWindow("Test GL");
	
		initGL();

		glutDisplayFunc(displayGL);
		glutReshapeFunc(glut_reshape);
		glutMouseFunc(glut_mouse);
		glutKeyboardFunc(glut_keyboard);
		glutIdleFunc(idleGL);
		glutMainLoop();

		return 0;
}

static void perspective(GLdouble fovy, GLdouble aspect, GLdouble z_near,
	GLdouble z_far) {
	GLdouble x_min, x_max, y_min, y_max;

	y_max = z_near * tan(fovy * M_PI / 360.0);
	y_min = -y_max;

	x_min = y_min * aspect;
	x_max = y_max * aspect;

	glFrustum(x_min, x_max, y_min, y_max, z_near, z_far);
}

static void glut_reshape(int width, int height) {
		glViewport(0, 0, (GLsizei) width, (GLsizei) height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		//glOrtho(0, width, height, 0, 0, 1);
		perspective(120, (GLdouble)width/(GLdouble)height, 0.5, 20.0);
		
		//glFrustum(-gl_width, gl_width, -gl_height, gl_height, 1.0, 20.0);
		glMatrixMode(GL_MODELVIEW);
}

static void glut_mouse(int button, int state, int x, int y) {
    switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
		break;
	default:
		break;
	}
}

static void glut_keyboard(unsigned char key, int x, int y) {
		switch(key) {
		case '2':
				glutIdleFunc(NULL);
				break;
		case '3':
				glutIdleFunc(idleGL);
				break;
		case 'x':
				move_cam(1, 0, 0);
				break;
		case 'y':
				move_cam(0, 1, 0);
				break;
		case 'z':
				cam_z *= 1.1;
				//move_cam(0, 0, 1);
				break;
		case 'a':
				cam_z *= 0.9;
				break;
		case 'q':
				exit(0);
				break;
				break;
		default:
			break;
		}
}

static void move_cam(double x, double y, double z) {
		if (x) cam_x = cam_x < gl_width ? cam_x + 0.05 : -gl_width;
		if (y) cam_y = cam_y < gl_height ? cam_y + 0.05 : -gl_width;
		if (z) cam_z = cam_z < 2.0 ? cam_z + 0.05 : 1.0;
}

static void initGL(void) {
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DOUBLEBUFFER);
}

static void idleGL(void) {
	glutPostRedisplay();
}

#define HDAPS "/sys/devices/platform/hdaps/position"
#define HD_ORIGIN_X 500
#define HD_ORIGIN_Y 500

static void hdaps(void) {
	FILE *fpos = NULL;
	char position[10];

	int xnew, ynew;

	if (!fpos) {
		fpos = fopen(HDAPS, "rt");
	}

	if (!fpos) {
		puts("failed to open hdaps");
		return;
	}

	fseek(fpos, 0, SEEK_SET);
	fread(position, 9, 1, fpos);
	fclose(fpos);

	position[9] = 0;
	sscanf(position, "(%d,%d)", &xnew, &ynew);

	double dx = ((xnew - HD_ORIGIN_X) * 1.0) / HD_ORIGIN_X;
	double dy = ((ynew - HD_ORIGIN_Y) * 1.0) / HD_ORIGIN_Y;

#if 0
	dx *= 360;
	dy *= 360;
#endif
	printf("dx %f dy %f\n", dx, dy);

#if 1
	//FIXME: calibrated by PI/2 position
	dx *= 0.81;
	dy *= 0.81;
#endif

#if 1
	//FIXME: magic numbers
	dx *= 5;
	dy *= 5;
#endif
	cam_x = dy;
	cam_y = dx;

//	glRotatef(dx, 0, 0, 1);
//	glRotatef(0, dy, 0, 1);
}

static void displayGL(void) {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(3.0f);

	glLoadIdentity();
	gluLookAt(cam_x, cam_y, cam_z, 0, 0, 0, 1, 0, 0);
	//gluLookAt(0, 0, 1, 0, 0, 0, 1, 0, 0);
	glColor4f(0.0, 1.0, 1.0, 1.0);
	hdaps();
	glutWireCube(0.8);
	
	glutSwapBuffers();
}

