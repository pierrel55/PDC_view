// blocks composed of several widgets often used functions. (widget macro)
#include <stdlib.h>
#include "widget.h"
#include "wg_priv.h"

// --------------------------------------------------
// label + numeric edit box + integrated spin control
// --------------------------------------------------

static wledns_t wgb_ledns_void = { &wg_void, &wg_void };

// init composed widget, label + numeric edit box + spin
wledns_t *wgb_ledns_init(hwin_t *hw, const char *label, float init_value, float min_value, float max_value,
                          float inc_step, int max_deci, int spc_digi, edit_box_num_value_changed_cb_t value_changed_cb, const char *help_text)
{
  C_NEW(eds, wledns_t);
  if (eds)
  {
    num_desc_init(&eds->nd, init_value, min_value, max_value, inc_step, max_deci, spc_digi, help_text);
    eds->lbl = wg_init_text(hw, label);
    eds->lbl->help_text = help_text;
    eds->ed = wg_init_edit_box_num(hw, &eds->nd, true, NULL, value_changed_cb);
    eds->ed->help_text = help_text;
    eds->ed->blk_ptr = eds;                      // required for mem free
    return eds;
  }
  return &wgb_ledns_void;
}

void wgb_ledns_enable_init(wledns_t *wg, bool enable)
{
  wg_enable_init(wg->lbl, enable);
  wg_enable_init(wg->ed, enable);
}

void wgb_ledns_enable(hwin_t *hw, wledns_t *wg, bool enable)
{
  wg_enable(hw, wg->lbl, enable);
  wg_enable(hw, wg->ed, enable);
}

void wlb_place_ledns(wledns_t *eds, int wx_eds)
{
  wl_place_2h_w1(eds->lbl, eds->ed, wx_eds, wg_place_margin.wg_dy.eds);
}

// -----------------------------------------------------------------------------
// label + numeric edit box + integrated spin control + numeric value list combo
// -----------------------------------------------------------------------------

static wlednsc_t wgb_lednsc_void = { &wg_void, &wg_void };

static void lednsc_menu_select_cb(hwin_t *hw, void *user_ptr, int sel_id)
{
  wlednsc_t *ed_cbo = (wlednsc_t *)user_ptr;
  const char *str = ed_cbo->cbo_menu.str_list[sel_id];
  float num = (float)atof(str);
  wg_edit_box_num_set_value(hw, ed_cbo->ed, num, true);
}

static void wgb_lednsc_bt_pressed_cb(hwin_t *hw, widget_t *wg)
{
  wlednsc_t *ed_cbo = (wlednsc_t *)wg->wg_parent->blk_ptr;
  W_ASSERT(wg->wg_parent == ed_cbo->ed);
  if (ed_cbo->cbo_menu.str_count)
  {
    int menu_wx = ed_cbo->ed->e_size.x - 2*win_get_border_width(win_border_pu1);
    int menu_x = ed_cbo->ed->e_pos.x;
    int menu_y = wg->e_pos.y + wg->e_size.y;

    wg_menu_init_ex(menu_x, menu_y, menu_wx, menu_wx, 
                    80, win_border_pu1, &wgs.menu_combo,
                    ed_cbo->cbo_menu.str_list, ed_cbo->cbo_menu.str_count, 
                    4, 2, lednsc_menu_select_cb, hw, ed_cbo);
  }
}

// init composed widget, label + numeric edit box + spin
wlednsc_t *wgb_lednsc_init(hwin_t *hw, const char *label, float init_value, float min_value, float max_value,
                          float inc_step, int max_deci, int spc_digi,
                          const char **cbo_str_list, int cbo_str_count,
                          edit_box_num_value_changed_cb_t value_changed_cb, const char *help_text)
{
  C_NEW(eds, wlednsc_t);
  if (eds)
  {
    // combo button
    widget_t *bt_cbo = NULL;
    if (cbo_str_list && cbo_str_count)
    {
      int flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_PTRIG|WG_IGFOCUS|WG_SHIFT|WG_CENTER;
      bt_cbo = get_wg_ctrl_button(wg_button_char.ar_d7, flags, wgb_lednsc_bt_pressed_cb);
      if (bt_cbo == &wg_void)
        bt_cbo = NULL;
      else
      {
        eds->cbo_menu.str_list = cbo_str_list;
        eds->cbo_menu.str_count = cbo_str_count;
      }
    }
    num_desc_init(&eds->nd, init_value, min_value, max_value, inc_step, max_deci, spc_digi, help_text);
    eds->lbl = wg_init_text(hw, label);
    eds->lbl->help_text = help_text;
    eds->ed = wg_init_edit_box_num(hw, &eds->nd, true, bt_cbo, value_changed_cb);
    eds->ed->help_text = help_text;
    eds->ed->blk_ptr = eds;                      // required for mem free and combo button cb
    return eds;
  }
  return &wgb_lednsc_void;
}

void wlb_place_lednsc(wlednsc_t *eds, int wx_eds)
{
  wl_place_2h_w1(eds->lbl, eds->ed, wx_eds, wg_place_margin.wg_dy.eds);
}
