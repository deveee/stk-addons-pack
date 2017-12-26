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

#include "button.hpp"
#include "device_manager.hpp"
#include "draw_utils.hpp"
#include "font_manager.hpp"

Button::Button()
{
    m_active = true;
    m_pos_x = 0;
    m_pos_y = 0;
    m_width = 0;
    m_height = 0;
    m_text_x = 0;
    m_text_y = 0;
    m_text_height = 0;
    m_normal_tex = NULL;
    m_hover_tex = NULL;
    m_inactive_tex = NULL;
}

Button::~Button()
{
    
}

bool Button::init(std::string name, std::string text, int pos_x, int pos_y, 
                  int width)
{
    TextureManager* texture_manager = TextureManager::getTextureManager();
    m_normal_tex = texture_manager->getTexture("button.png");
    m_hover_tex = texture_manager->getTexture("button_hover.png");
    m_inactive_tex = texture_manager->getTexture("button_inactive.png");

    m_name = name;
    m_pos_x = pos_x;
    m_pos_y = pos_y;
    m_width = width;
    m_height = (float)width * m_normal_tex->height / m_normal_tex->width;
    setText(text);

    return true;
}

void Button::setText(std::string text)
{
    FontManager* font_manager = FontManager::getFontManager();
    font_manager->changeFont("FreeSans.ttf");

    m_text = text;
    m_text_height = std::max((int)(m_height * 0.65f), 1);
    
    int real_text_height = font_manager->getRealFontHeight(m_text_height);
    int text_width = font_manager->getTextWidth(m_text, m_text_height);
    
    m_text_x = m_pos_x + (m_width - text_width) / 2;
    m_text_y = m_pos_y + (m_height + real_text_height) / 2;
}

void Button::draw()
{
    Texture* texture = m_normal_tex;
    
    if (m_active == false)
    {
        texture = m_inactive_tex;
    }
    else if (isCursorOverButton())
    {
        texture = m_hover_tex;
    }
    
    DrawUtils* draw_utils = DrawUtils::getDrawUtils();
    draw_utils->drawTexture2D(texture, m_pos_x, m_pos_y, m_width, m_height);
    
    GLfloat black[4] = { 0, 0, 0, 1 };
    FontManager* font_manager = FontManager::getFontManager();
    font_manager->changeFont("FreeSans.ttf");
    font_manager->drawText(m_text, m_text_x, m_text_y, m_text_height, black);
}

bool Button::isCursorOverButton()
{
    int pos_x = 0;
    int pos_y = 0;
    
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    device->getCursorPosition(&pos_x, &pos_y);
    
    return isCursorOverButton(pos_x, pos_y);
}

bool Button::isCursorOverButton(int pos_x, int pos_y)
{
    return (pos_x >= m_pos_x && pos_x <= m_pos_x + m_width &&
            pos_y >= m_pos_y && pos_y <= m_pos_y + m_height);
}
