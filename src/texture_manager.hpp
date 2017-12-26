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

#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP

#include <GLES3/gl3.h>

#include <map>
#include <string>

struct Image
{
    int width;
    int height;
    int channels;
    int data_length;
    unsigned char* data;
};

struct Texture
{
    GLuint id;
    int width;
    int height;
    float tex_w;
    float tex_h;
    int channels;
};

class TextureManager
{
private:
    bool m_supports_npot;
    std::map<std::string, Texture*> m_textures;
    static TextureManager* m_texture_manager;
    
    void loadTextures();
    int getPotDimension(int value);

public:
    TextureManager();
    ~TextureManager();
    
    bool init();
    Texture* createTexture(int width, int height, int channels, 
                           const void* data);
    void deleteTexture(Texture* texture);
    Texture* getTexture(std::string name) {return m_textures[name];}
    
    static TextureManager* getTextureManager() {return m_texture_manager;}
};

#endif
