// window host for x11 windows (https://tronche.com/gui/x/xlib/function-index.html)
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>                              // usleep
#include <pthread.h>
#include <stdio.h>
#include "screen.h"

#if 1
static pthread_mutex_t app_mutex = PTHREAD_MUTEX_INITIALIZER;
#define M_LOCK pthread_mutex_lock(&app_mutex) == 0
#define M_UNLOCK pthread_mutex_unlock(&app_mutex)
#else
#define M_LOCK 1
#define M_UNLOCK
#endif

#if 0
#define VM_DELAY usleep(20*1000)                 // on slow VM, avoid too many screen refresh
#else
#define VM_DELAY
#endif

// static states
struct screen_t scr = { 0 };
struct key_states_t key_states = { 0 };

static struct
{
  Display *disp;                                 // display
  Window root_win;                               // desktop
  XVisualInfo *vis_rgb888;                       // graphic mode
  Cursor cursor_list[MC_TEXT+1];                 // mouse cursor list
} app_x11 = { 0 };

#define GET_WX11 w_x11_t *w_x11 = (w_x11_t *)w

// host X11 window
typedef struct w_x11_t
{
  win_t w;                                       // window
  Window win;                                    // X11 window
  GC gc;                                         // Graphic context
  XImage *Ximage;                                // bitmap used to blit
} w_x11_t;

// return size to alloc for host window
int scr_win_sizeof(void)
{
  return sizeof(w_x11_t);
}

// create window bitmap
static void resize_bitmap(w_x11_t *w_x11, int dx, int dy)
{
  pix_t *pix_ptr;
  if (w_x11->Ximage)
  {
    XDestroyImage(w_x11->Ximage);                // note: free pix_ptr
    w_x11->Ximage = NULL;
  }

  pix_ptr = (pix_t *)malloc(dx*dy*sizeof(pix_t)); // note: cannot use W_MALLOC, free is done by XDestroyImage (no ptr adjust)
  if (pix_ptr)
  {
    // create image uses as frame buffer
    w_x11->Ximage = XCreateImage(
    app_x11.disp,                                // display
    app_x11.vis_rgb888->visual,                  // rgb 888 visual
    app_x11.vis_rgb888->depth,                   // depth must match vis depth
    ZPixmap,                                     // format of image
    0,                                           // number of pixels to ignore at begin of image
    (char *)pix_ptr,                             // image memory
    dx,
    dy,
    32,                                          // line byte alignment in bits
    dx*4);                                       // bytes per line (contiguous)

    if (w_x11->Ximage)
      bm_init(&w_x11->w.win_bm, dx, dy, dx, pix_ptr);
    else
    {
      free(pix_ptr);                             // supposed not done if XCreateImage fail ? todo
      bm_init(&w_x11->w.win_bm, 0, 0, 0, NULL);  // failed
    }
  }
}

// ----------------------------------------------
// window creation

#include <X11/Xatom.h>

// remove borders, title bar
static void wm_no_borders(Display *x11_disp, Window x11_root_win, Window x11_win, bool is_popup)
{
  Atom WM_HINTS = XInternAtom(x11_disp, "_MOTIF_WM_HINTS", True);
  if (WM_HINTS != None)
  {
    #define MWM_HINTS_FUNCTIONS     (1L << 0)
    #define MWM_HINTS_DECORATIONS   (1L << 1)
    #define MWM_HINTS_INPUT_MODE    (1L << 2)
    #define MWM_HINTS_STATUS        (1L << 3)
    // MWM no decorations values
    #define MWM_DECOR_NONE          0

    struct
    {
      unsigned long flags;
      unsigned long functions;
      unsigned long decorations;
               long input_mode;
      unsigned long status;
    } MWMHints = { MWM_HINTS_DECORATIONS, 0, MWM_DECOR_NONE, 0, 0 };

    XChangeProperty(x11_disp, x11_win, WM_HINTS, WM_HINTS, 32,
                    PropModeReplace, (unsigned char *)&MWMHints, sizeof(MWMHints)/4);
  }

  WM_HINTS = XInternAtom(x11_disp, "KWM_WIN_DECORATION", True);
  if (WM_HINTS != None)
  {
    #define KDE_noDecoration 0
    long KWMHints = KDE_noDecoration;
    XChangeProperty(x11_disp, x11_win, WM_HINTS, WM_HINTS, 32,
                    PropModeReplace, (unsigned char *)&KWMHints, sizeof(KWMHints)/4);
  }

  WM_HINTS = XInternAtom(x11_disp, "_WIN_HINTS", True);
  if (WM_HINTS != None)
  {
    long GNOMEHints = 0;
    XChangeProperty(x11_disp, x11_win, WM_HINTS, WM_HINTS, 32,
                    PropModeReplace, (unsigned char *)&GNOMEHints, sizeof(GNOMEHints)/4);
  }

  WM_HINTS = XInternAtom(x11_disp, "_NET_WM_WINDOW_TYPE", True);
  if (WM_HINTS != None)
  {
    Atom NET_WMHints[2];
    NET_WMHints[0] = XInternAtom(x11_disp, "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", True);
    NET_WMHints[1] = XInternAtom(x11_disp, is_popup ? "_NET_WM_WINDOW_TYPE_DOCK" : "_NET_WM_WINDOW_TYPE_NORMAL", True);
    XChangeProperty(x11_disp, x11_win, WM_HINTS, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&NET_WMHints, 2);
  }
  XSetTransientForHint(x11_disp, x11_win, x11_root_win);
  XUnmapWindow(x11_disp, x11_win);
  XMapWindow(x11_disp, x11_win);
}

// define events we want to receive
#define X11_EV_MASK   PointerMotionMask | ButtonPressMask | ButtonReleaseMask\
                    | KeyPressMask | KeyReleaseMask | DestroyNotify | FocusChangeMask | ExposureMask

// change name for window manager
static void scr_set_wm_win_name(Window w, const char *name)
{
  Atom a_name = XInternAtom(app_x11.disp, "WM_NAME", True);
  if (a_name != None)
  {
    Atom a_utf8 = XInternAtom(app_x11.disp, "UTF8_STRING", True);
    if (a_utf8 != None)
      XChangeProperty(app_x11.disp, w, a_name, a_utf8, 8, PropModeReplace, (unsigned char *)name, strlen(name));
  }
}

// create hidden window
bool scr_win_create(win_t *w, const char *name)
{
  GET_WX11;
  XSetWindowAttributes wa;
  wa.event_mask = X11_EV_MASK;                   // define event we want to receive

  // create host window
  w_x11->win = XCreateWindow(
    app_x11.disp,                                // display
    app_x11.root_win,                            // parent is desktop (else child clipped into parent !)
    0, 0,                                        // position (relative to parent = desktop)
    32, 32,                                      // size (client)
    0,                                           // borders width
    24,                                          // color depth
    InputOutput,                                 // class
    CopyFromParent,                              // use desktop format, if conversion required to blit, done in XPutImage
    CWEventMask,                                 // attribute mask of XSetWindowAttributes
    &wa);                                        // XSetWindowAttributes

  if (w_x11->win)
  {
    XGCValues GCValues;
    scr_set_wm_win_name(w_x11->win, name);       // define name for windows manager
    wm_no_borders(app_x11.disp, app_x11.root_win, w_x11->win, w->next != NULL);
    w_x11->gc = XCreateGC(app_x11.disp, w_x11->win, 0, &GCValues);
    if (w_x11->gc)
      return true;
    scr_win_destroy(w);
  }
  return false;
}

// destroy window
void scr_win_destroy(win_t *w)
{
  GET_WX11;
  if (w_x11->Ximage)
    XDestroyImage(w_x11->Ximage);
  if (w_x11->gc)
    XFreeGC(app_x11.disp, w_x11->gc);
  XDestroyWindow(app_x11.disp, w_x11->win);
  w_x11->win = 0;
}

// show/hide
void scr_win_show(win_t *w, bool show)
{
  GET_WX11;
  if (show)
    XMapRaised(app_x11.disp, w_x11->win);     // show
  else
    XUnmapWindow(app_x11.disp, w_x11->win);   // hide
}

// resize window
void scr_win_move_resize(win_t *w, int x, int y, int dx, int dy)
{
  GET_WX11;
  XMoveResizeWindow(app_x11.disp, w_x11->win, x, y, dx, dy);
  resize_bitmap(w_x11, dx, dy);
  w_x11->w.win_pos.x = x;
  w_x11->w.win_pos.y = y;
}

// move window
void scr_win_move(win_t *w, int x, int y)
{
  GET_WX11;
  XMoveWindow(app_x11.disp, w_x11->win, x, y);
  w_x11->w.win_pos.x = x;
  w_x11->w.win_pos.y = y;
  VM_DELAY;
}

void scr_win_blt_all(win_t *w)
{
  if (M_LOCK)
  {
    GET_WX11;
    if (w_x11->Ximage)
      XPutImage(app_x11.disp, w_x11->win, w_x11->gc, w_x11->Ximage, 0, 0, 0, 0, w_x11->w.win_bm.size.x, w_x11->w.win_bm.size.y);
    M_UNLOCK;
  }
}

void scr_win_blt_rect(win_t *w, int x, int y, int dx, int dy)
{
  if (M_LOCK)
  {
    GET_WX11;
    if (w_x11->Ximage)
      XPutImage(app_x11.disp, w_x11->win, w_x11->gc, w_x11->Ximage, x, y, x, y, dx, dy);
    M_UNLOCK;
  }
}

// desktop size
static bool get_screen_size(vec2i *size)
{
  int sc_cnt = ScreenCount(app_x11.disp);            // count of screen
  if (sc_cnt)
  {
    int sc_id = DefaultScreen(app_x11.disp);
    Screen *scr = ScreenOfDisplay(app_x11.disp, sc_id);  // use screen 0 as desktop
    if (scr)
    {
      size->x = WidthOfScreen(scr);
      size->y = HeightOfScreen(scr);
      return true;
    }
  }
  return false;
}

// ----------------------------------------------
// window messages
static const enum e_win_event msg_map_bt_down[4] = { 0, EV_LBUTTONDOWN, EV_MBUTTONDOWN, EV_RBUTTONDOWN };
static const enum e_win_event msg_map_bt_up[4] = { 0, EV_LBUTTONUP, EV_MBUTTONUP, EV_RBUTTONUP };

static unsigned char key_conv[256] = { 0 };

static XkbDescPtr keyboard_map;

// conversion for code 0xffxx
static void init_key_conv(void)
{
  // get X11 keymap
  keyboard_map = XkbGetMap(app_x11.disp, XkbAllClientInfoMask, XkbUseCoreKbd);

  // define locale conversion
  #define XK(k) ((k) & 0xff)
  #define K_CTRL(k) (k | 0x80)             // produce key_pressed event, not EV_CHAR
  key_conv[XK(XK_Return)]    = KEY_RETURN;
  key_conv[XK(XK_Left)]      = K_CTRL(KEY_LEFT);
  key_conv[XK(XK_Right)]     = K_CTRL(KEY_RIGHT);
  key_conv[XK(XK_Up)]        = K_CTRL(KEY_UP);
  key_conv[XK(XK_Down)]      = K_CTRL(KEY_DOWN);
  key_conv[XK(XK_Page_Up)]   = K_CTRL(KEY_PGUP);
  key_conv[XK(XK_Page_Down)] = K_CTRL(KEY_PGDOWN);
  key_conv[XK(XK_Escape)]    = K_CTRL(KEY_ESCAPE);
  key_conv[XK(XK_BackSpace)] = K_CTRL(KEY_BACK);
  key_conv[XK(XK_Insert)]    = K_CTRL(KEY_INS);
  key_conv[XK(XK_Delete)]    = K_CTRL(KEY_SUPPR1);
  key_conv[XK(XK_End)]       = K_CTRL(KEY_END);
  key_conv[XK(XK_Home)]      = K_CTRL(KEY_HOME);
  key_conv[XK(XK_Tab)]       = K_CTRL(KEY_TAB);
  key_conv[XK(XK_F1)]        = K_CTRL(KEY_F1);
  key_conv[XK(XK_F2)]        = K_CTRL(KEY_F2);
  key_conv[XK(XK_F3)]        = K_CTRL(KEY_F3);
  key_conv[XK(XK_F4)]        = K_CTRL(KEY_F4);
  key_conv[XK(XK_F5)]        = K_CTRL(KEY_F5);
  key_conv[XK(XK_F6)]        = K_CTRL(KEY_F6);
  key_conv[XK(XK_F7)]        = K_CTRL(KEY_F7);
  key_conv[XK(XK_F8)]        = K_CTRL(KEY_F8);
  key_conv[XK(XK_F9)]        = K_CTRL(KEY_F9);
  key_conv[XK(XK_F10)]       = K_CTRL(KEY_F10);
  key_conv[XK(XK_F11)]       = K_CTRL(KEY_F11);
  key_conv[XK(XK_F12)]       = K_CTRL(KEY_F12);

  // batched
  key_conv[XK(XK_Alt_L)]     = K_CTRL(KEY_ALT);
  key_conv[XK(XK_Alt_R)]     = K_CTRL(KEY_ALT);
  key_conv[XK(XK_Shift_L)]   = K_CTRL(KEY_SHIFT);
  key_conv[XK(XK_Shift_R)]   = K_CTRL(KEY_SHIFT);
  key_conv[XK(XK_Control_L)] = K_CTRL(KEY_CTRL);
  key_conv[XK(XK_Control_R)] = K_CTRL(KEY_CTRL);

  // keypad
  key_conv[XK(XK_KP_Enter)]     = 0xd;
  key_conv[XK(XK_KP_Multiply)]  = '*';
  key_conv[XK(XK_KP_Add)]       = '+';
  key_conv[XK(XK_KP_Subtract)]  = '-';
  key_conv[XK(XK_KP_Decimal)]   = '.';
  key_conv[XK(XK_KP_Divide)]    = '/';
  key_conv[XK(XK_KP_0)] = '0';
  key_conv[XK(XK_KP_1)] = '1';
  key_conv[XK(XK_KP_2)] = '2';
  key_conv[XK(XK_KP_3)] = '3';
  key_conv[XK(XK_KP_4)] = '4';
  key_conv[XK(XK_KP_5)] = '5';
  key_conv[XK(XK_KP_6)] = '6';
  key_conv[XK(XK_KP_7)] = '7';
  key_conv[XK(XK_KP_8)] = '8';
  key_conv[XK(XK_KP_9)] = '9';
}

// code to convert key code to key symbol
static KeySym get_key_sym(KeyCode key_code, int key_state)
{
  KeySym keysym = NoSymbol;
  if (keyboard_map)
  {
    unsigned char info = XkbKeyGroupInfo(keyboard_map, key_code);
    unsigned int num_groups = XkbKeyNumGroups(keyboard_map, key_code);
    int i, level = 0;
    unsigned int active_mods;
    XkbKeyTypePtr key_type;

    // get the group
    unsigned int group = 0x00;
    switch (XkbOutOfRangeGroupAction(info))
    {
      case XkbRedirectIntoRange:
        group = XkbOutOfRangeGroupInfo(info);
        if (group >= num_groups)
          group = 0;
      break;
      case XkbClampIntoRange:
        group = num_groups - 1;
      break;
      case XkbWrapIntoRange:
      default:
        if (num_groups != 0)
          group %= num_groups;
      break;
    }

    key_type = XkbKeyKeyType(keyboard_map, key_code, group);
    active_mods = key_state & key_type->mods.mask;
    for (i=0; i<key_type->map_count; i++)
    {
      if (key_type->map[i].active && (key_type->map[i].mods.mask == active_mods))
      {
        level = key_type->map[i].level;
        break;
      }
    }
    keysym = XkbKeySymEntry(keyboard_map, key_code, level, group);
  }
  return keysym;
}

static void x11_clipb_export_request(XEvent *xev);

#define DBL_CLICK_TIME 300                       // double click min time
static Time last_l_click = 0;                    // simulate double click (no X11 event for that)

static void ProcessEvent(XEvent *xev)
{
  w_x11_t *w_x11;
  win_t *w;

  // find user window using X11 window
  w_x11 = (w_x11_t *)scr.win_top;
  while (1)
  {
    if (!w_x11)
      return;
    if (w_x11->win == xev->xany.window)
      break;
    w_x11 = (w_x11_t *)w_x11->w.next;
  };

  w = (win_t *)w_x11;
  switch (xev->type)
  {
    case 0:
    case 1:
    break;
    case KeyPress:
    if (w == scr.win_top)
    {
      KeySym keysym = get_key_sym(xev->xkey.keycode, xev->xkey.state);
      if (keysym == NoSymbol)
        return;

      if ((keysym & 0xff00) == 0xff00)
      {
        int k = key_conv[keysym & 0xff];
        if (k & 0x80)
        {
          k &= 0x7f;
          w->ev.type = EV_KEYPRESSED;
        }
        else
          w->ev.type = EV_CHAR;
        if (k)
        {
          key_states.key_pressed[k] = 1;
          w->ev.key_pressed = k;
          w->ev_proc((hwin_t *)w);
        }
      }
      else
      if ((keysym & 0xff00) == 0)
      {
        int k = keysym;
        key_states.key_pressed[k] = 1;
        w->ev.type = EV_CHAR;
        if (key_states.key_pressed[KEY_CTRL])      // ctrl a..z code 1..26, must work in caps and minus
          k = (k >= 'a') ? ((k - 'a') + 1) : ((k - 'A') + 1);
        w->ev.key_char = k;
        w->ev_proc((hwin_t *)w);
      }
    }
    break;
    case KeyRelease:
    if (w == scr.win_top)
    {
      KeySym keysym = get_key_sym(xev->xkey.keycode, xev->xkey.state);
      if (keysym == NoSymbol)
        return;

      // note: no event produced, not required in user application
      if ((keysym & 0xff00) == 0xff00)
      {
        int k = key_conv[keysym & 0xff];
        if (k)
          key_states.key_pressed[k & 0x7f] = 0;
      }
      else
      if ((keysym & 0xff00) == 0)
      {
        int k = keysym;
        key_states.key_pressed[k] = 0;
      }
    }
    break;
    case ButtonPress:
      if ((xev->xbutton.button >= 1) && (xev->xbutton.button <= 3))
      {
        Time dt = xev->xbutton.time - last_l_click;
        last_l_click = xev->xbutton.time;
        w->ev.mouse_pos.x = xev->xbutton.x;
        w->ev.mouse_pos.y = xev->xbutton.y;
        if (dt < DBL_CLICK_TIME)
        {
          w->ev.type = EV_LBUTTONDBLCLK;
          app_mouse_event_other(w);
        }
        else
        {
          w->ev.type = msg_map_bt_down[xev->xbutton.button];
          app_mouse_set_capture(w);
        }
      }
      else
      if (xev->xbutton.button == 4)
      {
        w->ev.type = EV_MOUSEWHEEL;
        w->ev.mouse_whell = 1;
        app_mouse_event_other(w);
      }
      else
      if (xev->xbutton.button == 5)
      {
        w->ev.type = EV_MOUSEWHEEL;
        w->ev.mouse_whell = -1;
        app_mouse_event_other(w);
      }
    break;
    case ButtonRelease:
      if ((xev->xbutton.button >= 1) && (xev->xbutton.button <= 3))
      {
        w->ev.type = msg_map_bt_up[xev->xbutton.button];
        w->ev.mouse_pos.x = xev->xbutton.x;
        w->ev.mouse_pos.y = xev->xbutton.y;
        app_mouse_release_capture(w);
      }
    break;
    case MotionNotify:
      w->ev.type = EV_MOUSEMOVE;
      w->ev.mouse_pos.x = xev->xmotion.x;
      w->ev.mouse_pos.y = xev->xmotion.y;
      if (!scr.tmr.dis_time_ctr)
      {
        scr.tmr.mouse_time_ctr = 0;
        scr.tmr.mouse_w = w;
      }
      app_mouse_move(w);
    break;
    case EnterNotify:
      // printf("EnterNotify : foc:%d sta:%d\n", xev->xcrossing.focus, xev->xcrossing.state);
    break;
    case LeaveNotify:
    break;
    case FocusIn:
      // note: no WM_ACTIVATEAPP equivalent on X11
      if (scr.win_top->flags & WF_CTRL_POPUP)
        return;                                  // menu, help, nothing to do
      scr.app_active = true;
      app_activate(1);
    break;
    case FocusOut:
      if (w == scr.win_top)
      {
        app_activate(0);
        scr.app_active = false;
      }
    break;
    case KeymapNotify:
    break;
    case Expose:
      if (!xev->xexpose.count)
      {
        scr_win_blt_all(w);
        if (scr.e_ctrl == e_win_ctrl_sizing)
        {
          scr.sizing = false;
          VM_DELAY;                              // wait a small time when resize refresh is done
        }
      }
    break;
    case GraphicsExpose:
    break;
    case NoExpose:
    break;
    case VisibilityNotify:
    break;
    case CreateNotify:
    break;
    case DestroyNotify:
    break;
    case UnmapNotify:
    break;
    case MapNotify:
    break;
    case MapRequest:
    break;
    case ReparentNotify:
    break;
    case ConfigureNotify:
    break;
    case ConfigureRequest:
    break;
    case GravityNotify:
    break;
    case ResizeRequest:
    break;
    case CirculateNotify:
    break;
    case CirculateRequest:
    break;
    case PropertyNotify:
    break;
    case SelectionClear:
    break;
    case SelectionRequest:
      x11_clipb_export_request(xev);
    break;
    case SelectionNotify:
    break;
    case ColormapNotify:
    break;
    case ClientMessage:
    break;
    case MappingNotify:
    break;
    case GenericEvent:
    break;
  }
}

// set the mouse cursor shape
void scr_set_cursor(enum e_mouse_cursor mc)
{
  w_x11_t *w_x11 = (w_x11_t *)scr.win_top;
  if (w_x11)
  {
    W_ASSERT((unsigned int)mc <= MC_TEXT);
    if ((unsigned int)mc <= MC_TEXT)
    {
      if (M_LOCK)
      {
        XDefineCursor(app_x11.disp, w_x11->win, app_x11.cursor_list[mc]);
        M_UNLOCK;
      }
    }
  }
  scr.last_cursor = mc;
}

// return the mouse position in screen
void scr_get_cursor_pos(vec2i *pos)
{
  Window root_return, child_return;
  int root_x_return, root_y_return;
  int win_x_return, win_y_return;
  unsigned int mask_return;

  if (XQueryPointer(app_x11.disp, app_x11.root_win,
      &root_return, &child_return,
      &root_x_return, &root_y_return,
      &win_x_return, &win_y_return, &mask_return))
  {
    pos->x = root_x_return;
    pos->y = root_y_return;
  }
}

// get a rgb888 mode
static XVisualInfo *get_rgb888_vis_info(Display *disp)
{
  int vi_cnt = 0;
  XVisualInfo v, *vi;

  #define V_MSK VisualDepthMask|VisualRedMaskMask|VisualGreenMaskMask|VisualBlueMaskMask|VisualBitsPerRGBMask

  v.red_mask     = 0xff0000;
  v.green_mask   = 0xff00;
  v.blue_mask    = 0xff;
  v.bits_per_rgb = 8;

#if 0
  v.depth        = 32;                 // specify 4 bytes pixel (then use alpha ?)
  vi = XGetVisualInfo(disp, V_MSK, &v, &vi_cnt);
  if (vi_cnt)
    return vi;
#endif

  v.depth        = 24;                 // specify 3 bytes pixel, hope XCreateImage will code pixel on 4 bytes (3 bytes not supported)
  vi = XGetVisualInfo(disp, V_MSK, &v, &vi_cnt);
  if (vi_cnt)
    return vi;

  return NULL;
}

static void x11_clipb_init(Display *disp);
static void x11_clipb_delete(void);

int main(int argc, char *argv[])
{
#if 1
  if (XInitThreads() == 0)                       // check, some very old XLib may not work.
  {
    printf("Error: XInitThreads() returned 0.\n"
           "This program require support for multi threads.\n");
    return -1;
  }
  // note: if program use only main window thread, then can work if XInitThreads() return 0
#endif

  // window manager specific
  if (!init_win_style(win_style_xp))
    return -1;

  app_x11.disp = XOpenDisplay(NULL);
  if (!app_x11.disp)
    return -1;

  // get desktop
  app_x11.root_win = DefaultRootWindow(app_x11.disp);
  if (!app_x11.root_win)
    return -1;

  app_x11.vis_rgb888 = get_rgb888_vis_info(app_x11.disp);
  if (!app_x11.vis_rgb888)
    return -1;

  // get desktop size
  if (!get_screen_size(&scr.size))               // get desktop size
    return -1;

  init_key_conv();                               // key codes conversion
  x11_clipb_init(app_x11.disp);                  // clipboard

  // init mouse cursors
  app_x11.cursor_list[MC_ARROW]    = XCreateFontCursor(app_x11.disp, XC_arrow);
  app_x11.cursor_list[MC_SIZEX]    = XCreateFontCursor(app_x11.disp, XC_sb_h_double_arrow);
  app_x11.cursor_list[MC_SIZEY]    = XCreateFontCursor(app_x11.disp, XC_sb_v_double_arrow);
  app_x11.cursor_list[MC_SIZE_TL]  = XCreateFontCursor(app_x11.disp, XC_top_left_corner);
  app_x11.cursor_list[MC_SIZE_BR]  = XCreateFontCursor(app_x11.disp, XC_bottom_right_corner);
  app_x11.cursor_list[MC_SIZE_BL]  = XCreateFontCursor(app_x11.disp, XC_bottom_left_corner);
  app_x11.cursor_list[MC_SIZE_TR]  = XCreateFontCursor(app_x11.disp, XC_top_right_corner);
  app_x11.cursor_list[MC_SIZEALL]  = XCreateFontCursor(app_x11.disp, XC_fleur);
  app_x11.cursor_list[MC_TEXT]     = XCreateFontCursor(app_x11.disp, XC_xterm);

  if (win_app_main(""))
  {
    int x11_fd = ConnectionNumber(app_x11.disp);
    win_t *w_root = scr.win_top;
    W_ASSERT(!scr.win_top->next);                // single window created in win_app_main

    scr.app_active = true;
    while (scr.win_top)
    {
      XEvent xev;
      fd_set in_fds;
      struct timeval tv;

      FD_ZERO(&in_fds);
      FD_SET(x11_fd, &in_fds);

      tv.tv_usec = 400000;
      tv.tv_sec = 0;

      if (!select(x11_fd+1, &in_fds, 0, 0, &tv))
        app_tmr_update(w_root);

      while (XPending(app_x11.disp))
      {
        XNextEvent(app_x11.disp, &xev);        // blocks until an event is received
        ProcessEvent(&xev);
      }
    }
  }

  x11_clipb_delete();
#ifdef _DEBUG
  w_check_alloc();
#endif

  XkbFreeClientMap(keyboard_map, XkbAllClientInfoMask, true);
  return scr.app_exit_code;
}

// flush pending messages
void scr_msg_flush(void)
{
  XFlush(app_x11.disp);
  while (XPending(app_x11.disp))
  {
    XEvent xev;
    XNextEvent(app_x11.disp, &xev);        // blocks until an event is received
    ProcessEvent(&xev);
  }
}

// init timer message
int scr_set_timer(win_t *w, int delay_ms)
{
  //GET_WX11;
  W_ASSERT(0);     // todo, no support
  return 0;
}

void scr_kill_timer(win_t *w, int timer_id)
{
  //GET_WX11;
  W_ASSERT(0);     // todo
}

// sleep
void sleep_ms(int ms)
{
  usleep(ms*1000);
}

// ms time counter
unsigned int get_ctr_ms(void)
{
  clock_t cl = clock();
  const clock_t cps = CLOCKS_PER_SEC;            // note: is casted constant on linux GCC. Cannot use preprocessor :-(

  if (cps == 1000000)
    return cl / 1000;
  if (cps == 1000)
    return cl;
  if (cps < 1000)
    return (cl*1000) / cps;
  W_ASSERT(!(cps % 1000));
  return cl / (cps/1000);                        // hope cps is 1000 multiple !
}

// ----------------------------------------------
// basic clipboard for text only

#define CLIP_MAX_SIZE 1024

static void x11_clipb_export(const char *str, int len);
static void x11_clipb_import(void);

static struct
{
  int len;
  char str[CLIP_MAX_SIZE];
} clipboard = { 0 };

void win_clip_copy(hwin_t *hw, const char *str, int len)
{
  if (str && len)
  {
    x11_clipb_export(str, len);                  // export to os
    // copy to local
    if (len >= CLIP_MAX_SIZE)
      len = CLIP_MAX_SIZE-1;
    memcpy(clipboard.str, str, len);
    clipboard.str[len] = 0;
    clipboard.len = len;
  }
}

char *win_clip_paste(hwin_t *hw, int *len)
{
  x11_clipb_import();
  *len = clipboard.len;
  return clipboard.str;
}

// -------------------------------------------
// X11 clipboard (text only)
// https://github.com/exebook/x11clipboard

// -------------------------------------------
// export text to clipboard

// 1st step: specify server text is available
// 2nd step: on server request, send text

static struct
{
  unsigned char *text;
  int size;
  Atom xa_atom;
  Atom xa_string;
  Atom selection;
  Atom targets_atom;
  Atom text_atom;
  Atom utf8;
  // paste
  Atom xsel_data;
} x11_cb = { 0 };

static void x11_clipb_init(Display *disp)
{
  x11_cb.xa_atom      = 4;
  x11_cb.xa_string    = 31;
  x11_cb.selection    = XInternAtom(disp, "CLIPBOARD", False);
  x11_cb.targets_atom = XInternAtom(disp, "TARGETS", False);
	x11_cb.text_atom    = XInternAtom(disp, "TEXT", False);
	x11_cb.utf8         = XInternAtom(disp, "UTF8_STRING", True);
	x11_cb.xsel_data    = XInternAtom(disp, "XSEL_DATA", False);  // False: created if it does not exist
	if (x11_cb.utf8 == None)
    x11_cb.utf8 = x11_cb.xa_string;
}

static void x11_clipb_delete(void)
{
  if (x11_cb.text)
  {
    W_FREE(x11_cb.text);
    x11_cb.text = NULL;
  }
  x11_cb.size = 0;
}

// prepare string to export and inform server
static void x11_clipb_export(const char *str, int len)
{
  w_x11_t *w_x11 = (w_x11_t *)scr.win_top;
  if (w_x11)
  {
    if (x11_cb.text)
      W_FREE(x11_cb.text);
    x11_cb.size = 0;
    x11_cb.text = W_MALLOC(len+1);
    if (x11_cb.text)
    {
      memcpy(x11_cb.text, str, len);
      x11_cb.text[len] = 0;
      x11_cb.size = len;

      // inform server app have selection
	    XSetSelectionOwner(app_x11.disp, x11_cb.selection, w_x11->win, 0);
	    if (XGetSelectionOwner(app_x11.disp, x11_cb.selection) != w_x11->win)
	      x11_clipb_delete();                      // no server ack, keeping datas is useless
    }
  }
}

// send clipboard datas on server request
static void x11_clipb_export_request(XEvent *xev)
{
  if (x11_cb.size && (xev->xselectionrequest.selection == x11_cb.selection))
  {
    int R = 0;
	  XSelectionRequestEvent *xsr = &xev->xselectionrequest;
	  XSelectionEvent ev = {0};

	  ev.type      = SelectionNotify;
    ev.display   = xsr->display;
    ev.requestor = xsr->requestor;
	  ev.selection = xsr->selection;
    ev.time      = xsr->time;
    ev.target    = xsr->target;
    ev.property  = xsr->property;

		if (ev.target ==  x11_cb.targets_atom)
      R = XChangeProperty(ev.display, ev.requestor, ev.property, x11_cb.xa_atom, 32, PropModeReplace, (unsigned char *)&x11_cb.utf8, 1);
		else
    if ((ev.target == x11_cb.xa_string) || (ev.target == x11_cb.text_atom))
		  R = XChangeProperty(ev.display, ev.requestor, ev.property, x11_cb.xa_string, 8, PropModeReplace, x11_cb.text, x11_cb.size);
	  else
    if (ev.target == x11_cb.utf8)
		  R = XChangeProperty(ev.display, ev.requestor, ev.property, x11_cb.utf8, 8, PropModeReplace, x11_cb.text, x11_cb.size);
	  else
      ev.property = None;
	  if ((R & 2) == 0)
      XSendEvent(app_x11.disp, ev.requestor, 0, 0, (XEvent *)&ev);
  }
}

// -------------------------------------------
// import text from clipboard

static bool x11_paste_type(w_x11_t *w_x11, Atom past_type)
{
  bool res = false;
	XEvent event;
  XConvertSelection(app_x11.disp, x11_cb.selection, past_type, x11_cb.xsel_data, w_x11->win, CurrentTime);
	XSync(app_x11.disp, 0);
	XNextEvent(app_x11.disp, &event);

  if (    (event.type == SelectionNotify)
       && (event.xselection.selection == x11_cb.selection)
       && (event.xselection.property))
  {
    char *data = NULL;
    unsigned long N, size = 0;
    int format;
    Atom target;

    XGetWindowProperty(
      event.xselection.display,
      event.xselection.requestor,
      event.xselection.property,
      0L, (~0L), 0,
      AnyPropertyType,
      &target, &format, &size, &N, (unsigned char **)&data);

    if ((target == x11_cb.utf8) || (target == x11_cb.xa_string))
    {
      if (size > 0)
      {
        if (size >= sizeof(clipboard.str))
          size = sizeof(clipboard.str)-1;
        memcpy(clipboard.str, data, size);
        clipboard.str[size] = 0;
        clipboard.len = size;
        res = true;
      }
    }
    XFree(data);
    XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property);
  }
  return res;
}

static void x11_clipb_import(void)
{
  w_x11_t *w_x11 = (w_x11_t *)scr.win_top;
  if (w_x11)
  {
    if (!x11_paste_type(w_x11, x11_cb.utf8))
      x11_paste_type(w_x11, x11_cb.xa_string);
  }
}

// ----------------------------------------------
// thread

// very basic support for one thread.

static pthread_t x11_thrd;
static volatile bool x11_thrd_run = false;

// windows thread proc,
static void *thread_start_routine(void *arg)
{
  win_thread_proc thrd_proc = (win_thread_proc)arg;
  thrd_proc();
  x11_thrd_run = false;
  return NULL;
}

void win_create_thread(win_thread_proc thrd_proc)
{
  W_ASSERT(!x11_thrd_run);
  sleep_ms(20);                                  // not sure is needed, ensure some delayed task end on current thread.
  scr_msg_flush();                               // process current thread messages
  if (!x11_thrd_run)
    x11_thrd_run = pthread_create(&x11_thrd, NULL, thread_start_routine, thrd_proc) == 0;
}

void win_exit_thread(void)                       // thread canceled
{
  W_ASSERT(x11_thrd_run);
  x11_thrd_run = false;
  pthread_exit(NULL);
}

// wait for thread exit
void win_wait_thread_end(void)
{
  if (x11_thrd_run)
    pthread_join(x11_thrd, NULL);
}
