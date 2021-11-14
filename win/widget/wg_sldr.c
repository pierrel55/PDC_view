// sliders

#include <stdlib.h>
#include "widget.h"
#include "wg_priv.h"

// #define BOUND(a, min, max) ( ((a) < (min)) ? a=(min) : (a > (max) ? a=(max) : (void)0) )

// ----------------------------------------------
// slider

#define SL_MIN_SIZE 8                            // min button size
#define SL_CTRL_DIST 80                          // max control distance from mouse cursor

// vertical slider
typedef struct
{
  widget_t wg;
  int move_range;                                // user move range (in pixels)
  int move_ofs;                                  // user move position offset (in pixels)
  int move_step;                                 // move on mouse whell event
  int bt_size;                                   // drawing size
  int bt_ofs;                                    // drawing position offset
  bool ctrl_move;                                // mouse capture and move
  int ctrl_ofs;                                  // mouse click offset on button
  slider_moved_cb_t moved_cb;                    // user call back
} wg_vslider_t;

static void vslider_paint(wg_vslider_t *sl, bitmap_t *bm)
{
  widget_t *wg = &sl->wg;
  frm_t *frm = &wgs.slider.bt_frm;
  int y;

  if (sl->bt_ofs > 0)
    bm_paint_rect(bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, sl->bt_ofs, wgs.slider.slider_bk_color);

  // button background
  bm_paint_rect(bm, wg->c_pos.x + frm->width,
                    wg->c_pos.y + sl->bt_ofs + frm->width,
                    wg->c_size.x - 2*frm->width,
                    sl->bt_size - 2*frm->width, wgs.clear_bk_color);

  // button frame
  bm_draw_rect_shadow_width(bm,
                    wg->c_pos.x,
                    wg->c_pos.y + sl->bt_ofs,
                    wg->c_size.x,
                    sl->bt_size, frm->width, frm->tl, frm->br);

  y = sl->bt_ofs + sl->bt_size;
  if (wg->c_size.y > y)
    bm_paint_rect(bm, wg->c_pos.x, wg->c_pos.y + y, wg->c_size.x, wg->c_size.y - y, wgs.slider.slider_bk_color);
}

// set button size
static void vs_set_bt_size(wg_vslider_t *sl)
{
  widget_t *wg = &sl->wg;
  sl->bt_size = (wg->c_size.y * wg->c_size.y) / (wg->c_size.y + sl->move_range);
  if (sl->bt_size < SL_MIN_SIZE)
  {
    if (wg->c_size.y < SL_MIN_SIZE)
      sl->bt_size = 0;                   // wg->c_size too small, disable drawing
    else
      sl->bt_size = SL_MIN_SIZE;
  }
}

// set button position
static void vs_set_bt_ofs(wg_vslider_t *sl)
{
  if (sl->move_range)
    sl->bt_ofs = ((sl->wg.c_size.y - sl->bt_size) * sl->move_ofs) / sl->move_range;
  else
    sl->bt_ofs = 0;
}

// change offset
static bool vs_set_move_range(wg_vslider_t *sl, int move_range)
{
  if (move_range < 0)
    move_range = 0;
  if (move_range != sl->move_range)
  {
    sl->move_range = move_range;
    vs_set_bt_size(sl);
    vs_set_bt_ofs(sl);
    return true;
  }
  return false;
}

// change offset
static bool vs_set_move_ofs(hwin_t *hw, wg_vslider_t *sl, int move_ofs, bool call_cb)
{
  if (move_ofs < 0)
    move_ofs = 0;
  else
  if (move_ofs > sl->move_range)
    move_ofs = sl->move_range;
  if (move_ofs != sl->move_ofs)
  {
    sl->move_ofs = move_ofs;
    vs_set_bt_ofs(sl);
    if (call_cb && sl->moved_cb)
      sl->moved_cb(hw, &sl->wg, sl->move_ofs);
    return true;
  }
  return false;
}

static void vslider_mouse_move(hwin_t *hw, wg_vslider_t *sl, int ms_y)
{
  widget_t *wg = &sl->wg;
  int bt_ofs = ms_y - wg->c_pos.y - sl->ctrl_ofs;
  if (bt_ofs < 0)
    bt_ofs = 0;
  else
  if ((bt_ofs + sl->bt_size) > wg->c_size.y)
    bt_ofs = wg->c_size.y - sl->bt_size;
  if (bt_ofs != sl->bt_ofs)
  {
    int move_ofs = (bt_ofs * sl->move_range) / (wg->c_size.y - sl->bt_size);
    wg_vslider_set_ofs(hw, wg, move_ofs, true);
  }
}

static void wg_vslider_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_vslider_t *sl = (wg_vslider_t *)wg;
  switch (hw->ev.type)
  {
    case EV_MOUSEMOVE:
      if (sl->ctrl_move)
      {
        int cx = wg->c_pos.x + (wg->c_size.x >> 1);
        int dx = hw->ev.mouse_pos.x - cx;
        if ((dx > -SL_CTRL_DIST) && (dx < SL_CTRL_DIST))
          vslider_mouse_move(hw, sl, hw->ev.mouse_pos.y);
      }
    break;
    case EV_LBUTTONDOWN:
    case EV_LBUTTONDBLCLK:
    {
      vec2i bt_pos = { wg->c_pos.x, wg->c_pos.y + sl->bt_ofs };
      vec2i bt_size = { wg->c_size.x, sl->bt_size };
      if (!vec_select(&hw->ev.mouse_pos, &bt_pos, &bt_size))
      {
        int ofs = (sl->bt_size * sl->move_range) / wg->c_size.y;
        int dir = (hw->ev.mouse_pos.y < bt_pos.y) ? -ofs : ofs;
        wg_vslider_set_ofs(hw, wg, sl->move_ofs + dir, true);
      }
      else
      if (hw->ev.type == EV_LBUTTONDOWN)
      {
        // cursor captured, move with button
        sl->ctrl_ofs = hw->ev.mouse_pos.y - bt_pos.y;
        sl->ctrl_move = true;
      }
    }
    break;
    case EV_LBUTTONUP:
      sl->ctrl_move = false;
    break;
    case EV_MOUSEWHEEL:
      wg_vslider_set_ofs(hw, wg, sl->move_ofs + 3*(hw->ev.mouse_whell > 0 ? -sl->move_step : sl->move_step), true);
    break;
    case EV_PAINT:
      vslider_paint(sl, &hw->cli_bm);
    break;
    default:;
  }
}

// resize proc
static void wg_vslider_size_proc(widget_t *wg)
{
  wg_vslider_t *sl = (wg_vslider_t *)wg;
  vs_set_bt_size(sl);
  vs_set_bt_ofs(sl);
}

// get offset
int wg_vslider_get_ofs(widget_t *wg)
{
  GET_WG_RET(sl, vslider, 0);
  return sl->move_ofs;
}

// change offset and refresh
void wg_vslider_set_ofs(hwin_t *hw, widget_t *wg, int move_ofs, bool call_cb)
{
  GET_WG(sl, vslider);
  if (vs_set_move_ofs(hw, sl, move_ofs, call_cb))
    wg_draw_blit(hw, wg, false, true, true);         // redraw + blit
}

// change range and refresh
void wg_vslider_set_range(hwin_t *hw, widget_t *wg, int move_range)
{
  GET_WG(sl, vslider);
  if (vs_set_move_range(sl, move_range))
    wg_draw_blit(hw, wg, false, true, true);         // redraw + blit
}

// change range and offset and refresh
void wg_vslider_set_range_ofs(hwin_t *hw, widget_t *wg, int move_range, int move_ofs, bool call_cb, bool blit)
{
  bool bl;
  GET_WG(sl, vslider);
  bl  = vs_set_move_range(sl, move_range);
  bl |= vs_set_move_ofs(hw, sl, move_ofs, call_cb);
  if (bl && blit)
    wg_draw_blit(hw, wg, false, true, true);
}

// return if is mouse controlled
bool wg_slider_control(widget_t *wg)
{
  GET_WG_RET(sl, vslider, false);
  return sl->ctrl_move;
}

static widget_t *get_wg_vslider(int move_range, int move_ofs, int move_step, slider_moved_cb_t moved_cb)
{
  C_NEW(sl, wg_vslider_t);
  if (!sl)
    return &wg_void;

  sl->wg.c_size.x = 16;
  sl->wg.c_size.y = 200;
  sl->move_range = move_range;
  sl->move_ofs = move_ofs;
  sl->move_step = move_step;
  sl->moved_cb = moved_cb;
  sl->wg.sz_proc = wg_vslider_size_proc;
  return wg_init(&sl->wg, wg_vslider_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS, NULL);
}

widget_t *wg_init_vslider(hwin_t *hw, int move_range, int move_ofs, int move_step, slider_moved_cb_t moved_cb)
{
  return win_add_widget(hw, get_wg_vslider(move_range, move_ofs, move_step, moved_cb));
}

// ----------------------------------------------
// vertical scroll bar

static void vsb_bt_pressed_cb_up(hwin_t *hw, widget_t *wg)
{
  wg_vslider_t *sl = (wg_vslider_t *)wg->i_wg;
  wg_vslider_set_ofs(hw, &sl->wg, sl->move_ofs - sl->move_step, true);
  wg_change_focus(hw, &sl->wg);
}

static void vsb_bt_pressed_cb_dn(hwin_t *hw, widget_t *wg)
{
  wg_vslider_t *sl = (wg_vslider_t *)wg->wg_parent;
  wg_vslider_set_ofs(hw, &sl->wg, sl->move_ofs + sl->move_step, true);
  wg_change_focus(hw, &sl->wg);
}

widget_t *wg_get_scroll_bar_v(int move_range, int move_ofs, int move_step, slider_moved_cb_t moved_cb)
{
  int bt_flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER;
  widget_t *bt_up = get_wg_ctrl_button(wg_button_char.ar_u7, bt_flags, vsb_bt_pressed_cb_up);
  widget_t *bt_dn = get_wg_ctrl_button(wg_button_char.ar_d7, bt_flags, vsb_bt_pressed_cb_dn);
  widget_t *slider = get_wg_vslider(move_range, move_ofs, move_step, moved_cb);
  return wg_add_child(bt_up, wg_add_child(slider, bt_dn, WG_VSB_D), WG_VSB_S);
}

// insert into parent widget client area
void wg_insert_scroll_bar_v(widget_t *wg, int move_range, int move_ofs, int move_step, slider_moved_cb_t moved_cb)
{
  widget_t *sbar = wg_get_scroll_bar_v(move_range, move_ofs, move_step, moved_cb);
  wg_add_child(wg, sbar, WG_VSB);
}

// no auto resize, place with wg_set_pos_size() (window control)
widget_t *wg_init_scroll_bar_v(hwin_t *hw, int move_range, int move_ofs, int move_step, slider_moved_cb_t moved_cb)
{
  widget_t *sbar = wg_get_scroll_bar_v(move_range, move_ofs, move_step, moved_cb);
  return win_add_widget(hw, sbar);
}
