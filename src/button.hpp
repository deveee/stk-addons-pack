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

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "texture_manager.hpp"

#include <string>

class Button
{
private:
    bool m_active;
    int m_pos_x;
    int m_pos_y;
    int m_width;
    int m_height;
    int m_text_x;
    int m_text_y;
    int m_text_height;
    std::string m_name;
    std::string m_text;
    Texture* m_normal_tex;
    Texture* m_hover_tex;
    Texture* m_inactive_tex;
    
public:
    Button();
    ~Button();
    
    bool init(std::string name, std::string text, int pos_x, int pos_y, 
              int width);
    void draw();
    bool isCursorOverButton();
    bool isCursorOverButton(int pos_x, int pos_y);
    std::string getName() {return m_name;}
    void setText(std::string text);
    void setActive(bool active) {m_active = active;}
    void setPosX(int pos_x) {m_text_x += pos_x - m_pos_x; m_pos_x = pos_x;}
    void setPosY(int pos_y) {m_text_y += pos_y - m_pos_y; m_pos_y = pos_y;}
};

#endif
