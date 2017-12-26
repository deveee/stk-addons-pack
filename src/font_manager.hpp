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

#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include "file_manager.hpp"
#include "texture_manager.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <map>
#include <vector>

struct FontData
{
    std::string font_name;
    std::map<std::string, Texture*> chars;
    File* font_file;
    FT_Face ft_face;
};

class FontManager
{
private:
    FT_Library m_ft_library;
    std::vector<FontData*> m_fonts;
    FontData* m_current_font;
    static FontManager* m_font_manager;
    
    std::wstring convertToUTF32(std::string str);

public:
    FontManager();
    ~FontManager();
    
    bool init();
    bool initFont(std::string font_name);
    void changeFont(std::string font_name);
    void drawText(std::string text, int pos_x, int pos_y, int size, 
                  GLfloat color[4]);
    void drawText(std::wstring text, int pos_x, int pos_y, int size, 
                  GLfloat color[4]);
    int getTextWidth(std::string text, int size);
    int getTextWidth(std::wstring text, int size);
    int getRealFontHeight(int size);
    
    static FontManager* getFontManager() {return m_font_manager;}
};

#endif
