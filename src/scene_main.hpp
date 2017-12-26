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

#ifndef SCENE_MAIN_HPP
#define SCENE_MAIN_HPP

#include "scene_manager.hpp"
#include "texture_manager.hpp"

#include <string>

enum ExtractState
{
    ES_DEST_DIR_NOT_FOUND,
    ES_NOT_INSTALLED,
    ES_ALREADY_INSTALLED,
    ES_INSTALLING,
    ES_INSTALLED,
    ES_INSTALLATION_FAILED
};

class Button;
class ProgressBar;

class SceneMain : public Scene
{
private:
    ProgressBar* m_progress_bar;
    Button* m_button_install;
    Button* m_button_close;
    Texture* m_background;
    Texture* m_logo;
    Texture* m_screenshot;
    Texture* m_text_bg;
    std::string m_text;
    std::string m_text2;
    float m_gui_scale;
    int m_text_height;
    int m_btn_width;
    
    std::vector<std::string> m_extract_assets;
    std::string m_extract_dest;
    unsigned int m_extract_progress;
    ExtractState m_extract_state;
    std::string m_extract_title;
    std::string m_extract_screenshot;
    std::string m_extract_marker;
    
    void drawScene();
    void setState(ExtractState state);
    void readSettings();

public:
    SceneMain();
    ~SceneMain();
    
    void update(float dt);
    bool onEvent(Event event);
};

#endif
