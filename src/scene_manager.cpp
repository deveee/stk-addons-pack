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

#include "device_manager.hpp"
#include "scene_main.hpp"
#include "scene_manager.hpp"

SceneManager* SceneManager::m_scene_manager = NULL;

SceneManager::SceneManager()
{
    m_scene_manager = this;
    m_scene = NULL;
}

SceneManager::~SceneManager()
{
    delete m_scene;
}

bool SceneManager::init()
{
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    device->setEventReceiver(onEvent);
    
    m_scene = new SceneMain();
    return true;
}

bool SceneManager::update(float dt)
{
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    bool close = !device->processEvents();
    
    if (close)
        return true;
    
    glViewport(0, 0, device->getWindowWidth(), device->getWindowHeight());
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (m_scene != NULL)
    {
        m_scene->update(dt);
    }

    device->swapBuffers();
    
    return close;
}

void SceneManager::onEvent(Event event)
{
    SceneManager* scene_manager = SceneManager::getSceneManager();
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    Scene* scene = scene_manager->getScene();
    
    bool event_handled = false;
    
    if (scene != NULL)
    {
        event_handled = scene->onEvent(event);
    }
    
    if (event_handled == true)
        return;
    
    if (event.type == ET_KEY_EVENT)
    {
        KeyEvent key_event = event.key;
    
        if (key_event.pressed)
        {
            if (!key_event.control)
            {
                switch (key_event.id)
                {
                case KC_KEY_H:
                    device->setCursorVisible(!device->isCursorVisible());
                    break;
                case KC_KEY_F:
                    device->setWindowFullscreen(!device->isWindowFullscreen());
                    break;
                default:
                    break;
                }
            }
        }
    }
}
