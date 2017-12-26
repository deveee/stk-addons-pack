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

#include "device_android.hpp"
#include "device_linux.hpp"
#include "device_manager.hpp"

#include <GLES3/gl3.h>

#ifdef ANDROID
extern struct android_app* g_android_app;
#endif

#ifndef PROJECT_NAME
#define PROJECT_NAME "SimpleWindow"
#endif

#ifndef FULL_PROJECT_NAME
#define FULL_PROJECT_NAME "Simple Window"
#endif

DeviceManager* DeviceManager::m_device_manager = NULL;

DeviceManager::DeviceManager()
{
    m_device_manager = this;
    m_device = NULL;
}

DeviceManager::~DeviceManager()
{
    delete m_device;
}

bool DeviceManager::init()
{
    CreationParams params;
    params.window_width = 800;
    params.window_height = 600;
    params.fullscreen = false;
    params.vsync = true;
    params.handle_srgb = false;
    params.alpha_channel = false;
    params.force_legacy_device = false;
#ifdef ANDROID
    params.private_data = g_android_app;
#else
    params.private_data = NULL;
#endif
    params.joystick_support = false;
    params.driver_type = DRIVER_OPENGL_ES;
    
#ifdef ANDROID
    m_device = new DeviceAndroid();
#else
    m_device = new DeviceLinux();
#endif
    
    bool success = m_device->initDevice(params);
    
    if (!success)
    {
        printf("Error: Couldn't initialize device.\n");
        return false;
    }
    
    m_device->setWindowCaption(FULL_PROJECT_NAME);
    m_device->setWindowClass(PROJECT_NAME);
    
    printDeviceInfo();

    return true;
}

void DeviceManager::printDeviceInfo()
{
    int major_gl = 2;
    int minor_gl = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major_gl);
    glGetIntegerv(GL_MINOR_VERSION, &minor_gl);
    
    printf("OpenGL version: %d.%d\n", major_gl, minor_gl);
    printf("OpenGL vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version string: %s\n", glGetString(GL_VERSION));
    
    VideoMode desktop = m_device->getDesktopMode();
    printf("Desktop resolution: %i %i\n", desktop.width, desktop.height);
    printf("Available resolutions:\n");
    
    for (VideoMode mode : m_device->getVideoModeList())
    {
        printf("    %i %i\n", mode.width, mode.height);
    }
}
