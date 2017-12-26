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

#include <cstdio>

#include "file_manager.hpp"
#include "shader.hpp"

Shader::Shader()
{
    m_vert = 0;
    m_frag = 0;
    m_program = 0;
}

Shader::~Shader()
{
    glDeleteShader(m_vert);
    glDeleteShader(m_frag);
    glDeleteProgram(m_program);
}

bool Shader::init(std::string vs_name, std::string fs_name)
{
    m_vert = makeShader(GL_VERTEX_SHADER, vs_name);
    
    if (m_vert == 0)
        return false;

    m_frag = makeShader(GL_FRAGMENT_SHADER, fs_name);
    
    if (m_frag == 0)
        return false;

    m_program = makeProgram(m_vert, m_frag);
    
    if (m_program == 0)
        return false;
        
    return true;
}

bool Shader::assignAttrib(GLint& attrib, std::string attrib_name)
{
    attrib = glGetAttribLocation(m_program, attrib_name.c_str());
    
    if (attrib == -1)
    {
        printf("Error: Failed to get attrib location %s\n", 
               attrib_name.c_str());
        return false;
    }
    
    return true;
}

bool Shader::assignUniform(GLint& uniform, std::string uniform_name)
{
    uniform = glGetUniformLocation(m_program, uniform_name.c_str());
    
    if (uniform == -1)
    {
        printf("Error: Failed to get uniform location %s\n", 
               uniform_name.c_str());
        return false;
    }
    
    return true;
}

GLuint Shader::makeShader(GLenum type, std::string filename)
{
    FileManager* file_manager = FileManager::getFileManager();
    File* file = file_manager->loadFile(filename);
    
    if (file == NULL)
        return 0;

    std::string code = "//" + std::string(filename) + "\n";
    
    if (type == GL_FRAGMENT_SHADER)
    {
        int range[2]; 
        int precision;

        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range,
                                   &precision);
    
        if (precision > 0)
        {
            code += "precision highp float;\n";
        }
        else
        {
            code += "precision mediump float;\n";
        }
    }

    code.append(file->data, file->length);
    
    file_manager->closeFile(file);

    const char* code_ptr = code.c_str();
    int length = (int)code.size();
    
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code_ptr, &length);
    glCompileShader(shader);

    GLint shader_ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    
    if (!shader_ok)
    {
        printf("Error: Failed to compile %s\n", filename.c_str());
        
        int length = 1024;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        
        if (length > 0)
        {
            char msg[length] = {};
            
            glGetShaderInfoLog(shader, length, NULL, msg);
            
            printf("%s\n", msg);
        }
    }

    glGetError();

    return shader;
}

GLuint Shader::makeProgram(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint program_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    
    if (!program_ok)
    {
        printf("Error: Failed to link shader program\n");
        glDeleteProgram(program);
        return 0;
    }

    return program;
}
