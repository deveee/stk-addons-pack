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

#ifndef DEVICE_ANDROID_HPP
#define DEVICE_ANDROID_HPP

#if defined(ANDROID)

#include "device.hpp"

#include <android/window.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <map>
#include <string>

struct SensorInfo
{
    const ASensor* sensor;
    bool active;
    
    SensorInfo() : sensor(NULL), active(false) {};
};

class DeviceAndroid : public Device
{
private:
    android_app* m_android;
    ASensorManager* m_sensor_manager;
    ASensorEventQueue* m_sensor_event_queue;
    SensorInfo m_accelerometer;
    SensorInfo m_gyroscope;

    static bool m_is_paused;
    static bool m_is_focused;
    static bool m_is_started;
    
    bool m_close;
    bool m_is_mouse_pressed;
    int m_cursor_x;
    int m_cursor_y;

    std::map<int, KeyId> m_key_map;
    
    void createVideoModeList();
    void createKeyMap();
    void getKeyChar(Event& event, unsigned int system_key_code);
    void initSensors();
    void closeSensors();
    void pollSensors();
    
    static void onAppCommand(android_app* app, int32_t cmd);
    static int onEvent(android_app* app, AInputEvent* event);
    
public:
    DeviceAndroid();
    ~DeviceAndroid();

    bool initDevice(const CreationParams& creation_params);
    void closeDevice();
    bool processEvents();
    void clearSystemMessages() {};
    
    void setWindowCaption(const char* text) {};
    void setWindowClass(const char* text) {};
    void setWindowFullscreen(bool fullscreen) {};
    void setWindowResizable(bool resizable) {};
    bool isWindowActive() {return m_is_focused && !m_is_paused;}
    bool isWindowFocused() {return m_is_focused;}
    bool isWindowMinimized() {return m_is_paused;}
    bool isWindowFullscreen() {return true;}
    bool setWindowPosition(int x, int y) {return false;};
    bool getWindowPosition(int* x, int* y) {return false;}
    void setWindowMinimized() {};
    void setWindowMaximized() {};
    
    std::string getClipboardContent() {return "";}
    void setClipboardContent(std::string text) {};

    void setCursorVisible(bool visible) {};
    bool isCursorVisible() {return true;}
    void setCursorPosition(int x, int y) {m_cursor_x = x; m_cursor_y = y;}
    void getCursorPosition(int* x, int* y) {*x = m_cursor_x; *y = m_cursor_y;}
    
    static void onCreate();
    
    bool activateSensor(SensorInfo* sensor_info, float interval);
    bool deactivateSensor(SensorInfo* sensor_info);
    bool activateAccelerometer(float interval);
    bool deactivateAccelerometer();
    bool activateGyroscope(float interval);
    bool deactivateGyroscope();
    bool isAccelerometerActive() {return m_accelerometer.active;}
    bool isAccelerometerAvailable() {return m_accelerometer.sensor != NULL;}
    bool isGyroscopeActive() {return m_gyroscope.active;}
    bool isGyroscopeAvailable() {return m_gyroscope.sensor != NULL;}
};

#endif

#endif
