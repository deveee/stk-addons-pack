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

#ifndef SHADER_HPP
#define SHADER_HPP

#include <GLES3/gl3.h>

#include <string>

class Shader
{
protected:
    GLuint m_program;
    GLuint m_vert;
    GLuint m_frag;
    
    bool init(std::string vs_name, std::string fs_name);
    bool assignAttrib(GLint& attrib, std::string attrib_name);
    bool assignUniform(GLint& uniform, std::string uniform_name);
    GLuint makeShader(GLenum type, std::string filename);
    GLuint makeProgram(GLuint vertex_shader, GLuint fragment_shader);
    
public:
    Shader();
    virtual ~Shader();

    virtual bool create() = 0;
    
    GLuint getProgram() {return m_program;}
    GLuint getVert() {return m_vert;}
    GLuint getFrag() {return m_frag;}
};

#endif
