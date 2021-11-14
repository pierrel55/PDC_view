// edit box widget
#include <string.h>
#include <stdlib.h>
#include "widget.h"
#include "wg_priv.h"

// edit type
enum e_edb_type
{
  edb_text = 0,                   // text
  edb_num,                        // numerical value
  edb_num_spin,                   // numerical value + up/down spin buttons
};

// ----------------------------------------------
// formatted numbers

void num_desc_init(num_desc_t *nd, float init_value, float min_value, float max_value, float inc_step, int max_deci, int spc_digi, const char *help_text)
{
  nd->init_value = init_value;
  nd->min_value = min_value;
  nd->max_value = max_value;
  nd->inc_step = inc_step;
  nd->max_deci = max_deci;
  nd->spc_digi = spc_digi;
  nd->help_text = help_text;
}

// ----------------------------------------------

#define MAX_EDN_STR 32                           // max numeric edit string len

struct select_t
{
  int a;                                         // select origin
  int b;                                         // select end
};

typedef struct
{
  widget_t wg;
  enum e_edb_type type;                          // need to know behavehour
  text_t text;
  char *str_edit;
  bool mouse_select;                             // select using mouse
  int str_edit_buff_size;
  int str_cursor_ofs;                            // cursor position
  struct select_t select;                        // selected text
  edit_box_return_pressed_cb_t return_cb;        // user return callback
  struct                                         // numeric specific
  {
    num_desc_t *num_desc;                        // numerical descriptor value if used
    bool spc_do_unformat;                        // unformat required for keyboard edit
    char ed_str[MAX_EDN_STR];                    // numeric string
    float ed_value;                              // numeric value
    edit_box_num_value_changed_cb_t value_changed_cb;  // value changed user callback
  } num;
} wg_edit_box_t;

// --------------------------------------
// numeric formatted strings

// remove non numeric chars, return len
static int num_str_clean(char *str, const num_desc_t *nd)
{
  int i, l, dot = 0;
  for (l=0, i=0; str[i]; i++)
  {
    char c = str[i];
    if ((c >= '0') && (c <= '9'))
      str[l++] = c;
    else
    if (c == '-')
    {
      if (!l & (nd->min_value < 0))
        str[l++] = c;
    }
    else
    if (c == '.')
    {
      if (nd->max_deci && !dot++)
        str[l++] = c;
      else
        break;
    }
  }
  str[l] = 0;
  return l;
}

#define SPC_FORMAT                               // add spaces in big integers numbers

static void eb_refresh_blit_text(hwin_t *hw, wg_edit_box_t *eb);

#ifdef SPC_FORMAT
// warning: require integer value (no decimals)
static bool num_spc_str_format(char *str, int len_max, int spc_digi)
{
  bool res = false;
  int i, l;
  // remove non numeric chars
  for (l=0, i=0; str[i]; i++)
  {
    char c = str[i];
    if ((c >= '0') && (c <= '9'))
      str[l++] = c;
  }
  str[l] = 0;

  // add space chars every spc_digi
  i = l + (l/spc_digi);
  if (len_max > i)
  {
    int k;
    str[i] = 0;
    for (k=0, l--, i--; l >= 0; l--, i--)
    {
      str[i] = str[l];
      if (++k == spc_digi)
      {
        str[--i] = '\2';
        k = 0;
        res = true;                              // at least one to remove
      }
    }
  }
  return res;
}

static void edit_box_num_spc_format(wg_edit_box_t *eb)
{
  if (eb->num.num_desc && eb->num.num_desc->spc_digi && !eb->num.num_desc->max_deci)
    eb->num.spc_do_unformat = num_spc_str_format(eb->str_edit, eb->str_edit_buff_size, eb->num.num_desc->spc_digi);
}

#if 0
// remove small spaces
static void num_spc_str_unformat(char *str)
{
  char *d = str;
  for (;*str; str++)
    if (*str != '\2')
      *d++ = *str;
  *d = 0;
}

static void edit_box_num_spc_unformat(wg_edit_box_t *eb)
{
  if (eb->num.num_desc && eb->num.spc_do_unformat)
    num_spc_str_unformat(eb->str_edit);
}
#endif

static void edit_box_click_up_num_spc_unformat(hwin_t *hw, wg_edit_box_t *eb)
{
  if (eb->num.num_desc && eb->num.spc_do_unformat)
  {
    // remove spaces, adjust cursor pos and text selections, refresh display
    int i, j;
    char *s = eb->str_edit;
    for (i=0, j=0; s[i]; i++)
    {
      char c = s[i];
      if (c != '\2')
        s[j++] = c;
      else
      {
        if (eb->select.a > i)
          eb->select.a--;
        if (eb->select.b > i)
          eb->select.b--;
        if (eb->str_cursor_ofs > i)
        {
          eb->str_cursor_ofs--;
          hw->t_cursor.pos.x -= 2;
        }
      }
    }
    s[j] = 0;
    eb_refresh_blit_text(hw, eb);
  }
}
#else
  #define edit_box_num_spc_format(eb)
  #define edit_box_num_spc_unformat(eb)
  #define edit_box_click_up_num_spc_unformat(eb)
#endif

static void gcvt_fmt(float n, int max_deci, char *d);

// round and convert value, update string
static void edit_box_set_num_value(hwin_t *hw, wg_edit_box_t *eb, float n, bool refresh_blit, bool enable_value_changed_cb)
{
  const num_desc_t *nd = eb->num.num_desc;
  float n_prev = eb->num.ed_value;
  if (n > nd->max_value)
    n = nd->max_value;
  else
  if (n < nd->min_value)
    n = nd->min_value;
  else
  if (nd->max_deci)
  {
    // round n, avoid values like 0.999999, 0.500001
    #define EPSILON 0.000001f
    if (n > EPSILON)
      n = (int)((n * 10000.0f) + 0.5f)*0.0001f;
    else
    if (n < -EPSILON)
      n = (int)((n * 10000.0f) - 0.5f)*0.0001f;
    else
      n = 0;
  }
  W_ASSERT(eb->str_edit_buff_size == MAX_EDN_STR);
  gcvt_fmt(n, nd->max_deci, eb->str_edit);
  // eb->num.ed_value = (float)atof(eb->str_edit);  // read back, round to displayed value
  eb->num.ed_value = n;

#ifdef SPC_FORMAT
  eb->num.spc_do_unformat = eb->num.num_desc->spc_digi && !eb->num.num_desc->max_deci && num_spc_str_format(eb->str_edit, eb->str_edit_buff_size, eb->num.num_desc->spc_digi);
#endif

#if 0
  // disable up/down buttons if min/max reached
  if ((eb->type == edb_num_spin) && (eb->wg.flags & WG_ENABLE))
  {
    wg_enable_nr(eb->wg.i_wg      , eb->num.ed_value < nd->max_value, refresh_blit);  // bt up
    wg_enable_nr(eb->wg.i_wg->i_wg, eb->num.ed_value > nd->min_value, refresh_blit);  // bt dn
  }
#endif

  if (refresh_blit)
    eb_refresh_blit_text(hw, eb);

  if (enable_value_changed_cb && eb->num.value_changed_cb && (eb->num.ed_value != n_prev))
    eb->num.value_changed_cb(hw, &eb->wg, eb->num.ed_value);
}

// --------------------------------------

// selection origin and length
#define SEL_O (eb->select.a < eb->select.b) ? eb->select.a : eb->select.b
#define SEL_L (eb->select.a < eb->select.b) ? (eb->select.b - eb->select.a) : (eb->select.a - eb->select.b)

static void eb_draw_obj_text(hwin_t *hw, wg_edit_box_t *eb)
{
  bitmap_t *bm = &hw->cli_bm;
  text_t *t = eb->wg.dr_obj.text;
  if (!(eb->wg.flags & WG_ENABLE))               // disabled text
    bm_draw_string(bm, t->pos.x, t->pos.y, t->str, wgs.txt_dis_color, 0, t->font, -1);
  else
  if (eb->select.a == eb->select.b)              // no selection in text
    bm_draw_string(bm, t->pos.x, t->pos.y, t->str, t->color, t->aa_color, t->font, -1);
  else
  {
    int o = SEL_O;
    int l = SEL_L;
    char *s0 = eb->str_edit;
    char *s1 = eb->str_edit + o;
    char *s2 = eb->str_edit + o + l;
    int l0 = font_get_string_width_mc(s0, t->font, o);
    int l1 = font_get_string_width_mc(s1, t->font, l);
    int l2 = font_get_string_width(s2, t->font);

    bm_draw_string(bm, t->pos.x, t->pos.y, s0, t->color, t->aa_color, t->font, l0);
    bm_paint_rect(bm, t->pos.x+l0, t->pos.y, l1, t->font->dy+1, wgs.text_select.bk_color);
    bm_draw_string(bm, t->pos.x+l0, t->pos.y, s1, wgs.text_select.t_color, wgs.text_select.aa_color, t->font, l1);
    bm_draw_string(bm, t->pos.x+l0+l1, t->pos.y, s2, t->color, t->aa_color, t->font, l2);
  }
}

static void eb_refresh_blit_text(hwin_t *hw, wg_edit_box_t *eb)
{
  pix_t bk_col = (eb->wg.flags & WG_ENABLE) ? eb->wg.dr_obj.bk_color : wgs.clear_bk_color;
  bm_paint_rect(&hw->cli_bm, eb->wg.c_pos.x, eb->wg.c_pos.y, eb->wg.c_size.x, eb->wg.c_size.y, bk_col);
  eb_draw_obj_text(hw, eb);
  win_cli_blit_rect(hw, eb->wg.c_pos.x, eb->wg.c_pos.y, eb->wg.c_size.x, eb->wg.c_size.y);
  hw->t_cursor.vis = false;
}

static void eb_draw_obj(hwin_t *hw, wg_edit_box_t *eb)
{
  bitmap_t *bm = &hw->cli_bm;
  text_t *t;
  if ((eb->wg.dr_obj.bk_color != COL_ND) && (eb->wg.flags & WG_ENABLE))
    bm_paint_rect(bm, eb->wg.c_pos.x, eb->wg.c_pos.y, eb->wg.c_size.x, eb->wg.c_size.y, eb->wg.dr_obj.bk_color);

  t = eb->wg.dr_obj.text;
  t->pos.x = eb->wg.c_pos.x + TEXT_LMARGIN_PIXELS;
  t->pos.y = eb->wg.c_pos.y + ((eb->wg.c_size.y - t->size.y) >> 1); // always y center for text
  eb_draw_obj_text(hw, eb);

  wg_draw_c_frame(hw, &eb->wg);
  wg_draw_e_frame(hw, &eb->wg);
}

static void sel_update(hwin_t *hw, wg_edit_box_t *eb, bool do_select, struct select_t *s_prev)
{
  int xc;
  if (!do_select)
    eb->select.a = eb->str_cursor_ofs;
  eb->select.b = eb->str_cursor_ofs;
  if ((eb->select.a != eb->select.b) || !s_prev || (s_prev->a != s_prev->b))
    eb_refresh_blit_text(hw, eb);
  xc = eb->text.pos.x + font_get_string_width_mc(eb->str_edit, eb->text.font, eb->str_cursor_ofs) - 1;
  win_move_text_cursor(hw, xc, eb->text.pos.y);
}

// update mouse selection
static void mouse_sel_update(hwin_t *hw, wg_edit_box_t *eb, bool do_select)
{
  struct select_t select_prev = eb->select;
  int dx_ms = hw->ev.mouse_pos.x - eb->text.pos.x;
  font_get_str_click_ofs(eb->text.str, eb->text.font, dx_ms, &eb->str_cursor_ofs);
  sel_update(hw, eb, do_select, &select_prev);
}

static bool sel_delete(hwin_t *hw, wg_edit_box_t *eb)
{
  if (eb->select.a != eb->select.b)
  {
    int o = SEL_O;
    int l = SEL_L;
#if 0
    // GCC produce warning about this
    strcpy(&eb->str_edit[o], &eb->str_edit[o+l]);
#else
    char *s = &eb->str_edit[o+l];
    char *d = &eb->str_edit[o];
    while (*s)
      *d++ = *s++;
    *d = 0;
#endif
    eb->str_cursor_ofs = o;
    sel_update(hw, eb, false, NULL);
    return true;
  }
  return false;
}

// truncate string len to with of edit box
static int eb_truncate(wg_edit_box_t *eb)
{
  return font_string_truncate(eb->str_edit, eb->text.font, eb->wg.c_size.x - 2*TEXT_LMARGIN_PIXELS);
}

static void string_insert(hwin_t *hw, wg_edit_box_t *eb, const char *str)
{
  int l0 = eb->str_cursor_ofs;                   // size before cursor
  int l1 = strlen(str);                          // size to insert
  int lmax = eb->str_edit_buff_size - 1;         // max size + 0 end char
  char *s = &eb->str_edit[l0];
  if ((l0 + l1) > lmax)
  {
    l1 = lmax - l0;
    memcpy(s, str, l1);
    s[lmax] = 0;
  }
  else
  {
    int l2 = strlen(s);                          // size after cursor
    if ((l0 + l1 + l2) > lmax)
      l2 = lmax - (l0 + l1);
    memmove(s + l1, s, l2);
    memcpy(s, str, l1);
    s[l1+l2] = 0;
  }

  eb->str_cursor_ofs = l0 + l1;
  lmax = eb_truncate(eb);
  if (lmax < eb->str_cursor_ofs)
    eb->str_cursor_ofs = lmax;
  sel_update(hw, eb, false, NULL);
}

static void sel_copy(hwin_t *hw, wg_edit_box_t *eb)
{
  int l = SEL_L;
  if (l)
  {
    int o = SEL_O;
    char *s = &eb->str_edit[o];
    win_clip_copy(hw, s, l);
  }
}

static void sel_paste(hwin_t *hw, wg_edit_box_t *eb)
{
  int len;
  char *str = win_clip_paste(hw, &len);
  if (len)
    string_insert(hw, eb, str);
}

static void edit_kill_focus(hwin_t *hw, wg_edit_box_t *eb)
{
  hw->t_cursor.show = false;
  win_draw_text_cursor(hw, false);
  if (eb->num.num_desc)
  {
    eb->select.a = 0;
    eb->select.b = 0;
    num_str_clean(eb->str_edit, eb->num.num_desc);
    edit_box_set_num_value(hw, eb, (float)atof(eb->str_edit), true, true);
  }
  else
  if (eb->select.a != eb->select.b)
  {
    eb->select.a = 0;
    eb->select.b = 0;
    eb_refresh_blit_text(hw, eb);
  }
}

static void edit_key_event(hwin_t *hw, wg_edit_box_t *eb)
{
  struct win_event_t *ev = &hw->ev;
  // get ctrl and shift key state
  bool k_ctrl = key_states.key_pressed[KEY_CTRL];
  bool k_shift = key_states.key_pressed[KEY_SHIFT];
  struct select_t select_prev = eb->select;

  if (ev->type == EV_CHAR)
  {
    if (k_ctrl)
    {
      #define KCTRL(ch) ((ch - 'A') + 1)         // ctrl a..z code 1..26
      if (ev->key_char == KCTRL('A'))            // select all
      {
        eb->select.a = 0;
        while (eb->str_edit[eb->str_cursor_ofs])
          eb->str_cursor_ofs++;
        sel_update(hw, eb, true, NULL);
      }
      else
      if (ev->key_char == KCTRL('C'))            // copy
        sel_copy(hw, eb);
      else
      if (ev->key_char == KCTRL('X'))            // cut
      {
        sel_copy(hw, eb);
        sel_delete(hw, eb);
      }
      else
      if (ev->key_char == KCTRL('V'))            // paste
      {
        sel_delete(hw, eb);
        sel_paste(hw, eb);
      }
    }
    else
    {
      if (ev->key_char == KEY_RETURN)
      {
        edit_kill_focus(hw, eb);
        if (eb->return_cb)
          eb->return_cb(hw, &eb->wg);
      }
      else
      {
        sel_delete(hw, eb);
        if (ev->key_char >= ' ')
        {
          char str[2] = { ev->key_char, 0 };
          string_insert(hw, eb, str);
        }
      }
    }
  }
  else
  switch (ev->key_pressed)
  {
    case KEY_LEFT:
      if (eb->str_cursor_ofs)
      {
        eb->str_cursor_ofs--;
        if (k_ctrl)     // ctrl + left
        {
          while ((eb->str_cursor_ofs > 0) && (eb->str_edit[eb->str_cursor_ofs-1] == ' '))
            eb->str_cursor_ofs--;
          while ((eb->str_cursor_ofs > 0) && (eb->str_edit[eb->str_cursor_ofs-1] > ' '))
            eb->str_cursor_ofs--;
        }
      }
      sel_update(hw, eb, k_shift, &select_prev);
    break;
    case KEY_RIGHT:
      if (eb->str_edit[eb->str_cursor_ofs])
      {
        eb->str_cursor_ofs++;
        if (k_ctrl)     // ctrl + right
        {
          while (eb->str_edit[eb->str_cursor_ofs] > ' ')
            eb->str_cursor_ofs++;
          while (eb->str_edit[eb->str_cursor_ofs] == ' ')
            eb->str_cursor_ofs++;
        }
      }
      sel_update(hw, eb, k_shift, &select_prev);
    break;
    case KEY_END:
      while (eb->str_edit[eb->str_cursor_ofs])
        eb->str_cursor_ofs++;
      sel_update(hw, eb, k_shift, &select_prev);
    break;
    case KEY_HOME:
      eb->str_cursor_ofs = 0;
      sel_update(hw, eb, k_shift, &select_prev);
    break;
    case KEY_BACK:      // use back + suppr
      if (!sel_delete(hw, eb) && eb->str_cursor_ofs)
      {
        eb->select.a = eb->str_cursor_ofs - 1;
        eb->select.b = eb->str_cursor_ofs;
        sel_delete(hw, eb);
      }
    break;
    case KEY_SUPPR1:
      if (!sel_delete(hw, eb) && eb->str_edit[eb->str_cursor_ofs])
      {
        eb->select.a = eb->str_cursor_ofs;
        eb->select.b = eb->str_cursor_ofs + 1;
        sel_delete(hw, eb);
      }
    break;
  }
}

// edit box event proc
static void wg_edit_box_ev_proc(hwin_t *hw, widget_t *wg)
{
  struct win_event_t *ev = &hw->ev;
  wg_edit_box_t *eb = (wg_edit_box_t *)wg;
  switch (ev->type)
  {
    case EV_MOUSEMOVE:
      if (eb->mouse_select)
        mouse_sel_update(hw, eb, true);
    break;
    case EV_LBUTTONDOWN:
      eb->mouse_select = true;
      win_draw_text_cursor(hw, false);
      hw->t_cursor.show = false;
      mouse_sel_update(hw, eb, false);
    break;
    case EV_LBUTTONUP:
      eb->mouse_select = false;
      edit_box_click_up_num_spc_unformat(hw, eb);
      win_draw_text_cursor(hw, true);
      hw->t_cursor.show = true;
    break;
    case EV_LBUTTONDBLCLK:
    {
      struct select_t select_prev = eb->select;
      while ((eb->select.a > 0) && (eb->str_edit[eb->select.a-1] > ' '))
        eb->select.a--;
      while (eb->str_edit[eb->str_cursor_ofs] > ' ')
        eb->str_cursor_ofs++;
      sel_update(hw, eb, true, &select_prev);
    }
    break;
    case EV_BLINK:
      win_draw_text_cursor(hw, !hw->t_cursor.vis);
    break;
    case EV_SIZE:
      eb_truncate(eb);
    break;
    case EV_PAINT:
      eb_draw_obj(hw, eb);
    break;
    // key
    case EV_CHAR:
    case EV_KEYPRESSED:
      edit_key_event(hw, eb);
    break;
    // widget specific events
    case EV_WG_KILLFOCUS:
      edit_kill_focus(hw, eb);
    break;
    default:;
  }
}

static void edit_box_set_text(wg_edit_box_t *eb, char *str_edit, int str_edit_buff_size)
{
  W_ASSERT(str_edit && (str_edit_buff_size > 0));
  eb->str_edit = str_edit;
  eb->str_edit_buff_size = str_edit_buff_size;
  eb->select.a = 0;
  eb->select.b = 0;
  edit_box_num_spc_format(eb);
  text_init(&eb->text, str_edit, win_font_list[wgs.edit_box.text_font_id], wgs.edit_box.text_color, wgs.edit_box.text_aa_color);
}

void wg_edit_box_set_text(hwin_t *hw, widget_t *wg, char *str_edit, int str_edit_buff_size)
{
  GET_WG(edb, edit_box);
  font_string_truncate(str_edit, edb->text.font, edb->wg.c_size.x - 2*TEXT_LMARGIN_PIXELS);
  edit_box_set_text(edb, str_edit, str_edit_buff_size);
  eb_refresh_blit_text(hw, edb);
}

// init allocated edit box
static widget_t *init_edit_box(hwin_t *hw, wg_edit_box_t *eb, char *str_edit, int str_edit_buff_size, edit_box_return_pressed_cb_t return_cb)
{
  eb->return_cb = return_cb;
  edit_box_set_text(eb, str_edit, str_edit_buff_size);
  init_dr_obj(&eb->wg, e_obj_text, &eb->text, wgs.edit_box.text_bk_color, NULL);
  win_init_text_cursor(hw, eb->text.font->dy + 1, wgs.edit_box.cursor_color, wgs.edit_box.text_bk_color);
  return wg_init(&eb->wg, wg_edit_box_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS, &wgs.edit_box.e_frm);
}

widget_t *wg_init_edit_box(hwin_t *hw, char *str_edit, int str_edit_buff_size, edit_box_return_pressed_cb_t return_cb)
{
  C_NEW(eb, wg_edit_box_t);
  if (eb)
    return win_add_widget(hw, init_edit_box(hw, eb, str_edit, str_edit_buff_size, return_cb));
  return &wg_void;
}

// -------------------------------------------------------------
// edit box + integrated spin control to define numerical values
// -------------------------------------------------------------

#if 0

// do not require stdio but need more tests

#ifdef _CC_MSVC
#pragma warning( disable : 4996 )                // disable fcvt deprecated warning with visual studio
#endif

// format value returned by gcvt
static void gcvt_fmt(float n, int max_deci, char *d)
{
  // convert
  int dec, sign;
  char *s = fcvt(n, max_deci, &dec, &sign);
  if (sign)
    *d++ = '-';

  if (dec <= 0)                                  // ex: 0.5 => 5
    *d++ = '0';
  else
  for (;dec; dec--)                              // copy integer part
    *d++ = *s++;

  if (max_deci)                                  // add decimal part
  {
    *d++ = '.';
    for (;dec && max_deci; dec++, max_deci--)    // dec < 0 ex 0.005
      *d++ = '0';

    for (;*s && max_deci; max_deci--)
      *d++ = *s++;

    if (*(d-1) == '.')                           // convert 1. to 1.0 format
      *d++ = '0';
    else                                         // convert 1.00 to 1.0
    for (s = d-1; (*s == '0') && (*(s-1) != '.'); s--)
      *s = 0;
  }
  *d = 0;
}

#else

#include <stdio.h>

// format value
static void gcvt_fmt(float n, int max_deci, char *d)
{
  char fmt[8] = "%.0f";
  fmt[2] = '0'+max_deci;
  d += sprintf(d, fmt, n);
  if (max_deci)                                  // truncate ex 1.0000 => 1.0
  {
    for (d--; (*d == '0') && (*(d-1) != '.'); d--)
      *d = 0;
  }
}

#endif

// up/down button pressed, ajust value using step
static void edit_box_num_bt_cb(hwin_t *hw, widget_t *wg)
{
  wg_edit_box_t *eb;
  float d_value;

  if (wg->dr_obj.ch == wg_button_char.ar_u5)     // up button pressed
  {
    eb = (wg_edit_box_t *)wg->wg_parent;
    d_value = eb->num.num_desc->inc_step;
  }
  else
  {
    eb = (wg_edit_box_t *)wg->wg_parent->wg_parent;
    d_value = -eb->num.num_desc->inc_step;
  }

  edit_box_set_num_value(hw, eb, eb->num.ed_value + d_value, true, true);
}

widget_t *wg_init_edit_box_num(hwin_t *hw, num_desc_t *num_desc, bool add_spin, widget_t *cbo_button, edit_box_num_value_changed_cb_t value_changed_cb)
{
  W_ASSERT(num_desc);
  if (num_desc)
  {
    C_NEW(eb, wg_edit_box_t);
    if (eb)
    {
      if (init_edit_box(hw, eb, eb->num.ed_str, MAX_EDN_STR, NULL) != &wg_void)
      {
        eb->type = edb_num;                      // default type
        if (cbo_button)                          // insert combo button in edit box
          wg_add_child(&eb->wg, cbo_button, WG_IRIGHT);
        if (add_spin)                            // insert spin buttons in edit box
        {
          int flags = WG_SHOW|WG_ENABLE|WG_ENFOCUS|WG_SHIFT|WG_CENTER;
          widget_t *wg_spin = wg_add_child(get_wg_ctrl_button(wg_button_char.ar_u5, flags|WG_EVIWG, edit_box_num_bt_cb),
                                           get_wg_ctrl_button(wg_button_char.ar_d5, flags, edit_box_num_bt_cb), WG_IDOWN);
          wg_add_child(&eb->wg, wg_spin, WG_IRIGHT);
          eb->type = edb_num_spin;
        }
        eb->wg.help_text = num_desc->help_text;
        eb->num.num_desc = num_desc;
        eb->num.ed_value = num_desc->init_value;
        eb->num.value_changed_cb = value_changed_cb;
        edit_box_set_num_value(hw, eb, num_desc->init_value, false, false);
        return win_add_widget(hw, &eb->wg);
      }
      W_FREE(eb);
    }
  }
  return &wg_void;
}

void wg_edit_box_num_set_value(hwin_t *hw, widget_t *wg, float n, bool call_update_cb)
{
  GET_WG(eb, edit_box);
  if (n != eb->num.ed_value)
    edit_box_set_num_value(hw, eb, n, true, call_update_cb);
}

float wg_edit_box_num_get_value(widget_t *wg)
{
  GET_WG_RET(eb, edit_box, 0);
  return eb->num.ed_value;
}

bool wg_edit_box_num_update_min_max(hwin_t *hw, widget_t *wg, float min, float max)
{
  bool adjust;                                   // return true if num value clipped to min/max
  GET_WG_RET(eb, edit_box, false);
  eb->num.num_desc->min_value = min;
  eb->num.num_desc->max_value = max;
  adjust = (eb->num.ed_value < min) || (eb->num.ed_value > max);
  edit_box_set_num_value(hw, eb, eb->num.ed_value, true, false);
  return adjust;
}
