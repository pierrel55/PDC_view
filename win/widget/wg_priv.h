#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

// widget flags
#define WG_SHOW    (1 << 0)                      // display
#define WG_ENABLE  (1 << 1)                      // enable
#define WG_ENFOCUS (1 << 2)                      // can get focus
#define WG_FOCUS   (1 << 3)                      // have focus
#define WG_PRESSED (1 << 4)                      // button pressed
#define WG_CENTER  (1 << 5)                      // center drawing content
#define WG_ILEFT   (1 << 6)                      // insert at left
#define WG_IRIGHT  (1 << 7)                      // insert at right
#define WG_IUP     (1 << 8)                      // insert at up
#define WG_IDOWN   (1 << 9)                      // insert at bottom
#define WG_TOGGLE  (1 << 10)                     // toggle button mode (checkbox)
#define WG_CHECKED (1 << 11)                     // checkbox button state
#define WG_CFIXED  (0 << 12) // (disabled)       // client size cannot be resized (checkbox, radio button)
#define WG_SHIFT   (1 << 13)                     // (buttons) decal drawind position when pressed
#define WG_LMARGIN (1 << 14)                     // text left margin, (left aligned) add TEXT_LMARGIN_PIXELS value
#define WG_RALIGN  (1 << 15)                     // text right align, no margin
#define WG_RMARGIN (1 << 16)                     // text right align + margin, add TEXT_LMARGIN_PIXELS value
#define WG_PTRIG   (1 << 17)                     // triggered on EV_LBUTTONDOWN
#define WG_VSB     (1 << 18)                     // vertical scrool bar up button (root)
#define WG_VSB_S   (1 << 19)                     // vertical scrool bar slider
#define WG_VSB_D   (1 << 20)                     // vertical scrool bar up button
#define WG_CFRAME  (1 << 21)                     // client area is frame title text only
#define WG_EVIWG   (0 << 22) // (disabled)       // transfert event to i_wg even if disabled
#define WG_ENSAV   (1 << 23)                     // backup of WG_ENABLE
#define WG_ICENTER (1 << 24)                     // insert size is half of parent (spin box)
#define WG_IGFOCUS (1 << 25)                     // ignore event if windows gain focus before event

#define TEXT_LMARGIN_PIXELS 4                    // size of text left margin

// declare casted type + check type
#define GET_WG(name, typ) wg_##typ##_t *name = (wg_##typ##_t *)wg;\
        if (name->wg.ev_proc != wg_##typ##_ev_proc) { W_ASSERT(0); return; }

// declare casted type + check type, return ret if type invalid
#define GET_WG_RET(name, typ, ret) wg_##typ##_t *name = (wg_##typ##_t *)wg;\
        if (name->wg.ev_proc != wg_##typ##_ev_proc) { W_ASSERT(0); return ret; }

// ----------------------------------------------
// init functions

extern widget_t wg_void;

// add widget in window widget list
widget_t *win_add_widget(hwin_t *hw, widget_t *wg);

// init widget
widget_t *wg_init(widget_t *wg,
                  wg_ev_proc_t ev_proc,
                  int flags,
                  frm_t *e_frm);                 // external frame to draw, NULL if no frame

// composed widgets. insert sub widget (i_wg) in widget
widget_t *wg_add_child(widget_t *wg, widget_t *i_wg, int i_flags);

// ----------------------------------------------
// drawing functions

extern const uint32_t ch_ck_9[];                 // char used to draw checkbox
extern const uint32_t ch_ar_rad_bt_tl0[];        // char used to draw radio button

// clear/redraw a widget
void wg_draw_blit(hwin_t *hw, widget_t *wg, bool clear_bk, bool draw, bool blit);

// draw text
void draw_wg_text(hwin_t *hw, widget_t *wg, const text_t *t, int w_max);

// draw a char (control button)
void draw_wg_char(hwin_t *hw, widget_t *wg, const uint32_t *ch, int x, int y);

// draw external frame (single state)
void wg_draw_e_frame(hwin_t *hw, widget_t *wg);

// draw client frame
void wg_draw_c_frame(hwin_t *hw, widget_t *wg);

// draw check box
void wg_draw_check_box(hwin_t *hw, widget_t *wg, int x, int y);

// draw radio button
void wg_draw_radio_button(hwin_t *hw, widget_t *wg, int x, int y);

// init drawing for base object: text/control button/bitmap
void init_dr_obj(widget_t *wg,
                 enum e_dr_obj_typ obj_type,     // type of object to draw
                 const void *obj,                // pointer to object to draw
                 pix_t bk_color,                 // color to set if enable
                 c_frm_t *c_frm);                // object client frame, set NULL if no frame

// draw base object
void wg_draw_obj(hwin_t *hw, widget_t *wg);

// ----------------------------------------------
// text util

void text_init(text_t *t, const char *str, const font_t *font, pix_t color, pix_t aa_color);
void text_update(text_t *t, const char *str);
int text_draw(text_t *t, bitmap_t *bm, int w_max);

// ----------------------------------------------
// generic button widget

// special chars for buttons
struct wg_button_char_t
{
  const uint32_t *ar_u7, *ar_d7, *ar_l7, *ar_r7,   // arrow size 7
                 *ar_u5, *ar_d5, *ar_l5, *ar_r5;   // arrow size 5
};

// arrow chars
extern const struct wg_button_char_t wg_button_char;

// base widget
typedef struct
{
  widget_t wg;
  text_t text;
  int prev_flags;                                // previous state
  button_pressed_cb_t pressed_cb;
} wg_button_t;

void wg_button_ev_proc(hwin_t *hw, widget_t *wg);
widget_t *get_wg_ctrl_button(const uint32_t *ch_ar, int wg_flags, button_pressed_cb_t pressed_cb);

void wg_change_focus(hwin_t *hw, widget_t *wg);  // focus change

// non recursive enable (i_wg)
void wg_enable_nr(widget_t *wg, bool enable, bool draw_blit);
