//    STK Add-ons pack - Simple add-ons installer for Android
//    Copyright (C) 2017 Dawid Gan <deveee@gmail.com>
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef DRAW_UTILS_HPP
#define DRAW_UTILS_HPP

#include "shader.hpp"
#include "texture_manager.hpp"

class DrawTextProgram : public Shader
{
public:
    GLint m_coord;
    GLint m_tex;
    GLint m_color;

    bool create();
};

class DrawTextureProgram : public Shader
{
public:
    GLint m_coord;
    GLint m_tex;

    bool create();
};

class DrawUtils
{
private:
    GLuint m_vbo;
    DrawTextProgram* m_draw_text;
    DrawTextureProgram* m_draw_texture;
    static DrawUtils* m_draw_utils;

public:
    DrawUtils();
    ~DrawUtils();
    
    bool init();
    void drawText(Texture* texture, int pos_x, int pos_y, GLfloat color[4]);
    void drawTexture2D(Texture* texture, int pos_x, int pos_y, int width, 
                       int height);
    
    static DrawUtils* getDrawUtils() {return m_draw_utils;}
};

#endif
