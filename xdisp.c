#include "glcorearb.h"

static HWND g_window_handle;
static HDC g_device_context;
static HGLRC g_rendering_context;
static GLuint g_shader;
static unsigned g_window_size;
static unsigned g_window_resolution;
static unsigned g_spacing;
static unsigned g_block_size;
static float g_projection_matrix[16];
static int g_up, g_down, g_left, g_right;
static int64_t g_counter_start;
static double g_counter_frequency;

static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLDISABLEPROC glDisable;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
static PFNGLDRAWARRAYSPROC glDrawArrays;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLFLUSHPROC glFlush;
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLUNIFORM3FVPROC glUniform3fv;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

static void *get_proc(HMODULE libgl, const char *proc)
{
    void* res = (void*)wglGetProcAddress(proc);
    if (res)
        return res;
    return GetProcAddress(libgl, proc);
}

static void init_gl()
{
    HMODULE libgl = LoadLibraryA("opengl32.dll");
    #define GetExt(type, name) name = (type) get_proc(libgl, #name)
    GetExt(PFNGLATTACHSHADERPROC, glAttachShader);
    GetExt(PFNGLBINDBUFFERPROC, glBindBuffer);
    GetExt(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    GetExt(PFNGLBUFFERDATAPROC, glBufferData);
    GetExt(PFNGLCOMPILESHADERPROC, glCompileShader);
    GetExt(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    GetExt(PFNGLCREATESHADERPROC, glCreateShader);
    GetExt(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    GetExt(PFNGLDELETESHADERPROC, glDeleteShader);
    GetExt(PFNGLDISABLEPROC, glDisable);
    GetExt(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
    GetExt(PFNGLDRAWARRAYSPROC, glDrawArrays);
    GetExt(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    GetExt(PFNGLFLUSHPROC, glFlush);
    GetExt(PFNGLGENBUFFERSPROC, glGenBuffers);
    GetExt(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    GetExt(PFNGLLINKPROGRAMPROC, glLinkProgram);
    GetExt(PFNGLSHADERSOURCEPROC, glShaderSource);
    GetExt(PFNGLUNIFORM3FVPROC, glUniform3fv);
    GetExt(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    GetExt(PFNGLUSEPROGRAMPROC, glUseProgram);
    GetExt(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    FreeLibrary(libgl);
}

static GLuint compile_glsl(const char* shader_source, GLenum shader_type)
{
    GLuint result = glCreateShader(shader_type);
    glShaderSource(result, 1, &shader_source, NULL);
    glCompileShader(result);
    return result;
}

static GLuint load_shader(const char* vertex_source, const char* fragment_source)
{
    GLuint vertex_shader = compile_glsl(vertex_source, GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_glsl(fragment_source, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

static void key_down(int key)
{
    switch (key)
    {
        case VK_ESCAPE:
            SendMessage(g_window_handle, WM_CLOSE, 0, 0);
            break;
        case VK_UP:
            g_up = 1;
            break;
        case VK_DOWN:
            g_down = 1;
            break;
        case VK_LEFT:
            g_left = 1;
            break;
        case VK_RIGHT:
            g_right = 1;
            break;
    }
}

static void key_up(int key)
{
    switch (key)
    {
        case VK_UP:
            g_up = 0;
            break;
        case VK_DOWN:
            g_down = 0;
            break;
        case VK_LEFT:
            g_left = 0;
            break;
        case VK_RIGHT:
            g_right = 0;
            break;
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        key_down(wparam);
        return 0;
    case WM_KEYUP:
        key_up(wparam);
        return 0;
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

void xdisp_init(const char* window_title, unsigned size, unsigned scale, unsigned spacing, unsigned char cr, unsigned char cg, unsigned char cb)
{
    g_up, g_down, g_left, g_right = 0;
    HINSTANCE h = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    GLuint vao;
    int border_width = GetSystemMetrics(SM_CXFIXEDFRAME);
    int h_border_thickness = GetSystemMetrics(SM_CXSIZEFRAME) + border_width;
    int v_border_thickness = GetSystemMetrics(SM_CYSIZEFRAME) + border_width;
    int caption_thickness = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXPADDEDBORDER);
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                       //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        0,                       //Number of bits for the depthbuffer
        0,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    static const char* vertex_shader = 
        "#version 410 core\n"
        "uniform mat4 projection_matrix;"
        "in vec2 in_position;"
        "in float in_color;"
        "out float v_color;"
        "void main()"
        "{"
            "v_color = in_color;"
            "gl_Position = projection_matrix * vec4(in_position.x, in_position.y, 0, 1);"
        "}";

    static const char* fragment_shader = 
        "#version 410 core\n"
        "uniform vec3 set_color;"
        "in float v_color;"
        "out vec4 color;"
        "void main()"
        "{"
            "color = vec4(set_color * v_color, 1);"
        "}";

    g_spacing = spacing;
    g_window_size = size;
    g_window_resolution = size * scale + spacing * (size - 1);
    g_block_size = scale;
    wc.hInstance = h;
    wc.lpfnWndProc = window_proc;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.lpszClassName = window_title;
    wc.style = CS_OWNDC;
    RegisterClass(&wc);
    g_window_handle = CreateWindow(window_title, window_title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, g_window_resolution + 2 * h_border_thickness + 1, g_window_resolution + 2 * v_border_thickness + caption_thickness, 0, 0, h, 0);
    g_device_context = GetDC(g_window_handle);
    SetPixelFormat(g_device_context, ChoosePixelFormat(g_device_context, &pfd), &pfd);
    g_rendering_context = wglCreateContext(g_device_context);
    wglMakeCurrent(g_device_context,g_rendering_context);
    init_gl();
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(load_shader(vertex_shader, fragment_shader));
    glUniformMatrix4fv(0, 1, GL_FALSE, (GLfloat*)g_projection_matrix);

    float set_color[3] = {(float)cr/255.0f, (float)cg/255.0f, (float)cb/255.0f};
    glUniform3fv(1, 1, set_color);

    float near_plane = 0;
    float far_plane = 1.0f;
    
    g_projection_matrix[0] = 2.0f/(g_window_resolution - 1.0f);
    g_projection_matrix[1] = 0;
    g_projection_matrix[2] = 0;
    g_projection_matrix[3] = 0;

    g_projection_matrix[4] = 0;
    g_projection_matrix[5] = -2.0f/(g_window_resolution - 1.0f);
    g_projection_matrix[6] = 0;
    g_projection_matrix[7] = 0;

    g_projection_matrix[8] = 0;
    g_projection_matrix[9] = 0;
    g_projection_matrix[10] = 2.0f/(far_plane/near_plane);
    g_projection_matrix[11] = 0;

    g_projection_matrix[12] = -1;
    g_projection_matrix[13] = 1;
    g_projection_matrix[14] = (near_plane+far_plane)/(near_plane-far_plane);
    g_projection_matrix[15] = 1;

    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    g_counter_frequency = (double)(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    g_counter_start = li.QuadPart;
}

void xdisp_deinit()
{
    DestroyWindow(g_window_handle);
}

void xdisp_process_events()
{
    MSG msg = {0};

    while(PeekMessage(&msg,0,0,0,PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void xdisp_set(unsigned x, unsigned y, unsigned state)
{
    const unsigned ps = g_block_size;
    const float c = (float)state;
    const unsigned sx = g_spacing * x;
    const unsigned sy = g_spacing * y;
    const GLfloat buffer_data[18] = {
        (float)(x * ps + sx), (float)(y * ps + sy),
        (float)(c),
        (float)(x * ps + ps + sx), (float)(y * ps + sy),
        (float)(c),
        (float)(x * ps + sx), (float)(y * ps + ps + sy),
        (float)(c),

        (float)(x * ps + ps + sx), (float)(y * ps + sy),
        (float)(c),
        (float)(x * ps + ps + sx), (float)(y * ps + ps + sy),
        (float)(c),
        (float)(x * ps + sx), (float)(y * ps + ps + sy),
        (float)(c)
    };
    GLuint geometry;
    glGenBuffers(1, &geometry);
    glBindBuffer(GL_ARRAY_BUFFER, geometry);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, geometry);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)(2 * sizeof(float)));

    glUniformMatrix4fv(0, 1, GL_FALSE, (GLfloat*)g_projection_matrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glFlush();
    glDeleteBuffers(1, &geometry);
}

int xdisp_is_window_open()
{
    return IsWindow(g_window_handle);
}

int xdisp_up_held()
{
    return g_up;
}

int xdisp_left_held()
{
    return g_left;
}

int xdisp_right_held()
{
    return g_right;
}

int xdisp_down_held()
{
    return g_down;
}

float xdisp_time()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (float)((double)(li.QuadPart - g_counter_start) / g_counter_frequency / 1000.0);
}
