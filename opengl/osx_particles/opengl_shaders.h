#ifndef __OPENGL_SHADERS__H__
#define __OPENGL_SHADERS__H__

#import "common.h"

#define QUOTE(A) #A

const char * const FRAG = "#version 150 core\n" QUOTE(
	in vec4 vert_position;
	uniform int rng_seed;
	uniform vec2 win_size;
	out vec4 out_color;

	void main(void) {
		vec2 unitFragCoord = 2.0 * (gl_FragCoord.xy / win_size) - vec2(1.0);

		vec2 dx = vert_position.xy - unitFragCoord;
		vec2 aspect = vec2(1.0, win_size.x / win_size.y);
		dx = dx / aspect;
		float mag = dot(dx, dx);
		if (mag > (60.0 * 60.0) / dot(win_size, win_size)) {
			discard;
		}

		out_color = vert_position + vec4(0.0, 0.0, 0.0, 1.0);
		//out_color = vec4(dx.x, dx.y, 0.0, 1.0);
		//out_color = vec4(unitFragCoord.x, unitFragCoord.y, 0.0, 1.0);
		//out_color = vec4(vec3(sqrt(mag)), 1.0);

		float depthRange = gl_DepthRange.far - gl_DepthRange.near;
		float z = (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far)
			/ depthRange;
		vec3 normal = vec3(0.0, 0.0, -vert_position.z);
		const vec3 light = vec3(0.2, 0.2, 20.0);
		const vec3 eye = vec3(0.0, 0.0, 1.0);

		vec3 ndc = vec3(unitFragCoord, z);
		vec3 lv = normalize(light - ndc);
		vec3 ev = normalize(eye - ndc);
		vec3 ref = 2.0 * dot(normal, lv) * normal - lv;
		float coeff = max(0.0, dot(ref, ev));
		out_color = vert_position * vec4(coeff, coeff, coeff, 0.0)
			+ vec4(0.0, 0.0, 0.0, 1.0);
	}
);

const char * const VERT = "#version 150 core\n" QUOTE(
	in vec4 position;
	uniform vec2 win_size;
	uniform int rng_seed;
	out vec4 vert_position;

	void main(void) {
		gl_Position = position;
		//gl_PointSize = 8.0 + 0.2 * mod(rng_seed, 10.0);
		gl_PointSize = 60.0;
		vert_position = position;
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
