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

#include <ctime>
#include <cstdlib>

#include "device_android.hpp"
#include "device_manager.hpp"
#include "draw_utils.hpp"
#include "font_manager.hpp"
#include "texture_manager.hpp"
#include "scene_manager.hpp"

#ifdef ANDROID
struct android_app* g_android_app;
#endif

void main_loop()
{
    #ifdef DEBUG_FPS
    float dbg_counter = 0;
    unsigned int frames_count = 0;
    #endif
    
    bool close = false;
    const int max_fps = 120;

    SceneManager* scene_manager = SceneManager::getSceneManager();
    Device* device = DeviceManager::getDeviceManager()->getDevice();
    
    unsigned long ticks_per_frame = (unsigned long)(1000000.0f / max_fps);
    unsigned long base_ticks = device->getMicroTickCount();
    unsigned long current_ticks = 0;

    while (!close)
    {
        unsigned long ticks = device->getMicroTickCount() - base_ticks;

        if (ticks > current_ticks && ticks < current_ticks + ticks_per_frame)
        {
            unsigned long sleep_us = current_ticks - ticks + ticks_per_frame;
            int sleep_ms = sleep_us / 1000;

            if (sleep_ms > 0)
            {
                device->sleep(sleep_ms);
            }

            while (device->getMicroTickCount() - base_ticks < current_ticks +
                   ticks_per_frame)
            {
                device->sleep(0);
            }
        }

        unsigned long old_ticks = current_ticks;
        current_ticks = device->getMicroTickCount() - base_ticks;
        float dt = (float)(current_ticks - old_ticks) / 1000000.0f;

        close = scene_manager->update(dt);

        #ifdef DEBUG_FPS
        if (dbg_counter >= 1.0f)
        {
            float fps = (float)frames_count / dbg_counter;
            printf("fps: %f\n", fps);
            frames_count = 0;
            dbg_counter = 0;
        }

        dbg_counter += dt;
        frames_count++;
        #endif
    }
}

int main(int argc, char *argv[])
{
    DeviceManager* device_manager = new DeviceManager();
    bool success = device_manager->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize device manager.\n");
        return 1;
    }
    
    FileManager* file_manager = new FileManager();
    success = file_manager->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize file manager.\n");
        return 1;
    }
    
    DrawUtils* draw_utils = new DrawUtils();
    success = draw_utils->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize draw utils.\n");
        return 1;
    }
    
    TextureManager* texture_manager = new TextureManager();
    success = texture_manager->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize texture manager.\n");
        return 1;
    }
        
    FontManager* font_manager = new FontManager();
    success = font_manager->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize font manager.\n");
        return 1;
    }
    
    SceneManager* scene_manager = new SceneManager();
    success = scene_manager->init();
    
    if (!success)
    {
        printf("Error: Couldn't initialize scene manager.\n");
        return 1;
    }
               
    std::srand(std::time(0));
    
    main_loop();
    
    delete scene_manager;
    delete font_manager;
    delete texture_manager;
    delete draw_utils;
    delete file_manager;
    delete device_manager;

    return 0;
}

#ifdef ANDROID
void android_main(struct android_app* app) 
{
    app_dummy();
    
    DeviceAndroid::onCreate();
    
    g_android_app = app;
    main(0, {});
}
#endif
