// window private, interface with screen
#include "win.h"

// ----------------------------------------------
// window frame and title bar

// title bar string max len
#define TITLE_BAR_MAX_TITLE_LEN 256

// help trig
#define SCR_TIMER_PERIOD 400
#define HELP_DIS_CNT 2                           // disable retrig period count (in SCR_TIMER_PERIOD units)
#define HELP_TRG_CNT 3                           // mouse no move trig period count

// button unpressed/pressed bitmaps
struct bt_img_t
{
  bitmap_t bm_u;                                 // unpressed bitmap
  bitmap_t bm_p;                                 // pressed bitmap
};

// title bar button
struct button_t
{
  int pos_r;                                     // x position in bitmap (relative to right border)
  struct bt_img_t *img;
};

// title bar
struct title_bar_t
{
  int size_y;                                    // y size
  int bt_count;                                  // count of buttons
  int bt_pos_y;                                  // y position of buttons
  struct button_t bt_list[3];                    // list of buttons
  char title_str[TITLE_BAR_MAX_TITLE_LEN];       // title bar string, window name
};

// ----------------------------------------------
// window

// window control flags
#define WF_CTRL_FOCUS        (1 << 8)            // have focus
#define WF_CTRL_POPUP        (1 << 9)            // is popup
#define WF_CTRL_HELP         (1 << 10)           // is help popup

typedef struct win_t
{
  hwin_t;
  int flags;
  enum e_win_border_type border_type;
  vec2i win_pos;
  struct
  {
    int flags;
    enum e_win_border_type border_type;
    vec2i win_pos;
    vec2i win_size;
  } restore;                                     // restore parameters if resized

  bitmap_t win_bm;                               // full window bitmap, contain frame
  vec2i cli_ofs;                                 // offset of client area in window
  vec2i min_size;                                // resize min size
  vec2i max_size;                                // resize max size
  win_ev_proc_t ev_proc;                         // event proc
  struct title_bar_t title_bar;                  // the title bar
  struct win_t *next;                            // chained list next
} win_t;

// interface screen/win (code in win.c)
void app_mouse_move(win_t *w);
void app_mouse_set_capture(win_t *w);
void app_mouse_release_capture(win_t *w);
void app_mouse_event_other(win_t *w);
void app_tmr_update(win_t *w);

bool app_close_query(void);
void app_activate(bool active);

// ----------------------------------------------
// screen driver, ms windows, x11

// current mouse capture control on window
enum e_win_ctrl
{
  e_win_ctrl_none = 0,
  e_win_ctrl_moving,
  e_win_ctrl_sizing,
  e_win_ctrl_button,
  e_win_ctrl_client,
};

struct screen_t
{
  vec2i size;                                    // screen size
  // ------------------------
  // windows list
  win_t *win_top;                                // screen window list

  // ------------------------
  // window control states
  enum e_win_ctrl e_ctrl;                        // current control type
  // move/resize control
  vec2i mouse_ofs;                               // mouse click origin offset for control
  vec2i pos_ini;                                 // initial select position for window resize
  vec2i size_ini;                                // initial select size for window resize
  int border_sel_msk;                            // selected border mask for mouse resize
  bool sizing;                                   // current resize not completed
  bool notify_leave;                             // client EV_LEAVE emulation
  // frame buttons controls
  bool bt_pressed;                               // state of button
  struct button_t *bt_ctrl;                      // controlled button
  vec2i bt_pos;                                  // position of button
  enum e_mouse_cursor last_cursor;               // last defined cursor
  // process control
  bool peek_mode;                                // send EV_IDLE
  int app_exit_code;                             // exit code
  vec2i help_clip_pos;                           // allowed mouse position in parent
  vec2i help_clip_size;
  // timer control
  bool app_active;
  struct
  {
    win_t *mouse_w;
    int dis_time_ctr;
    int mouse_time_ctr;
    vec2i clip_pos;
    vec2i clip_size;
  } tmr;
};

// screen object
extern struct screen_t scr;

// ------------------------
// screen functions

int scr_win_sizeof(void);
bool scr_win_create(win_t *w, const char *name);
void scr_win_destroy(win_t *w);
void scr_msg_flush(void);
void scr_win_show(win_t *w, bool show);
void scr_win_move_resize(win_t *w, int x, int y, int dx, int dy);
void scr_win_move(win_t *w, int x, int y);
void scr_win_blt_all(win_t *w);
void scr_win_blt_rect(win_t *w, int x, int y, int dx, int dy);
// mouse
void scr_set_cursor(enum e_mouse_cursor mc);
void scr_get_cursor_pos(vec2i *pos);
// timer
int scr_set_timer(win_t *w, int delay_ms);
void scr_kill_timer(win_t *w, int timer_id);
