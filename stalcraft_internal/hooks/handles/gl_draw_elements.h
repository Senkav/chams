// Глобальные переменные из dllmain.cpp
extern bool  g_chams_enabled;
extern float g_chams_color[3];

typedef GLint(APIENTRY* glGetUniformLocation_fn)(GLuint, const char*);
typedef void(APIENTRY* glGetUniformiv_fn)(GLuint program, GLint location, GLint* params);
typedef void(APIENTRY* glActiveTexture_fn)(GLenum);
typedef void(APIENTRY* glGetUniformfv_fn)(GLuint program, GLint location, GLfloat* params);
typedef void(APIENTRY* glUniform3f_fn)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void(APIENTRY* glBlendColor_fn)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

#define GL_CURRENT_PROGRAM  0x8B8D
#define GL_ACTIVE_UNIFORMS  0x8B86
#define GL_TEXTURE0         0x84C0
#define GL_CONSTANT_COLOR   0x8001
#define GL_BLEND_COLOR      0x8005

VOID WINAPI hooks::handles::gl_draw_elements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    // Если чамсы выключены — рисуем как обычно
    if (!g_chams_enabled) {
        handles::originals::gl_draw_elements(mode, count, type, indices);
        return;
    }

    static glGetUniformLocation_fn  glGetUniformLocation    = (glGetUniformLocation_fn)wglGetProcAddress("glGetUniformLocation");
    static glGetUniformiv_fn        glGetUniformiv          = (glGetUniformiv_fn)wglGetProcAddress("glGetUniformiv");
    static glActiveTexture_fn       glActiveTexture         = (glActiveTexture_fn)wglGetProcAddress("glActiveTexture");
    static glGetUniformfv_fn        glGetUniformfv          = (glGetUniformfv_fn)wglGetProcAddress("glGetUniformfv");
    static glUniform3f_fn           glUniform3f             = (glUniform3f_fn)wglGetProcAddress("glUniform3f");
    static glBlendColor_fn          glBlendColor            = (glBlendColor_fn)wglGetProcAddress("glBlendColor");

    GLint current_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);

    GLint is_animated_location = glGetUniformLocation(current_program, "isAnimated");
    GLint emission_color_location = glGetUniformLocation(current_program, "emissionColor");
    GLint diffuse_color_location = glGetUniformLocation(current_program, "diffuseColor");
    GLint specular_color_location = glGetUniformLocation(current_program, "specularColor");
    GLint light_sun_color_location = glGetUniformLocation(current_program, "g_LightSunColor");

    bool is_player = false;

    if (is_animated_location != -1 && emission_color_location != -1 && diffuse_color_location != -1 && specular_color_location != -1 && light_sun_color_location != -1)
    {
        if (
            count != 24 && count != 48 && count != 60 && count != 102 && count != 108
            && count != 141 && count != 216 && count != 222 && count != 321 && count != 384
            && count != 570 && count != 576 && count != 594 && count != 840 && count != 864
            && count != 972 && count != 1230 && count != 1419 && count != 1434 && count != 1740
            && count != 1842 && count != 1866 && count != 2160 && count != 2628 && count != 2721
            && count != 3672 && count != 4320 && count != 4902 && count != 5148 && count != 5274
            && count != 6756 && count != 7530 && count != 7548 && count != 9033 && count != 9057
            && count != 9648 && count != 233670 && count != 16344 && count != 3318 && count != 1917
            && count != 4284 && count != 1662 && count != 4098 && count != 1827 && count != 828
            && count != 960 && count != 6366 && count != 414 && count != 912 && count != 939
            && count != 510 && count != 396 && count != 543 && count != 3792 && count != 1887
            && count != 330 && count != 465 && count != 2193 && count != 1596 && count != 297
            && count != 459 && count != 1095 && count != 1026 && count != 366 && count != 1242
            && count != 2052 && count != 948 && count != 846 && count != 852 && count != 1464
            && count != 1386 && count != 228 && count != 1692 && count != 1758 && count != 444
            && count != 480 && count != 894 && count != 1698 && count != 3408 && count != 426
            && count != 432 && count != 327 && count != 420 && count != 429 && count != 861
            && count != 711 && count != 645 && count != 897 && count != 2070 && count != 930
            && count != 345 && count != 4158 && count != 7170 && count != 447 && count != 891
            && count != 1788 && count != 735 && count != 582 && count != 597 && count != 1224
            && count != 1302 && count != 672 && count != 7032
            )
        {
            is_player = true;
        }
    }

    if (is_player)
    {
        GLfloat backup_emission_color[3];
        GLfloat backup_diffuse_color[3];
        GLfloat backup_specular_color[3];
        GLfloat backup_light_sun_color[3];
        GLfloat backup_depth_range[2];
        GLint backup_depth_func;
        GLint backup_texture;
        GLint num_textures = 99;
        std::vector<GLint> saved_samplers(num_textures);
        std::vector<GLuint> saved_textures(num_textures);
        GLfloat backup_blend_color[4];
        GLint backup_blend_src, backup_blend_dst;

        // SAVE BACKUP
        {
            glGetFloatv(GL_BLEND_COLOR, backup_blend_color);
            glGetIntegerv(GL_BLEND_SRC, &backup_blend_src);
            glGetIntegerv(GL_BLEND_DST, &backup_blend_dst);
            glGetUniformfv(current_program, emission_color_location, backup_emission_color);
            glGetUniformfv(current_program, diffuse_color_location, backup_diffuse_color);
            glGetUniformfv(current_program, specular_color_location, backup_specular_color);
            glGetUniformfv(current_program, light_sun_color_location, backup_light_sun_color);
            glGetFloatv(GL_DEPTH_RANGE, backup_depth_range);
            glGetIntegerv(GL_DEPTH_FUNC, &backup_depth_func);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &backup_texture);
        }

        {
            static GLuint texture = 0;
            if (!texture) glGenTextures(1, &texture);

            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_CONSTANT_COLOR);
            glBlendColor(g_chams_color[0], g_chams_color[1], g_chams_color[2], 1.0f);

            glUniform3f(emission_color_location, g_chams_color[0], g_chams_color[1], g_chams_color[2]);
            glUniform3f(diffuse_color_location,  g_chams_color[0], g_chams_color[1], g_chams_color[2]);
            glUniform3f(specular_color_location, g_chams_color[0], g_chams_color[1], g_chams_color[2]);
            glUniform3f(light_sun_color_location,g_chams_color[0], g_chams_color[1], g_chams_color[2]);

            unsigned char color[4] = {
                (unsigned char)(g_chams_color[0]*255),
                (unsigned char)(g_chams_color[1]*255),
                (unsigned char)(g_chams_color[2]*255),
                255
            };

            glDepthRange(0.0, 0.0);

            for (GLint i = 0; i < num_textures; ++i)
            {
                GLint location = glGetUniformLocation(current_program, ("usedTextures[" + std::to_string(i) + "]").c_str());
                if (location == -1) continue;
                GLint sampler;
                glGetUniformiv(current_program, location, &sampler);
                glActiveTexture(GL_TEXTURE0 + sampler);
                GLint bound_texture;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
                saved_samplers[i] = sampler;
                saved_textures[i] = bound_texture;
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
            }
        }

        handles::originals::gl_draw_elements(mode, count, type, indices);

        glUniform3f(emission_color_location, backup_emission_color[0], backup_emission_color[1], backup_emission_color[2]);
        glUniform3f(diffuse_color_location, backup_diffuse_color[0], backup_diffuse_color[1], backup_diffuse_color[2]);
        glUniform3f(specular_color_location, backup_specular_color[0], backup_specular_color[1], backup_specular_color[2]);
        glUniform3f(light_sun_color_location, backup_light_sun_color[0], backup_light_sun_color[1], backup_light_sun_color[2]);
        glDepthRange(backup_depth_range[0], backup_depth_range[1]);
        glDepthFunc(backup_depth_func);
        glEnable(GL_BLEND);
        glBlendColor(backup_blend_color[0], backup_blend_color[1], backup_blend_color[2], backup_blend_color[3]);
        glBlendFunc(backup_blend_src, backup_blend_dst);

        for (GLint i = 0; i < num_textures; ++i)
        {
            GLint sampler = saved_samplers[i];
            GLuint texture = saved_textures[i];
            if (sampler != -1)
            {
                glActiveTexture(GL_TEXTURE0 + sampler);
                glBindTexture(GL_TEXTURE_2D, texture);
            }
        }

        return;
    }

    handles::originals::gl_draw_elements(mode, count, type, indices);
}
