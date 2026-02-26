// ─────────────────────────────────────────────────────────────────────────────
// gl_draw_elements.h — чамсы + 2D боксы для Stalcraft X
// Боксы через собственный VAO/VBO/шейдер (современный OpenGL)
// ─────────────────────────────────────────────────────────────────────────────

extern bool  g_chams_enabled;
extern bool  g_boxes_enabled;
extern float g_chams_color[3];

typedef GLint    (APIENTRY* glGetUniformLocation_fn)(GLuint, const char*);
typedef void     (APIENTRY* glGetUniformiv_fn)(GLuint, GLint, GLint*);
typedef void     (APIENTRY* glGetUniformfv_fn)(GLuint, GLint, GLfloat*);
typedef void     (APIENTRY* glUniform3f_fn)(GLint, GLfloat, GLfloat, GLfloat);
typedef void     (APIENTRY* glUniform4f_fn)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void     (APIENTRY* glUniform2f_fn)(GLint, GLfloat, GLfloat);
typedef void     (APIENTRY* glActiveTexture_fn)(GLenum);
typedef void     (APIENTRY* glBlendColor_fn)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef GLuint   (APIENTRY* glCreateShader_fn)(GLenum);
typedef void     (APIENTRY* glShaderSource_fn)(GLuint, GLsizei, const char**, const GLint*);
typedef void     (APIENTRY* glCompileShader_fn)(GLuint);
typedef GLuint   (APIENTRY* glCreateProgram_fn)(void);
typedef void     (APIENTRY* glAttachShader_fn)(GLuint, GLuint);
typedef void     (APIENTRY* glLinkProgram_fn)(GLuint);
typedef void     (APIENTRY* glUseProgram_fn)(GLuint);
typedef void     (APIENTRY* glGenVertexArrays_fn)(GLsizei, GLuint*);
typedef void     (APIENTRY* glBindVertexArray_fn)(GLuint);
typedef void     (APIENTRY* glGenBuffers_fn)(GLsizei, GLuint*);
typedef void     (APIENTRY* glBindBuffer_fn)(GLenum, GLuint);
typedef void     (APIENTRY* glBufferData_fn)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void     (APIENTRY* glEnableVertexAttribArray_fn)(GLuint);
typedef void     (APIENTRY* glVertexAttribPointer_fn)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void     (APIENTRY* glDeleteShader_fn)(GLuint);
typedef void     (APIENTRY* glGetShaderiv_fn)(GLuint, GLenum, GLint*);
typedef void     (APIENTRY* glGetProgramiv_fn)(GLuint, GLenum, GLint*);

#define GL_CURRENT_PROGRAM         0x8B8D
#define GL_TEXTURE0                0x84C0
#define GL_CONSTANT_COLOR          0x8001
#define GL_BLEND_COLOR             0x8005
#define GL_VERTEX_SHADER           0x8B31
#define GL_FRAGMENT_SHADER         0x8B30
#define GL_ARRAY_BUFFER            0x8892
#define GL_DYNAMIC_DRAW            0x88E8
#define GL_COMPILE_STATUS          0x8B81
#define GL_LINK_STATUS             0x8B82
#define GL_VERTEX_ARRAY_BINDING    0x85B5

static glGetUniformLocation_fn      _glGetUniformLocation      = nullptr;
static glGetUniformiv_fn            _glGetUniformiv            = nullptr;
static glGetUniformfv_fn            _glGetUniformfv            = nullptr;
static glUniform3f_fn               _glUniform3f               = nullptr;
static glUniform4f_fn               _glUniform4f               = nullptr;
static glUniform2f_fn               _glUniform2f               = nullptr;
static glActiveTexture_fn           _glActiveTexture           = nullptr;
static glBlendColor_fn              _glBlendColor              = nullptr;
static glCreateShader_fn            _glCreateShader            = nullptr;
static glShaderSource_fn            _glShaderSource            = nullptr;
static glCompileShader_fn           _glCompileShader           = nullptr;
static glCreateProgram_fn           _glCreateProgram           = nullptr;
static glAttachShader_fn            _glAttachShader            = nullptr;
static glLinkProgram_fn             _glLinkProgram             = nullptr;
static glUseProgram_fn              _glUseProgram              = nullptr;
static glGenVertexArrays_fn         _glGenVertexArrays         = nullptr;
static glBindVertexArray_fn         _glBindVertexArray         = nullptr;
static glGenBuffers_fn              _glGenBuffers              = nullptr;
static glBindBuffer_fn              _glBindBuffer              = nullptr;
static glBufferData_fn              _glBufferData              = nullptr;
static glEnableVertexAttribArray_fn _glEnableVertexAttribArray = nullptr;
static glVertexAttribPointer_fn     _glVertexAttribPointer     = nullptr;
static glDeleteShader_fn            _glDeleteShader            = nullptr;
static glGetShaderiv_fn             _glGetShaderiv             = nullptr;
static glGetProgramiv_fn            _glGetProgramiv            = nullptr;
static bool                         s_funcs_loaded             = false;

static void loadGLFuncs() {
    if (s_funcs_loaded) return;
    #define LOAD(n) _ ## n = (n ## _fn)wglGetProcAddress(#n)
    LOAD(glGetUniformLocation); LOAD(glGetUniformiv);  LOAD(glGetUniformfv);
    LOAD(glUniform3f);          LOAD(glUniform4f);     LOAD(glUniform2f);
    LOAD(glActiveTexture);      LOAD(glBlendColor);
    LOAD(glCreateShader);       LOAD(glShaderSource);  LOAD(glCompileShader);
    LOAD(glCreateProgram);      LOAD(glAttachShader);  LOAD(glLinkProgram);
    LOAD(glUseProgram);         LOAD(glGenVertexArrays); LOAD(glBindVertexArray);
    LOAD(glGenBuffers);         LOAD(glBindBuffer);    LOAD(glBufferData);
    LOAD(glEnableVertexAttribArray); LOAD(glVertexAttribPointer);
    LOAD(glDeleteShader);       LOAD(glGetShaderiv);   LOAD(glGetProgramiv);
    #undef LOAD
    s_funcs_loaded = true;
}

// ── Box Renderer ──────────────────────────────────────────────────────────────
struct BoxRenderer {
    GLuint prog = 0, vao = 0, vbo = 0;
    bool   ready = false;

    void init() {
        if (ready) return;
        if (!_glCreateShader) return;

        const char* vs = R"(
#version 330 core
layout(location=0) in vec2 aPos;
uniform vec2 u_pos, u_size, u_screen;
void main(){
    vec2 p = u_pos + aPos * u_size;
    vec2 ndc = p / u_screen * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
})";
        const char* fs = R"(
#version 330 core
out vec4 c;
uniform vec4 u_color;
void main(){ c = u_color; })";

        GLuint v = _glCreateShader(GL_VERTEX_SHADER);
        _glShaderSource(v, 1, &vs, nullptr); _glCompileShader(v);
        GLint ok; _glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
        if (!ok) { _glDeleteShader(v); return; }

        GLuint f = _glCreateShader(GL_FRAGMENT_SHADER);
        _glShaderSource(f, 1, &fs, nullptr); _glCompileShader(f);
        _glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
        if (!ok) { _glDeleteShader(v); _glDeleteShader(f); return; }

        prog = _glCreateProgram();
        _glAttachShader(prog, v); _glAttachShader(prog, f);
        _glLinkProgram(prog);
        _glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        _glDeleteShader(v); _glDeleteShader(f);
        if (!ok) return;

        float verts[] = { 0,0, 1,0, 1,1, 0,1 };
        _glGenVertexArrays(1, &vao); _glGenBuffers(1, &vbo);
        _glBindVertexArray(vao);
        _glBindBuffer(GL_ARRAY_BUFFER, vbo);
        _glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        _glEnableVertexAttribArray(0);
        _glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, (void*)0);
        _glBindVertexArray(0);
        ready = true;
    }

    void draw(float x, float y, float w, float h, float r, float g, float b) {
        if (!ready) return;
        GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
        GLint prev_prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prev_prog);
        GLint prev_vao;  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
        GLboolean dt = glIsEnabled(GL_DEPTH_TEST), bl = glIsEnabled(GL_BLEND);

        glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
        glLineWidth(2.0f);
        _glUseProgram(prog); _glBindVertexArray(vao);

        auto ul = [&](const char* n){ return _glGetUniformLocation(prog, n); };
        _glUniform2f(ul("u_pos"),    x, y);
        _glUniform2f(ul("u_size"),   w, h);
        _glUniform2f(ul("u_screen"), (float)vp[2], (float)vp[3]);
        _glUniform4f(ul("u_color"),  r, g, b, 1.f);

        glDrawArrays(GL_LINE_LOOP, 0, 4);

        _glBindVertexArray(prev_vao); _glUseProgram(prev_prog);
        glLineWidth(1.0f);
        if (dt) glEnable(GL_DEPTH_TEST);
        if (bl) glEnable(GL_BLEND);
    }
};
static BoxRenderer g_box;

static bool getScreenBounds(float& ox, float& oy, float& ow, float& oh) {
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    GLfloat mv[16], pr[16];
    glGetFloatv(GL_MODELVIEW_MATRIX,  mv);
    glGetFloatv(GL_PROJECTION_MATRIX, pr);

    // MVP = proj * modelview (column-major)
    GLfloat mvp[16] = {};
    for (int r=0;r<4;r++) for (int c=0;c<4;c++)
        for (int k=0;k<4;k++) mvp[c*4+r] += pr[k*4+r]*mv[c*4+k];

    float pts[8][3] = {
        {-.4f,0,-.4f},{.4f,0,-.4f},{.4f,0,.4f},{-.4f,0,.4f},
        {-.4f,1.8f,-.4f},{.4f,1.8f,-.4f},{.4f,1.8f,.4f},{-.4f,1.8f,.4f}
    };
    float minX=1e9f,minY=1e9f,maxX=-1e9f,maxY=-1e9f; bool any=false;
    for (auto& p : pts) {
        float cx=mvp[0]*p[0]+mvp[4]*p[1]+mvp[8]*p[2]+mvp[12];
        float cy=mvp[1]*p[0]+mvp[5]*p[1]+mvp[9]*p[2]+mvp[13];
        float cw=mvp[3]*p[0]+mvp[7]*p[1]+mvp[11]*p[2]+mvp[15];
        if (cw<.01f) continue;
        float sx=( cx/cw*.5f+.5f)*vp[2];
        float sy=(-cy/cw*.5f+.5f)*vp[3];
        minX=min(minX,sx); minY=min(minY,sy);
        maxX=max(maxX,sx); maxY=max(maxY,sy); any=true;
    }
    if (!any||maxX-minX<2||maxY-minY<2) return false;
    ox=minX; oy=minY; ow=maxX-minX; oh=maxY-minY;
    return true;
}

// ── Основной хук ──────────────────────────────────────────────────────────────
VOID WINAPI hooks::handles::gl_draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (!g_chams_enabled) {
        handles::originals::gl_draw_elements(mode, count, type, indices);
        return;
    }

    loadGLFuncs();

    GLint cur_prog;
    glGetIntegerv(GL_CURRENT_PROGRAM, &cur_prog);
    if (!cur_prog) { handles::originals::gl_draw_elements(mode,count,type,indices); return; }

    GLint loc_anim  = _glGetUniformLocation(cur_prog, "isAnimated");
    GLint loc_emit  = _glGetUniformLocation(cur_prog, "emissionColor");
    GLint loc_diff  = _glGetUniformLocation(cur_prog, "diffuseColor");
    GLint loc_spec  = _glGetUniformLocation(cur_prog, "specularColor");
    GLint loc_light = _glGetUniformLocation(cur_prog, "g_LightSunColor");
    GLint loc_bones = _glGetUniformLocation(cur_prog, "boneMatrices");

    bool is_player = (loc_anim!=-1 && loc_emit!=-1 && loc_diff!=-1 && loc_spec!=-1 && loc_light!=-1)
                  || (loc_bones!=-1);

    if (!is_player) { handles::originals::gl_draw_elements(mode,count,type,indices); return; }

    // Бекап
    GLfloat bk_em[3],bk_di[3],bk_sp[3],bk_li[3],bk_dr[2],bk_bc[4];
    GLint bk_df,bk_bs,bk_bd;
    GLboolean bk_bl=glIsEnabled(GL_BLEND), bk_dt=glIsEnabled(GL_DEPTH_TEST);
    glGetFloatv(GL_BLEND_COLOR,bk_bc); glGetIntegerv(GL_BLEND_SRC,&bk_bs); glGetIntegerv(GL_BLEND_DST,&bk_bd);
    glGetFloatv(GL_DEPTH_RANGE,bk_dr); glGetIntegerv(GL_DEPTH_FUNC,&bk_df);

    bool has_col = (loc_emit!=-1);
    if (has_col) {
        _glGetUniformfv(cur_prog,loc_emit,bk_em); _glGetUniformfv(cur_prog,loc_diff,bk_di);
        _glGetUniformfv(cur_prog,loc_spec,bk_sp); _glGetUniformfv(cur_prog,loc_light,bk_li);
    }

    // Текстуры
    const int NT=16;
    GLint  ss[NT]; memset(ss,-1,sizeof(ss));
    GLuint st[NT]; memset(st, 0,sizeof(st));
    static GLuint flat_tex=0;
    if (!flat_tex) glGenTextures(1,&flat_tex);

    for (int i=0;i<NT;i++) {
        char nm[32]; sprintf_s(nm,"usedTextures[%d]",i);
        GLint loc=_glGetUniformLocation(cur_prog,nm); if(loc==-1) continue;
        GLint smp; _glGetUniformiv(cur_prog,loc,&smp);
        _glActiveTexture(GL_TEXTURE0+smp);
        GLint bt; glGetIntegerv(GL_TEXTURE_BINDING_2D,&bt);
        ss[i]=smp; st[i]=(GLuint)bt;
        unsigned char px[4]={(unsigned char)(g_chams_color[0]*255),(unsigned char)(g_chams_color[1]*255),(unsigned char)(g_chams_color[2]*255),255};
        glBindTexture(GL_TEXTURE_2D,flat_tex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,px);
    }

    // Применяем чамсы
    glEnable(GL_BLEND); glBlendFunc(GL_ONE,GL_CONSTANT_COLOR);
    _glBlendColor(g_chams_color[0],g_chams_color[1],g_chams_color[2],1.f);
    if (has_col) {
        _glUniform3f(loc_emit, g_chams_color[0],g_chams_color[1],g_chams_color[2]);
        _glUniform3f(loc_diff, g_chams_color[0],g_chams_color[1],g_chams_color[2]);
        _glUniform3f(loc_spec, g_chams_color[0],g_chams_color[1],g_chams_color[2]);
        _glUniform3f(loc_light,g_chams_color[0],g_chams_color[1],g_chams_color[2]);
    }
    glDepthRange(0.0,0.0);

    handles::originals::gl_draw_elements(mode,count,type,indices);

    // Боксы
    if (g_boxes_enabled) {
        g_box.init();
        float bx,by,bw,bh;
        if (getScreenBounds(bx,by,bw,bh))
            g_box.draw(bx,by,bw,bh,g_chams_color[0],g_chams_color[1],g_chams_color[2]);
    }

    // Восстанавливаем
    if (has_col) {
        _glUniform3f(loc_emit, bk_em[0],bk_em[1],bk_em[2]);
        _glUniform3f(loc_diff, bk_di[0],bk_di[1],bk_di[2]);
        _glUniform3f(loc_spec, bk_sp[0],bk_sp[1],bk_sp[2]);
        _glUniform3f(loc_light,bk_li[0],bk_li[1],bk_li[2]);
    }
    glDepthRange(bk_dr[0],bk_dr[1]); glDepthFunc(bk_df);
    if (bk_bl) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (bk_dt) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    _glBlendColor(bk_bc[0],bk_bc[1],bk_bc[2],bk_bc[3]);
    glBlendFunc(bk_bs,bk_bd);
    for (int i=0;i<NT;i++) {
        if (ss[i]==-1) continue;
        _glActiveTexture(GL_TEXTURE0+ss[i]);
        glBindTexture(GL_TEXTURE_2D,st[i]);
    }
}
