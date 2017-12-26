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

#include "device_manager.hpp"
#include "draw_utils.hpp"

bool DrawTextProgram::create()
{
    bool success = init("draw_text.vert", "draw_text.frag");
    
    if (!success)
        return false;
        
    success = assignAttrib(m_coord, "coord");
    if (!success)
        return false;

    success = assignUniform(m_tex, "tex");
    if (!success)
        return false;

    success = assignUniform(m_color, "color");
    if (!success)
        return false;

    return true;
}

bool DrawTextureProgram::create()
{
    bool success = init("draw_texture.vert", "draw_texture.frag");
    
    if (!success)
        return false;

    success = assignAttrib(m_coord, "coord");
    if (!success)
        return false;

    success = assignUniform(m_tex, "tex");
    if (!success)
        return false;

    return true;
}

DrawUtils* DrawUtils::m_draw_utils = NULL;

DrawUtils::DrawUtils()
{
    m_draw_utils = this;
    m_draw_text = NULL;
    m_draw_texture = NULL;
}

DrawUtils::~DrawUtils()
{
    glDeleteBuffers(1, &m_vbo);
    
    delete m_draw_text;
    delete m_draw_texture;
}

bool DrawUtils::init()
{
    m_draw_text = new DrawTextProgram();
    bool success = m_draw_text->create();
    
    m_draw_texture = new DrawTextureProgram();
    success = success && m_draw_texture->create();
    
    glGenBuffers(1, &m_vbo);
    
    return success;
}


void DrawUtils::drawText(Texture* texture, int pos_x, int pos_y, 
                             GLfloat color[4])
{
    glUseProgram(m_draw_text->getProgram());

    glEnableVertexAttribArray(m_draw_text->m_coord);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(m_draw_text->m_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glUniform4fv(m_draw_text->m_color, 1, color);
    glUniform1i(m_draw_text->m_tex, 0);

    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    unsigned int window_w = device->getWindowWidth();
    unsigned int window_h = device->getWindowHeight();

    float x = (float)(pos_x) / window_w * 2.0f - 1.0f;
    float y = (float)(pos_y) / window_h * 2.0f - 1.0f;
    float w = (float)(texture->width) / window_w * 2.0f;
    float h = (float)(texture->height) / window_h * 2.0f;
    float tex_w = texture->tex_w;
    float tex_h = texture->tex_h;
    
    GLfloat box[4][4] = { {x, -y,         0, 0},
                          {x + w, -y,     tex_w, 0},
                          {x, -y - h,     0, tex_h},
                          {x + w, -y - h, tex_w, tex_h} };
    
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisable(GL_BLEND);
    glDisableVertexAttribArray(m_draw_text->m_coord);
    glUseProgram(0);
}

void DrawUtils::drawTexture2D(Texture* texture, int pos_x, int pos_y, 
                                  int width, int height)
{
    glUseProgram(m_draw_texture->getProgram());

    glEnableVertexAttribArray(m_draw_texture->m_coord);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(m_draw_texture->m_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glUniform1i(m_draw_texture->m_tex, 0);

    glActiveTexture(GL_TEXTURE0);

    if (texture->channels != 3)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    unsigned int window_w = device->getWindowWidth();
    unsigned int window_h = device->getWindowHeight();

    float x = (float)(pos_x) / window_w * 2.0f - 1.0f;
    float y = (float)(pos_y) / window_h * 2.0f - 1.0f;
    float w = (float)(width) / window_w * 2.0f;
    float h = (float)(height) / window_h * 2.0f;
    float tex_w = texture->tex_w;
    float tex_h = texture->tex_h;
    
    GLfloat box[4][4] = { {x, -y,         0, 0},
                          {x + w, -y,     tex_w, 0},
                          {x, -y - h,     0, tex_h},
                          {x + w, -y - h, tex_w, tex_h} };

    glBindTexture(GL_TEXTURE_2D, texture->id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (texture->channels != 3)
    {
        glDisable(GL_BLEND);
    }
    
    glDisableVertexAttribArray(m_draw_texture->m_coord);
    glUseProgram(0);
}
