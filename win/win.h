#include <stdint.h>
#include <stdbool.h>
#include "debug.h"

#ifndef NULL
#define NULL (void *)0
#endif

// ----------------------------------------------
// data types

// 2d vector
typedef struct
{
  int x;
  int y;
} vec2i;

// return true if v position is in area defined by pos and size
bool vec_select(const vec2i *mc, const vec2i *pos, const vec2i *size);

// ensure v values between min/max (inclusive)
void vec_clip_min_max(vec2i *v, const vec2i *min, const vec2i *max);

// adjust coordinates range into size, return false if completly clipped.
bool vec_clip_rect(vec2i *size, int *x, int *y, int *dx, int *dy);

// ----------------------------------------------
// bitmap

// define pixel color
#define COL_RGB(r,g,b) (uint32_t)((r << 16) | (g << 8) | b)
#define COL_R(col) ((col >> 16) & 0xff)
#define COL_G(col) ((col >> 8) & 0xff)
#define COL_B(col) (col & 0xff)

typedef uint32_t pix_t;                          // pixel type

// bitmap
typedef struct
{
  vec2i size;                                    // true or logical size
  int l_size;                                    // line size
  pix_t *pix_ptr;                                // pointer to 1st pixel
} bitmap_t;

// image resource
typedef struct
{
  vec2i img_size;
  int idx_len;
  unsigned char dat[];
} res_img8c_t;

// ------------------------
// bitmap drawing functions

// address of a pixel in bitmap (no clip check)
#define BM_PIX_ADDR(bm, x, y) (bm->pix_ptr + y*bm->l_size + x)

void bm_init(bitmap_t *bm, int dx, int dy, int line_dx, pix_t *pixel_mem);
void bm_init_child(bitmap_t *child, const bitmap_t *parent, int x, int y, int dx, int dy);
pix_t *bm_get_pix_addr(bitmap_t *bm, int x, int y);
void bm_put_pixel(bitmap_t *bm, int x, int y, pix_t color);
void bm_draw_line_h(bitmap_t *bm, int x, int y, int len, pix_t color);
void bm_draw_line_v(bitmap_t *bm, int x, int y, int len, pix_t color);
void bm_draw_rect_width(bitmap_t *bm, int x, int y, int dx, int dy, int width, pix_t color);
void bm_draw_rect_shadow(bitmap_t *bm, int x, int y, int dx, int dy, pix_t col_tl, pix_t col_br);
void bm_draw_rect_shadow_width(bitmap_t *bm, int x, int y, int dx, int dy, int width, const pix_t *col_tl, const pix_t *col_br);
void bm_paint(bitmap_t *bm, pix_t color);
void bm_paint_rect(bitmap_t *bm, int x, int y, int dx, int dy, pix_t color);
void bm_paint_rect_c2(bitmap_t *bm, int x, int y, int dx, int dy, pix_t c0, pix_t c1);
void bm_paint_rect_clone_color_no_clip(bitmap_t *bm, int x, int y, int dx, int dy, pix_t *col);
void bm_copy_img(bitmap_t *bm, int x, int y, const bitmap_t *img);
void bm_rect_scroll_up(bitmap_t *bm, vec2i *pos, int size_x, int sc_dy, int clr_dy, pix_t clr_color);
bool bm_init_from_res(bitmap_t *bm, const res_img8c_t *res);

// ------------------------
// font functions

// list of fonts
enum e_font_id
{
  fnt_fbold7 = 0,                                // fixed size bold width = 7  (linux console)
  fnt_fbold8,                                    // fixed size bold width = 8  (xp fixedsys)
  fnt_fnorm7,                                    // fixed size width = 7
  fnt_vthm8,                                     // variable size tahoma 8 (xp tahoma 8)
  fnt_vthm9,                                     // variable size tahoma 9 (xp tahoma 9)
  fnt_vnorm,                                     // variable size (linux)
};

typedef struct
{
  int dy;                                        // ysize of font
  int dx;                                        // xsize of font if fixed, else 0
  const char *extend;                            // extended ascii char list (code > 127)
  uint32_t *bit_mask;                            // char width + bitmask for each char
} font_t;

extern font_t *win_font_list[fnt_vnorm+1];

void font_init_aa(void);                         // init anti aliasing
pix_t font_get_aa_color(pix_t color, pix_t bk_color, int intensity);
pix_t font_gen_bold(const font_t *fnt, pix_t aa_color, font_t *d_fnt, int w_inc);

int font_get_char_width(char c, const font_t *fnt);
int font_string_truncate(char *str, const font_t *fnt, int dx_max);
int font_get_string_width(const char *str, const font_t *fnt);
int font_get_string_width_mc(const char *str, const font_t *fnt, int max_chars);
int font_get_str_click_ofs(const char *str, const font_t *fnt, int dx, int *str_ofs);

void font_eval_text_rect(const char *text, const font_t *f, int line_dy, vec2i *size);
void font_draw_text_rect(bitmap_t *bm, const char *text, const font_t *f, pix_t t_color, pix_t aa_color,
                         const vec2i *pos, int line_dy, const vec2i *max_size);

int bm_draw_raw_char(bitmap_t *bm, int x, int y, pix_t color, pix_t aa_color, const uint32_t *cpix, int dy);
int bm_draw_char(bitmap_t *bm, int x, int y, char c, pix_t color, pix_t aa_color, const font_t *fnt);
int bm_draw_string(bitmap_t *bm, int x, int y, const char *str, pix_t color, pix_t aa_color, const font_t *fnt, int dx_max);
int bm_draw_string_truncate(bitmap_t *bm, int x, int y, const char *str, pix_t color, pix_t aa_color, const font_t *fnt, int dx_max);
int bm_draw_string_bold(bitmap_t *bm, int x, int y, const char *str, pix_t color, pix_t aa_color, const font_t *fnt, int dx_max);

// ----------------------------------------------
// mouse

// cursors
enum e_mouse_cursor
{
  MC_ARROW = 0,                                  // base arrow
  MC_SIZEX,                                      // size horizontal
  MC_SIZEY,                                      // size vertical
  MC_SIZE_TL,                                    // x/y top left
  MC_SIZE_BR,                                    // x/y bottom right
  MC_SIZE_BL,                                    // x/y bottom left
  MC_SIZE_TR,                                    // x/y top right
  MC_SIZEALL,                                    // all directions
  MC_TEXT,                                       // text selection cursor
};

// ----------------------------------------------
// keyboard

// keys codes
enum
{
  KEY_RETURN  = 0x0d,
  KEY_LEFT    = 0x25,
  KEY_RIGHT   = 0x27,
  KEY_UP      = 0x26,
  KEY_DOWN    = 0x28,
  KEY_PGUP    = 0x21,
  KEY_PGDOWN  = 0x22,
  KEY_ESCAPE  = 0x1B,
  KEY_BACK    = 0x08,
  KEY_SHIFT   = 0x10,
  KEY_INS     = 0x2d,
  KEY_SUPPR1  = 0x2e,
  KEY_END     = 0x23,
  KEY_HOME    = 0x24,
  KEY_TAB     = 0x09,
  KEY_CTRL    = 0x11,
  KEY_ALT     = 0x12,
  KEY_F1      = 0x70,
  KEY_F2      = 0x71,
  KEY_F3      = 0x72,
  KEY_F4      = 0x73,
  KEY_F5      = 0x74,
  KEY_F6      = 0x75,
  KEY_F7      = 0x76,
  KEY_F8      = 0x77,
  KEY_F9      = 0x78,
  KEY_F10     = 0x79,
  KEY_F11     = 0x7A,
  KEY_F12     = 0x7B,
};

struct key_states_t
{
  char key_pressed[256];                         // 1: pressed
};

extern struct key_states_t key_states;

// ----------------------------------------------
// text cursor

struct text_cursor_t
{
  bool show;                                     // show if position initialized
  bool vis;                                      // 1 : visible
  vec2i pos;                                     // position in window
  // configuration
  struct
  {
    int y_size;                                  // height (width = 1)
    int color;
    int bk_color;
  } conf;
};

// ----------------------------------------------
// window events (do not change order)

enum e_win_event
{
  EV_IDLE = 0,                                   // no window message event (full cpu used)

  // mouse messages
  EV_MOUSEMOVE,
  EV_LBUTTONDOWN,
  EV_RBUTTONDOWN,
  EV_MBUTTONDOWN,
  EV_LBUTTONUP,
  EV_RBUTTONUP,
  EV_MBUTTONUP,
  EV_LBUTTONDBLCLK,
  EV_RBUTTONDBLCLK,
  EV_MBUTTONDBLCLK,
  EV_MOUSEWHEEL,

  // keyboard
  EV_CHAR,
  EV_KEYPRESSED,                                 // ascii or named key pressed
  EV_KEYRELEASED,                                // ascii or named key unpressed

  EV_BLINK,                                      // text cursor blink event
  EV_TIMER,                                      // timer event
  EV_2HZ,                                        // 2 Hertz event (always active)
  EV_SETFOCUS,                                   // keyboard focus gained notification
  EV_KILLFOCUS,                                  // keyboard focus lost notification

  EV_PAINT,                                      // paint request (full client area)
  EV_CREATE,                                     // window creation, set hw->ev.err_code if user inits fail
  EV_DESTROY,                                    // window is destroyed

  // window only
  EV_CLOSE,                                      // window close, set hw->ev.close.cancel to cancel
  EV_SIZE,                                       // window sized (result stored in window bitmap size)
  EV_SHOW,                                       // window show notification
  EV_HIDE,                                       // window hide notification
  EV_LEAVE,                                      // cursor leave focus window

  EV_HELP,                                       // mouse move paused (help bubble trig)

  // widget special events
  EV_WG_KILLFOCUS,                               // widget lost focus, maybe not the window
  EV_WG_SIZE,                                    // widget resized
};

// event informations.
struct win_event_t
{
  enum e_win_event type;                         // type of event
  vec2i mouse_pos;                               // EV_MOUSEMOVE   : client relative last position, maybe out of window if cursor captured
  bool gain_focus;                               // window gain focus before event occur (widget behavehour control)
  union                                          // event datas
  {
    int mouse_whell;                             // EV_MOUSEWHEEL  : whell variation + up / - down
    int timer_id;                                // EV_TIMER       : id of timer
    int key_char;                                // EV_CHAR        : pressed key
    int key_mask;                                // EV_KEYMASK     : system key toggle mask
    int key_pressed;                             // EV_KEYPRESSED  : ascii id
    int key_released;                            // EV_KEYRELEASED : ascii id
    bool close_cancel;                           // EV_CLOSE       : set to true to cancel close
    int exit_code;                               // EV_CREATE or EV_DESTROY : set application exit code != 0 if error
  };
};

// ----------------------------------------------
// window style

enum e_win_style
{
  win_style_base = 0,
  win_style_xp,
};

bool init_win_style(enum e_win_style win_style);
void close_win_style(void);

// ----------------------------------------------
// window api

// client min/max size
#define CLI_MIN_SIZE_X 80                        // minimum windows client size x
#define CLI_MIN_SIZE_Y 32                        // minimum windows client size y

// window show mode
enum e_show_mode
{
  show_normal = 0,                               // normal display
  show_minimized,                                // minimized, title bar visible, client area is null
  show_maximized,                                // window sized to root window, title bar visible
  show_fullscreen,                               // client area sized to root window, title bar not visible
};

// window datas for client (note: is public part of window handle, do not copy, alloc, ..)
typedef struct
{
  struct win_event_t ev;                         // received event
  void *user_ptr;                                // user usage pointer, can be NULL
  bitmap_t cli_bm;                               // client bitmap
  struct widget_t *wg_list;                      // window attached widgets
  struct widget_t *wg_focus;                     // widget having focus
  struct text_cursor_t t_cursor;                 // text cursor
  enum e_show_mode show_mode;                    // current show mode (do not change, use as read only)
} hwin_t;

// window event proc
typedef void (* win_ev_proc_t)(hwin_t *hw);

// ----------------------------------------------
// create application window
hwin_t *win_create(const char *name,
                   const vec2i *win_pos,         // position, if NULL window is centered on screen
                   const vec2i *cli_req_size,    // client requested size, if NULL minimal size is used
                   const vec2i *cli_min_size,    // min client size, if NULL, no min size defined
                   const vec2i *cli_max_size,    // max client size, if NULL, no max size defined
                   win_ev_proc_t ev_proc, hwin_t *parent, void *user_ptr);

// ----------------------------------------------
// create non sizeable popup window with fin border
// and no title bar (for menus).
// must have focus parent.

#define MAX_BORDER_TYPE 5                        // size of e_win_border_type enum list

// type of window border
enum e_win_border_type
{
  win_border_resize = 0,                         // window mouse resize border (default)
  win_border_fixed,                              // window fixed size border
  win_border_pu1,                                // menu popup1 (combo box)
  win_border_pu2,                                // menu popup2 (context right click)
  win_border_none,                               // no border (maximized mode)
};

// return with of border
int win_get_border_width(enum e_win_border_type border_type);

// create non sizeable window without title bar (menus)
hwin_t *win_create_popup(vec2i *win_pos,         // position, cannot be NULL
                         const vec2i *cli_size,  // size, cannot be NULL
                         enum e_win_border_type border_type, // border type
                         win_ev_proc_t ev_proc, hwin_t *parent, void *user_ptr);

// display help bubble popup
void win_help_open(const char *text, vec2i *clip_pos, vec2i *clip_size, hwin_t *parent);
void win_help_suspend(void);                     // temporary suspend activation

// ----------------------------------------------
// window functions

// return window having focus
hwin_t *win_get_winfocus(void);

// return size of screen containing window
const vec2i *win_get_screen_size(void);

// return client origin position on screen
void win_get_cli_pos(hwin_t *hw, vec2i *scr_pos);

void win_set_name(hwin_t *hw, const char *title_bar_name);
bool win_close(hwin_t *hw);                      // close window

void win_blit_all(hwin_t *hw);                   // blit full client area
void win_cli_blit_rect(hwin_t *hw, int x, int y, int dx, int dy); // blit sub client area

void win_set_cursor(hwin_t *hw, enum e_mouse_cursor shape);
void win_show_cursor(hwin_t *hw, bool show);

int win_set_timer(hwin_t *hw, int delay_ms);
void win_kill_timer(hwin_t *hw, int timer_id);

// ----------------------------------------------
// utils functions

// mouse capture
bool win_mouse_captured(void);                   // return true if mouse cursor captured by client area

// window resize in progress
bool win_ctrl_resize(void);                      // return true if window is being resized

// clipboard
void win_clip_copy(hwin_t *hw, const char *str, int len);    // copy string to clipboard
char *win_clip_paste(hwin_t *hw, int *len);                  // return len and string in cliboard

// time
unsigned int get_ctr_ms(void);                   // return millisecond time counter
void sleep_ms(int ms);                           // enter sleep mode for ms milliseconds

// enable EV_IDLE message if no window messages. Uses 100% cpu on main thread
void win_enable_ev_idle(bool enable);

// ----------------------------------------------
// text cursor

void win_init_text_cursor(hwin_t *hw, int y_size, int color, int bk_color);
void win_draw_text_cursor(hwin_t *hw, bool vis);
void win_move_text_cursor(hwin_t *hw, int x, int y);

// ----------------------------------------------
// main window application (winmain)

bool win_app_main(char *cmd_line);

// ----------------------------------------------
// thread (very basic, added for epr application)

typedef void (*win_thread_proc)(void);

void win_create_thread(win_thread_proc thrd_proc);
void win_exit_thread(void);
void win_wait_thread_end(void);

