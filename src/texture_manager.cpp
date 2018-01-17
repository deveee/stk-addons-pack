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

#include "file_manager.hpp"
#include "image_loader.hpp"
#include "texture_manager.hpp"

#include <cstring>

TextureManager* TextureManager::m_texture_manager = NULL;

TextureManager::TextureManager()
{
    m_texture_manager = this;
    m_supports_npot = false;
}

TextureManager::~TextureManager()
{
    for (auto texture : m_textures)
    {
        deleteTexture(texture.second);
    }
}

bool TextureManager::init()
{
    int major_gl = 2;
    glGetIntegerv(GL_MAJOR_VERSION, &major_gl);
    
    const char* renderer = (const char*)glGetString(GL_RENDERER);

    m_supports_npot = (major_gl >= 3);

    // GLES 3.0 is broken under emulator
    if (renderer == NULL || strstr(renderer, "Android Emulator") != NULL)
    {
        m_supports_npot = false;
    }    
    
    loadTextures();
    
    return true;
}

void TextureManager::loadTextures()
{
    FileManager* file_manager = FileManager::getFileManager();
    std::vector<std::string> assets_list = file_manager->getAssetsList();
    
    for (std::string name : assets_list)
    {
        Image* image = ImageLoader::loadImage(name);
        
        if (image == NULL)
            continue;
        
        m_textures[name] = createTexture(image->width, image->height,
                                         image->channels, image->data);
        ImageLoader::closeImage(image);
    }
}

int TextureManager::getPotDimension(int value)
{
    int value_pot = 1;
    
    while (value_pot < value)
    {
        value_pot <<= 1;
    }
    
    return value_pot;
}

Texture* TextureManager::createTexture(int width, int height, int channels,
                                       const void* data)
{
    GLint internal_format;
    GLenum format;
        
    switch (channels)
    {
    case 1:
        format = GL_LUMINANCE;
        internal_format = GL_LUMINANCE;
        break;
    case 3:
        format = GL_RGB;
        internal_format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        internal_format = GL_RGBA;
        break;
    default:
        return NULL;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Texture* texture = new Texture();
    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
    if (m_supports_npot)
    {
        texture->tex_w = 1.0f;
        texture->tex_h = 1.0f;
        
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, 
                     format, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        int width_pot = getPotDimension(width);
        int height_pot = getPotDimension(height);
        texture->tex_w = (float)(width) / width_pot;
        texture->tex_h = (float)(height) / height_pot;
        
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_pot, height_pot, 
                     0, format, GL_UNSIGNED_BYTE, NULL);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, 
                        GL_UNSIGNED_BYTE, data);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return texture;
}

void TextureManager::deleteTexture(Texture* texture)
{
    glDeleteTextures(1, &texture->id);
    
    delete texture;
}
