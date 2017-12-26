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

#include "progress_bar.hpp"

bool ProgressBarProgram::create()
{
    bool success = init("progress_bar.vert", "progress_bar.frag");
    
    if (!success)
        return false;

    success = assignAttrib(m_coord, "coord");
    if (!success)
        return false;

    success = assignUniform(m_progress, "progress");
    if (!success)
        return false;
    
    success = assignUniform(m_color, "color");
    if (!success)
        return false;

    return true;
}

ProgressBar::ProgressBar()
{
    m_vbo = 0;
    m_value = 0.0f;
}

ProgressBar::~ProgressBar()
{
    glDeleteBuffers(1, &m_vbo);
    
    delete m_program;
}

bool ProgressBar::init(float pos_x, float pos_y, float width, float height)
{
    m_program = new ProgressBarProgram();
    bool success = m_program->create();
    
    if (!success)
        return false;

    float x = pos_x * 2.0f - 1.0f;
    float y = pos_y * 2.0f - 1.0f;
    float w = width * 2.0f;
    float h = height * 2.0f;
    
    GLfloat box[4][2] = {{x, -y}, {x + w, -y}, {x, -y - h}, {x + w, -y - h}};

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);

    return true;
}

void ProgressBar::draw(GLfloat color[4])
{
    glUseProgram(m_program->getProgram());
    
    glEnableVertexAttribArray(m_program->m_coord);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(m_program->m_coord, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glUniform4fv(m_program->m_color, 1, color);
    glUniform1f(m_program->m_progress, std::min(m_value, 1.0f));
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(m_program->m_coord);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
