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

#if (defined(__linux__) || defined(__CYGWIN__)) && !defined(ANDROID)

#include "device_linux.hpp"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <locale.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#ifndef __CYGWIN__
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>
#endif

#define XRANDR_ROTATION_LEFT    (1 << 1)
#define XRANDR_ROTATION_RIGHT   (1 << 3)

DeviceLinux::DeviceLinux()
{
    m_display = NULL;
    m_visual = NULL;
    m_window = 0;
    m_std_hints = NULL;
    m_egl_context = NULL;
    m_screen_nr = 0;

    m_window_width = 0;
    m_window_height = 0;
    m_window_has_focus = false;
    m_window_minimized = false;
    m_close = false;
    m_netwm_supported = false;
    m_window_fullscreen = false;
    m_atom_clipboard = None;
    m_atom_targets = None;
    m_atom_utf8_string = None;

    m_output_id = 0;
    m_default_mode = 0;
    m_crtc_x = 0;
    m_crtc_y = 0;
    m_mode_changed = false;
    
    m_input_method = 0;
    m_input_context = 0;
    m_numlock_mask = 0;

    m_cursor_invisible = 0;
    m_cursor_is_visible = true;
    m_cursor_x = 0;
    m_cursor_y = 0;
}

bool DeviceLinux::initDevice(const CreationParams& creation_params)
{
    m_creation_params = creation_params;
    m_window_width = creation_params.window_width;
    m_window_height = creation_params.window_height;
    
    XSetErrorHandler(printXError);

    m_display = XOpenDisplay(0);
    
    if (!m_display)
    {
        printf("Error: Couldn't open display.\n");
        return false;
    }

    m_screen_nr = DefaultScreen(m_display);

    createVideoModeList();
    
    bool success = createWindow();
    
    if (!success)
    {
        printf("Error: Couldn't create a window\n");
        return false;
    }

    initCursor();
    createKeyMap();
    findNumlockMask();
    
    success = createInputContext();
    
    if (!success)
    {
        printf("Warning: Couldn't create input context.\n");
    }
    
    if (creation_params.joystick_support)
    {
        activateJoysticks();
    }
    
    return true;
}

DeviceLinux::~DeviceLinux()
{
    delete m_egl_context;
    
    if (m_display)
    {
        destroyInputContext();
        closeCursor();
        XDestroyWindow(m_display, m_window);
        restoreResolution();        

        if (m_visual)
        {
            XFree(m_visual);
        }

        if (m_std_hints)
        {
            XFree(m_std_hints);
        }

        XCloseDisplay(m_display);
    }
    
    closeJoysticks();
}

int DeviceLinux::printXError(Display* display, XErrorEvent* event)
{
    char msg[256];
    char msg2[256];
    
    snprintf(msg, 256, "%d", event->request_code);
    XGetErrorDatabaseText(display, "XRequest", msg, "unknown", msg2, 256);
    XGetErrorText(display, event->error_code, msg, 256);
    printf("Error: %s\n", msg);
    printf("Error: From call %s\n", msg2);
    return 0;
}

bool DeviceLinux::createVideoModeList()
{
    if (!m_video_modes.empty() || !m_display)
        return false;
    
    int eventbase, errorbase;
    if (!XRRQueryExtension(m_display, &eventbase, &errorbase))
        return false;
    
    Window root_window = DefaultRootWindow(m_display);
    
    XRRScreenResources* res = XRRGetScreenResources(m_display, root_window);
    if (!res)
        return false;
    
    XRROutputInfo* output = NULL;
    XRRCrtcInfo* crtc = NULL;
    RROutput primary_id = XRRGetOutputPrimary(m_display, root_window);

    for (int i = 0; i < res->noutput; i++) 
    {
        XRROutputInfo* output_tmp = XRRGetOutputInfo(m_display, res, 
                                                     res->outputs[i]);
        if (!output_tmp || !output_tmp->crtc || 
            output_tmp->connection == RR_Disconnected) 
        {
            XRRFreeOutputInfo(output_tmp);
            continue;
        }

        XRRCrtcInfo* crtc_tmp = XRRGetCrtcInfo(m_display, res, output_tmp->crtc);
        if (!crtc_tmp) 
        {
            XRRFreeOutputInfo(output_tmp);
            continue;
        }
        
        if (m_output_id == 0 || res->outputs[i] == primary_id ||
            crtc_tmp->x < crtc->x ||
            (crtc_tmp->x == crtc->x && crtc_tmp->y < crtc->y))
        {
            XRRFreeCrtcInfo(crtc);
            XRRFreeOutputInfo(output);      
            
            crtc = crtc_tmp;                    
            output = output_tmp;
            m_output_id = res->outputs[i];
        }
        else
        {
            XRRFreeCrtcInfo(crtc_tmp);
            XRRFreeOutputInfo(output_tmp);          
        }
        
        if (res->outputs[i] == primary_id)
            break;
    }
    
    if (m_output_id != 0)
    {
        m_crtc_x = crtc->x;
        m_crtc_y = crtc->y;
        
        int default_depth = DefaultDepth(m_display, m_screen_nr);

        for (int i = 0; i < res->nmode; i++)
        {
            const XRRModeInfo* mode = &res->modes[i];
            bool rotated = crtc->rotation & 
                          (XRANDR_ROTATION_LEFT | XRANDR_ROTATION_RIGHT);
            
            unsigned int width = rotated ? mode->height : mode->width;
            unsigned int height = rotated ? mode->width : mode->height;
            VideoMode video_mode(width, height, default_depth);

            for (int j = 0; j < output->nmode; j++)
            {            
                if (mode->id == output->modes[j])
                {
                    m_video_modes.push_back(video_mode);
                    break;
                }
            }

            if (mode->id == crtc->mode)
            {
                m_default_mode = crtc->mode;
                m_video_desktop = video_mode;
            }
        }
    }
    
    XRRFreeCrtcInfo(crtc);
    XRRFreeOutputInfo(output);
    XRRFreeScreenResources(res);                        

    return true;
}

bool DeviceLinux::restoreResolution()
{
    if (!m_mode_changed || m_default_mode == 0)
        return true;

    Window root_window = DefaultRootWindow(m_display);
    XRRScreenResources* res = XRRGetScreenResources(m_display, root_window);
    if (!res)
        return false;

    XRROutputInfo* output = XRRGetOutputInfo(m_display, res, m_output_id);
    if (!output || !output->crtc || output->connection == RR_Disconnected) 
    {
        XRRFreeScreenResources(res);
        XRRFreeOutputInfo(output);
        return false;
    }

    XRRCrtcInfo* crtc = XRRGetCrtcInfo(m_display, res, output->crtc);
    if (!crtc) 
    {
        XRRFreeScreenResources(res);
        XRRFreeOutputInfo(output);
        return false;
    }

    Status s = XRRSetCrtcConfig(m_display, res, output->crtc, CurrentTime,
                                crtc->x, crtc->y, m_default_mode,
                                crtc->rotation, &m_output_id, 1);

    XRRFreeOutputInfo(output);
    XRRFreeCrtcInfo(crtc);
    XRRFreeScreenResources(res);

    m_mode_changed = (s != Success);
    return !m_mode_changed;
}

bool DeviceLinux::changeResolution()
{
    if (m_output_id == 0)
        return false;

    Window root_window = DefaultRootWindow(m_display);
    XRRScreenResources* res = XRRGetScreenResources(m_display, root_window);
    if (!res)
        return false;

    XRROutputInfo* output = XRRGetOutputInfo(m_display, res, m_output_id);
    if (!output || !output->crtc || output->connection == RR_Disconnected)
    {
        XRRFreeOutputInfo(output);
        XRRFreeScreenResources(res);
        return false;
    }
    
    XRRCrtcInfo* crtc = XRRGetCrtcInfo(m_display, res, output->crtc);
    if (!crtc)
    {
        XRRFreeOutputInfo(output);
        XRRFreeScreenResources(res);
        return false;
    }

    int bestMode = -1;
    float refresh_rate = 0;

    for (int i = 0; i < res->nmode; i++)
    {
        const XRRModeInfo* mode = &res->modes[i];
        bool rotated = crtc->rotation & 
                      (XRANDR_ROTATION_LEFT | XRANDR_ROTATION_RIGHT);
        
        int width = rotated ? mode->height : mode->width;
        int height = rotated ? mode->width : mode->height;

        if (width == m_creation_params.window_width && 
            height == m_creation_params.window_height)
        {
            float refresh_rate_new = (mode->dotClock * 1000.0) / 
                                     (mode->hTotal * mode->vTotal);

            if (bestMode != -1 && refresh_rate_new <= refresh_rate)
                continue;

            for (int j = 0; j < output->nmode; j++)
            {
                if (mode->id == output->modes[j])
                {
                    bestMode = j;
                    refresh_rate = refresh_rate_new;
                    break;
                }
            }
        }
    }

    if (bestMode != -1)
    {
        Status s = XRRSetCrtcConfig(m_display, res, output->crtc, CurrentTime,
                                    crtc->x, crtc->y, output->modes[bestMode],
                                    crtc->rotation, &m_output_id, 1);
        m_mode_changed = (s == Success);
    }

    XRRFreeCrtcInfo(crtc);
    XRRFreeOutputInfo(output);
    XRRFreeScreenResources(res);

    return m_mode_changed;
}

void DeviceLinux::setWindowFullscreen(bool fullscreen)
{
    if (m_netwm_supported == false)
        return;
        
    if (m_window_fullscreen == fullscreen)
        return;
    
    if (fullscreen)
    {
        changeResolution();
    }
    else
    {
        restoreResolution();
    }
    
    // Some window managers don't respect values from XCreateWindow and
    // place window in random position. This may cause that fullscreen
    // window is showed in wrong screen.
    XMoveResizeWindow(m_display, m_window, m_crtc_x, m_crtc_y, 
                      m_window_width, m_window_height);
    XRaiseWindow(m_display, m_window);
    XFlush(m_display);

    Atom wm_state = XInternAtom(m_display, "_NET_WM_STATE", true);
    Atom wm_fullscreen = XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", true);

    XEvent xev = {0}; 
    xev.type = ClientMessage;
    xev.xclient.window = m_window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = fullscreen ? 1 : 0;
    xev.xclient.data.l[1] = wm_fullscreen;
    Window root_window = RootWindow(m_display, m_visual->screen);
    XSendEvent(m_display, root_window, false, 
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    XFlush(m_display);
    
    // Wait until window state is already changed to fullscreen
    for (int i = 0; i < 500; i++)
    {
        Atom type;
        int format;
        unsigned long num, remain;
        Atom* atoms = NULL;
        bool done = false;

        int s = XGetWindowProperty(m_display, m_window, wm_state, 0, 1024, 
                                   false, XA_ATOM, &type, &format, &num, 
                                   &remain, (unsigned char**)&atoms);
        
        if (s == Success) 
        {
            for (unsigned int i = 0; i < num; i++) 
            {
                if (atoms[i] == wm_fullscreen) 
                {
                    done = true;
                    break;
                }
            }
            
            XFree(atoms);
        }
        
        if (done == true)
            break;
        
        sleep(1);
    }
    
    m_window_fullscreen = fullscreen;
}

bool DeviceLinux::createWindow()
{
    XVisualInfo vinfo_template;
    vinfo_template.screen = m_screen_nr;
    vinfo_template.depth = m_creation_params.alpha_channel ? 32 : 24;
    
    while ((!m_visual) && (vinfo_template.depth >= 16))
    {
        int num;
        m_visual = XGetVisualInfo(m_display, VisualScreenMask | VisualDepthMask,
                                  &vinfo_template, &num);
        vinfo_template.depth -= 8;
    }

    if (!m_visual)
    {
        XCloseDisplay(m_display);
        m_display = NULL;
        return false;
    }
    
    Window root_window = RootWindow(m_display, m_visual->screen);
    
    Atom type;
    int format;
    unsigned long remain, num;
    unsigned char* data = NULL;

    Atom wm_check = XInternAtom(m_display, "_NET_SUPPORTING_WM_CHECK", false);
    Status s = XGetWindowProperty(m_display, root_window, wm_check, 0, 1, false, 
                                  XA_WINDOW, &type, &format, &num, &remain, &data);
                                  
    if (s == Success)
    {
        XFree(data);
        m_netwm_supported = (num > 0);
    }

    Colormap colormap = XCreateColormap(m_display, root_window, 
                                        m_visual->visual, AllocNone);

    XSetWindowAttributes attributes;
    attributes.colormap = colormap;
    attributes.border_pixel = 0;
    attributes.override_redirect = false;
    attributes.event_mask = StructureNotifyMask | FocusChangeMask | 
                            ExposureMask | PointerMotionMask | ButtonPressMask | 
                            KeyPressMask | ButtonReleaseMask | KeyReleaseMask;

    m_window = XCreateWindow(m_display, root_window, 0, 0, m_window_width, 
                             m_window_height, 0, m_visual->depth, InputOutput, 
                             m_visual->visual, CWBorderPixel | CWColormap | 
                             CWEventMask | CWOverrideRedirect, &attributes);
                           
    XMapRaised(m_display, m_window);
    Atom wm_delete = XInternAtom(m_display, "WM_DELETE_WINDOW", true);
    XSetWMProtocols(m_display, m_window, &wm_delete, 1);

    if (m_creation_params.fullscreen)
    {
        setWindowFullscreen(true);
    }
    
    bool success = createEGLContext(m_display, m_window);
    
    if (!success)
        return false;
        
    int w = 0;
    int h = 0;

    if (m_egl_context->getSurfaceDimensions(&w, &h))
    {
        m_window_width = w;
        m_window_height = h;
    }
    
    if (m_netwm_supported == true)
    {
        Atom opaque_region = XInternAtom(m_display, "_NET_WM_OPAQUE_REGION", true);
        
        if (opaque_region != None)
        {
            unsigned long window_rect[4] = {0, 0, m_window_width, m_window_height};
            XChangeProperty(m_display, m_window, opaque_region, XA_CARDINAL, 32,
                            PropModeReplace, (unsigned char*)window_rect, 4);
        }
    }
    
    m_atom_clipboard = XInternAtom(m_display, "CLIPBOARD", false);
    m_atom_targets = XInternAtom(m_display, "TARGETS", false);
    m_atom_utf8_string = XInternAtom (m_display, "UTF8_STRING", false);

    m_std_hints = XAllocSizeHints();
    long num_hints;
    XGetWMNormalHints(m_display, m_window, m_std_hints, &num_hints);

    return true;
}

bool DeviceLinux::createInputContext()
{
    std::string old_locale = setlocale(LC_CTYPE, NULL);
    setlocale(LC_CTYPE, "");

    if (!XSupportsLocale())
    {
        setlocale(LC_CTYPE, old_locale.c_str());
        return false;
    }

    m_input_method = XOpenIM(m_display, NULL, NULL, NULL);
    
    if (!m_input_method)
    {
        setlocale(LC_CTYPE, old_locale.c_str());
        return false;
    }

    XIMStyles* im_supported_styles = NULL;
    XGetIMValues(m_input_method, XNQueryInputStyle, &im_supported_styles, NULL);
    
    if (!im_supported_styles)
    {
        setlocale(LC_CTYPE, old_locale.c_str());
        return false;
    }

    unsigned long input_style = XIMPreeditNothing | XIMStatusNothing;
    bool found = false;

    for (int i = 0; i < im_supported_styles->count_styles; i++)
    {
        XIMStyle cur_style = im_supported_styles->supported_styles[i];
        
        if (cur_style == input_style)
        {
            found = true;
            break;
        }
    }
    
    XFree(im_supported_styles);

    if (!found)
    {
        setlocale(LC_CTYPE, old_locale.c_str());
        return false;
    }

    m_input_context = XCreateIC(m_input_method, XNInputStyle, input_style,
                                XNClientWindow, m_window, XNFocusWindow, 
                                m_window, NULL);

    if (!m_input_context)
    {
        setlocale(LC_CTYPE, old_locale.c_str());
        return false;
    }

    XSetICFocus(m_input_context);
    setlocale(LC_CTYPE, old_locale.c_str());
    return true;
}

void DeviceLinux::destroyInputContext()
{
    if (m_input_context)
    {
        XUnsetICFocus(m_input_context);
        XDestroyIC(m_input_context);
        m_input_context = NULL;
    }
    
    if (m_input_method)
    {
        XCloseIM(m_input_method);
        m_input_method = NULL;
    }
}

void DeviceLinux::findNumlockMask()
{
    int mask_table[8] = {ShiftMask, LockMask, ControlMask, Mod1Mask,
                         Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask};       

    KeyCode numlock_keycode = XKeysymToKeycode(m_display, XK_Num_Lock);

    if (numlock_keycode == NoSymbol)
        return;

    XModifierKeymap* map = XGetModifierMapping(m_display);
    if (!map)
        return;

    for (int i = 0; i < 8 * map->max_keypermod; i++) 
    {
        if (map->modifiermap[i] != numlock_keycode)
            continue;

        m_numlock_mask = mask_table[i/map->max_keypermod];
        break;
    }

    XFreeModifiermap(map);
}

KeyId DeviceLinux::getKeyCode(XEvent &event)
{
    KeyId key_code = KC_KEY_UNKNOWN;
    int keysym = 0;
    
    // First check for numpad keys
    bool is_numpad_key = false;
    
    if (event.xkey.state & m_numlock_mask)
    {
        keysym = XkbKeycodeToKeysym(m_display, event.xkey.keycode, 0, 1);
        
        if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
        {
            is_numpad_key = true;
        }
    }
    
    // If it's not numpad key, then get keycode in typical way
    if (!is_numpad_key)
    {
        keysym = XkbKeycodeToKeysym(m_display, event.xkey.keycode, 0, 0);
    }
    
    key_code = m_key_map[keysym];
    
    if (key_code == KC_KEY_UNKNOWN)
    {
        key_code = keysym == 0 ? (KeyId)event.xkey.keycode : (KeyId)keysym;
    }
    
    return key_code;
}

void DeviceLinux::createKeyMap()
{
    m_key_map[XK_BackSpace] = KC_KEY_BACK;
    m_key_map[XK_Tab] = KC_KEY_TAB;
    m_key_map[XK_ISO_Left_Tab] = KC_KEY_TAB;
    m_key_map[XK_Linefeed] = KC_KEY_UNKNOWN;
    m_key_map[XK_Clear] = KC_KEY_CLEAR;
    m_key_map[XK_Return] = KC_KEY_RETURN;
    m_key_map[XK_Pause] = KC_KEY_PAUSE;
    m_key_map[XK_Scroll_Lock] = KC_KEY_SCROLL;
    m_key_map[XK_Sys_Req] = KC_KEY_UNKNOWN;
    m_key_map[XK_Escape] = KC_KEY_ESCAPE;
    m_key_map[XK_Insert] = KC_KEY_INSERT;
    m_key_map[XK_Delete] = KC_KEY_DELETE;
    m_key_map[XK_Home] = KC_KEY_HOME;
    m_key_map[XK_Left] = KC_KEY_LEFT;
    m_key_map[XK_Up] = KC_KEY_UP;
    m_key_map[XK_Right] = KC_KEY_RIGHT;
    m_key_map[XK_Down] = KC_KEY_DOWN;
    m_key_map[XK_Prior] = KC_KEY_PRIOR;
    m_key_map[XK_Page_Up] = KC_KEY_PRIOR;
    m_key_map[XK_Next] = KC_KEY_NEXT;
    m_key_map[XK_Page_Down] = KC_KEY_NEXT;
    m_key_map[XK_End] = KC_KEY_END;
    m_key_map[XK_Begin] = KC_KEY_HOME;
    m_key_map[XK_Num_Lock] = KC_KEY_NUMLOCK;
    m_key_map[XK_KP_Space] = KC_KEY_SPACE;
    m_key_map[XK_KP_Tab] = KC_KEY_TAB;
    m_key_map[XK_KP_Enter] = KC_KEY_RETURN;
    m_key_map[XK_KP_F1] = KC_KEY_F1;
    m_key_map[XK_KP_F2] = KC_KEY_F2;
    m_key_map[XK_KP_F3] = KC_KEY_F3;
    m_key_map[XK_KP_F4] = KC_KEY_F4;
    m_key_map[XK_KP_Home] = KC_KEY_HOME;
    m_key_map[XK_KP_Left] = KC_KEY_LEFT;
    m_key_map[XK_KP_Up] = KC_KEY_UP;
    m_key_map[XK_KP_Right] = KC_KEY_RIGHT;
    m_key_map[XK_KP_Down] = KC_KEY_DOWN;
    m_key_map[XK_Print] = KC_KEY_PRINT;
    m_key_map[XK_KP_Prior] = KC_KEY_PRIOR;
    m_key_map[XK_KP_Page_Up] = KC_KEY_PRIOR;
    m_key_map[XK_KP_Next] = KC_KEY_NEXT;
    m_key_map[XK_KP_Page_Down] = KC_KEY_NEXT;
    m_key_map[XK_KP_End] = KC_KEY_END;
    m_key_map[XK_KP_Begin] = KC_KEY_HOME;
    m_key_map[XK_KP_Insert] = KC_KEY_INSERT;
    m_key_map[XK_KP_Delete] = KC_KEY_DELETE;
    m_key_map[XK_KP_Equal] = KC_KEY_UNKNOWN;
    m_key_map[XK_KP_Multiply] = KC_KEY_MULTIPLY;
    m_key_map[XK_KP_Add] = KC_KEY_ADD;
    m_key_map[XK_KP_Separator] = KC_KEY_SEPARATOR;
    m_key_map[XK_KP_Subtract] = KC_KEY_SUBTRACT;
    m_key_map[XK_KP_Decimal] = KC_KEY_DECIMAL;
    m_key_map[XK_KP_Divide] = KC_KEY_DIVIDE;
    m_key_map[XK_KP_0] = KC_KEY_NUMPAD0;
    m_key_map[XK_KP_1] = KC_KEY_NUMPAD1;
    m_key_map[XK_KP_2] = KC_KEY_NUMPAD2;
    m_key_map[XK_KP_3] = KC_KEY_NUMPAD3;
    m_key_map[XK_KP_4] = KC_KEY_NUMPAD4;
    m_key_map[XK_KP_5] = KC_KEY_NUMPAD5;
    m_key_map[XK_KP_6] = KC_KEY_NUMPAD6;
    m_key_map[XK_KP_7] = KC_KEY_NUMPAD7;
    m_key_map[XK_KP_8] = KC_KEY_NUMPAD8;
    m_key_map[XK_KP_9] = KC_KEY_NUMPAD9;
    m_key_map[XK_F1] = KC_KEY_F1;
    m_key_map[XK_F2] = KC_KEY_F2;
    m_key_map[XK_F3] = KC_KEY_F3;
    m_key_map[XK_F4] = KC_KEY_F4;
    m_key_map[XK_F5] = KC_KEY_F5;
    m_key_map[XK_F6] = KC_KEY_F6;
    m_key_map[XK_F7] = KC_KEY_F7;
    m_key_map[XK_F8] = KC_KEY_F8;
    m_key_map[XK_F9] = KC_KEY_F9;
    m_key_map[XK_F10] = KC_KEY_F10;
    m_key_map[XK_F11] = KC_KEY_F11;
    m_key_map[XK_F12] = KC_KEY_F12;
    m_key_map[XK_Shift_L] = KC_KEY_LSHIFT;
    m_key_map[XK_Shift_R] = KC_KEY_RSHIFT;
    m_key_map[XK_Control_L] = KC_KEY_LCONTROL;
    m_key_map[XK_Control_R] = KC_KEY_RCONTROL;
    m_key_map[XK_Caps_Lock] = KC_KEY_CAPITAL;
    m_key_map[XK_Shift_Lock] = KC_KEY_CAPITAL;
    m_key_map[XK_Meta_L] = KC_KEY_LWIN;
    m_key_map[XK_Meta_R] = KC_KEY_RWIN;
    m_key_map[XK_Alt_L] = KC_KEY_LMENU;
    m_key_map[XK_Alt_R] = KC_KEY_RMENU;
    m_key_map[XK_ISO_Level3_Shift] = KC_KEY_RMENU;
    m_key_map[XK_Menu] = KC_KEY_MENU;
    m_key_map[XK_space] = KC_KEY_SPACE;
    m_key_map[XK_exclam] = KC_KEY_UNKNOWN;
    m_key_map[XK_quotedbl] = KC_KEY_UNKNOWN;
    m_key_map[XK_section] = KC_KEY_UNKNOWN;
    m_key_map[XK_numbersign] = KC_KEY_OEM_2;
    m_key_map[XK_dollar] = KC_KEY_UNKNOWN;
    m_key_map[XK_percent] = KC_KEY_UNKNOWN;
    m_key_map[XK_ampersand] = KC_KEY_UNKNOWN;
    m_key_map[XK_apostrophe] = KC_KEY_OEM_7;
    m_key_map[XK_parenleft] = KC_KEY_UNKNOWN;
    m_key_map[XK_parenright] = KC_KEY_UNKNOWN;
    m_key_map[XK_asterisk] = KC_KEY_UNKNOWN;
    m_key_map[XK_plus] = KC_KEY_PLUS;
    m_key_map[XK_comma] = KC_KEY_COMMA;
    m_key_map[XK_minus] = KC_KEY_MINUS;
    m_key_map[XK_period] = KC_KEY_PERIOD;
    m_key_map[XK_slash] = KC_KEY_OEM_2;
    m_key_map[XK_0] = KC_KEY_0;
    m_key_map[XK_1] = KC_KEY_1;
    m_key_map[XK_2] = KC_KEY_2;
    m_key_map[XK_3] = KC_KEY_3;
    m_key_map[XK_4] = KC_KEY_4;
    m_key_map[XK_5] = KC_KEY_5;
    m_key_map[XK_6] = KC_KEY_6;
    m_key_map[XK_7] = KC_KEY_7;
    m_key_map[XK_8] = KC_KEY_8;
    m_key_map[XK_9] = KC_KEY_9;
    m_key_map[XK_colon] = KC_KEY_UNKNOWN;
    m_key_map[XK_semicolon] = KC_KEY_OEM_1;
    m_key_map[XK_less] = KC_KEY_OEM_102;
    m_key_map[XK_equal] = KC_KEY_PLUS;
    m_key_map[XK_greater] = KC_KEY_UNKNOWN;
    m_key_map[XK_question] = KC_KEY_UNKNOWN;
    m_key_map[XK_at] = KC_KEY_2;
    m_key_map[XK_mu] = KC_KEY_UNKNOWN;
    m_key_map[XK_EuroSign] = KC_KEY_UNKNOWN;
    m_key_map[XK_A] = KC_KEY_A;
    m_key_map[XK_B] = KC_KEY_B;
    m_key_map[XK_C] = KC_KEY_C;
    m_key_map[XK_D] = KC_KEY_D;
    m_key_map[XK_E] = KC_KEY_E;
    m_key_map[XK_F] = KC_KEY_F;
    m_key_map[XK_G] = KC_KEY_G;
    m_key_map[XK_H] = KC_KEY_H;
    m_key_map[XK_I] = KC_KEY_I;
    m_key_map[XK_J] = KC_KEY_J;
    m_key_map[XK_K] = KC_KEY_K;
    m_key_map[XK_L] = KC_KEY_L;
    m_key_map[XK_M] = KC_KEY_M;
    m_key_map[XK_N] = KC_KEY_N;
    m_key_map[XK_O] = KC_KEY_O;
    m_key_map[XK_P] = KC_KEY_P;
    m_key_map[XK_Q] = KC_KEY_Q;
    m_key_map[XK_R] = KC_KEY_R;
    m_key_map[XK_S] = KC_KEY_S;
    m_key_map[XK_T] = KC_KEY_T;
    m_key_map[XK_U] = KC_KEY_U;
    m_key_map[XK_V] = KC_KEY_V;
    m_key_map[XK_W] = KC_KEY_W;
    m_key_map[XK_X] = KC_KEY_X;
    m_key_map[XK_Y] = KC_KEY_Y;
    m_key_map[XK_Z] = KC_KEY_Z;
    m_key_map[XK_bracketleft] = KC_KEY_OEM_4;
    m_key_map[XK_backslash] = KC_KEY_OEM_5;
    m_key_map[XK_bracketright] = KC_KEY_OEM_6;
    m_key_map[XK_asciicircum] = KC_KEY_OEM_5;
    m_key_map[XK_degree] = KC_KEY_UNKNOWN;
    m_key_map[XK_underscore] = KC_KEY_MINUS;
    m_key_map[XK_grave] = KC_KEY_OEM_3;
    m_key_map[XK_acute] = KC_KEY_OEM_6;
    m_key_map[XK_a] = KC_KEY_A;
    m_key_map[XK_b] = KC_KEY_B;
    m_key_map[XK_c] = KC_KEY_C;
    m_key_map[XK_d] = KC_KEY_D;
    m_key_map[XK_e] = KC_KEY_E;
    m_key_map[XK_f] = KC_KEY_F;
    m_key_map[XK_g] = KC_KEY_G;
    m_key_map[XK_h] = KC_KEY_H;
    m_key_map[XK_i] = KC_KEY_I;
    m_key_map[XK_j] = KC_KEY_J;
    m_key_map[XK_k] = KC_KEY_K;
    m_key_map[XK_l] = KC_KEY_L;
    m_key_map[XK_m] = KC_KEY_M;
    m_key_map[XK_n] = KC_KEY_N;
    m_key_map[XK_o] = KC_KEY_O;
    m_key_map[XK_p] = KC_KEY_P;
    m_key_map[XK_q] = KC_KEY_Q;
    m_key_map[XK_r] = KC_KEY_R;
    m_key_map[XK_s] = KC_KEY_S;
    m_key_map[XK_t] = KC_KEY_T;
    m_key_map[XK_u] = KC_KEY_U;
    m_key_map[XK_v] = KC_KEY_V;
    m_key_map[XK_w] = KC_KEY_W;
    m_key_map[XK_x] = KC_KEY_X;
    m_key_map[XK_y] = KC_KEY_Y;
    m_key_map[XK_z] = KC_KEY_Z;
    m_key_map[XK_ssharp] = KC_KEY_OEM_4;
    m_key_map[XK_adiaeresis] = KC_KEY_OEM_7;
    m_key_map[XK_odiaeresis] = KC_KEY_OEM_3;
    m_key_map[XK_udiaeresis] = KC_KEY_OEM_1;
    m_key_map[XK_Super_L] = KC_KEY_LWIN;
    m_key_map[XK_Super_R] = KC_KEY_RWIN;
}

bool DeviceLinux::processEvents()
{
    if (!m_display)
        return false;

    Event event;

    while (XPending(m_display) > 0 && !m_close)
    {
        XEvent xevent;
        XNextEvent(m_display, &xevent);

        switch (xevent.type)
        {
        case ConfigureNotify:
            m_window_width = xevent.xconfigure.width;
            m_window_height = xevent.xconfigure.height;
            break;

        case MapNotify:
            m_window_minimized = false;
            break;

        case UnmapNotify:
            m_window_minimized = true;
            break;

        case FocusIn:
            m_window_has_focus = true;
            break;

        case FocusOut:
            m_window_has_focus = false;
            break;

        case MotionNotify:
            event.type = ET_MOUSE_EVENT;
            event.mouse.type = ME_MOUSE_MOVED;
            event.mouse.x = xevent.xbutton.x;
            event.mouse.y = xevent.xbutton.y;
            event.mouse.control = (xevent.xkey.state & ControlMask);
            event.mouse.shift = (xevent.xkey.state & ShiftMask);
            event.mouse.button_state_left = (xevent.xbutton.state & Button1Mask);
            event.mouse.button_state_right = (xevent.xbutton.state & Button3Mask);
            event.mouse.button_state_middle = (xevent.xbutton.state & Button2Mask);

            sendEvent(event);
            break;

        case ButtonPress:
        case ButtonRelease:
            event.type = ET_MOUSE_EVENT;
            event.mouse.x = xevent.xbutton.x;
            event.mouse.y = xevent.xbutton.y;
            event.mouse.control = (xevent.xkey.state & ControlMask);
            event.mouse.shift = (xevent.xkey.state & ShiftMask);
            event.mouse.button_state_left = (xevent.xbutton.state & Button1Mask);
            event.mouse.button_state_right = (xevent.xbutton.state & Button3Mask);
            event.mouse.button_state_middle = (xevent.xbutton.state & Button2Mask);
            event.mouse.type = ME_COUNT;

            if (xevent.type == ButtonPress)
            {
                switch (xevent.xbutton.button)
                {
                case Button1:
                    event.mouse.type = ME_LEFT_PRESSED;
                    event.mouse.button_state_left = true;
                    break;
                case Button2:
                    event.mouse.type = ME_MIDDLE_PRESSED;
                    event.mouse.button_state_middle = true;
                    break;
                case Button3:
                    event.mouse.type = ME_RIGHT_PRESSED;
                    event.mouse.button_state_right = true;
                    break;
                case Button4:
                    event.mouse.type = ME_MOUSE_WHEEL;
                    event.mouse.wheel = 1.0f;
                    break;
                case Button5:
                    event.mouse.type = ME_MOUSE_WHEEL;
                    event.mouse.wheel = -1.0f;
                default:
                    break;
                }
            }
            else
            {
                switch (xevent.xbutton.button)
                {
                case Button1:
                    event.mouse.type = ME_LEFT_RELEASED;
                    event.mouse.button_state_left = false;
                    break;
                case Button2:
                    event.mouse.type = ME_MIDDLE_RELEASED;
                    event.mouse.button_state_middle = false;
                    break;
                case Button3:
                    event.mouse.type = ME_RIGHT_RELEASED;
                    event.mouse.button_state_right = false;
                    break;
                default:
                    break;
                }
            }

            if (event.mouse.type != ME_COUNT)
            {
                sendEvent(event);
            }

            event.mouse.type = checkMouseClick(event.mouse);
            
            if (event.mouse.type != ME_COUNT)
            {
                sendEvent(event);
            }
            
            break;

        case MappingNotify:
            XRefreshKeyboardMapping(&xevent.xmapping);
            break;

        case KeyRelease:
            if (XPending(m_display) > 0)
            {
                XEvent next_event;
                XPeekEvent(xevent.xkey.display, &next_event);

                if ((next_event.type == KeyPress) &&
                    (next_event.xkey.keycode == xevent.xkey.keycode) &&
                    (next_event.xkey.time - xevent.xkey.time) < 2)
                {
                    break;
                }
            }

            event.type = ET_KEY_EVENT;
            event.key.pressed = false;
            event.key.text[0] = 0;
            event.key.control = (xevent.xkey.state & ControlMask) != 0;
            event.key.shift = (xevent.xkey.state & ShiftMask) != 0;
            event.key.id = getKeyCode(xevent);

            sendEvent(event);
            break;

        case KeyPress:
            event.type = ET_KEY_EVENT;
            event.key.pressed = true;
            event.key.text[0] = 0;
            
            if (m_input_context)
            {
                char buf[4] = {0};
                KeySym keysym;
                Status s;
                int len = XmbLookupString(m_input_context, &xevent.xkey, buf, 
                                          sizeof(buf), &keysym, &s);
                    
                if ((s == XLookupChars || s == XLookupBoth) && len > 0)
                {
                    memcpy(event.key.text, buf, 4);
                }
            }

            event.key.control = (xevent.xkey.state & ControlMask) != 0;
            event.key.shift = (xevent.xkey.state & ShiftMask) != 0;
            event.key.id = getKeyCode(xevent);

            sendEvent(event);
            break;

        case ClientMessage:
        {
            char* atom_name = XGetAtomName(m_display, xevent.xclient.message_type);
            std::string name = atom_name ? atom_name : "";
            
            if (name == "WM_PROTOCOLS")
            {
                m_close = true;
            }

            XFree(atom_name);
        }
            break;

        case SelectionRequest:
        {
            XEvent respond;
            respond.xselection.property = None;
            
            XSelectionRequestEvent* req = &(xevent.xselectionrequest);
            
            if (req->target == m_atom_utf8_string)
            {
                XChangeProperty(m_display, req->requestor, req->property, 
                                req->target, 8, PropModeReplace,
                                (unsigned char*) m_clipboard.c_str(),
                                m_clipboard.size());
                respond.xselection.property = req->property;
            }
            else if (req->target == m_atom_targets)
            {
                Atom data[1] = {m_atom_utf8_string};

                XChangeProperty(m_display, req->requestor, req->property, 
                                XA_ATOM, 32, PropModeReplace,
                                (unsigned char*)data, 1);
                respond.xselection.property = req->property;
            }
            
            respond.xselection.type= SelectionNotify;
            respond.xselection.display= req->display;
            respond.xselection.requestor= req->requestor;
            respond.xselection.selection=req->selection;
            respond.xselection.target= req->target;
            respond.xselection.time = req->time;
            
            XSendEvent(m_display, req->requestor, 0, 0, &respond);
            XFlush(m_display);
        }
            break;

        default:
            break;
        }
    }

    if (!m_close)
    {
        pollJoysticks();
    }

    return !m_close;
}

void DeviceLinux::setWindowCaption(const char* text)
{
    XTextProperty txt;
    int s = XmbTextListToTextProperty(m_display, (char**)&text, 1, 
                                      XStdICCTextStyle, &txt);
    
    if (s == Success)
    {
        XSetWMName(m_display, m_window, &txt);
        XSetWMIconName(m_display, m_window, &txt);
        XFree(txt.value);
    }
}

void DeviceLinux::setWindowClass(const char* text)
{
    XClassHint* classhint = XAllocClassHint();
    classhint->res_name = (char*)text;
    classhint->res_class = (char*)text;
    XSetClassHint(m_display, m_window, classhint);
    XFree(classhint);
}

void DeviceLinux::setWindowResizable(bool resizable)
{
    if (m_window_fullscreen)
        return;

    XUnmapWindow(m_display, m_window);
    
    if (!resizable)
    {
        XSizeHints* hints = XAllocSizeHints();
        hints->flags = PSize | PMinSize | PMaxSize;
        hints->min_width = hints->max_width = hints->base_width = m_window_width;
        hints->min_height = hints->max_height = hints->base_height = m_window_height;
        XSetWMNormalHints(m_display, m_window, hints);
        XFree(hints);
    }
    else
    {
        XSetWMNormalHints(m_display, m_window, m_std_hints);
    }
    
    XMapWindow(m_display, m_window);
    XFlush(m_display);
}

void DeviceLinux::setWindowMinimized()
{
    XIconifyWindow(m_display, m_window, m_screen_nr);
}

void DeviceLinux::setWindowMaximized()
{
    XMapWindow(m_display, m_window);
}

Window DeviceLinux::getToplevelParent()
{
    Window current_window = m_window;
    Window parent;
    Window root;
    Window* children;
    unsigned int num_children;
    
    while (true)
    {
        bool success = XQueryTree(m_display, current_window, &root, &parent, 
                                  &children, &num_children);
        
        if (!success)
            return 0;
        
        if (children) 
        {
            XFree(children);
        }
        
        if (current_window == root || parent == root) 
        {
            return current_window;
        }
        else
        {
            current_window = parent;
        }
    }

    return 0;
}

bool DeviceLinux::setWindowPosition(int x, int y)
{
    if (m_window_fullscreen)
        return false;
        
    int display_width = XDisplayWidth(m_display, m_screen_nr);
    int display_height = XDisplayHeight(m_display, m_screen_nr);

    std::min(x, display_width - (int)m_window_width);
    std::min(y, display_height - (int)m_window_height);
    
    XMoveWindow(m_display, m_window, x, y);
    return true;
}

bool DeviceLinux::getWindowPosition(int* x, int* y)
{
    if (m_window_fullscreen)
        return false;
        
    Window tp_window = getToplevelParent();
    
    if (tp_window == 0)
        return false;

    XWindowAttributes xwa;
    bool success = XGetWindowAttributes(m_display, tp_window, &xwa);
    
    if (!success)
        return false;
        
    *x = xwa.x;
    *y = xwa.y;

    return true;
}

std::string DeviceLinux::getClipboardContent()
{
    if (m_atom_clipboard == None) 
        return m_clipboard;

    Window owner_window = XGetSelectionOwner(m_display, m_atom_clipboard);
    
    if (owner_window == None || owner_window == m_window)
        return m_clipboard;

    Atom selection = XInternAtom(m_display, "CLIPBOARD_SELECTION", false);
    XConvertSelection(m_display, m_atom_clipboard, m_atom_utf8_string, 
                      selection, m_window, CurrentTime);

    bool data_ok = false;
    
    for (int i = 0; i < 500; i++)
    {
        XEvent xevent;
        bool res = XCheckTypedWindowEvent(m_display, m_window, SelectionNotify, 
                                          &xevent);
        
        if (res && xevent.xselection.selection == m_atom_clipboard)
        {
            data_ok = true;
            break;
        }

        sleep(1);
    }

    if (!data_ok)
        return "";

    Atom type;
    int format;
    unsigned long num_items, dummy;
    char* data = NULL;

    int result = XGetWindowProperty(m_display, m_window, selection, 0, INT_MAX/4, 
                                    false, AnyPropertyType, &type, &format, 
                                    &num_items, &dummy, (unsigned char**)&data);

    if (result == Success)
    {
        m_clipboard = data;
        XFree(data);
    }

    return m_clipboard;
}

void DeviceLinux::setClipboardContent(std::string text)
{
    m_clipboard = text;
    XSetSelectionOwner(m_display, m_atom_clipboard, m_window, CurrentTime);
    XFlush(m_display);
}

Bool isEventType(Display* display, XEvent* event, XPointer arg)
{
    return (event && event->type == *(int*)arg);
}

void DeviceLinux::clearSystemMessages()
{
    std::array<int, 5> args = {ButtonPress, ButtonRelease, MotionNotify, 
                               KeyRelease, KeyPress};

    for (int arg : args)
    {
        XEvent event;
        while (XCheckIfEvent(m_display, &event, isEventType, XPointer(&arg))) {}
    }
}

void DeviceLinux::initCursor()
{
    Pixmap invisible_bitmap = XCreatePixmap(m_display, m_window, 32, 32, 1);
    Pixmap mask_bitmap = XCreatePixmap(m_display, m_window, 32, 32, 1);
    Colormap colormap = XDefaultColormap(m_display, m_screen_nr);
    
    XColor fg, bg;
    XAllocNamedColor(m_display, colormap, "black", &fg, &fg);
    XAllocNamedColor(m_display, colormap, "white", &bg, &bg);

    XGCValues values;
    unsigned long valuemask = 0;
    GC gc = XCreateGC(m_display, invisible_bitmap, valuemask, &values);
    unsigned long black_pixel = XBlackPixel(m_display, m_screen_nr);
    XSetForeground(m_display, gc, black_pixel);
    XFillRectangle(m_display, invisible_bitmap, gc, 0, 0, 32, 32);
    XFillRectangle(m_display, mask_bitmap, gc, 0, 0, 32, 32);

    m_cursor_invisible = XCreatePixmapCursor(m_display, invisible_bitmap, 
                                             mask_bitmap, &fg, &bg, 1, 1);

    XFreeGC(m_display, gc);
    XFreePixmap(m_display, invisible_bitmap);
    XFreePixmap(m_display, mask_bitmap);
}

void DeviceLinux::closeCursor()
{
    if (m_display && m_cursor_invisible)
    {
        XFreeCursor(m_display, m_cursor_invisible);
    }
}

void DeviceLinux::setCursorVisible(bool visible)
{
    m_cursor_is_visible = visible;

    if (!m_cursor_is_visible)
    {
        XDefineCursor(m_display, m_window, m_cursor_invisible);
    }
    else
    {
        XUndefineCursor(m_display, m_window);
    }
}

void DeviceLinux::setCursorPosition(int x, int y)
{
    XWarpPointer(m_display, None, m_window, 0, 0, m_window_width, 
                 m_window_height, x, y);
    XFlush(m_display);
    
    m_cursor_x = x;
    m_cursor_y = y;
}

void DeviceLinux::getCursorPosition(int* x, int* y)
{
    Window root, child;
    int root_x, root_y;
    unsigned int mask;
    XQueryPointer(m_display, m_window, &root, &child, &root_x, &root_y, 
                  &m_cursor_x, &m_cursor_y, &mask);

    m_cursor_x = std::min(std::max(m_cursor_x, 0), (int)m_window_width);
    m_cursor_y = std::min(std::max(m_cursor_y, 0), (int)m_window_height);
    
    *x = m_cursor_x;
    *y = m_cursor_y;
}

void DeviceLinux::activateJoysticks()
{
#ifndef __CYGWIN__
    for (unsigned int i = 0; i < 32; i++)
    {
        JoystickInfo joystick_info;
        joystick_info.id = i;

        std::string device_name = "/dev/js" + std::to_string(i);
        joystick_info.fd = open(device_name.c_str(), O_RDONLY);
        
        if (joystick_info.fd == -1)
        {
            device_name = "/dev/input/js" + std::to_string(i);
            joystick_info.fd = open(device_name.c_str(), O_RDONLY);
        }
        
        if (joystick_info.fd == -1)
        {
            device_name = "/dev/joy" + std::to_string(i);
            joystick_info.fd = open(device_name.c_str(), O_RDONLY);
        }

        if (joystick_info.fd == -1)
            continue;

        ioctl(joystick_info.fd, JSIOCGAXES, &(joystick_info.axes));
        ioctl(joystick_info.fd, JSIOCGBUTTONS, &(joystick_info.buttons));
        fcntl(joystick_info.fd, F_SETFL, O_NONBLOCK);

        char name[80];
        ioctl(joystick_info.fd, JSIOCGNAME(80), name);
        joystick_info.name = name;
        
        memset(&joystick_info.data, 0, sizeof(joystick_info.data));
        joystick_info.data.id = joystick_info.id;
        
        m_active_joysticks.push_back(joystick_info);
    }
#endif
}

void DeviceLinux::pollJoysticks()
{
#ifndef __CYGWIN__
    if (m_active_joysticks.empty())
        return;

    bool event_received = false;
    
    for (JoystickInfo& joystick_info : m_active_joysticks)
    {
        struct js_event evt;
        
        while (read(joystick_info.fd, &evt, sizeof(evt)) == sizeof(evt))
        {
            if (evt.number >= 32)
                continue;
            
            switch (evt.type & ~JS_EVENT_INIT)
            {
            case JS_EVENT_BUTTON:
                event_received = true;
                joystick_info.data.button_states[evt.number] = evt.value;
                break;
                
            case JS_EVENT_AXIS:
                event_received = true;
                joystick_info.data.axis[evt.number] = evt.value;
                break;
                
            default:
                break;
            }
        }
        
        Event event;
        event.type = ET_JOYSTICK_EVENT;
        event.joystick = joystick_info.data;

        sendEvent(event);
    }
    
    if (event_received)
    {
        XResetScreenSaver(m_display);
    }
#endif
}

void DeviceLinux::closeJoysticks()
{
#ifndef __CYGWIN__
    for (unsigned int i = 0; i < m_active_joysticks.size(); i++)
    {
        if (m_active_joysticks[i].fd < 0)
            continue;
        
        close(m_active_joysticks[i].fd);
    }
    
    m_active_joysticks.clear();
#endif
}

#endif
