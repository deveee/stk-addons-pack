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

#ifndef PROGRESS_BAR_HPP
#define PROGRESS_BAR_HPP

#include "shader.hpp"

class ProgressBarProgram : public Shader
{
public:
    GLint m_coord;
    GLint m_progress;
    GLint m_color;

    bool create();
};

class ProgressBar
{
private:
    ProgressBarProgram* m_program;
    GLuint m_vbo;
    float m_value;

public:
    ProgressBar();
    ~ProgressBar();

    bool init(float pos_x, float pos_y, float width, float height);
    void draw(GLfloat color[4]);
    float getValue() {return m_value;}
    void setValue(float value) {m_value = value;}
};

#endif
