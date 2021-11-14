// widget private
#include <string.h>
#include <stdlib.h>
#include "widget.h"
#include "wg_priv.h"

// checkbox char
const uint32_t ch_ck_9[10] = { 9, 0x0000, 0x0080, 0x00c0, 0x00e2, 0x0076, 0x003e, 0x001c, 0x0008, 0x0000 };

// radio button chars
const uint32_t ch_ar_rad_bt_tl0[13] = { 12, 0x00f0, 0x030c, 0x0002, 0x0002, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0000, 0x0000 };
static const uint32_t ch_ar_rad_bt_br0[13] = { 12, 0x0000, 0x0000, 0x0400, 0x0400, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x030c, 0x00f0 };
static const uint32_t ch_ar_rad_bt_tl1[13] = { 12, 0x0000, 0x00f0, 0x030c, 0x0004, 0x0002, 0x0002, 0x0002, 0x0002, 0x0004, 0x0000, 0x0000, 0x0000 };
static const uint32_t ch_ar_rad_bt_br1[13] = { 12, 0x0000, 0x0000, 0x0000, 0x0200, 0x0400, 0x0400, 0x0400, 0x0400, 0x0200, 0x030c, 0x00f0, 0x0000 };
static const uint32_t ch_ar_rad_bt_dot[13] = { 12, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060, 0x00f0, 0x00f0, 0x0060, 0x0000, 0x0000, 0x0000, 0x0000 };
static const uint32_t ch_ar_rad_bt_bk [13] = { 12, 0x0000, 0x0000, 0x00f0, 0x01f8, 0x03fc, 0x03fc, 0x03fc, 0x03fc, 0x01f8, 0x00f0, 0x0000, 0x0000 };

// arrow button chars
static const uint32_t ch_ar_u7[8] = { 7, 0x0000, 0x0000, 0x0008, 0x001c, 0x003e, 0x007f, 0x0000 };
static const uint32_t ch_ar_d7[8] = { 7, 0x0000, 0x0000, 0x007f, 0x003e, 0x001c, 0x0008, 0x0000 };
static const uint32_t ch_ar_l7[8] = { 7, 0x0010, 0x0018, 0x001c, 0x001e, 0x001c, 0x0018, 0x0010 };
static const uint32_t ch_ar_r7[8] = { 7, 0x0004, 0x000c, 0x001c, 0x003c, 0x001c, 0x000c, 0x0004 };
static const uint32_t ch_ar_u5[6] = { 5, 0x0000, 0x0004, 0x000e, 0x001f, 0x0000 };
static const uint32_t ch_ar_d5[6] = { 5, 0x0000, 0x001f, 0x000e, 0x0004, 0x0000 };
static const uint32_t ch_ar_l5[6] = { 5, 0x0008, 0x000c, 0x000e, 0x000c, 0x0008 };
static const uint32_t ch_ar_r5[6] = { 5, 0x0002, 0x0006, 0x000e, 0x0006, 0x0002 };

const struct wg_button_char_t wg_button_char
       = { ch_ar_u7, ch_ar_d7, ch_ar_l7, ch_ar_r7,
           ch_ar_u5, ch_ar_d5, ch_ar_l5, ch_ar_r5 };

// ----------------------------------------------
// void widget doing nothing, returned if widget alloc fail

// event proc doing noting
static void wg_void_ev_proc(hwin_t *hw, widget_t *wg)
{
  W_ASSERT(0);                                   // should never occur as wg_void never inserted in widget list
}

widget_t wg_void = { wg_void_ev_proc, 0 };

// ----------------------------------------------
// text util

void text_init(text_t *t, const char *str, const font_t *font, pix_t color, pix_t aa_color)
{
  t->font = font;
  t->color = color;
  t->aa_color = aa_color;
  t->size.y = t->font->dy;
  text_update(t, str);
}

void text_update(text_t *t, const char *str)
{
  t->str = str ? str : "<NULL>";
  t->size.x = font_get_string_width(t->str, t->font);
}

int text_draw(text_t *t, bitmap_t *bm, int w_max)
{
  return bm_draw_string_truncate(bm, t->pos.x, t->pos.y, t->str, t->color, 0, t->font, w_max);
}

// ----------------------------------------------
// add widget in window widget list

widget_t *win_add_widget(hwin_t *hw, widget_t *wg)
{
  if (wg && (wg != &wg_void))
  {
    W_ASSERT(!wg->next);
    wg->next = hw->wg_list;
    hw->wg_list = wg;
    return wg;
  }
  W_ASSERT(0);                                   // debug inform problem occured (memory alloc ?)
  return &wg_void;                               // return dummy widget to avoid crash
}

// ----------------------------------------------
// widget events

static void wg_send_event(hwin_t *hw, widget_t *wg, enum e_win_event win_event)
{
  struct win_event_t ev_save = hw->ev;           // backup state
  hw->ev.type = win_event;
  wg->ev_proc(hw, wg);
  hw->ev = ev_save;                              // restore state
}

void wg_change_focus(hwin_t *hw, widget_t *wg)
{
  if (hw->wg_focus != wg)                        // test if changed
  {
    if (hw->wg_focus)
      wg_send_event(hw, hw->wg_focus, EV_WG_KILLFOCUS);
    if (wg)
      wg_send_event(hw, wg, EV_SETFOCUS);
    hw->wg_focus = wg;
  }
}

// dispatch click event to single widget
static int wg_dispatch_ev_click(hwin_t *hw, widget_t *wg)
{
  while (wg)
  {
    vec2i *mouse_pos = &hw->ev.mouse_pos;
    if (    (wg->flags & WG_ENFOCUS)             // skip passive objects like wg_frame than cannot receive click events
         && vec_select(mouse_pos, &wg->e_pos, &wg->e_size))
    {
      if (wg->flags & (WG_ENABLE|WG_EVIWG))
      {
        if (vec_select(mouse_pos, &wg->c_pos, &wg->c_size))
        {
#if WG_EVIWG
          if (wg->flags & WG_ENABLE)
#endif
          {
            win_help_suspend();
            if ((wg->flags & WG_IGFOCUS) && hw->ev.gain_focus)
              return 1;                          // ignore event proc if window gained focus

            wg_change_focus(hw, wg);
            wg->ev_proc(hw, wg);
            return 2;                            // enabled widget selected
          }
#if WG_EVIWG
          return 1;                              // disabled widget selected
#endif
        }
        W_ASSERT(wg->i_wg);                      // dead area if NULL ?
        return wg_dispatch_ev_click(hw, wg->i_wg); // test childs
      }
    }
    wg = wg->next;
  }
  return 0;                                      // no widget found
}

// dispatch event to widget tree
static void wg_dispatch_ev_all(hwin_t *hw, widget_t *wg)
{
  if (wg)
  {
    wg_dispatch_ev_all(hw, wg->next);
    if ((hw->ev.type == EV_PAINT) && !(wg->flags & WG_SHOW))
      return;                                    // ignore childs

    wg_dispatch_ev_all(hw, wg->i_wg);
    wg->ev_proc(hw, wg);
    if (hw->ev.type == EV_DESTROY)
    {
      if (wg->blk_ptr)
        W_FREE(wg->blk_ptr);
      W_FREE(wg);
    }
  }
}

// find mouse selected wg
static widget_t *find_wg_cli_hover(widget_t *wg, vec2i *mouse_pos)
{
  for (;wg ;wg = wg->next)
    //if ((wg->flags & WG_ENFOCUS) && vec_select(mouse_pos, &wg->c_pos, &wg->c_size))
    if (vec_select(mouse_pos, &wg->c_pos, &wg->c_size))
      return wg;
  return NULL;
}

static void wg_show_help(hwin_t *hw)
{
  widget_t *wg = find_wg_cli_hover(hw->wg_list, &hw->ev.mouse_pos);

  if (wg && wg->help_text)
    win_help_open(wg->help_text, &wg->c_pos, &wg->c_size, hw);
}

// return widget hwin, if window own widget and have focus.
// note: avoid use for speed if hwin available, as require search loop.
hwin_t *widget_find_focus_hwin(widget_t *wg)
{
  hwin_t *hw = win_get_winfocus();
  if (hw)
  {
    widget_t *wg_list = hw->wg_list;
    while (wg_list)
    {
      if (wg_list == wg)
        return hw;
      wg_list = wg_list->next;
    }
  }
  return NULL;
}

// dispatch widget events from tree root
void widget_dispatch_events(hwin_t *hw)
{
  widget_t *wg = hw->wg_list;

  // send mouse move events only if cursor captured
  if (hw->ev.type == EV_MOUSEMOVE)
  {
    if (win_mouse_captured() && hw->wg_focus)
      hw->wg_focus->ev_proc(hw, hw->wg_focus);
    return;
  }

  // widget focus change
  if (    (hw->ev.type == EV_LBUTTONDOWN)        // find clicked widget
       || (hw->ev.type == EV_LBUTTONDBLCLK))
  {
    if (wg_dispatch_ev_click(hw, wg) < 2)
      wg_change_focus(hw, NULL);                 // none or disabled clicked
    return;
  }

  // send to current widget having focus
  if ((hw->ev.type <= EV_KILLFOCUS) && hw->wg_focus)
  {
    hw->wg_focus->ev_proc(hw, hw->wg_focus);
    return;
  }

  // dispatched to all widgets
  switch (hw->ev.type)
  {
    case EV_CREATE:
    case EV_DESTROY:
    case EV_PAINT:
      wg_dispatch_ev_all(hw, wg);                // dispatch full tree from end
    break;
    case EV_SIZE:                                // window resized
      if (hw->wg_focus)                          // todo: do this or manage EV_MOVE_WG event
        wg_change_focus(hw, NULL);
    break;
    case EV_HELP:
      wg_show_help(hw);
    break;
    default:;
  }
}

// ----------------------------------------------
// immediate draw/refresh functions

// draw using event proc
void wg_draw_blit(hwin_t *hw, widget_t *wg, bool clear_bk, bool draw, bool blit)
{
  if (wg->flags & WG_SHOW)
  {
    if (clear_bk)
      bm_paint_rect(&hw->cli_bm,
          wg->c_pos.x, wg->c_pos.y,
          wg->c_size.x, wg->c_size.y,
          wgs.clear_bk_color);
    if (draw)
      wg_send_event(hw, wg, EV_PAINT);
    if (blit)
      win_cli_blit_rect(hw, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
  }
}

void wg_show(hwin_t *hw, widget_t *wg, bool show)
{
  if (show)
  {
    if (!(wg->flags & WG_SHOW))
    {
      wg->flags |= WG_SHOW;
      wg_draw_blit(hw, wg, false, true, true);
    }
  }
  else
  if (wg->flags & WG_SHOW)
  {
    wg_draw_blit(hw, wg, true, false, true);
    wg->flags &= ~WG_SHOW;
  }
}

// return if widget is enabled
bool wg_is_enabled(widget_t *wg)
{
  return (wg->flags & WG_ENABLE) != 0;
}

// set flag only, no refresh (init)
void wg_enable_init(widget_t *wg, bool enable)
{
  if (enable)
    wg->flags |= WG_ENABLE;
  else
    wg->flags &= ~WG_ENABLE;
  if (wg->i_wg)
    wg_enable_init(wg->i_wg, enable);
}

// enable/disable and refresh
void wg_enable(hwin_t *hw, widget_t *wg, bool enable)
{
  W_ASSERT(wg);
  if (enable)
  {
    if (!(wg->flags & WG_ENABLE))
    {
      wg->flags |= WG_ENABLE;
      wg_draw_blit(hw, wg, true, true, true);
      if (wg->i_wg)
        wg_enable(hw, wg->i_wg, enable);
    }
  }
  else
  {
    if (wg->flags & WG_ENABLE)
    {
      if (hw->wg_focus == wg)
        wg_change_focus(hw, NULL);               // disabled can't have focus
      wg->flags &= ~(WG_ENABLE | WG_PRESSED);
      wg_draw_blit(hw, wg, true, true, true);
      if (wg->i_wg)
        wg_enable(hw, wg->i_wg, enable);
    }
  }
}

// enable: enable/disable widgets in list
// invert_others : if true, invert enable for widgets not in list
// refresh: immediate draw/blit
// note: action can be reverted with wg_enable_restore_backup
void wg_enable_trigger_list(hwin_t *hw, const widget_t **wg_list, bool enable, bool invert_others, bool refresh)
{
  widget_t *wg;
  for (wg = hw->wg_list; wg; wg = wg->next)
  {
    if (wg->flags & WG_ENFOCUS)
    {
      // test if wg in list
      const widget_t **l = wg_list;
      for (l=wg_list; *l && (*l != wg); l++)
        ;

      // backup current state (for optional restore)
      wg->flags = (wg->flags & WG_ENABLE) ? wg->flags | WG_ENSAV : wg->flags & ~WG_ENSAV;

      // change state
      if (wg == *l)                       // in list
      {
        if (refresh)
          wg_enable(hw, wg, enable);
        else
          wg_enable_init(wg, enable);
      }
      else
      if (invert_others)
      {
        if (refresh)
          wg_enable(hw, wg, !enable);
        else
          wg_enable_init(wg, !enable);
      }
    }
  }
}

// restore initial states
void wg_enable_restore_trigger_backup(hwin_t *hw)
{
  widget_t *wg;
  for (wg = hw->wg_list; wg; wg = wg->next)
  {
    if (wg->flags & WG_ENFOCUS)
      wg_enable(hw, wg, (wg->flags & WG_ENSAV) != 0);
  }
}

#if 0
// non recursive enable (i_wg)
void wg_enable_nr(hwin_t *hw, widget_t *wg, bool enable, bool draw_blit)
{
  W_ASSERT(wg);
  if (enable)
  {
    if (!(wg->flags & WG_ENABLE))
    {
      wg->flags |= WG_ENABLE;
      if (draw_blit)
        wg_draw_blit(hw, wg, true, true, true);
    }
  }
  else
  {
    if (wg->flags & WG_ENABLE)
    {
      wg->flags &= ~WG_ENABLE;
      if (draw_blit)
        wg_draw_blit(hw, wg, true, true, true);
    }
  }
}
#endif

// allow focus only for list (thread in progress)
void wg_focus_list_only(hwin_t *hw, widget_t **wg_list)
{
  widget_t *wg;
  for (wg = hw->wg_list; wg; wg = wg->next)
  {
    int flags;
    widget_t **l = wg_list;
    for (l=wg_list; *l && (*l != wg); l++)
      ;
    flags = (wg->flags & WG_ENFOCUS) ? wg->flags | WG_ENSAV : wg->flags & ~WG_ENSAV;
    wg->flags = (wg == *l) ? flags | WG_ENFOCUS : flags & ~WG_ENFOCUS;
  }
}

// restore focus state before call to wg_focus_list_only
void wg_focus_list_restore(hwin_t *hw)
{
  widget_t *wg;
  for (wg = hw->wg_list; wg; wg = wg->next)
    wg->flags = (wg->flags & WG_ENSAV) ? wg->flags | WG_ENFOCUS : wg->flags & ~WG_ENFOCUS;
}

// set pressed/checked state flags (init, no refreh)
void wg_set_state_init(widget_t *wg, bool state)
{
  if (state)
    wg->flags |= WG_CHECKED;
  else
    wg->flags &= ~WG_CHECKED;
}

// set cheched on/off state
void wg_set_state(hwin_t *hw, widget_t *wg, bool state)
{
  bool st = (wg->flags & WG_CHECKED) != 0;
  if (st ^ state)
  {
    wg->flags ^= WG_CHECKED;
    wg_draw_blit(hw, wg, true, true, true);
  }
}

// get pressed/checked state (buttons/check box/radio buttons)
bool wg_get_state(widget_t *wg)
{
  return (wg->flags & WG_CHECKED) != 0;
}

void wg_set_text_align(widget_t *wg, enum e_text_align text_align)
{
  wg->flags &= ~(WG_CENTER|WG_LMARGIN|WG_RALIGN|WG_RMARGIN);
  switch (text_align)
  {
    case wg_ta_none:
    break;
    case wg_ta_left_ma:
      wg->flags |= WG_LMARGIN;
    break;
    case wg_ta_center:
      wg->flags |= WG_CENTER;
    break;
    case wg_ta_right:
      wg->flags |= WG_RALIGN;
    break;
    case wg_ta_right_ma:
      wg->flags |= WG_RMARGIN;
    break;
  }
}

// ----------------------------------------------
// init functions

// init widget
widget_t *wg_init(widget_t *wg, wg_ev_proc_t ev_proc, int flags, frm_t *e_frm)
{
  // init initial e_size if resize not done by user
  W_ASSERT(wg->c_size.x && wg->c_size.y);
  wg->e_size = wg->c_size;
  if (wg->c_frm)
  {
    int w = wg->c_frm->frm_u.width;
    int w2 = w << 1;
    wg->c_size.x += w2;
    wg->c_size.y += w2;
    wg->e_size.x += w2;
    wg->e_size.y += w2;
    wg->c_pos.x = w;
    wg->c_pos.y = w;
  }

  wg->ev_proc = ev_proc;
  wg->flags = flags;
  wg->e_frm = e_frm;
  return wg;
}

// composed widgets. insert sub widget (i_wg) in widget
widget_t *wg_add_child(widget_t *wg, widget_t *i_wg, int i_flags)
{
  W_ASSERT(i_flags & (WG_ILEFT|WG_IRIGHT|WG_IUP|WG_IDOWN|WG_VSB_S|WG_VSB|WG_VSB_D));
  W_ASSERT(!i_wg->next && !i_wg->wg_parent);     // must be NULL

  i_wg->flags |= i_flags;
  i_wg->next = wg->i_wg;
  i_wg->wg_parent = wg;
  wg->i_wg = i_wg;
  return wg;
}

// -----------------------------------------------
// draw base widget object

void draw_wg_text(hwin_t *hw, widget_t *wg, const text_t *t, int w_max)
{
  if (wg->flags & WG_ENABLE)
    bm_draw_string(&hw->cli_bm, t->pos.x, t->pos.y, t->str, t->color, t->aa_color, t->font, w_max);
  else
  {
    if (wgs.txt_dis_color_s != COL_ND)
      bm_draw_string(&hw->cli_bm, t->pos.x+1, t->pos.y+1, t->str, wgs.txt_dis_color_s, 0, t->font, w_max);
    bm_draw_string(&hw->cli_bm, t->pos.x, t->pos.y, t->str, wgs.txt_dis_color, 0, t->font, w_max);
  }
}

void draw_wg_char(hwin_t *hw, widget_t *wg, const uint32_t *ch, int x, int y)
{
  if (wg->flags & WG_ENABLE)
    bm_draw_raw_char(&hw->cli_bm, x, y, wgs.bt_ctrl_color, 0, ch, ch[0]);
  else
  {
    if (wgs.txt_dis_color_s != COL_ND)
      bm_draw_raw_char(&hw->cli_bm, x+1, y+1, wgs.txt_dis_color_s, 0, ch, ch[0]);
    bm_draw_raw_char(&hw->cli_bm, x, y, wgs.txt_dis_color, 0, ch, ch[0]);
  }
}

// draw check box, return x size
void wg_draw_check_box(hwin_t *hw, widget_t *wg, int x, int y)
{
  int w_box = ch_ck_9[0] + (wgs.frm_check_box.width << 1);
  bm_draw_rect_shadow_width(&hw->cli_bm, x, y, w_box, w_box,
    wgs.frm_check_box.width, wgs.frm_check_box.tl, wgs.frm_check_box.br);

  x += wgs.frm_check_box.width;
  y += wgs.frm_check_box.width;
  if ((wg->flags & (WG_ENABLE|WG_PRESSED)) == WG_ENABLE)
    bm_paint_rect(&hw->cli_bm, x, y, ch_ck_9[0], ch_ck_9[0], wgs.txt_en_bk_color);

  if (wg->flags & WG_CHECKED)
    draw_wg_char(hw, wg, ch_ck_9, x, y);
}

// draw radio button
void wg_draw_radio_button(hwin_t *hw, widget_t *wg, int x, int y)
{
  bitmap_t *bm = &hw->cli_bm;
  const frm_t *frm = &wgs.frm_check_box;         // use checkbox colors

  // draw frame
  bm_draw_raw_char(bm, x, y, frm->tl[0], 0, ch_ar_rad_bt_tl0, ch_ar_rad_bt_tl0[0]);
  bm_draw_raw_char(bm, x, y, frm->br[0], 0, ch_ar_rad_bt_br0, ch_ar_rad_bt_br0[0]);
  bm_draw_raw_char(bm, x, y, frm->tl[1], 0, ch_ar_rad_bt_tl1, ch_ar_rad_bt_tl1[0]);
  bm_draw_raw_char(bm, x, y, frm->br[1], 0, ch_ar_rad_bt_br1, ch_ar_rad_bt_br1[0]);

  // clear background
  if ((wg->flags & (WG_ENABLE|WG_PRESSED)) == WG_ENABLE)
     bm_draw_raw_char(bm, x, y, wgs.txt_en_bk_color, 0, ch_ar_rad_bt_bk, ch_ar_rad_bt_bk[0]);

  if (wg->flags & WG_CHECKED)                    // center dot
    draw_wg_char(hw, wg, ch_ar_rad_bt_dot, x, y);
}

// draw external frame (single state)
void wg_draw_e_frame(hwin_t *hw, widget_t *wg)
{
  if (wg->e_frm)
    bm_draw_rect_shadow_width(&hw->cli_bm,
      wg->e_pos.x, wg->e_pos.y,
      wg->e_size.x, wg->e_size.y,
      wg->e_frm->width, wg->e_frm->tl, wg->e_frm->br);
}

// draw client frame
void wg_draw_c_frame(hwin_t *hw, widget_t *wg)
{
  if (wg->c_frm)
  {
    const frm_t *c_frm = (wg->flags & WG_PRESSED) ? &wg->c_frm->frm_p : &wg->c_frm->frm_u;
    bm_draw_rect_shadow_width(&hw->cli_bm,
      wg->c_pos.x, wg->c_pos.y,
      wg->c_size.x, wg->c_size.y,
      c_frm->width, c_frm->tl, c_frm->br);
  }
}

// draw widget
void wg_draw_obj(hwin_t *hw, widget_t *wg)
{
  vec2i dr_pos = wg->c_pos;
  int shift = (wg->flags & (WG_PRESSED|WG_SHIFT)) == (WG_PRESSED|WG_SHIFT);

  if ((wg->dr_obj.bk_color != COL_ND) && (wg->flags & WG_ENABLE))
    bm_paint_rect(&hw->cli_bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y, wg->dr_obj.bk_color);

  if (wg->dr_obj.type == e_obj_text)
  {
    text_t *t = wg->dr_obj.text;
    int dx = shift;                              // text x decal
    if ((wg->flags & WG_CENTER) && (wg->c_size.x > t->size.x))
      dx += ((wg->c_size.x - t->size.x) >> 1);
    else
    if (wg->flags & WG_LMARGIN)
      dx += TEXT_LMARGIN_PIXELS;
    else
    if (wg->flags & WG_RALIGN)
      dx += (wg->c_size.x - t->size.x);
    else
    if (wg->flags & WG_RMARGIN)
      dx += (wg->c_size.x - t->size.x) - TEXT_LMARGIN_PIXELS;

    t->pos.x = dr_pos.x + dx;
    t->pos.y = dr_pos.y + shift + ((wg->c_size.y - t->size.y) >> 1); // always y center for text
    draw_wg_text(hw, wg, t, wg->c_size.x - dx);
  }
  else
  if (wg->dr_obj.type == e_obj_char)
  {
    int w_ch = wg->dr_obj.ch[0];
    dr_pos.x += shift + ((wg->c_size.x - w_ch) >> 1);
    dr_pos.y += shift + ((wg->c_size.y - w_ch) >> 1);
    draw_wg_char(hw, wg, wg->dr_obj.ch, dr_pos.x, dr_pos.y);
  }
  else
  if (wg->dr_obj.type == e_obj_bitmap)
  {
    const bitmap_t *bm = wg->dr_obj.bm;
    if (wg->flags & WG_CENTER)
    {
      const vec2i *dr_size = &bm->size;
      dr_pos.x += shift + ((wg->c_size.x - dr_size->x) >> 1);
      dr_pos.y += shift + ((wg->c_size.y - dr_size->y) >> 1);
    }
    bm_copy_img(&hw->cli_bm, dr_pos.x, dr_pos.y, bm);
  }

  // draw client frame
  wg_draw_c_frame(hw, wg);

  // draw external frame
  wg_draw_e_frame(hw, wg);
}

// -----------------------------------------------
// init base widget

// init a widget draw object, init minimal default size
void init_dr_obj(widget_t *wg,
                 enum e_dr_obj_typ obj_type,     // type of object to draw
                 const void *obj,                // pointer to object to draw
                 pix_t bk_color,                 // color to set if enable
                 c_frm_t *c_frm)                 // object client frame, set NULL if no frame
{
  wg->dr_obj.type = obj_type;
  wg->dr_obj.obj = obj;
  wg->dr_obj.bk_color = bk_color;
  wg->c_frm = c_frm;

  // define an initial minimal size for object be visible if not resized.
  switch (obj_type)
  {
    case e_obj_none:
      wg->c_size.x = 16;                         // define a minimal size
      wg->c_size.y = 16;
    break;
    case e_obj_text:
      wg->c_size.x = wg->dr_obj.text->size.x ? wg->dr_obj.text->size.x : 16;
      wg->c_size.y = wg->dr_obj.text->size.y;
    break;
    case e_obj_char:
      wg->c_size.x = wg->dr_obj.ch[0];
      wg->c_size.y = wg->c_size.x;
    break;
    case e_obj_bitmap:
      wg->c_size = wg->dr_obj.bm->size;
    break;
    default:
      W_ASSERT(0);
  }
}

// ----------------------------------------------
// generic button widget

// generic button control event proc
void wg_button_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_button_t *bt = (wg_button_t *)wg;
  switch (hw->ev.type)
  {
    case EV_MOUSEMOVE:
      if (vec_select(&hw->ev.mouse_pos, &wg->c_pos, &wg->c_size))
      {
        if (!(wg->flags & WG_PRESSED))
        {
          wg->flags |= WG_PRESSED;
          wg_draw_blit(hw, wg, true, true, true);
        }
      }
      else  // not selected, restore previous state
      if ((wg->flags & WG_PRESSED) && !(bt->prev_flags & WG_PRESSED))
      {
        wg->flags &= ~WG_PRESSED;
        wg_draw_blit(hw, wg, true, true, true);
      }
    break;
    case EV_LBUTTONDBLCLK:
    case EV_LBUTTONDOWN:
      bt->prev_flags = wg->flags;                  // save post pressed state
      if (!(bt->prev_flags & WG_PRESSED))          // not dual state pressed button
      {
        wg->flags |= WG_PRESSED;
        wg_draw_blit(hw, wg, true, true, true);
        if ((wg->flags & WG_PTRIG) && bt->pressed_cb)
          bt->pressed_cb(hw, &bt->wg);
      }
    break;
    case EV_LBUTTONUP:
      if (wg->flags & WG_TOGGLE)
      {
        if (!vec_select(&hw->ev.mouse_pos, &wg->c_pos, &wg->c_size))
          return;
        wg->flags ^= (bt->prev_flags & WG_PRESSED);
      }
      else
      {
        if (!(wg->flags & WG_PRESSED))
          return;
        wg->flags &= ~WG_PRESSED;
        wg->flags = (bt->prev_flags & WG_CHECKED) ? wg->flags & ~WG_CHECKED : wg->flags | WG_CHECKED;
      }
      wg_draw_blit(hw, wg, true, true, true);
      if (!(wg->flags & WG_PTRIG) && bt->pressed_cb)
        bt->pressed_cb(hw, &bt->wg);
    break;
    case EV_PAINT:
      wg_draw_obj(hw, wg);
    break;
    default:;
  }
}
