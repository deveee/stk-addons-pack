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

#include "draw_utils.hpp"
#include "font_manager.hpp"

#include <sstream>

FontManager* FontManager::m_font_manager = NULL;

FontManager::FontManager()
{
    m_current_font = NULL;
    m_font_manager = this;
}

bool FontManager::init()
{
    int err = FT_Init_FreeType(&m_ft_library);
    
    if (err != 0) 
    {
        printf("Error: Could not initialize freetype library\n");
        return false;
    }
    
    FileManager* file_manager = FileManager::getFileManager();
    std::vector<std::string> assets_list = file_manager->getAssetsList();
    
    for (std::string font_name : assets_list)
    {
        std::string extension = file_manager->getExtension(font_name);
        
        if (extension != ".ttf" && extension != ".otf")
            continue;

        File* file = file_manager->loadFile(font_name);
        
        if (file == NULL)
            continue;
        
        FontData* font = new FontData();
        font->font_name = font_name;
        font->font_file = file;
        
        int err = FT_New_Memory_Face(m_ft_library, (FT_Byte*)file->data, 
                                     file->length, 0, &font->ft_face);
        
        if (err != 0) 
        {
            printf("Error: Could not create font face for %s\n", 
                   font_name.c_str());
            file_manager->closeFile(file);
            delete font;
            continue;
        }
    
        FT_Select_Charmap(font->ft_face, ft_encoding_unicode);
        
        m_fonts.push_back(font);
    }

    return true;
}

FontManager::~FontManager()
{
    FileManager* file_manager = FileManager::getFileManager();
    TextureManager* texture_manager = TextureManager::getTextureManager();

    for (FontData* font : m_fonts)
    {
        for (auto texture : font->chars)
        {
            texture_manager->deleteTexture(texture.second);
        }
        
        FT_Done_Face(font->ft_face);
        
        file_manager->closeFile(font->font_file);
        delete font;
    }
    
    FT_Done_FreeType(m_ft_library);
}

void FontManager::changeFont(std::string font_name)
{
    for (FontData* font : m_fonts)
    {
        if (font->font_name == font_name)
        {
            m_current_font = font;
            return;
        }
    }
    
    m_current_font = NULL;
}

std::wstring FontManager::convertToUTF32(std::string str)
{
    // std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
    // std::u32string str32 = cvt.from_bytes(str);
    
    std::wstring result;
    unsigned int codepoint;

    for (unsigned int i = 0; i < str.length(); i++)
    {
        unsigned char ch = (unsigned char)(str.at(i));
        
        if (ch <= 0x7f)
        {
            codepoint = ch;
        }
        else if (ch <= 0xbf)
        {
            codepoint = (codepoint << 6) | (ch & 0x3f);
        }
        else if (ch <= 0xdf)
        {
            codepoint = ch & 0x1f;
        }
        else if (ch <= 0xef)
        {
            codepoint = ch & 0x0f;
        }
        else
        {
            codepoint = ch & 0x07;
        }

        if ((i == str.length() - 1 || (str.at(i+1) & 0xc0) != 0x80) && 
            (codepoint <= 0x10ffff))
        {
            if (sizeof(wchar_t) > 2)
            {
                result += (wchar_t)(codepoint);
            }
            else if (codepoint > 0xffff)
            {
                result += (wchar_t)(0xd800 + (codepoint >> 10));
                result += (wchar_t)(0xdc00 + (codepoint & 0x03ff));
            }
            else if (codepoint < 0xd800 || codepoint >= 0xe000)
            {
                result += (wchar_t)(codepoint);
            }
        }
    }
    
    return result;
}

void FontManager::drawText(std::string text, int pos_x, int pos_y, int size, 
                           float color[4])
{
    std::wstring text32 = convertToUTF32(text);
    drawText(text32, pos_x, pos_y, size, color);
}

void FontManager::drawText(std::wstring text, int pos_x, int pos_y, int size, 
                           float color[4]) 
{
    if (m_current_font == NULL)
        return;
       
    DrawUtils* draw_utils = DrawUtils::getDrawUtils();
    TextureManager* texture_manager = TextureManager::getTextureManager();
    
    FT_Set_Pixel_Sizes(m_current_font->ft_face, 0, size);
    FT_GlyphSlot g = m_current_font->ft_face->glyph;
    
    for (wchar_t c : text) 
    {
        if (FT_Load_Char(m_current_font->ft_face, c, FT_LOAD_RENDER))
            continue;
                    
        std::ostringstream code_ss;
        code_ss << size;
        code_ss << c;
        std::string code = code_ss.str();
        
        Texture* texture = m_current_font->chars[code];

        if (texture == NULL)
        {
            texture = texture_manager->createTexture(g->bitmap.width, 
                                                     g->bitmap.rows, 1,                                                   
                                                     g->bitmap.buffer);
            m_current_font->chars[code] = texture;
        }

        draw_utils->drawText(texture, pos_x + g->bitmap_left, 
                             pos_y - g->bitmap_top, color);

        pos_x += (g->advance.x / 64);
        pos_y += (g->advance.y / 64);
    }
}

int FontManager::getTextWidth(std::string text, int size)
{
    std::wstring text32 = convertToUTF32(text);
    return getTextWidth(text32, size);
}

int FontManager::getTextWidth(std::wstring text, int size) 
{
    if (m_current_font == NULL)
        return 0;
       
    FT_Set_Pixel_Sizes(m_current_font->ft_face, 0, size);
    FT_GlyphSlot g = m_current_font->ft_face->glyph;
    
    int width = 0;
    
    for (wchar_t c : text) 
    {
        if (FT_Load_Char(m_current_font->ft_face, c, FT_LOAD_RENDER))
            continue;

        width += (g->advance.x / 64);
    }
    
    return width;
}

int FontManager::getRealFontHeight(int size)
{
    if (m_current_font == NULL)
        return 0;
        
    FT_Set_Pixel_Sizes(m_current_font->ft_face, 0, size);
    
    if (FT_Load_Char(m_current_font->ft_face, L'X', FT_LOAD_RENDER))
        return 0;
        
    return (m_current_font->ft_face->glyph->bitmap.rows);
}
