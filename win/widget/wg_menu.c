// menu. use new window
#include <string.h>
#include <stdlib.h>
#include "widget.h"

#define MENU_LRMARGIN 2

struct win_menu_t
{
  int str_count;
  const char **str_list;
  vec2i str_ofs;
  int item_dy;
  int item_sel_id;                     // -1 if none
  const menu_style_t *style;
  win_menu_select_cb_t select_cb;
  hwin_t *select_hw;
  void *user_ptr;                      // user pointer
};

static void menu_draw_item(hwin_t *hw, struct win_menu_t *m, int id, bool select, bool blit)
{
  const char *str = m->str_list[id];
  bitmap_t *bm = &hw->cli_bm;
  int y = id*m->item_dy;

  if (str[0] == '\n')
  {
    bm_paint_rect(bm, 0, y, bm->size.x, m->item_dy, m->style->u_col.bk_color);
    y += m->item_dy >> 1;
    bm_draw_line_h(bm, 4, y  , bm->size.x - 8, m->style->h_line[0]);
    bm_draw_line_h(bm, 4, y+1, bm->size.x - 8, m->style->h_line[1]);
  }
  else
  {
    const font_t *fnt = win_font_list[m->style->text_font_id];
    const text_color_t *tc = select ? &m->style->s_col : &m->style->u_col;
    int x, w_max;
    if (m->str_ofs.x >= 0)               // fixed left margin defined
    {
      w_max = bm->size.x - m->str_ofs.x - MENU_LRMARGIN;
      x = m->str_ofs.x;
    }
    else                                 // centered
    {
      int w = font_get_string_width(str, fnt);
      w_max = bm->size.x - 2*MENU_LRMARGIN;
      if (w < w_max)
        x = ((bm->size.x - w) >> 1);
      else
        x = MENU_LRMARGIN;
    }

    bm_paint_rect(bm, 0, y, bm->size.x, m->item_dy, tc->bk_color);
    bm_draw_string_truncate(bm, x, y+m->str_ofs.y, str, tc->t_color, tc->aa_color, fnt, w_max);
    if (blit)
      win_cli_blit_rect(hw, 0, y, bm->size.x, m->item_dy);
  }
}

static void menu_draw_all(hwin_t *hw, struct win_menu_t *m)
{
  int i;
  for (i=0; i<m->str_count; i++)
    menu_draw_item(hw, m, i, i == m->item_sel_id, false);
}

// return id of mouse selected item
static int item_select_id(hwin_t *hw, struct win_menu_t *m)
{
  vec2i *ms = &hw->ev.mouse_pos;

  // test if cursor into window (can be captured)
  if (    ((unsigned int)ms->x < (unsigned int)hw->cli_bm.size.x)
       && ((unsigned int)ms->y < (unsigned int)hw->cli_bm.size.y))
  {
    int id = ms->y / m->item_dy;
    if (m->str_list[id][0] != '\n')
      return id;
  }
  return -1;
}

static void menu_select_item(hwin_t *hw, struct win_menu_t *m)
{
  int sel_id = item_select_id(hw, m);
  if (sel_id >= 0)
  {
    if (sel_id != m->item_sel_id)
    {
      if (m->item_sel_id >= 0)
        menu_draw_item(hw, m, m->item_sel_id, false, true);
      menu_draw_item(hw, m, sel_id, true, true);
      m->item_sel_id = sel_id;
    }
  }
  else
  if (m->item_sel_id >= 0)
  {
    menu_draw_item(hw, m, m->item_sel_id, false, true);
    m->item_sel_id = -1;
  }
}

// selection is done
static void menu_click_up(hwin_t *hw, struct win_menu_t *m)
{
  win_menu_select_cb_t select_cb = m->select_cb;
  hwin_t *select_hw = m->select_hw;
  void *user_ptr = m->user_ptr;
  int select_id = item_select_id(hw, m);
  win_close(hw);
  if ((select_id >= 0) && select_cb)
    select_cb(select_hw, user_ptr, select_id);
}

static void menu_event_proc(hwin_t *hw)
{
  struct win_menu_t *m = (struct win_menu_t *)hw->user_ptr;
  switch (hw->ev.type)
  {
    case EV_MOUSEMOVE:
      menu_select_item(hw, m);
    break;
    case EV_LBUTTONUP:
      menu_click_up(hw, m);
    break;
    case EV_PAINT:
      menu_draw_all(hw, m);
    break;
    case EV_DESTROY:
      W_FREE(hw->user_ptr);
    break;
    case EV_LEAVE:
    if (m->item_sel_id >= 0)
    {
      menu_draw_item(hw, m, m->item_sel_id, false, true);
      m->item_sel_id = -1;
    }
    break;
    default:;
  };
}

bool wg_menu_init_ex(int x, int y, int dx_min, int dx_max, int w_expand,
                     enum e_win_border_type border_type, const menu_style_t *menu_style,
                     const char **str_list, int str_count, int l_margin, int dy_line, win_menu_select_cb_t select_cb, hwin_t *parent, void *user_ptr)
{
  int i, bw, w_max;
  const font_t *fnt;
  vec2i w_pos, c_pos, c_size;
  const vec2i *scr_size = win_get_screen_size();
  C_NEW(m, struct win_menu_t);
  if (!m)
    return false;

  W_ASSERT(parent && dx_max);
  if (!parent)
  {
    W_FREE(m);
    return false;
  }

  fnt = win_font_list[menu_style->text_font_id];
  m->str_count   = str_count;
  m->str_list    = str_list;
  m->str_ofs.x   = l_margin;
  m->str_ofs.y   = dy_line >> 1;
  m->item_dy     = fnt->dy + dy_line;
  m->item_sel_id = -1;
  m->style       = menu_style;
  m->select_cb   = select_cb;
  m->select_hw   = parent;
  m->user_ptr    = user_ptr;

  // define new window position
  win_get_cli_pos(parent, &c_pos);               // get parent client origin position on screen
  w_pos.x = c_pos.x + x;
  w_pos.y = c_pos.y + y;

  // define window width
  w_max = dx_max + w_expand;
  if (dx_min == w_max)                           // size is fixed
    c_size.x = w_max;
  else
  {
    c_size.x = 0;
    for (i=0; i<str_count; i++)
    {
      int dx = font_get_string_width(str_list[i], fnt);
      if (dx > c_size.x)
        c_size.x = dx;
    }
    if (l_margin >= 0)
      c_size.x += (l_margin + MENU_LRMARGIN);    // user defined left margin
    else
      c_size.x -= 2*l_margin;                    // centered

    if (dx_min && (c_size.x < dx_min))
      c_size.x = dx_min;
    else
    if (dx_max && (c_size.x > dx_max))           // size is limited
    {
      if (c_size.x >= w_max)
        c_size.x = w_max;
      w_pos.x -= (c_size.x - dx_max) >> 1;       // center on expand range
    }
  }

  c_size.y = str_count*(m->item_dy);

  // ensure visible position
  bw = win_get_border_width(border_type)*2;
  if ((w_pos.x + c_size.x + bw) > scr_size->x)
    w_pos.x = scr_size->x - (c_size.x + bw);
  if ((w_pos.y + c_size.y + bw) > scr_size->y)
    w_pos.y = scr_size->y - (c_size.y + bw);

  if (win_create_popup(&w_pos, &c_size, border_type, menu_event_proc, parent, m))
    return true;

  W_FREE(m);
  return false;
}

bool wg_menu_ctx_init(int x, int y, int dx_max, const char **str_list, int str_count, win_menu_select_cb_t select_cb, hwin_t *parent, void *user_ptr)
{
  return wg_menu_init_ex(x, y, dx_max, dx_max, 0, win_border_pu2, &wgs.menu_ctx, str_list, str_count, 16, 6, select_cb, parent, user_ptr);
}
