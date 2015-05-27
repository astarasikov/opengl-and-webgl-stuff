#include <stdio.h>
#include <stdlib.h>
//#include <sys/error.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//#define SKIP_YUVCONV
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

#define SHADER(name, text) static const char *name = "#version 120\nprecision mediump float;\n" #text

/*****************************************************************************
 * RGB -> YUV
 ****************************************************************************/
SHADER(frag_texture,
        varying vec2 frag_texcoord;
        uniform sampler2D tex_input;

        void main(void) {
                gl_FragColor = texture2D(tex_input, frag_texcoord);
        }
);

SHADER(vert_passthru,
        attribute vec4 position;
        attribute vec2 texcoord;
        varying vec2 frag_texcoord;
        
        void main(void) {
                gl_Position = position;
                frag_texcoord = texcoord;
        }
);

SHADER(frag_rgb2yuv,
        varying vec2 frag_texcoord;
        uniform sampler2D tex_input;
        uniform vec2 framesize;

        const float y_size = 1.0 / 3.0;
        const vec2 uv_scaler = vec2(3.0, 6.0);
        const mat4 rgb2yuv_mat = mat4 (
                0.257,  0.439, -0.148, 0.0,
                0.504, -0.368, -0.291, 0.0,
                0.098, -0.071,  0.439, 0.0,
                0.0625, 0.500,  0.500, 1.0
        );

        vec4 wrappedTexel(vec2 coord) {
                return texture2D(tex_input, mod(coord, vec2(1.0)));
        }

        void main(void) {
                vec2 xy_offset = vec2(1.0, 1.0) / framesize;
                vec2 x_offset = vec2(1.0, 0.0) / framesize;
                vec2 y_offset = vec2(0.0, 1.0) / framesize;
                /* first, Y plane */
                if (frag_texcoord.y <= y_size) {
                        vec2 real_coord = floor(frag_texcoord * framesize);
                        float idx = 3.0 * (real_coord.y * framesize.x + real_coord.x);
                        vec2 coord = vec2(mod(idx, framesize.x), idx / framesize.x) / framesize;

                        mat3x4 rgbs = mat3x4(
                                wrappedTexel(coord),
                                wrappedTexel(coord + x_offset),
                                wrappedTexel(coord + 2.0 * x_offset)
                        );
                        mat3x4 yuvs = rgb2yuv_mat * rgbs;
                        gl_FragColor = vec4(yuvs[0].x, yuvs[1].x, yuvs[2].x, 0.0);
                }
                /* U and V interspersed */
                else if (frag_texcoord.y <= 0.5) {
                        vec2 shifted_texcoord = frag_texcoord - vec2(0.0, y_size);
                        vec2 real_coord = shifted_texcoord * framesize;

                        vec2 tex0_offset = uv_scaler * shifted_texcoord;
                        vec2 tex1_offset = uv_scaler * shifted_texcoord + x_offset;

                        /* fetch adjacent Y and X texels for 2 subsequent coordinates */
                        mat4x4 yuvs_0 = rgb2yuv_mat * mat4x4(
                                wrappedTexel(tex0_offset),
                                wrappedTexel(tex0_offset + y_offset),
                                wrappedTexel(tex0_offset + x_offset),
                                wrappedTexel(tex0_offset + xy_offset)
                        );
                        float u_0 = 0.25 * (yuvs_0[0].y + yuvs_0[1].y + yuvs_0[2].y + yuvs_0[3].y);
                        float v_0 = 0.25 * (yuvs_0[0].z + yuvs_0[1].z + yuvs_0[2].z + yuvs_0[3].z);
                        
                        mat4x4 yuvs_1 = rgb2yuv_mat * mat4x4(
                                wrappedTexel(tex1_offset),
                                wrappedTexel(tex1_offset + y_offset),
                                wrappedTexel(tex1_offset + x_offset),
                                wrappedTexel(tex1_offset + xy_offset)
                        );
                        float u_1 = 0.25 * (yuvs_1[0].y + yuvs_1[1].y + yuvs_1[2].y + yuvs_1[3].y);
                        float v_1 = 0.25 * (yuvs_1[0].z + yuvs_1[1].z + yuvs_1[2].z + yuvs_1[3].z);

                        /* pack alternating UVU and VUV into RGB vectors */
                        /* XXX: inverted order? */
                        if (mod(floor(real_coord.x), 2.0) <= 0.1) {
                                gl_FragColor = vec4(u_0, v_0, u_1, 0.0);
                        }
                        else {
                                gl_FragColor = vec4(v_0, u_1, v_1, 0.0);
                        }
                }
                else {
                        gl_FragColor = vec4(0.0);
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
static GLuint _fb_rgb2yuv;
static GLuint _texture_rgb2yuv_fb;

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
        ogl(glBindFragDataLocation(_program_id, 0, "out_color"));

        ogl(glLinkProgram(_program_id));
        ogl(oglProgramLog(_program_id));
        
        ogl(_position_attr = glGetAttribLocation(_program_id, "position"));
        ogl(_tex_coord_attr = glGetAttribLocation(_program_id, "texcoord"));
}

static void setGlProgramForRgb2Yuv(void) {
#ifndef SKIP_YUVCONV
        setGlProgram(frag_rgb2yuv, vert_passthru);
#else
        setGlProgram(frag_texture, vert_passthru);
#endif
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

        ogl(glGenFramebuffers(1, &_fb_rgb2yuv));
        ogl(glGenTextures(1, &_texture_rgb2yuv_fb));
        ogl(glBindTexture(GL_TEXTURE_2D, _texture_rgb2yuv_fb));
        ogl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
                GL_RGB, GL_UNSIGNED_BYTE, NULL));
        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        ogl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
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
        FILE *fin = fopen("cat_1024_768.rgb", "rb");
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
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
        ogl(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));

        ogl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                TEX_WIDTH, TEX_HEIGHT,
                0, GL_RGB,
                GL_UNSIGNED_BYTE, buf));

        fclose(fin);
        free(buf);
}

static void renderFbToYuv(void) {
        ogl(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        ogl(glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT));
        ogl(glClear(GL_COLOR_BUFFER_BIT));
        setGlProgramForRgb2Yuv();
        ogl(glBindTexture(GL_TEXTURE_2D, _texture_rgb2yuv_fb));
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

#ifndef SKIP_YUVCONV
        buf_size >>= 1;
#endif
        writeToFile(buf, buf_size, "out.bin");
        writeToFile(buf, TEX_WIDTH * TEX_HEIGHT, "y.bin");
        writeToFile(buf + TEX_WIDTH * TEX_HEIGHT,
                TEX_WIDTH * TEX_HEIGHT / 2, "uv.bin");
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
        GLFWwindow* window = glfwCreateWindow(TEX_WIDTH, TEX_HEIGHT,
                "OpenGL", NULL, NULL);
        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        glewInit();

        initializeContext();
        uploadTexture();

        ogl(glBindFramebuffer(GL_FRAMEBUFFER, _fb_rgb2yuv));
        ogl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, _texture_rgb2yuv_fb, 0));

        GLenum status;
        ogl(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (status != GL_FRAMEBUFFER_COMPLETE) {
                puts("failed binding framebuffer");
                exit(-1);
        }
        /* render the scene */
        renderTexturedQuad(true);
        renderFbToYuv();
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
