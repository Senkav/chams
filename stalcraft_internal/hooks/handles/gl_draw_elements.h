// ─────────────────────────────────────────────────────────────────────────────
// gl_draw_elements.h — чамсы + 3D боксы для Stalcraft X
// Детекция игроков через glUniform4fv/glUniform4ui (стабильно при апдейтах)
// ─────────────────────────────────────────────────────────────────────────────

extern bool  g_chams_enabled;
extern bool  g_boxes_enabled;
extern float g_chams_color[3];

// ── Типы OpenGL функций ───────────────────────────────────────────────────────
typedef GLint  (APIENTRY* glGetUniformLocation_fn)(GLuint, const char*);
typedef void   (APIENTRY* glGetUniformiv_fn)(GLuint, GLint, GLint*);
typedef void   (APIENTRY* glGetUniformfv_fn)(GLuint, GLint, GLfloat*);
typedef void   (APIENTRY* glUniform3f_fn)(GLint, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY* glUniform4f_fn)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY* glActiveTexture_fn)(GLenum);
typedef void   (APIENTRY* glBlendColor_fn)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRY* glUniform4fv_fn)(GLint, GLsizei, const GLfloat*);
typedef void   (APIENTRY* glUniform4uiv_fn)(GLint, GLsizei, const GLuint*);

#define GL_CURRENT_PROGRAM  0x8B8D
#define GL_TEXTURE0         0x84C0
#define GL_CONSTANT_COLOR   0x8001
#define GL_BLEND_COLOR      0x8005

// ── Хелперы для получения матриц/вьюпорта ────────────────────────────────────
static bool worldToScreen(const GLfloat* mvp, float wx, float wy, float wz,
                           float& sx, float& sy, GLint* viewport)
{
    // Умножаем на MVP матрицу (column-major OpenGL)
    float x = mvp[0]*wx + mvp[4]*wy + mvp[8]*wz  + mvp[12];
    float y = mvp[1]*wx + mvp[5]*wy + mvp[9]*wz  + mvp[13];
    float w = mvp[3]*wx + mvp[7]*wy + mvp[11]*wz + mvp[15];

    if (w < 0.01f) return false; // за камерой

    x /= w; y /= w;

    sx = (x * 0.5f + 0.5f) * viewport[2] + viewport[0];
    sy = (1.0f - (y * 0.5f + 0.5f)) * viewport[3] + viewport[1];
    return true;
}

static void drawBox2D(float x, float y, float w, float h)
{
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

// ── Основная функция ──────────────────────────────────────────────────────────
VOID WINAPI hooks::handles::gl_draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (!g_chams_enabled) {
        handles::originals::gl_draw_elements(mode, count, type, indices);
        return;
    }

    // Загружаем OpenGL функции один раз
    static glGetUniformLocation_fn  glGetUniformLocation = (glGetUniformLocation_fn)wglGetProcAddress("glGetUniformLocation");
    static glGetUniformiv_fn        glGetUniformiv       = (glGetUniformiv_fn)wglGetProcAddress("glGetUniformiv");
    static glGetUniformfv_fn        glGetUniformfv       = (glGetUniformfv_fn)wglGetProcAddress("glGetUniformfv");
    static glUniform3f_fn           glUniform3f          = (glUniform3f_fn)wglGetProcAddress("glUniform3f");
    static glActiveTexture_fn       glActiveTexture      = (glActiveTexture_fn)wglGetProcAddress("glActiveTexture");
    static glBlendColor_fn          glBlendColor         = (glBlendColor_fn)wglGetProcAddress("glBlendColor");

    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    if (current_program == 0) {
        handles::originals::gl_draw_elements(mode, count, type, indices);
        return;
    }

    // ── Детекция игрока через uniform-переменные ──────────────────────────────
    // Проверяем наличие характерных шейдерных переменных анимированных моделей
    GLint loc_isAnimated     = glGetUniformLocation(current_program, "isAnimated");
    GLint loc_emissionColor  = glGetUniformLocation(current_program, "emissionColor");
    GLint loc_diffuseColor   = glGetUniformLocation(current_program, "diffuseColor");
    GLint loc_specularColor  = glGetUniformLocation(current_program, "specularColor");
    GLint loc_lightSunColor  = glGetUniformLocation(current_program, "g_LightSunColor");

    // Дополнительно — через glUniform4fv/glUniform4ui как в qxzxf репо
    // Эти uniform-локации специфичны для шейдера персонажей
    GLint loc_boneMatrices   = glGetUniformLocation(current_program, "boneMatrices");
    GLint loc_color          = glGetUniformLocation(current_program, "color");

    bool has_character_uniforms = (loc_isAnimated != -1 &&
                                   loc_emissionColor != -1 &&
                                   loc_diffuseColor != -1 &&
                                   loc_specularColor != -1 &&
                                   loc_lightSunColor != -1);

    // Детекция через boneMatrices (кости = анимированный персонаж)
    bool has_bones = (loc_boneMatrices != -1);

    bool is_player = (has_character_uniforms || has_bones);

    if (!is_player) {
        handles::originals::gl_draw_elements(mode, count, type, indices);
        return;
    }

    // ── Сохраняем OpenGL состояние ────────────────────────────────────────────
    GLfloat backup_emission[3], backup_diffuse[3], backup_specular[3], backup_light[3];
    GLfloat backup_depth_range[2];
    GLint   backup_depth_func;
    GLfloat backup_blend_color[4];
    GLint   backup_blend_src, backup_blend_dst;
    GLboolean backup_blend, backup_depth_test;

    glGetFloatv(GL_BLEND_COLOR, backup_blend_color);
    glGetIntegerv(GL_BLEND_SRC, &backup_blend_src);
    glGetIntegerv(GL_BLEND_DST, &backup_blend_dst);
    glGetFloatv(GL_DEPTH_RANGE, backup_depth_range);
    glGetIntegerv(GL_DEPTH_FUNC, &backup_depth_func);
    backup_blend      = glIsEnabled(GL_BLEND);
    backup_depth_test = glIsEnabled(GL_DEPTH_TEST);

    if (has_character_uniforms) {
        glGetUniformfv(current_program, loc_emissionColor, backup_emission);
        glGetUniformfv(current_program, loc_diffuseColor,  backup_diffuse);
        glGetUniformfv(current_program, loc_specularColor, backup_specular);
        glGetUniformfv(current_program, loc_lightSunColor, backup_light);
    }

    // ── Сохраняем текстуры ────────────────────────────────────────────────────
    const GLint num_textures = 16;
    std::vector<GLint>  saved_samplers(num_textures, -1);
    std::vector<GLuint> saved_textures(num_textures, 0);
    static GLuint flat_texture = 0;
    if (!flat_texture) glGenTextures(1, &flat_texture);

    for (GLint i = 0; i < num_textures; ++i) {
        GLint loc = glGetUniformLocation(current_program, ("usedTextures[" + std::to_string(i) + "]").c_str());
        if (loc == -1) continue;
        GLint sampler;
        glGetUniformiv(current_program, loc, &sampler);
        glActiveTexture(GL_TEXTURE0 + sampler);
        GLint bound;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
        saved_samplers[i] = sampler;
        saved_textures[i] = (GLuint)bound;

        // Заливаем плоским цветом
        unsigned char px[4] = {
            (unsigned char)(g_chams_color[0]*255),
            (unsigned char)(g_chams_color[1]*255),
            (unsigned char)(g_chams_color[2]*255),
            255
        };
        glBindTexture(GL_TEXTURE_2D, flat_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }

    // ── Применяем чамсы ───────────────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
    glBlendColor(g_chams_color[0], g_chams_color[1], g_chams_color[2], 1.0f);

    if (has_character_uniforms) {
        glUniform3f(loc_emissionColor, g_chams_color[0], g_chams_color[1], g_chams_color[2]);
        glUniform3f(loc_diffuseColor,  g_chams_color[0], g_chams_color[1], g_chams_color[2]);
        glUniform3f(loc_specularColor, g_chams_color[0], g_chams_color[1], g_chams_color[2]);
        glUniform3f(loc_lightSunColor, g_chams_color[0], g_chams_color[1], g_chams_color[2]);
    }

    glDepthRange(0.0, 0.0); // рисуем поверх всего (wallhack)

    handles::originals::gl_draw_elements(mode, count, type, indices);

    // ── 3D Боксы ──────────────────────────────────────────────────────────────
    if (g_boxes_enabled)
    {
        // Получаем MVP матрицу и вьюпорт
        GLfloat mvp[16];
        GLint   viewport[4];
        glGetFloatv(GL_MODELVIEW_MATRIX, mvp); // берём ModelView как приближение
        glGetIntegerv(GL_VIEWPORT, viewport);

        // Примерный bounding box персонажа (в локальных координатах)
        // Подобраны под типичную модель Stalcraft
        const float bx = 0.4f, bz = 0.4f;
        const float by_min = 0.0f, by_max = 1.8f;

        // 8 углов AABB
        float corners[8][3] = {
            {-bx, by_min, -bz}, { bx, by_min, -bz},
            { bx, by_min,  bz}, {-bx, by_min,  bz},
            {-bx, by_max, -bz}, { bx, by_max, -bz},
            { bx, by_max,  bz}, {-bx, by_max,  bz},
        };

        float sx[8], sy[8];
        bool  valid[8];
        for (int i = 0; i < 8; ++i)
            valid[i] = worldToScreen(mvp, corners[i][0], corners[i][1], corners[i][2],
                                     sx[i], sy[i], viewport);

        // Проверяем что хотя бы часть точек валидна
        float minX =  1e9f, minY =  1e9f;
        float maxX = -1e9f, maxY = -1e9f;
        bool any_valid = false;
        for (int i = 0; i < 8; ++i) {
            if (!valid[i]) continue;
            any_valid = true;
            if (sx[i] < minX) minX = sx[i];
            if (sy[i] < minY) minY = sy[i];
            if (sx[i] > maxX) maxX = sx[i];
            if (sy[i] > maxY) maxY = sy[i];
        }

        if (any_valid) {
            // Переходим в 2D режим для рисования бокса
            GLint vp[4];
            glGetIntegerv(GL_VIEWPORT, vp);

            glMatrixMode(GL_PROJECTION);
            GLfloat proj_backup[16];
            glGetFloatv(GL_PROJECTION_MATRIX, proj_backup);
            glLoadIdentity();
            glOrtho(vp[0], vp[0]+vp[2], vp[1]+vp[3], vp[1], -1, 1);

            glMatrixMode(GL_MODELVIEW);
            GLfloat mv_backup[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, mv_backup);
            glLoadIdentity();

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glLineWidth(2.0f);

            // Рисуем бокс цветом чамсов
            glColor3f(g_chams_color[0], g_chams_color[1], g_chams_color[2]);
            drawBox2D(minX, minY, maxX - minX, maxY - minY);

            // Восстанавливаем матрицы
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(proj_backup);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(mv_backup);

            glLineWidth(1.0f);
            if (backup_depth_test) glEnable(GL_DEPTH_TEST);
        }
    }

    // ── Восстанавливаем OpenGL состояние ──────────────────────────────────────
    if (has_character_uniforms) {
        glUniform3f(loc_emissionColor, backup_emission[0], backup_emission[1], backup_emission[2]);
        glUniform3f(loc_diffuseColor,  backup_diffuse[0],  backup_diffuse[1],  backup_diffuse[2]);
        glUniform3f(loc_specularColor, backup_specular[0], backup_specular[1], backup_specular[2]);
        glUniform3f(loc_lightSunColor, backup_light[0],    backup_light[1],    backup_light[2]);
    }

    glDepthRange(backup_depth_range[0], backup_depth_range[1]);
    glDepthFunc(backup_depth_func);
    if (backup_blend)      glEnable(GL_BLEND);      else glDisable(GL_BLEND);
    if (backup_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glBlendColor(backup_blend_color[0], backup_blend_color[1],
                 backup_blend_color[2], backup_blend_color[3]);
    glBlendFunc(backup_blend_src, backup_blend_dst);

    // Восстанавливаем текстуры
    for (GLint i = 0; i < num_textures; ++i) {
        if (saved_samplers[i] == -1) continue;
        glActiveTexture(GL_TEXTURE0 + saved_samplers[i]);
        glBindTexture(GL_TEXTURE_2D, saved_textures[i]);
    }
}
