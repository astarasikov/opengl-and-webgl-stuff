#include <stdio.h>
#include <stdlib.h>

#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB

#include <GLFW/glfw3.h>

//#define SHOW_IMAGE

/*****************************************************************************
 * OpenGL Helpers
 ****************************************************************************/
#define ogl(x) do { \
        x; \
        int _err = glGetError(); \
        if (_err) { \
                printf("GL Error %d at %d, %s\n", _err, __LINE__, __func__); \
                exit (-1); \
        } \
} while (0)

static inline void oglProgramLog(int pid)
{
        GLint logLen;
        GLsizei realLen;

        glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &logLen);
        if (!logLen) {
                return;
        }
        char *log = (char *)malloc(logLen);
        if (!log) {
                perror("malloc");
                return;
        }
        glGetProgramInfoLog(pid, logLen, &realLen, log);
        if (realLen) {
                printf("program %d log %s\n", pid, log);
        }
        free(log);
}

static inline void oglShaderLog(int sid) {
        GLint logLen;
        GLsizei realLen;

        glGetShaderiv(sid, GL_INFO_LOG_LENGTH, &logLen);
        if (!logLen) {
                return;
        }
        char *log = (char *)malloc(logLen);
        if (!log) {
                perror("malloc");
                return;
        }
        glGetShaderInfoLog(sid, logLen, &realLen, log);
        if (realLen) {
                printf("shader %d log %s\n", sid, log);
        }
        free(log);
}

#define SHADER(name, text) static const char *name = "#version 150 core\n" #text

/*****************************************************************************
 * RGB -> YUV
 ****************************************************************************/
SHADER(vert_passthru,
        in vec4 position;
        in vec2 texcoord;
        out vec2 frag_texcoord;

        void main(void) {
                gl_Position = position;
                frag_texcoord = texcoord;
        }
);

SHADER(frag_texture,
        in vec2 frag_texcoord;
        uniform sampler2D tex_input;
		out vec4 FragColor;

        void main(void) {
                FragColor = texture(tex_input, frag_texcoord);
        }
);

SHADER(frag_hexagonalize,
	in vec2 frag_texcoord;
	uniform sampler2D tex_input;
	uniform vec2 framesize;
	out vec4 FragColor;

	const float poly_rad = 15.0;
	const int num_points = 6;
	const float line_width = 2.0;
	const float half_line_width = 0.5 * line_width;

	vec2 findOrigin(void)
	{
		//return vec2(200.0);
		vec2 fragCoord = gl_FragCoord.xy;

		vec2 vrad = vec2(2.0 * poly_rad, 1.7 * poly_rad);
		vec2 coord_scale = fragCoord / vrad;
		vec2 coord_down = floor(coord_scale) * vrad;
		vec2 coord_up = ceil(coord_scale) * vrad;

		vec2 du = vec2(coord_down.x, coord_up.y);
		vec2 ud = vec2(coord_up.x, coord_down.y);

		vec2 origin = coord_down;
		if (distance(coord_up, fragCoord) <= distance(origin, fragCoord)) {
			origin = coord_up;
		}
		if (distance(du, fragCoord) <= distance(origin, fragCoord)) {
			origin = du;
		}
		if (distance(ud, fragCoord) <= distance(origin, fragCoord)) {
			origin = ud;
		}
		return origin;
	}

	bool is_in_polygon(vec2 coord, vec2 origin) {
		float PI = acos(-1.0);
		float d_phase = 2.0 * PI / float(num_points);
		for (int i = 0; i < num_points; i++) {
			float phi0 = float(i) * d_phase;
			float phi1 = phi0 + d_phase;
			float x0 = origin.x + poly_rad * cos(phi0);
			float y0 = origin.y + poly_rad * sin(phi0);

			float x1 = origin.x + poly_rad * cos(phi1);
			float y1 = origin.y + poly_rad * sin(phi1);

			float k = (y1 - y0) / (x1 - x0);

			vec2 v0 = vec2(x0, y0);
			vec2 v1 = vec2(x1, y1);

			vec2 progress = coord - v0;
			vec2 dpoints = v1 - v0;
			vec2 vscale = progress / dpoints;

			vec2 scale_lim = clamp(vscale, vec2(0.0), vec2(1.0));
			bool on_line = scale_lim == vscale;

			vec2 vmin = vec2(min(v0.x, v1.x), min(v0.y, v1.y));
			vec2 vmax = vec2(max(v0.x, v1.x), max(v0.y, v1.y));
			vec2 clamp_coord = clamp(coord, vmin, vmax);

			bool on_straight_line = false;

			if (abs(dpoints.y) < half_line_width
				&& abs(progress.y) < half_line_width
				&& clamp_coord.x == coord.x)
			{
				on_straight_line = true;
			}
			if (abs(dpoints.x) < half_line_width
				&& abs(progress.x) < half_line_width
				&& clamp_coord.y == coord.y)
			{
				on_straight_line = true;
			}

			if (abs(y0 + (coord.x - x0) * k - coord.y) < line_width && on_line
				|| on_straight_line)
			{
				return true;
			}
		}
		return false;
	}

	vec4 bg_color(vec2 fragCoord) {
		vec2 tc = fragCoord / framesize;
		vec4 color = texture(tex_input, tc);
		if (frag_texcoord.x > 0.5) {
			float gray = dot(vec3(1.0 / 3.0), color.xyz);
			float radius = max(tc.s, tc.t);
			float coeff = floor(radius * 50.0);
			float result = gray * pow(0.947, coeff);
			color = vec4(vec3(result), 1.0);
		}
		return color;
	}

	void main(void) {
		vec2 fragCoord = gl_FragCoord.xy;
		vec4 hex_color = vec4(0.0, 1.0, 1.0, 1.0);
		vec2 origin = findOrigin();

		if (!is_in_polygon(fragCoord, origin))
		{
			FragColor = vec4(0.0, 0.0, 0.0, 1.0);
			return;
		}
		else {
			const int num_samples = 20;
			float rad_sc = poly_rad;
			vec4 color = bg_color(origin);

			float PI = acos(-1.0);
			float d_phase = 2.0 * PI / float(num_samples);
			for (int i = 0; i < num_samples; i++) {
				float phase = float(i) * d_phase;
				float mult = float(i + 1) / float(num_samples);
				vec2 diff = mult * rad_sc * vec2(cos(phase), sin(phase));
				color += bg_color(origin + diff);
			}

			color /= float(num_samples + 1);
			FragColor = color;
		}
	}
);

/*****************************************************************************
 * Rendering the texture to framebuffer
 ****************************************************************************/
enum {
        TEX_WIDTH = 1024,
        TEX_HEIGHT = 768,
};

static const GLfloat QuadSide = 1.0f;

static GLfloat QuadData[] = {
        //vertex coordinates
        QuadSide, QuadSide, 0.0f,
        -QuadSide, QuadSide, 0.0f,
        -QuadSide, -QuadSide, 0.0f,
        QuadSide, -QuadSide, 0.0f,

        //texture coordinates
        0, 0,
        1, 0,
        1, 1,
        0, 1,
};

static GLfloat QuadData_inverted[] = {
        //vertex coordinates
        -QuadSide, -QuadSide, 0.0f,
        QuadSide, -QuadSide, 0.0f,
        QuadSide, QuadSide, 0.0f,
        -QuadSide, QuadSide, 0.0f,

        //texture coordinates
        0, 0,
        1, 0,
        1, 1,
        0, 1,
};

static GLuint QuadIndices[] = {
        0, 1, 2,
        0, 2, 3,
};

static const size_t VertexStride = 3;
static const size_t TexCoordStride = 2;

static const size_t CoordOffset = 0;
static const size_t TexCoordOffset = 12;

static const size_t NumVertices = 4;
static const size_t NumIndices = 6;

static GLuint _program_id;
static GLuint _texture;
static GLuint _fb_hexagonalize;
static GLuint _texture_hexagonalize_fb;

static GLuint _vao;
static GLuint _vbo;
static GLuint _vbo_idx;

static GLuint _position_attr;
static GLuint _tex_coord_attr;
static GLuint _frame_size_attr;
static GLuint _texture_location_in_shader;

static void setGlProgram(const char *frag_source, const char *vert_source) {
        ogl(_program_id = glCreateProgram());

        GLuint vert, frag;
        ogl(vert = glCreateShader(GL_VERTEX_SHADER));
        ogl(frag = glCreateShader(GL_FRAGMENT_SHADER));

        const char **vsrc = const_cast<const char**>(&vert_source);
        const char **fsrc = const_cast<const char**>(&frag_source);

        ogl(glShaderSource(vert, 1, vsrc, NULL));
        ogl(glCompileShader(vert));
        oglShaderLog(vert);

        ogl(glShaderSource(frag, 1, fsrc, NULL));
        ogl(glCompileShader(frag));
        oglShaderLog(frag);

        ogl(glAttachShader(_program_id, frag));
        ogl(glAttachShader(_program_id, vert));

        ogl(glBindAttribLocation(_program_id, 0, "position"));
        ogl(glBindAttribLocation(_program_id, 1, "texcoord"));
        ogl(glBindFragDataLocation(_program_id, 0, "FragColor"));

        ogl(glLinkProgram(_program_id));
        ogl(oglProgramLog(_program_id));

        ogl(_position_attr = glGetAttribLocation(_program_id, "position"));
        ogl(_tex_coord_attr = glGetAttribLocation(_program_id, "texcoord"));
}

static void setGlProgramForRgb2Yuv(void) {
        setGlProgram(frag_hexagonalize, vert_passthru);
        ogl(glUseProgram(_program_id));
        ogl(_frame_size_attr = glGetUniformLocation(_program_id, "framesize"));
        ogl(glUniform2f(_frame_size_attr, TEX_WIDTH, TEX_HEIGHT));
}

static void initializeContext(void) {
        ogl(glGenVertexArrays(1, &_vao));
        ogl(glBindVertexArray(_vao));
        ogl(glGenBuffers(1, &_vbo));
        ogl(glGenBuffers(1, &_vbo_idx));
        ogl(glGenTextures(1, &_texture));

        setGlProgram(frag_texture, vert_passthru);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        ogl(glGenFramebuffers(1, &_fb_hexagonalize));
        ogl(glGenTextures(1, &_texture_hexagonalize_fb));
        ogl(glBindTexture(GL_TEXTURE_2D, _texture_hexagonalize_fb));
        ogl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
                GL_RGB, GL_UNSIGNED_BYTE, NULL));
        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        ogl(glBindTexture(GL_TEXTURE_2D, 0));
}

static void renderTexturedQuad(bool inverted) {
        ogl(glClearColor(0, 1, 0, 1));
        ogl(glUseProgram(_program_id));

        ogl(_texture_location_in_shader = glGetUniformLocation(_program_id, "tex_input"));
        ogl(glUniform1i(_texture_location_in_shader, 0));

        ogl(glBindVertexArray(_vao));

        ogl(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
        if (inverted) {
                ogl(glBufferData(GL_ARRAY_BUFFER, sizeof(QuadData),
                        QuadData_inverted, GL_STATIC_DRAW));
        }
        else {
                ogl(glBufferData(GL_ARRAY_BUFFER, sizeof(QuadData),
                        QuadData, GL_STATIC_DRAW));
        }

        ogl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_idx));
        ogl(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW));

        ogl(glVertexAttribPointer(_position_attr, VertexStride,
                GL_FLOAT, GL_FALSE, 0,
                (GLvoid *) (CoordOffset * sizeof(GLfloat))));
        ogl(glVertexAttribPointer(_tex_coord_attr, TexCoordStride,
                GL_FLOAT, GL_FALSE, 0,
                (GLvoid *) (TexCoordOffset * sizeof(GLfloat))));

        ogl(glEnableVertexAttribArray(_position_attr));
        ogl(glEnableVertexAttribArray(_tex_coord_attr));

        ogl(glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_INT, 0));

        ogl(glDisableVertexAttribArray(_tex_coord_attr));
        ogl(glDisableVertexAttribArray(_position_attr));

        ogl(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        ogl(glBindBuffer(GL_ARRAY_BUFFER, 0));
        ogl(glBindVertexArray(0));
}

/*****************************************************************************
 * Rendering RGB to YUV
 ****************************************************************************/
static void uploadTexture(void) {
        FILE *fin = fopen("me_no_bg.rgb", "rb");
        //FILE *fin = fopen("cat_1024_768.rgb", "rb");
        if (!fin) {
                perror("fopen");
                exit(-1);
        }

        size_t buf_size = TEX_WIDTH * TEX_HEIGHT * 3;
        void *buf = malloc(buf_size);
        if (!buf) {
                perror("malloc");
                exit(-1);
        }

        if (1 != fread(buf, buf_size, 1, fin)) {
                perror("fread");
                exit(-1);
        }

        ogl(glActiveTexture(GL_TEXTURE0));
        ogl(glBindTexture(GL_TEXTURE_2D, _texture));

        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        ogl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                TEX_WIDTH, TEX_HEIGHT,
                0, GL_BGR,
                GL_UNSIGNED_BYTE, buf));

        fclose(fin);
        free(buf);
}

static void renderFbWithShader(void) {
        ogl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        ogl(glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT));
        ogl(glClear(GL_COLOR_BUFFER_BIT));
        setGlProgramForRgb2Yuv();
        ogl(glBindTexture(GL_TEXTURE_2D, _texture_hexagonalize_fb));
        renderTexturedQuad(true);
}

static void writeToFile(void *data, size_t size, const char *fname)
{
        FILE *fout = fopen(fname, "wb");
        if (!fout) {
                perror("fopen");
                exit (-1);
        }

        if (1 != fwrite(data, size, 1, fout)) {
                perror("fwrite");
                exit (-1);
        }

        fclose(fout);
}

static void dumpOutputToFile(void) {
        size_t buf_size = TEX_WIDTH * TEX_HEIGHT * 3;
        char *buf = (char*)malloc(buf_size);
        if (!buf) {
                perror("malloc");
                exit (-1);
        }
        ogl(glFinish());
        ogl(glPixelStorei(GL_PACK_ALIGNMENT, 1));
        ogl(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        ogl(glPixelStorei(GL_PACK_ROW_LENGTH, 0));
        ogl(glPixelStorei(GL_PACK_SKIP_ROWS, 0));
        ogl(glPixelStorei(GL_PACK_SKIP_PIXELS, 0));
        ogl(glReadPixels(0, 0, TEX_WIDTH, TEX_HEIGHT,
                GL_RGB, GL_UNSIGNED_BYTE, buf));

        writeToFile(buf, buf_size, "out.bin");
        free(buf);
}

int main(void) {
        glfwInit();
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifndef SHOW_IMAGE
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
#endif
        glfwWindowHint(GLFW_ALPHA_BITS, 0);
        glfwWindowHint(GLFW_DEPTH_BITS, 0);
        glfwWindowHint(GLFW_STENCIL_BITS, 0);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        GLFWwindow* window = glfwCreateWindow(TEX_WIDTH, TEX_HEIGHT,
                "OpenGL", NULL, NULL);
        glfwMakeContextCurrent(window);

        initializeContext();
        uploadTexture();

        ogl(glBindFramebuffer(GL_FRAMEBUFFER, _fb_hexagonalize));
        ogl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, _texture_hexagonalize_fb, 0));

        GLenum status;
        ogl(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (status != GL_FRAMEBUFFER_COMPLETE) {
                puts("failed binding framebuffer");
                exit(-1);
        }
        /* render the scene */
        ogl(glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT));
        renderTexturedQuad(false);
        renderFbWithShader();
        dumpOutputToFile();

#ifdef SHOW_IMAGE
        while (!glfwWindowShouldClose(window)) {
                glfwSwapBuffers(window);
                glfwPollEvents();
        }
#endif

        glfwTerminate();
        return 0;
}
