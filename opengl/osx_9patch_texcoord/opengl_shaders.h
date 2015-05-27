#ifndef __OPENGL_SHADERS__H__
#define __OPENGL_SHADERS__H__

#import "common.h"

#define QUOTE(A) #A

const char * const FRAG = "#version 150 core\n" QUOTE(
	in vec2 vert_texcoord;
	out vec4 out_color;
	uniform sampler2D texture_Y;

	void main(void) {
		out_color = texture(texture_Y, vert_texcoord);
	}
);

const char * const VERT = "#version 150 core\n" QUOTE(
	in vec4 position;
	in vec2 texcoord;
	out vec2 vert_texcoord;

	void main(void) {
		gl_Position = position;
		vert_texcoord = texcoord;
	}
);

#undef QUOTE

static inline void oglShaderLog(int sid) {
	GLint logLen;
	GLsizei realLen;

	glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &logLen);
	if (!logLen) {
		return;
	}
	char* log = (char*)malloc(logLen);
	if (!log) {
		NSLog(@"Failed to allocate memory for the shader log");
		return;
	}
	glGetShaderInfoLog(sid, logLen, &realLen, log);
	NSLog(@"shader %d log %s", sid, log);
	free(log);
}

#endif //__OPENGL_SHADERS__H__
