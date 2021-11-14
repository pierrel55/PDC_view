#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "widget.h"
#include "wg_priv.h"

#if _DEBUG
const char *guiXP_version = "widget v1.1 "__DATE__" DEBUG";
#else
const char *guiXP_version = "widget v1.1 "__DATE__;
#endif

// ----------------------------------------------
// window image
// ----------------------------------------------

typedef struct
{
  widget_t wg;
} wg_image_t;

static void wg_image_ev_proc(hwin_t *hw, widget_t *wg)
{
  if (hw->ev.type == EV_PAINT)
    wg_draw_obj(hw, wg);
}

static widget_t *get_wg_image(const bitmap_t *img_bm)
{
  C_NEW(wg_image, wg_image_t);
  if (wg_image)
  {
    init_dr_obj(&wg_image->wg, e_obj_bitmap, img_bm, COL_ND, NULL);
    return wg_init(&wg_image->wg, wg_image_ev_proc, WG_SHOW, NULL);
  }
  return &wg_void;
}

widget_t *wg_init_image(hwin_t *hw, const bitmap_t *img_bm)
{
  return win_add_widget(hw, get_wg_image(img_bm));
}

// ----------------------------------------------
// window text
// ----------------------------------------------

typedef struct
{
  widget_t wg;
  text_t text;
} wg_text_t;

static void wg_text_ev_proc(hwin_t *hw, widget_t *wg)
{
  if (hw->ev.type == EV_PAINT)
    wg_draw_obj(hw, wg);
}

static widget_t *get_wg_text_ex(const char *str, const font_t *font, pix_t color, pix_t aa_color)
{
  C_NEW(wg_text, wg_text_t);
  if (wg_text)
  {
    text_init(&wg_text->text, str, font, color, aa_color);
    init_dr_obj(&wg_text->wg, e_obj_text, &wg_text->text, COL_ND, NULL);
    return wg_init(&wg_text->wg, wg_text_ev_proc, WG_SHOW|WG_ENABLE|WG_CFIXED, NULL);
  }
  return &wg_void;
}

static widget_t *get_wg_text(const char *str)
{
  return get_wg_text_ex(str, win_font_list[wgs.text.text_font_id], wgs.text.text_color, wgs.text.text_aa_color);
}

widget_t *wg_init_text_ex(hwin_t *hw, const char *str, const font_t *font, pix_t color, pix_t aa_color)
{
  return win_add_widget(hw, get_wg_text_ex(str, font, color, aa_color));
}

widget_t *wg_init_text(hwin_t *hw, const char *str)
{
  return win_add_widget(hw, get_wg_text(str));
}

void wg_text_update(hwin_t *hw, widget_t *wg, const char *str)
{
  GET_WG(txt, text);
  text_update(&txt->text, str);
  if (txt->text.size.x < wg->c_size.x)
  {
    wg_draw_blit(hw, wg, true, true, true);      // erase old end + draw new
    wg->c_size.x = txt->text.size.x;             // update size
  }
  else
  {
    wg->c_size.x = (txt->text.size.x < wg->e_size.x) ? txt->text.size.x : wg->e_size.x;
    wg_draw_blit(hw, wg, true, true, true);      // draw new
  }
}

void wg_text_update_ex(hwin_t *hw, widget_t *wg, font_t *font, pix_t color, pix_t aa_color, const char *str)
{
  GET_WG(txt, text);
  txt->text.color = color;
  txt->text.aa_color = aa_color;
  if (font)
    txt->text.font = font;
  wg_text_update(hw, wg, str);
}

// ----------------------------------------------
// frame with title
// ----------------------------------------------

// draw a rec with empty region to display name
static void bm_draw_rect_named_frame(bitmap_t *bm, int x, int y, int dx, int dy, int n_x, int n_dx, pix_t col_tl, pix_t col_br)
{
  bm_draw_line_h(bm, x, y, n_x-2, col_tl);       // top
  bm_draw_line_h(bm, x+n_x+n_dx+1, y, dx-n_x-n_dx-2, col_tl);
  bm_draw_line_v(bm, x, y+1, dy-2, col_tl);      // left
  bm_draw_line_h(bm, x, y+dy-1, dx, col_br);     // bottom
  bm_draw_line_v(bm, x+dx-1, y, dy-1, col_br);   // right
}

static void bm_draw_rect_named_frame_width(bitmap_t *bm, int x, int y, int dx, int dy, int n_x, int n_dx, int width, const pix_t *tl_color, const pix_t *br_color)
{
  while (width--)
  {
    bm_draw_rect_named_frame(bm, x++, y++, dx, dy, n_x--, n_dx, *tl_color++, *br_color++);
    dx-=2;
    dy-=2;
  }
}

typedef struct
{
  widget_t wg;
  text_t text;
} wg_frame_t;

static void wg_frame_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_frame_t *wg_frame = (wg_frame_t *)wg;
  if (hw->ev.type == EV_PAINT)
  {
    int dy = 0;
    int wt = 0;
    if (wg_frame->text.str)
    {
      wg_frame->text.pos = wg->c_pos;
      wt = wg_frame->text.size.x + 2;
      draw_wg_text(hw, wg, &wg_frame->text, wg->e_size.x - 16);
      dy = (wg_frame->text.size.y - wgs.frame.e_frm.width) >> 1;
    }
    bm_draw_rect_named_frame_width(
      &hw->cli_bm,
      wg->e_pos.x, wg->e_pos.y + dy, wg->e_size.x, wg->e_size.y - dy,
      7,                                         // x name offset in frame
      wt,
      wgs.frame.e_frm.width, wgs.frame.e_frm.tl, wgs.frame.e_frm.br);
  }
}

widget_t *wg_init_frame_ex(hwin_t *hw, const char *title, const font_t *font, pix_t color, pix_t aa_color)
{
  C_NEW(wg_frame, wg_frame_t);
  if (wg_frame)
  {
    if (title)
    {
      text_init(&wg_frame->text, title, font, color, aa_color);
      init_dr_obj(&wg_frame->wg, e_obj_text, &wg_frame->text, COL_ND, NULL);
      // set client position/size
      wg_frame->wg.c_pos.x = 8;
    }
    return win_add_widget(hw, wg_init(&wg_frame->wg, wg_frame_ev_proc, WG_SHOW|WG_ENABLE|WG_CFRAME, NULL));
  }
  return &wg_void;
}

widget_t *wg_init_frame(hwin_t *hw, const char *title)
{
  return wg_init_frame_ex(hw, title, win_font_list[wgs.frame.text_font_id], wgs.frame.text_color, wgs.frame.text_aa_color);
}

// ----------------------------------------------
// base buttons types
// ----------------------------------------------

// framed text button
static widget_t *get_wg_text_button(const char *name, const font_t *font, pix_t text_color, pix_t text_aa_color, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    bt->pressed_cb = pressed_cb;
    text_init(&bt->text, name, font, text_color, text_aa_color);
    init_dr_obj(&bt->wg, e_obj_text, &bt->text, COL_ND, &wgs.text_button.c_frm);
    return wg_init(&bt->wg, wg_button_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER, NULL);
  }
  return &wg_void;
}

widget_t *wg_init_text_button(hwin_t *hw, const char *name, button_pressed_cb_t pressed_cb)
{
  return win_add_widget(hw, 
    get_wg_text_button(name, win_font_list[wgs.text_button.text_font_id], wgs.text_button.text_color, wgs.text_button.text_aa_color, pressed_cb));
}

widget_t *wg_init_text_button_fnt(hwin_t *hw, const char *name, const font_t *fnt, button_pressed_cb_t pressed_cb)
{
  return win_add_widget(hw, 
    get_wg_text_button(name, fnt, wgs.text_button.text_color, wgs.text_button.text_aa_color, pressed_cb));
}

void wg_text_button_uptate(hwin_t *hw, widget_t *wg, const char *name)
{
  GET_WG(bt, button);
  if (bt->wg.dr_obj.type == e_obj_text)
  {
    text_update(bt->wg.dr_obj.text, name);
    wg_draw_blit(hw, wg, true, true, true);
  }
}

#if 0 // reserved for future use
// not framed text button (click text)
static widget_t *get_wg_text_click(const char *name, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    bt->pressed_cb = pressed_cb;
    text_init(&bt->text, name, win_font_list[wgs.text_button.text_font_id], wgs.text_button.text_color, wgs.text_button.text_aa_color);
    init_dr_obj(&bt->wg, e_obj_text, &bt->text, wgs.combo_box.text_bk_color, NULL);
    return wg_init(&bt->wg, wg_button_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_CENTER, NULL);
  }
  return &wg_void;
}
#endif

// framed control button
widget_t *get_wg_ctrl_button(const uint32_t *ch_ar, int wg_flags, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    bt->pressed_cb = pressed_cb;
    init_dr_obj(&bt->wg, e_obj_char, ch_ar, COL_ND, &wgs.combo_box.c_frm_bt_down);
    return wg_init(&bt->wg, wg_button_ev_proc, wg_flags, NULL);
  }
  return &wg_void;
}

#if 0 // reserved for future use
// framed bitmap button
static widget_t *get_wg_bitmap_button(const bitmap_t *bm, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    bt->pressed_cb = pressed_cb;
    init_dr_obj(&bt->wg, e_obj_bitmap, bm, COL_ND, &wgs.combo_box.c_frm_bt_down);
    return wg_init(&bt->wg, wg_button_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER, NULL);
  }
  return &wg_void;
}
#endif

// ----------------------------------------------
// checkbox without text
// ----------------------------------------------

#if 0  // reserved for future use
static void wg_ckbox_ev_proc(hwin_t *hw, widget_t *wg)
{
  if (hw->ev.type == EV_PAINT)
    wg_draw_check_box(hw, wg, wg->c_pos.x, wg->c_pos.y);
  else
    wg_button_ev_proc(hw, wg);                       // call button code
}

static widget_t *get_wg_ckbox_button(button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    bt->pressed_cb = pressed_cb;
    bt->wg.c_size.x = ch_ck_9[0] + (wgs.frm_check_box.width << 1);
    bt->wg.c_size.y = bt->wg.c_size.x;
    return wg_init(&bt->wg, wg_ckbox_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_CFIXED, NULL);
  }
  return &wg_void;
}
#endif

// ----------------------------------------------
// checkbox with text
// ----------------------------------------------

static void wg_text_ckbox_ev_proc(hwin_t *hw, widget_t *wg)
{
  if (hw->ev.type == EV_PAINT)
  {
    wg_button_t *bt = (wg_button_t *)wg;
    int w_box = ch_ck_9[0] + (wgs.frm_check_box.width << 1);
    int x = wg->c_pos.x;
    int y = wg->c_pos.y;
    wg_draw_check_box(hw, wg, x, y + ((wg->c_size.y - w_box) >> 1));
    bt->text.pos.x = x + w_box + (w_box >> 1);
    bt->text.pos.y = y + ((wg->c_size.y - bt->text.size.y + 1) >> 1);
    draw_wg_text(hw, wg, &bt->text, wg->c_size.x);
  }
  else
    wg_button_ev_proc(hw, wg);                   // call button code
}

static widget_t *get_wg_text_ckbox(const char *name, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    int w_box = ch_ck_9[0] + (wgs.frm_check_box.width << 1);
    bt->pressed_cb = pressed_cb;
    text_init(&bt->text, name, win_font_list[wgs.text_button.text_font_id], wgs.text_button.text_color, wgs.text_button.text_aa_color);
    bt->wg.c_size.x = w_box*2 + bt->text.size.x;
    bt->wg.c_size.y = MAX(w_box, bt->text.size.y);
    return wg_init(&bt->wg, wg_text_ckbox_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_CFIXED, NULL);
  }
  return &wg_void;
}

widget_t *wg_init_check_box(hwin_t *hw, const char *name, button_pressed_cb_t check_cb)
{
  return win_add_widget(hw, get_wg_text_ckbox(name, check_cb));
}

// ----------------------------------------------
// radio button with text
// ----------------------------------------------

static void text_radio_bt_draw(hwin_t *hw, widget_t *wg)
{
  wg_button_t *bt = (wg_button_t *)wg;
  int w_box = ch_ar_rad_bt_tl0[0];
  int x = wg->c_pos.x;
  int y = wg->c_pos.y;
  wg_draw_radio_button(hw, wg, x, y + ((wg->c_size.y - w_box) >> 1));
  bt->text.pos.x = x + w_box + (w_box >> 1);
  bt->text.pos.y = y + ((wg->c_size.y - bt->text.size.y + 1) >> 1);
  draw_wg_text(hw, wg, &bt->text, wg->c_size.x);
}

static void wg_text_radio_bt_ev_proc(hwin_t *hw, widget_t *wg)
{
  if (hw->ev.type == EV_PAINT)
    text_radio_bt_draw(hw, wg);
  else
  {
    if ((hw->ev.type < EV_CREATE) && (wg->flags & WG_CHECKED))
      return;                                    // radio buttons can be set only
    wg_button_ev_proc(hw, wg);                   // call button code
  }
}

static widget_t *get_wg_radio_button(const char *name, button_pressed_cb_t pressed_cb)
{
  C_NEW(bt, wg_button_t);
  if (bt)
  {
    int w_box = ch_ar_rad_bt_tl0[0];
    bt->pressed_cb = pressed_cb;
    text_init(&bt->text, name, win_font_list[wgs.text_button.text_font_id], wgs.text_button.text_color, wgs.text_button.text_aa_color);
    bt->wg.c_size.x = w_box*2 + bt->text.size.x;
    bt->wg.c_size.y = MAX(w_box, bt->text.size.y);
    return wg_init(&bt->wg, wg_text_radio_bt_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_CFIXED, NULL);
  }
  return &wg_void;
}

widget_t *wg_init_radio_button(hwin_t *hw, const char *name, button_pressed_cb_t check_cb)
{
  return win_add_widget(hw, get_wg_radio_button(name, check_cb));
}

// ----------------------------------------------
// combo box
// ----------------------------------------------

typedef struct
{
  wg_button_t text_bt;
  struct
  {
    const char **str_list;
    int str_count;
    int l_margin;
    int dy_line;
    int w_expand;
  } menu;
  combo_box_select_cb_t combo_select_cb;
  int sel_id;
} wg_combo_t;

// menu selection done. update selection, call user cb if defined
static void cb_menu_select_cb(hwin_t *hw, void *user_ptr, int sel_id)
{
  wg_combo_t *cb = (wg_combo_t *)user_ptr;
  cb->sel_id = sel_id;
  wg_text_button_uptate(hw, &cb->text_bt.wg, cb->menu.str_list[sel_id]);
  if (cb->combo_select_cb)
    cb->combo_select_cb(hw, sel_id);
}

// combo text clicked, open menu
static void combo_box_text_clicked_cb(hwin_t *hw, widget_t *wg)
{
  wg_combo_t *cb = (wg_combo_t *)wg;
  if (cb->menu.str_count)
  {
    int menu_wx = wg->e_size.x - 2*win_get_border_width(win_border_pu1);
    wg_menu_init_ex(wg->e_pos.x, wg->e_pos.y + wg->e_size.y, menu_wx, menu_wx, cb->menu.w_expand, win_border_pu1, &wgs.menu_combo,
                    cb->menu.str_list, cb->menu.str_count, cb->menu.l_margin, cb->menu.dy_line, cb_menu_select_cb, hw, wg);
  }
}

// combo included right button triggered
static void combo_box_bt_pressed_cb(hwin_t *hw, widget_t *wg)
{
  W_ASSERT(wg->wg_parent);
  combo_box_text_clicked_cb(hw, wg->wg_parent);      // simulate click on parent text area
}

static const char *combo_empty_str = " -- ";     // displayed if combo is empty

widget_t *wg_init_combo_box(hwin_t *hw, const char **str_list, int str_count, int sel_id, int l_margin,
                            int dy_line, int w_expand, combo_box_select_cb_t combo_select_cb)
{
  C_NEW(cb, wg_combo_t);
  if (cb)
  {
    wg_button_t *bt = &cb->text_bt;
    int flags = WG_SHOW|WG_ENFOCUS|WG_PTRIG|WG_IGFOCUS|WG_SHIFT| ((l_margin < 0) ? WG_CENTER : WG_LMARGIN);
    if (str_count)
      flags |= WG_ENABLE;
    else
      str_list[0] = combo_empty_str;

    cb->menu.str_list   = str_list;
    cb->menu.str_count  = str_count;
    cb->menu.l_margin   = l_margin;
    cb->menu.dy_line    = dy_line;
    cb->menu.w_expand   = w_expand;
    cb->combo_select_cb = combo_select_cb;
    cb->sel_id = sel_id;

    bt->pressed_cb = combo_box_text_clicked_cb;
    text_init(&bt->text, str_list[sel_id], win_font_list[wgs.combo_box.text_font_id], wgs.combo_box.text_color, wgs.combo_box.text_aa_color);
    init_dr_obj(&bt->wg, e_obj_text, &bt->text, wgs.combo_box.text_bk_color, NULL);
    wg_init(&bt->wg, wg_button_ev_proc, flags, &wgs.combo_box.e_frm);

    return win_add_widget(hw, wg_add_child(&cb->text_bt.wg,
      get_wg_ctrl_button(wg_button_char.ar_d7, flags, combo_box_bt_pressed_cb), WG_IRIGHT));
  }
  return &wg_void;
}

int wg_combo_get_select_id(widget_t *wg)
{
  wg_combo_t *cb = (wg_combo_t *)wg;
  W_ASSERT(cb->text_bt.pressed_cb == combo_box_text_clicked_cb);
  return cb->sel_id;
}

void wg_combo_set_select_id(hwin_t *hw, widget_t *wg, int sel_id)
{
  wg_combo_t *cb = (wg_combo_t *)wg;
  W_ASSERT(cb->text_bt.pressed_cb == combo_box_text_clicked_cb);
  cb->sel_id = sel_id;
  wg_text_button_uptate(hw, &cb->text_bt.wg, cb->menu.str_list[sel_id]);
}

void wg_combo_update_list_size(hwin_t *hw, widget_t *wg, int str_count)
{
  wg_combo_t *cb = (wg_combo_t *)wg;
  W_ASSERT(cb->text_bt.pressed_cb == combo_box_text_clicked_cb);
  cb->menu.str_count = str_count;
  cb->sel_id = 0;
  if (!str_count)
  {
    wg->flags &= ~WG_ENABLE;
    wg_enable(hw, wg->i_wg, false);
    cb->menu.str_list[0] = combo_empty_str;
  }
  else
  {
    wg->flags |= WG_ENABLE;
    wg_enable(hw, wg->i_wg, true);
  }
  wg_combo_set_select_id(hw, wg, cb->sel_id);
}

// ----------------------------------------------
// spin control (up/down buttons)
// ----------------------------------------------

widget_t *wg_init_spin_button_ud(hwin_t *hw, button_pressed_cb_t pressed_cb_up, button_pressed_cb_t pressed_cb_dn)
{
  int flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER|WG_ICENTER;
  return win_add_widget(hw,
         wg_add_child(get_wg_ctrl_button(wg_button_char.ar_u7, flags, pressed_cb_up),
                      get_wg_ctrl_button(wg_button_char.ar_d7, flags, pressed_cb_dn), WG_IDOWN));
}

// ----------------------------------------------
// spin control (left/right buttons)
// ----------------------------------------------

widget_t *wg_init_spin_button_lr(hwin_t *hw, button_pressed_cb_t pressed_cb_left, button_pressed_cb_t pressed_cb_right)
{
  int flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER|WG_ICENTER;
  return win_add_widget(hw,
         wg_add_child(get_wg_ctrl_button(wg_button_char.ar_l7, flags, pressed_cb_left),
                      get_wg_ctrl_button(wg_button_char.ar_r7, flags, pressed_cb_right), WG_IRIGHT));
}

// ----------------------------------------------
// edit box + integrated spin control
// ----------------------------------------------

widget_t *wg_init_edit_spin_box(hwin_t *hw, char *str_edit, int str_edit_buff_size, edit_box_return_pressed_cb_t return_cb,
                                button_pressed_cb_t pressed_cb_up, button_pressed_cb_t pressed_cb_dn)
{
  int flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER;
  widget_t *wg_ed = wg_init_edit_box(hw, str_edit, str_edit_buff_size, return_cb);
  widget_t *wg_spin = wg_add_child(get_wg_ctrl_button(wg_button_char.ar_u5, flags, pressed_cb_up),
                                   get_wg_ctrl_button(wg_button_char.ar_d5, flags, pressed_cb_dn), WG_IDOWN);
  wg_add_child(wg_ed, wg_spin, WG_IRIGHT);
  return wg_ed;
}

// ----------------------------------------------
// progress bar
// ----------------------------------------------

typedef struct
{
  widget_t wg;
  int progress_100;
  pix_t fill_color;
} wg_progress_bar_t;

static void wg_progress_bar_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_progress_bar_t *prb = (wg_progress_bar_t *)wg;
  if (hw->ev.type == EV_PAINT)
  {
    bitmap_t *bm = &hw->cli_bm;
    int w = (prb->progress_100*wg->c_size.x)/100;
    int color = prb->fill_color == -1 ? wgs.progress_bar.fill_color : prb->fill_color;
    wg_draw_e_frame(hw, wg);
    bm_paint_rect(bm, wg->c_pos.x, wg->c_pos.y, w, wg->c_size.y, color);
    bm_paint_rect(bm, wg->c_pos.x+w, wg->c_pos.y, wg->c_size.x-w, wg->c_size.y, wgs.clear_bk_color);
  }
}

widget_t *wg_init_progress_bar(hwin_t *hw, int progress_100, pix_t fill_color)
{
  C_NEW(prb, wg_progress_bar_t);
  if (prb)
  {
    prb->wg.c_size.x = 100;
    prb->wg.c_size.y = 20;              // define minimum initial size
    prb->progress_100 = progress_100 % 101;
    prb->fill_color = fill_color;
    return win_add_widget(hw, wg_init(&prb->wg, wg_progress_bar_ev_proc, WG_SHOW|WG_ENABLE, &wgs.progress_bar.e_frm));
  }
  return &wg_void;
}

void wg_progress_bar_set_color(widget_t *wg, pix_t fill_color)
{
  GET_WG(prb, progress_bar);
  prb->fill_color = fill_color;
}

void wg_progress_bar_set_ratio(hwin_t *hw, widget_t *wg, int progress_100)
{
  GET_WG(prb, progress_bar);
  progress_100 %= 101;
  if (progress_100 != prb->progress_100)
  {
    prb->progress_100 = progress_100;
    wg_draw_blit(hw, wg, false, true, true);
  }
}
