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
#include "file_manager.hpp"
#include "font_manager.hpp"
#include "progress_bar.hpp"
#include "scene_main.hpp"
#include "scene_manager.hpp"
#include "texture_manager.hpp"

#include <cmath>
#include <sstream>

SceneMain::SceneMain()
{
    FileManager* file_manager = FileManager::getFileManager();
    std::vector<std::string> assets_list = file_manager->getAssetsList();

    for (std::string name : assets_list)
    {
        if (name.find("extract/") != 0)
            continue;

        m_extract_assets.push_back(name);
    }
    
    m_extract_progress = 0;
    m_extract_dest = file_manager->findExternalDataDir("stk", "supertuxkart", 
                                                       "org.supertuxkart.stk", 
                                                       "SUPERTUXKART_DATADIR");
    m_extract_title = "Add-ons pack";
    m_extract_marker = "/.addon_extracted";
    m_extract_screenshot = "text_bg.png";

    readSettings();
    
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    int window_h = device->getWindowHeight();
    
    m_gui_scale = (float)window_h / 600.0f;
    m_text_height = 24 * m_gui_scale;
    m_btn_width = 128 * m_gui_scale;
    
    m_button_install = new Button();
    m_button_install->init("install", "Install", 0, 0, m_btn_width);

    m_button_close = new Button();
    m_button_close->init("close", "Close", 0, 0, m_btn_width);

    m_progress_bar = new ProgressBar();
    m_progress_bar->init(0.0f, 0.925f, 1.0f, 0.05f);

    TextureManager* texture_manager = TextureManager::getTextureManager();
    m_background = texture_manager->getTexture("background.jpg");
    m_logo = texture_manager->getTexture("logo.png");
    m_text_bg = texture_manager->getTexture("text_bg.png");
    m_screenshot = texture_manager->getTexture(m_extract_screenshot);
    
    if (m_extract_dest.empty())
    {
        setState(ES_DEST_DIR_NOT_FOUND);
    }
    else if (file_manager->fileExists(m_extract_dest + m_extract_marker))
    {
        setState(ES_ALREADY_INSTALLED);
    }
    else
    {
        setState(ES_NOT_INSTALLED);
    }
}

SceneMain::~SceneMain()
{
    delete m_progress_bar;
    delete m_button_install;
    delete m_button_close;
}

void SceneMain::readSettings()
{
    FileManager* file_manager = FileManager::getFileManager();
    
    File* file = file_manager->loadFile("extract_settings.txt");
    
    if (file == NULL)
        return;
        
    std::stringstream stream;
    stream.write(file->data, file->length);
    
    while (!stream.eof())
    {
        std::string line;
        std::getline(stream, line);
        
        if (line.empty())
            continue;
        
        unsigned int pos = line.find("=");
        
        if (pos == std::string::npos)
            continue;
            
        std::string name = line.substr(0, pos);
        std::string arg = line.substr(pos + 1);
        
        if (name == "name")
        {
            m_extract_marker = "/." + arg + "_extracted";
        }
        else if (name == "title")
        {
            m_extract_title = arg;
        }
        else if (name == "screenshot")
        {
            m_extract_screenshot = arg;
        }
    }
    
    file_manager->closeFile(file);
}

void SceneMain::setState(ExtractState state)
{
    switch (state)
    {
    case ES_DEST_DIR_NOT_FOUND:
        m_button_install->setText("Install");
        m_button_install->setActive(false);
        m_text = "Data directory not found.";
        m_text2 = "Make sure that the game is installed.";
        break;
    case ES_NOT_INSTALLED:
        m_button_install->setText("Install");
        m_text = "Data directory found in: " + m_extract_dest;
        m_text2 = "Press install to extract included add-ons.";
        break;
    case ES_ALREADY_INSTALLED:
        m_button_install->setText("Reinstall");
        m_text = "Add-ons are already installed in: " + m_extract_dest;
        m_text2 = "";
        break;
    case ES_INSTALLING:
        m_button_install->setActive(false);
        m_button_close->setActive(false);
        m_text = "Installing...";
        m_text2 = "";
        m_progress_bar->setValue(0.0f);
        m_extract_progress = 0;
        break;
    case ES_INSTALLED:
        m_button_install->setText("Reinstall");
        m_button_install->setActive(true);
        m_button_close->setActive(true);
        m_text = "Add-ons have been successfully installed.";
        m_text2 = "Have a nice day :)";
        m_progress_bar->setValue(1.0f);
        m_extract_progress = m_extract_assets.size();
        break;
    case ES_INSTALLATION_FAILED:
        m_button_install->setActive(true);
        m_button_close->setActive(true);
        m_button_install->setText("Install");
        m_text = "Installation failed.";
        m_text2 = "Couldn't extract some files.";
        break;
    }
    
    m_extract_state = state;
}

void SceneMain::update(float dt)
{
    FileManager* file_manager = FileManager::getFileManager();
    
    if (m_extract_state == ES_INSTALLING &&
        m_extract_progress < m_extract_assets.size())
    {
        std::string file_path = m_extract_assets[m_extract_progress];
        
        bool success = file_manager->extractFromAssets(file_path, "extract/", 
                                                       m_extract_dest);

        if (!success)
        {
            setState(ES_INSTALLATION_FAILED);
        }
        
        m_extract_progress++;

        float value = (float)m_extract_progress / m_extract_assets.size();
        m_progress_bar->setValue(value);

        if (m_extract_progress == m_extract_assets.size())
        {
            file_manager->touchFile(m_extract_dest + m_extract_marker);
            setState(ES_INSTALLED);
        }
    }

    drawScene();
}

void SceneMain::drawScene()
{
    DrawUtils* draw_utils = DrawUtils::getDrawUtils();
    FontManager* font_manager = FontManager::getFontManager();

    GLfloat black[4] = {0, 0, 0, 1};
    GLfloat blue[4] = {0.15f, 0.65f, 0.8f, 1.0f};
    
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    int window_w = device->getWindowWidth();
    int window_h = device->getWindowHeight();

    draw_utils->drawTexture2D(m_background, 0, 0, window_w, window_h);
    
    int logo_w = 256 * m_gui_scale;
    int logo_h = logo_w * m_logo->height / m_logo->width;
    int logo_x = (window_w - logo_w) / 2;
    int logo_y = 0 * m_gui_scale;
    
    draw_utils->drawTexture2D(m_logo, logo_x, logo_y, logo_w, logo_h);

    int text_bg_w = window_w - 40 * m_gui_scale;
    int text_bg_h = window_h * 0.93 - 370 * m_gui_scale;
    int text_bg_x = 20 * m_gui_scale;
    int text_bg_y = 260 * m_gui_scale;
    
    draw_utils->drawTexture2D(m_text_bg, text_bg_x, text_bg_y, text_bg_w, text_bg_h);
    
    int sshot_w = 100 * m_gui_scale;
    int sshot_h = sshot_w * m_screenshot->height / m_screenshot->width;
    int sshot_x = window_w - 40 * m_gui_scale - sshot_w;
    int sshot_y = 330 * m_gui_scale;
    
    draw_utils->drawTexture2D(m_screenshot, sshot_x, sshot_y, sshot_w, sshot_h);
           
    font_manager->changeFont("SigmarOne.otf");

    int title_h = m_text_height * 1.5f;
    int title_w = font_manager->getTextWidth(m_extract_title, title_h);
    int title_x = (window_w - title_w) / 2;
    int title_y = 300 * m_gui_scale;
    
    font_manager->drawText(m_extract_title, title_x, title_y, title_h, black);

    font_manager->changeFont("FreeSans.ttf");
    
    int text_x = 30 * m_gui_scale;
    int text_y1 = 350 * m_gui_scale;
    int text_y2 = 400 * m_gui_scale;
    
    font_manager->drawText(m_text, text_x, text_y1, m_text_height, black);
    font_manager->drawText(m_text2, text_x, text_y2, m_text_height, black);
    
    int btn_center = (window_w - m_btn_width) / 2;
    int btn_x1 = btn_center - 100 * m_gui_scale;
    int btn_x2 = btn_center + 100 * m_gui_scale;
    int btn_y = window_h * 0.93 - 70 * m_gui_scale;
    
    m_button_install->setPosX(btn_x1);
    m_button_install->setPosY(btn_y);
    m_button_install->draw();

    m_button_close->setPosX(btn_x2);
    m_button_close->setPosY(btn_y);
    m_button_close->draw();
    
    m_progress_bar->draw(blue);
}

bool SceneMain::onEvent(Event event)
{
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    
    bool result = false;
    
    if (event.type == ET_MOUSE_EVENT)
    {
        MouseEvent mouse_event = event.mouse;

        if (mouse_event.type == ME_LEFT_PRESSED)
        {
            if (m_button_install->isCursorOverButton(mouse_event.x, 
                                                     mouse_event.y))
            {
                if (m_extract_state != ES_INSTALLING &&
                    m_extract_state != ES_DEST_DIR_NOT_FOUND)
                {
                    setState(ES_INSTALLING);
                }
            }
            else if (m_button_close->isCursorOverButton(mouse_event.x, 
                                                        mouse_event.y))
            {
                if (m_extract_state != ES_INSTALLING)
                {
                    device->closeDevice();
                }
            }
        }
        
        result = true;
    }
    else if (event.type == ET_KEY_EVENT)
    {
        KeyEvent key_event = event.key;

        if (key_event.pressed)
        {
            switch (key_event.id)
            {
            case KC_KEY_ESCAPE:
            case KC_KEY_Q:
                if (m_extract_state != ES_INSTALLING)
                {
                    device->closeDevice();
                }
                
                result = true;
                break;
            default:
                break;
            }
        }
    }

    return result;
}
