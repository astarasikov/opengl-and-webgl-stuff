#import "inset_view.h"
#import "inset_gl_controller.h"
#import "opengl_shaders.h"
#import "opengl_utils.h"
#import "opengl_view.h"
#import <math.h>

#define TextureName @"texture.png"

#define CLAMP(x, a, b) do { \
	if (x < a) { \
		x = a; \
	} \
	else if (x > b) { \
		x = b; \
	} \
} while (0);

static const float QuadSide = 0.7f;
#define QUAD(a, b) (b), (a), ((b) + 4), ((b) + 4), (a), ((a) + 4)

static GLuint QuadIndices[] = {
	QUAD(0, 1),
	QUAD(1, 2),
	QUAD(2, 3),
	QUAD(4, 5),
	QUAD(5, 6),
	QUAD(6, 7),
	QUAD(8, 9),
	QUAD(9, 10),
	QUAD(10, 11)
};

static const size_t NumVertices = 16;

static const size_t CoordStride = 2;
static const size_t TexCoordStride = 2;

static const size_t CoordOffset = 0;
static const size_t TexCoordOffset = CoordOffset + CoordStride * NumVertices;
static const size_t QuadDataCount = TexCoordOffset + TexCoordStride * NumVertices;
static const size_t QuadDataSize = QuadDataCount * sizeof(GLfloat);

static const size_t NumIndices = sizeof(QuadIndices) / sizeof(QuadIndices[0]);

@implementation InsetView
{
	GLuint _programId;
	GLuint _vao;
	GLuint _vbo;
	GLuint _vbo_idx;

	GLuint _positionAttr;
	GLuint _texCoordAttr;

	GLuint _texture;
	CGSize _textureSize;
	RectInset _insets;

	GLfloat _quadData[QuadDataCount];
}

-(void)initializeContext
{
	static int init = 0;
	if (init) {
		return;
	}

	memset(_quadData, 0, QuadDataSize);

	ogl(glGenVertexArrays(1, &_vao));
	ogl(glBindVertexArray(_vao));
	ogl(glGenBuffers(1, &_vbo));
	ogl(glGenBuffers(1, &_vbo_idx));
	
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
	ogl(glBindAttribLocation(_programId, 2, "texcoord"));
	ogl(glBindFragDataLocation(_programId, 0, "out_color"));

	ogl(glLinkProgram(_programId));
	ogl(oglProgramLog(_programId));

	ogl(glGenTextures(1, &_texture));
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	ogl(glEnable(GL_DEPTH_TEST));
	ogl(glEnable(GL_BLEND));
    ogl(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	
	ogl(_positionAttr = glGetAttribLocation(_programId, "position"));
	ogl(_texCoordAttr = glGetAttribLocation(_programId, "texcoord"));

	//XXX: fix this
	init = 1;
}

-(void)renderQuad
{
	ogl(glUseProgram(_programId));
	
	GLuint texLoc;
	ogl(texLoc = glGetUniformLocation(_programId, "texture_Y"));
	ogl(glUniform1i(texLoc, 0));

	ogl(glBindVertexArray(_vao));

	ogl(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
	ogl(glBufferData(GL_ARRAY_BUFFER,
		QuadDataSize, _quadData, GL_STATIC_DRAW));

	ogl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_idx));
	ogl(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW));

	ogl(glVertexAttribPointer(_positionAttr, CoordStride,
		GL_FLOAT, GL_FALSE, 0,
		(GLvoid*)(CoordOffset * sizeof(GLfloat))));
	ogl(glVertexAttribPointer(_texCoordAttr, TexCoordStride,
		GL_FLOAT, GL_FALSE, 0,
		(GLvoid*)(TexCoordOffset * sizeof(GLfloat))));

	ogl(glEnableVertexAttribArray(_positionAttr));
	ogl(glEnableVertexAttribArray(_texCoordAttr));

	ogl(glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_INT, 0));

	ogl(glDisableVertexAttribArray(_texCoordAttr));
	ogl(glDisableVertexAttribArray(_positionAttr));

	ogl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
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
	ogl(glClearColor(1, 1, 1, 1));
	ogl(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

	[self renderQuad];
	[[self openGLContext] flushBuffer];

	CGLUnlockContext(contextObj);
	[self unlockFocus];
}

-(void)setInsets:(RectInset)insets
{
	_insets = insets;
	const float width = self.frame.size.width * QuadSide;
	const float height = self.frame.size.height * QuadSide;

	const float TextureLeft = (_insets.left / _textureSize.width);
	const float TextureRight = (_insets.right / _textureSize.width);
	const float TextureBottom = (_insets.bottom / _textureSize.height);
	const float TextureTop = (_insets.top / _textureSize.height);

	const float RightBorderWidth = (_textureSize.width - _insets.right);
	const float BottomBorderWidth = (_textureSize.height - _insets.bottom);

	float OutputInsetRight = width - RightBorderWidth;
	float OutputInsetBottom = height - BottomBorderWidth;
	CLAMP(OutputInsetRight, 0.0f, width);
	CLAMP(OutputInsetBottom, 0.0f, height)

	const float OutputLeft = (_insets.left / width);
	const float OutputRight = (OutputInsetRight / width);
	const float OutputBottom = (OutputInsetBottom / height);
	const float OutputTop = (_insets.top / height);

	GLfloat newQuadData[QuadDataCount] = {
		0, 0,
		OutputLeft, 0,
		OutputRight, 0,
		1, 0,
		0, OutputTop,
		OutputLeft, OutputTop,
		OutputRight, OutputTop,
		1, OutputTop,
		0, OutputBottom,
		OutputLeft, OutputBottom,
		OutputRight, OutputBottom,
		1, OutputBottom,
		0, 1,
		OutputLeft, 1,
		OutputRight, 1,
		1, 1,
	
		//texture coordinates
		0, 0,
		TextureLeft, 0,
		TextureRight, 0,
		1, 0,
		0, TextureTop,
		TextureLeft, TextureTop,
		TextureRight, TextureTop,
		1, TextureTop,
		0, TextureBottom,
		TextureLeft, TextureBottom,
		TextureRight, TextureBottom,
		1, TextureBottom,
		0, 1,
		TextureLeft, 1,
		TextureRight, 1,
		1, 1,
	};

	//flip Y coordinate and scale the quad (could be done statically
	//at array initialization but leave it as is for readability)
	size_t i;
	for (i = 0; i < NumVertices * 2; i++) {
		newQuadData[i] = (-0.5f + newQuadData[i]);
		CLAMP(newQuadData[i], -1.0f, 1.0f);
		if (i & 1) {
			newQuadData[TexCoordOffset + i] = 1.0f - newQuadData[TexCoordOffset + i];
		}
	}

	memcpy(_quadData, newQuadData, QuadDataSize);
}

-(void)setTexture:(char*)data andWidth:(unsigned)width
	andHeight:(unsigned)height
	withInsets:(RectInset)insets
{
	if ([self lockFocusIfCanDraw] == NO) {
		return;
	}
	CGLContextObj contextObj = [[self openGLContext] CGLContextObj];
	CGLLockContext(contextObj);

	ogl(glActiveTexture(GL_TEXTURE0));
	ogl(glBindTexture(GL_TEXTURE_2D, _texture));

	ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	ogl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		width,
		height,
		0, GL_RGBA,
		GL_UNSIGNED_BYTE, data));

	_textureSize = CGSizeMake(width, height);
	[self setInsets:insets];

	CGLUnlockContext(contextObj);
	[self unlockFocus];
}

@end

static void loadImage(id controller) {
	NSImage *image = [[NSImage alloc] initWithContentsOfFile: TextureName];
	NSBitmapImageRep *repr = [[image representations] objectAtIndex: 0];

	[[controller glView] setTexture: (char*)[repr bitmapData]
		andWidth: (unsigned)[repr pixelsWide]
		andHeight: (unsigned)[repr pixelsHigh]
		withInsets: (RectInset){8, 236, 8, 26}];

	[repr release];
	[image release];
}

int main(int argc, char** argv) {
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	NSApplication *app = [NSApplication sharedApplication];
	GLController *controller = [[GLController alloc] init];
	loadImage(controller);
	[app run];
	[pool release];
	return 0;
}
