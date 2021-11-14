// window host for ms windows

#include <windows.h>
#include "screen.h"

// static states
struct screen_t scr = { 0 };
static const char *CFG_class_name = "wm x11";
static HINSTANCE app_hInstance;                  // WinMain() hInstance
static HCURSOR cursor_list[MC_TEXT+1];
struct key_states_t key_states = { 0 };

#define GET_WMS(w) (w_ms_t *)(w)

// host ms window
typedef struct w_ms_t
{
  win_t    w;                                    // window
  HWND     hWnd;                                 // ms window handle
  HDC      hDCWin;                               // window device context w (for BitBlt)
  HDC      hDCMem;                               // memory device context w (for BitBlt)
  HBITMAP  hBitmap;                              // bitmap used to blit (for BitBlt)
} w_ms_t;

static struct
{
  w_ms_t *w_ms;
  int timer_id;
} w_root = { 0 };

// return size to alloc for host window
int scr_win_sizeof(void)
{
  return sizeof(w_ms_t);
}

// create window bitmap
static void resize_bitmap(w_ms_t *w_ms, int dx, int dy)
{
  pix_t *pix_ptr;
  struct
  {
    BITMAPINFOHEADER bmiHeader;
    unsigned long ColorMask[3];
  } bi = {0};
  
  if (w_ms->hBitmap)                             // delete old bitmap
    DeleteObject(w_ms->hBitmap);

  bi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth         = dx;
  bi.bmiHeader.biHeight        = -dy;
  bi.bmiHeader.biPlanes        = 1;
  bi.bmiHeader.biBitCount      = 32;
  bi.bmiHeader.biCompression   = BI_RGB;
  bi.bmiHeader.biSizeImage     = dx*dy*4;
  // RGB 888
  bi.ColorMask[0]              = 0xFF0000;
  bi.ColorMask[1]              = 0x00FF00;
  bi.ColorMask[2]              = 0x0000FF;

  w_ms->hBitmap = CreateDIBSection(w_ms->hDCWin, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void **)&pix_ptr, NULL, 0);
  if (w_ms->hBitmap)
    bm_init(&w_ms->w.win_bm, dx, dy, dx, pix_ptr);
  else
    bm_init(&w_ms->w.win_bm, 0, 0, 0, NULL);
}

// destroy window and free used resources
void scr_win_destroy(win_t *w)
{
  w_ms_t *w_ms = GET_WMS(w);
  if (w_ms == w_root.w_ms)
    KillTimer(w_root.w_ms->hWnd, w_root.timer_id);
  if (w_ms->hBitmap)
    DeleteObject(w_ms->hBitmap);
  if (w_ms->hDCMem)
    ReleaseDC(w_ms->hWnd, w_ms->hDCMem);
  if (w_ms->hDCWin)
    ReleaseDC(w_ms->hWnd, w_ms->hDCWin);
  DestroyWindow(w_ms->hWnd);
  w_ms->hWnd = NULL;
}

// create hidden window
bool scr_win_create(win_t *w, const char *name)
{
  w_ms_t *w_ms = GET_WMS(w);
  w_ms_t *parent = (w_ms_t *)(w->next);

  // create host window
  w_ms->hWnd = CreateWindow(
     CFG_class_name,
     name,                                       // not displayed, but set for task manager
     WS_OVERLAPPED,                              // no decorations, hidden
     0, 0,                                       // use create sizes, move/resize is mandatory
     32, 32,
     parent ? parent->hWnd : NULL,
     NULL,
     app_hInstance,
     NULL); 

  if (w_ms->hWnd)
  {
    SetWindowLong(w_ms->hWnd, GWL_STYLE, 0);     // window with no border, no menu bar
    w_ms->hDCWin = GetDC(w_ms->hWnd);
    if (w_ms->hDCWin)
    {
      w_ms->hDCMem = CreateCompatibleDC(w_ms->hDCWin);
      if (w_ms->hDCMem)
        return true;
    }
    scr_win_destroy(w);
  }
  return false;
}

// show/hide
void scr_win_show(win_t *w, bool show)
{
  w_ms_t *w_ms = GET_WMS(w);
  int flg; 
  if (show)
    flg = SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW;
  else
    flg = SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW;
  SetWindowPos(w_ms->hWnd, NULL, 0, 0, 0, 0, flg);
}

// resize window
void scr_win_move_resize(win_t *w, int x, int y, int dx, int dy)
{
  w_ms_t *w_ms = GET_WMS(w);
  resize_bitmap(w_ms, dx, dy);
  if (SetWindowPos(w_ms->hWnd, NULL, x, y, dx, dy, SWP_NOCOPYBITS|SWP_NOSENDCHANGING))
  {
    w_ms->w.win_pos.x = x;
    w_ms->w.win_pos.y = y;
  }
}

// move window
void scr_win_move(win_t *w, int x, int y)
{
  w_ms_t *w_ms = GET_WMS(w);
  if (SetWindowPos(w_ms->hWnd, NULL, x, y, 0, 0, SWP_NOSENDCHANGING|SWP_NOSIZE))
  {
    w_ms->w.win_pos.x = x;
    w_ms->w.win_pos.y = y;
  }
}

// blit window area
void scr_win_blt_all(win_t *w)
{
  w_ms_t *w_ms = GET_WMS(w);
  if (w_ms->hBitmap && SelectObject(w_ms->hDCMem, w_ms->hBitmap))
    BitBlt(w_ms->hDCWin, 0, 0, w_ms->w.win_bm.size.x, w_ms->w.win_bm.size.y, w_ms->hDCMem, 0, 0, SRCCOPY);
}

// blit sub window area
void scr_win_blt_rect(win_t *w, int x, int y, int dx, int dy)
{
  w_ms_t *w_ms = GET_WMS(w);
  if (w_ms->hBitmap && SelectObject(w_ms->hDCMem, w_ms->hBitmap))
    BitBlt(w_ms->hDCWin, x, y, dx, dy, w_ms->hDCMem, x, y, SRCCOPY);
}

// return size of scr containing mouse cursor (multi scr)
static bool get_screen_size(vec2i *size)
{
  POINT pt;
  HMONITOR hMonitor;
  GetCursorPos(&pt);                             // get mouse position

  hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  if (hMonitor)
  {
    MONITORINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &mi))
    {
      size->x = mi.rcWork.right - mi.rcWork.left;
      size->y = mi.rcWork.bottom - mi.rcWork.top;
      return true;
    }
  }
  return false;
}

// set the mouse cursor shape
void scr_set_cursor(enum e_mouse_cursor mc)
{
  W_ASSERT((unsigned int)mc <= MC_TEXT);
  if ((unsigned int)mc <= MC_TEXT)
    SetCursor(cursor_list[mc]);
  scr.last_cursor = mc;
}

// return the mouse position in scr
void scr_get_cursor_pos(vec2i *pos)
{
  POINT p;
  GetCursorPos(&p);
  pos->x = p.x;
  pos->y = p.y;
}

// init timer message
int scr_set_timer(win_t *w, int delay_ms)
{
  w_ms_t *w_ms = GET_WMS(w);
  return SetTimer(w_ms->hWnd, 1, delay_ms, NULL);
}

void scr_kill_timer(win_t *w, int timer_id)
{
  w_ms_t *w_ms = GET_WMS(w);
  KillTimer(w_ms->hWnd, timer_id);
}

// sleep
void sleep_ms(int ms)
{
  Sleep(ms);
}

// ms time counter
unsigned int get_ctr_ms(void)
{
  return GetTickCount();
}

// enable EV_IDLE message if no window messages.
// Uses 100% cpu on main thread
void win_enable_ev_idle(bool enable)
{
  scr.peek_mode = enable;
}

// ----------------------------------------------
// window messages

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

static LRESULT CALLBACK ms_win_msg_proc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  w_ms_t *w_ms;
  win_t *w;
  RECT r;

  if (Message == WM_NCHITTEST)             // frequent message, result is always client
    return HTCLIENT;
  if (Message == WM_SETCURSOR)
    return TRUE;                           // controlled by code

  // find user window using hWnd
  w_ms = (w_ms_t *)scr.win_top;
  while (1)
  {
    if (!w_ms)
      goto no_w_ms;
    if (w_ms->hWnd == hWnd)
      break;
    w_ms = (w_ms_t *)w_ms->w.next;
  };

  w = &w_ms->w;

  // frequent case, avoid switch
  if (Message == WM_MOUSEMOVE)
  {
    w->ev.mouse_pos.x = LOWORD(lParam);
    w->ev.mouse_pos.y = HIWORD(lParam);
    w->ev.type = EV_MOUSEMOVE;
    if (!scr.tmr.dis_time_ctr)
    {
      scr.tmr.mouse_time_ctr = 0;
      scr.tmr.mouse_w = w;
    }
    app_mouse_move(w);
    return 0;
  }

  // other mouse events
  if ((Message >= WM_LBUTTONDOWN) && (Message <= WM_MOUSEWHEEL))
  {
    w->ev.mouse_pos.x = LOWORD(lParam);
    w->ev.mouse_pos.y = HIWORD(lParam);
    switch (Message)
    {
      case WM_LBUTTONDOWN:   w->ev.type = EV_LBUTTONDOWN; SetCapture(hWnd); app_mouse_set_capture(w); break;
      case WM_RBUTTONDOWN:   w->ev.type = EV_RBUTTONDOWN; SetCapture(hWnd); app_mouse_set_capture(w); break;
      case WM_MBUTTONDOWN:   w->ev.type = EV_MBUTTONDOWN; SetCapture(hWnd); app_mouse_set_capture(w); break;
      case WM_LBUTTONUP:     w->ev.type = EV_LBUTTONUP;   ReleaseCapture(); app_mouse_release_capture(w); break;
      case WM_RBUTTONUP:     w->ev.type = EV_RBUTTONUP;   ReleaseCapture(); app_mouse_release_capture(w); break;
      case WM_MBUTTONUP:     w->ev.type = EV_MBUTTONUP;   ReleaseCapture(); app_mouse_release_capture(w); break;
      case WM_LBUTTONDBLCLK: w->ev.type = EV_LBUTTONDBLCLK; app_mouse_event_other(w); break;
      case WM_RBUTTONDBLCLK: w->ev.type = EV_RBUTTONDBLCLK; app_mouse_event_other(w); break;
      case WM_MBUTTONDBLCLK: w->ev.type = EV_MBUTTONDBLCLK; app_mouse_event_other(w); break;
      case WM_MOUSEWHEEL:    w->ev.type = EV_MOUSEWHEEL; w->ev.mouse_whell = (int)wParam; app_mouse_event_other(w); break;
    };
    return 0;
  }

  switch (Message)
  {
    case WM_TIMER:
      W_ASSERT(w_ms == w_root.w_ms);
      app_tmr_update(w);                         // event timer, manage cursor blink and tooltip
#if 0
      // reserved for futur use, epr app do not use timer (and not coded on X11)
      w->ev.type = EV_TIMER;
      w->ev.timer_id = (int)wParam;
      w->ev_proc((hwin_t *)w);
#endif
    break;
    case WM_SETCURSOR:
      return TRUE;                               // cursor set done by win.c
    break;
    case WM_CHAR:
      w->ev.type = EV_CHAR;
      w->ev.key_char = wParam;
      w->ev_proc((hwin_t *)w);
      return 0;
    break;
    case WM_SYSCHAR:
      return 0;
    break;
    case WM_KEYDOWN:
      scr.tmr.dis_time_ctr = HELP_DIS_CNT;
      scr.tmr.mouse_time_ctr = HELP_TRG_CNT;     // disable help bubble until mouse mouved
      w->ev.key_pressed = wParam & 0xff;
      key_states.key_pressed[w->ev.key_pressed] = 1;
      w->ev.type = EV_KEYPRESSED;
      w->ev_proc((hwin_t *)w);
      return 0;
    break;
    case WM_KEYUP:
      w->ev.key_released = wParam & 0xff;
      key_states.key_pressed[w->ev.key_released] = 0;
      w->ev.type = EV_KEYRELEASED;
      w->ev_proc((hwin_t *)w);
      return 0;
    break;
    case WM_ACTIVATEAPP:
      W_ASSERT(w->win_bm.pix_ptr);
      app_activate(wParam);
    break;
    case WM_WINDOWPOSCHANGING:                   // cancel owner z moves
      ((WINDOWPOS *)lParam)->flags |= SWP_NOOWNERZORDER;
    break;
    case WM_CLOSE:                               // external close (menu)
      if (!win_close((hwin_t *)w))
        return 0;
    break;
    case WM_DESTROY:
    break;
    case WM_PAINT:
      scr.sizing = false;
      if (GetUpdateRect(w_ms->hWnd, &r, false))
      {
        if (w_ms->hBitmap && SelectObject(w_ms->hDCMem, w_ms->hBitmap))
          BitBlt(w_ms->hDCWin, r.left, r.top, r.right - r.left, r.bottom - r.top, w_ms->hDCMem, r.left, r.top, SRCCOPY);
        ValidateRect(w_ms->hWnd, &r);            // validate to stop WM_PAINT messages production
        return 0;                                // do not call DefWindowProc
      }
    break;
    case WM_QUERYENDSESSION:                     // computer off or reboot
      // if (!app_close_query())                    // close from taskbar menu or other
      //   return 0;                                // return 0 to cancel
    break;
    case WM_ENDSESSION:                          // computer off/reboot
      // nothing to save
    break;
  }

no_w_ms:
  if (Message == WM_ERASEBKGND)
    return 1;                                    // erase not needed
  return DefWindowProc(hWnd, Message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  WNDCLASS wClass;
  app_hInstance = hInstance;

  if (!get_screen_size(&scr.size))
    return -1;

  // window manager specific
  if (!init_win_style(win_style_xp))
    return -1;

  // init mouse cursors
  cursor_list[MC_ARROW]    = LoadCursor(NULL, IDC_ARROW);
  cursor_list[MC_SIZEX]    = LoadCursor(NULL, IDC_SIZEWE);
  cursor_list[MC_SIZEY]    = LoadCursor(NULL, IDC_SIZENS);
  cursor_list[MC_SIZE_TL]  = LoadCursor(NULL, IDC_SIZENWSE);
  cursor_list[MC_SIZE_BR]  = cursor_list[MC_SIZE_TL];
  cursor_list[MC_SIZE_BL]  = LoadCursor(NULL, IDC_SIZENESW);
  cursor_list[MC_SIZE_TR]  = cursor_list[MC_SIZE_BL];
  cursor_list[MC_SIZEALL]  = LoadCursor(NULL, IDC_SIZEALL);
  cursor_list[MC_TEXT]     = LoadCursor(NULL, IDC_IBEAM);

  wClass.style         = CS_DBLCLKS;
  wClass.lpfnWndProc   = ms_win_msg_proc;
  wClass.cbClsExtra    = 0;
  wClass.cbWndExtra    = 0;
  wClass.hInstance     = hInstance;
  wClass.hIcon         = NULL;                   // LoadIcon(win_st.hInstance, MAKEINTRESOURCE(IDI_ICON1));
  wClass.hCursor       = NULL;
  wClass.hbrBackground = NULL;
  wClass.lpszMenuName  = NULL;
  wClass.lpszClassName = CFG_class_name;

  if (!RegisterClass(&wClass))
    return -1;

  if (win_app_main(lpCmdLine))                   // todo: extract argc/argv
  {
    if (scr.win_top)                             // root window created
    {
      W_ASSERT(!scr.win_top->next);              // single window created in win_app_main
      w_root.w_ms = (w_ms_t *)(scr.win_top);
      w_root.timer_id = SetTimer(w_root.w_ms->hWnd, 1, SCR_TIMER_PERIOD, NULL);
      if (w_root.timer_id)
      {
        while (scr.win_top)
        {
          MSG msg;
          if (scr.peek_mode)
          {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
              DispatchMessage(&msg);
            else
            {
              win_t *w = scr.win_top;
              w->ev.type = EV_IDLE;
              w->ev_proc((hwin_t *)w);
            }
          }
          else
          {
            GetMessage(&msg, NULL, 0, 0);
            TranslateMessage(&msg);              //  produces WM_CHAR messages
            DispatchMessage(&msg);
          }
        }
      }
    }
  }
  UnregisterClass(CFG_class_name, hInstance);
#ifdef _DEBUG
  w_check_alloc();
#endif
  return scr.app_exit_code;
}

// flush pending messages
void scr_msg_flush(void)
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    DispatchMessage(&msg);
}

// ----------------------------------------------
// basic clipboard for text only

#define CLIP_MAX_SIZE 1024

static void cb_sys_export(w_ms_t *w_ms, const char *str, int len);
static int cb_sys_import(w_ms_t *w_ms, char *str, int len_max);

static struct
{
  int len;
  char str[CLIP_MAX_SIZE];
} clipboard = { 0 };

void win_clip_copy(hwin_t *hw, const char *str, int len)
{
  if (str && len)
  {
    w_ms_t *w_ms = GET_WMS(hw);
    cb_sys_export(w_ms, str, len);             // export to os
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
  w_ms_t *w_ms = GET_WMS(hw);
  int l = cb_sys_import(w_ms, clipboard.str, sizeof(clipboard.str));
  if (l)
    clipboard.len = l;
  *len = clipboard.len;
  return clipboard.str;
}

// export string to clipboard
static void cb_sys_export(w_ms_t *w_ms, const char *str, int len)
{
  if (w_ms && OpenClipboard(w_ms->hWnd))
  {
    HGLOBAL hglbCopy;
    EmptyClipboard();
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (len+1) * sizeof(TCHAR)); 
    if (hglbCopy) 
    { 
      int i;
      LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
      for (i=0; i<len; i++)
        lptstrCopy[i] = (TCHAR)str[i];
      lptstrCopy[i] = (TCHAR)0;                  // null character 
      GlobalUnlock(hglbCopy); 
      SetClipboardData(CF_TEXT, hglbCopy);
    }
    CloseClipboard(); 
  }
}

// import string from clipboard
static int cb_sys_import(w_ms_t *w_ms, char *str, int len_max)
{
  int l = 0;
  if (w_ms && IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(w_ms->hWnd))
  {
    HGLOBAL hglb = GetClipboardData(CF_TEXT); 
    if (hglb != NULL) 
    { 
      LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
      if (lptstr != NULL) 
      {
        len_max--;
        while (*lptstr && (l < len_max))
        {
          char c = *lptstr++;
          if (c >= ' ')
            str[l++] = c;
          else
          if (c == '\n')
            str[l++] = ' ';
        }
        str[l] = 0;
        GlobalUnlock(hglb);
      }
    }
    CloseClipboard();
  }
  return l;
}

// ----------------------------------------------
// thread
// very basic support for one extra thread.

static volatile HANDLE h_win_thrd = NULL;

// windows thread proc, 
static DWORD WINAPI ThreadProc(LPVOID lpParam) 
{
  win_thread_proc thrd_proc = (win_thread_proc)lpParam;
  thrd_proc();
  CloseHandle(h_win_thrd);
  h_win_thrd = NULL;
  return 0;
}

void win_create_thread(win_thread_proc thrd_proc)
{
  DWORD thrd_id;
  W_ASSERT(!h_win_thrd);
  sleep_ms(20);                                  // not sure is needed, ensure some delayed task end on current thread.
  scr_msg_flush();                               // process current thread messages
  if (!h_win_thrd)
  {
    h_win_thrd = CreateThread(
    NULL,                                        // default security
    0,                                           // default stack size
    ThreadProc,                                  // name of the thread function
    (void *)thrd_proc,                           // thread parameters
    0,                                           // default startup flags
    &thrd_id);
  }
}

void win_exit_thread(void)                       // thread canceled
{
  W_ASSERT(h_win_thrd);
  CloseHandle(h_win_thrd);
  h_win_thrd = NULL;
  ExitThread(0);
}

// wait for thread exit
void win_wait_thread_end(void)
{
  while (h_win_thrd)                             // pool handle
    sleep_ms(50);
}
