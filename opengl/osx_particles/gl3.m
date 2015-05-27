#import "inset_view.h"
#import "inset_gl_controller.h"
#import "opengl_shaders.h"
#import "opengl_utils.h"
#import "opengl_view.h"
#import <math.h>
#import <assert.h>

#define CLAMP(x, a, b) do { \
	if (x < a) { \
		x = a; \
	} \
	else if (x > b) { \
		x = b; \
	} \
} while (0);

static const size_t NumVertices = 1000;
static const size_t CoordStride = 3;
static const size_t QuadDataCount = CoordStride * NumVertices;
static const size_t QuadDataSize = QuadDataCount * sizeof(GLfloat);

@implementation InsetView
{
	GLuint _programId;
	GLuint _vao;
	GLuint _vbo;

	GLuint _positionAttr;
	GLuint _rngSeedUniform;
	GLuint _winSizeUniform;

	GLfloat *_quadData;
}

-(void)initializeContext
{
	static int init = 0;
	if (init) {
		return;
	}

	_quadData = malloc(QuadDataSize);
	assert(_quadData != NULL);
	memset(_quadData, 0, QuadDataSize);

	for (size_t i = 0; i < QuadDataCount; i++) {
		_quadData[i] = -1.0 + (rand() % 2000) / 1000.0;
	}

	ogl(glGenVertexArrays(1, &_vao));
	ogl(glBindVertexArray(_vao));
	ogl(glGenBuffers(1, &_vbo));
	
	ogl(_programId = glCreateProgram());

	const char * const vsrc = VERT;
	const char * const fsrc = FRAG;

	GLuint vert, frag;
	ogl(vert = glCreateShader(GL_VERTEX_SHADER));
	ogl(frag = glCreateShader(GL_FRAGMENT_SHADER));

	ogl(glShaderSource(vert, 1, &vsrc, NULL));
	ogl(glCompileShader(vert));
	oglShaderLog(vert);

	ogl(glShaderSource(frag, 1, &fsrc, NULL));
	ogl(glCompileShader(frag));
	oglShaderLog(frag);

	ogl(glAttachShader(_programId, frag));
	ogl(glAttachShader(_programId, vert));

	ogl(glBindAttribLocation(_programId, 0, "position"));
	ogl(glBindFragDataLocation(_programId, 0, "out_color"));

	ogl(glLinkProgram(_programId));
	ogl(oglProgramLog(_programId));

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	ogl(glEnable(GL_DEPTH_TEST));
	ogl(glEnable(GL_PROGRAM_POINT_SIZE));
	ogl(glEnable(GL_BLEND));
    ogl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	
	ogl(_positionAttr = glGetAttribLocation(_programId, "position"));
	ogl(_rngSeedUniform = glGetUniformLocation(_programId, "rng_seed"));
	ogl(_winSizeUniform = glGetUniformLocation(_programId, "win_size"));

	//XXX: fix this
	init = 1;
}

-(void)renderQuad
{
	ogl(glUseProgram(_programId));
	ogl(glUniform1i(_rngSeedUniform, rand()));
	
	ogl(glBindVertexArray(_vao));

	ogl(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
	ogl(glBufferData(GL_ARRAY_BUFFER,
		QuadDataSize, _quadData, GL_STATIC_DRAW));

	ogl(glVertexAttribPointer(_positionAttr, CoordStride,
		GL_FLOAT, GL_FALSE, 0,
		0));

	ogl(glEnableVertexAttribArray(_positionAttr));

	ogl(glDrawArrays(GL_POINTS, 0, QuadDataCount));

	ogl(glDisableVertexAttribArray(_positionAttr));

	ogl(glBindBuffer(GL_ARRAY_BUFFER, 0));
	ogl(glBindVertexArray(0));
}

-(void)renderForTime:(CVTimeStamp)time
{
	NSLog(@"Render");
	if ([self lockFocusIfCanDraw] == NO) {
		return;
	}
	CGLContextObj contextObj = [[self openGLContext] CGLContextObj];
	CGLLockContext(contextObj);

	[self initializeContext];
	ogl(glViewport(0, 0, self.frame.size.width, self.frame.size.height));
	ogl(glUniform2f(_winSizeUniform, self.frame.size.width, self.frame.size.height));
	ogl(glClearColor(1, 1, 1, 1));
	ogl(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

	[self renderQuad];
	[[self openGLContext] flushBuffer];

	CGLUnlockContext(contextObj);
	[self unlockFocus];
}

@end

int main(int argc, char** argv) {
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	NSApplication *app = [NSApplication sharedApplication];
	GLController *controller = [[GLController alloc] init];
	[app run];
	[pool release];
	return 0;
}
