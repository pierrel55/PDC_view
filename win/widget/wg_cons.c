#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "widget.h"
#include "wg_priv.h"

// ----------------------------------------------
// text console
// ----------------------------------------------

#define LN_DY (co->font->dy + 0)                 // line dy size
#define P_NEXT(p) (((p+1) < co->tb.mem_end) ? (p+1) : co->tb.mem)

static void wg_console_out_ev_proc(hwin_t *hw, widget_t *wg);
static void sbarv_cb(hwin_t *hw, widget_t *wg, int move_ofs);

typedef struct
{
  widget_t wg;
  vec2i vis_ch;                                  // visible size in chars units
  vec2i cur_pos;                                 // cursor pos in char units
  struct                                         // text round buffer
  {
    char *mem;
    char *mem_end;
    char *rd;                                    // read index = begin of text
    char *wr;                                    // write index
    int n_lf;                                    // count of lf in buffer
    int sk_id;                                   // current line index of seek
  } tb;
  const font_t *font;                            // font
} wg_console_out_t;

// blit
static void console_blit(hwin_t *hw, wg_console_out_t *co)
{
  widget_t *wg = &co->wg;
  win_cli_blit_rect(hw, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
}

// put a char, update cursor
static int console_putchar(hwin_t *hw, wg_console_out_t *co, char c, bool scroll)
{
  widget_t *wg = &co->wg;
  bitmap_t *bm = &hw->cli_bm;
  vec2i *cp = &co->cur_pos;
  int bl = 0;                                    // blit mask

  if (c == '\n')                                 // new line
  {
    cp->x = 0;
    if ((cp->y + 1) < co->vis_ch.y)
      cp->y++;
    else
    {
      bl |= 2;
      if (scroll)                                // diplay with scroll bar
      {
        bm_rect_scroll_up(bm, &wg->c_pos, wg->c_size.x,
        co->vis_ch.y * LN_DY, LN_DY, wgs.co_out.text_bk_color);
        co->tb.sk_id++;
      }
    }
  }
  else
  if (c == '\t')                                 // tab
    cp->x += 2 + (cp->x & 1);
  else
  if (c == ' ')
    cp->x++;
  else
  if ((c > ' ') && (cp->x < co->vis_ch.x))
  {
    bl |= 1;
    bm_draw_char(bm, wg->c_pos.x + cp->x*co->font->dx, wg->c_pos.y + cp->y*LN_DY,
                 c, wgs.co_out.text_color, wgs.co_out.text_aa_color, co->font);
    cp->x++;
  }
  return bl;
}

// update scroll bar
static void update_sbar_v(hwin_t *hw, wg_console_out_t *co, bool blit)
{
  int r = (co->tb.n_lf - co->vis_ch.y + 1)*LN_DY;    // set variation range
  wg_vslider_set_range_ofs(hw, co->wg.i_wg->i_wg, r, r, false, blit);
}

// save string to buffer
static int str_save(wg_console_out_t *co, const char *s)
{
  int n = co->tb.n_lf;
  while (*s)
  {
    if (*s == '\n')
      n++;
    *co->tb.wr = *s++;
    co->tb.wr = P_NEXT(co->tb.wr);
    if (co->tb.wr == co->tb.rd)
    {
      if (*co->tb.rd == '\n')
        n--;
      co->tb.rd = P_NEXT(co->tb.rd);
    }
  }
  W_ASSERT(n >= 0);
  return n;
}

static char *seek_line_id(wg_console_out_t *co, int sk_id)
{
  char *s = co->tb.rd;
  co->tb.sk_id = 0;
  while ((co->tb.sk_id < sk_id) && (s != co->tb.wr))
  {
    if (*s == '\n')
      co->tb.sk_id++;
    s = P_NEXT(s);
  }
  return s;
}

static void console_refresh(hwin_t *hw, wg_console_out_t *co, int sk_id)
{
  widget_t *wg = &co->wg;
  char *s = seek_line_id(co, sk_id);
  co->cur_pos.x = 0;
  co->cur_pos.y = 0;
  bm_paint_rect(&hw->cli_bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y, wgs.co_out.text_bk_color);
  while (s != co->tb.wr)
  {
    if (console_putchar(hw, co, *s, false) & 2)
      break;
    s = P_NEXT(s);
  }
}

static void seek_end(hwin_t *hw, wg_console_out_t *co)
{
  if (co->tb.n_lf >= co->vis_ch.y)
  {
    int sk_end = (co->tb.n_lf + 1) - co->vis_ch.y;     // end position
    if (co->tb.sk_id != sk_end)
      console_refresh(hw, co, sk_end);
  }
}

void wg_console_print(hwin_t *hw, widget_t *wg, const char *str)
{
  vec2i cp0;
  int n_lf, bl = 0;
  GET_WG(co, console_out);
  cp0 = co->cur_pos;                             // cursor pos before print

  // win_lock_mutex(co->mutex_id);                  // avoid thread print text mix.

  if (    win_ctrl_resize()                      // mouse window resize in progress
       || wg_slider_control(co->wg.i_wg->i_wg))  // scroll bar control
    co->tb.n_lf = str_save(co, str);             // only save text and exit
  else
  {
    seek_end(hw, co);
    n_lf = str_save(co, str);
    if (n_lf != co->tb.n_lf)                     // line count changed
    {
      co->tb.n_lf = n_lf;
      update_sbar_v(hw, co, true);
    }

    while (*str)
      bl |= console_putchar(hw, co, *str++, true);

    if ((bl == 1) && (cp0.y == co->cur_pos.y))   // line/char blit only
      win_cli_blit_rect(hw, wg->c_pos.x + cp0.x*co->font->dx,
                            wg->c_pos.y + cp0.y*LN_DY, (co->cur_pos.x - cp0.x)*co->font->dx, LN_DY);
    else
    if (bl)
      console_blit(hw, co);                      // blit full
  }

  // win_unlock_mutex(co->mutex_id);
}

void wg_console_printf(hwin_t *hw, widget_t *wg, const char *fmt, ...)
{
  char str[1024];
  va_list args;
  GET_WG(co, console_out);
  va_start(args, fmt);
  vsnprintf(str, sizeof(str), fmt, args);
  va_end(args);
  wg_console_print(hw, wg, str);
}

void wg_console_clear(hwin_t *hw, widget_t *wg)
{
  GET_WG(co, console_out);
  co->tb.rd = co->tb.mem;
  co->tb.wr = co->tb.mem;
  *co->tb.wr = 0;
  co->tb.n_lf = 0;
  console_refresh(hw, co, 0);
  console_blit(hw, co);
  wg_vslider_set_range_ofs(hw, co->wg.i_wg->i_wg, 0, 0, false, true);
}

// copy content to clipboard
void wg_console_copy_to_clipboard(hwin_t *hw, widget_t *wg)
{
  char *mem, *s;
  int l;
  GET_WG(co, console_out);
  for (l=0, s=co->tb.rd; s != co->tb.wr; s = P_NEXT(s))
    l++;
  if (!l)
    return;
  mem = W_MALLOC(l+1);
  if (mem)
  {
    for (l=0, s=co->tb.rd; s != co->tb.wr; s = P_NEXT(s), l++)
      mem[l] = *s;
    mem[l] = 0;
    win_clip_copy(hw, mem, l);
    W_FREE(mem);
  }
}

static void wg_console_out_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_console_out_t *co = (wg_console_out_t *)wg;
  switch (hw->ev.type)
  {
    case EV_PAINT:
    {
      wg_draw_e_frame(hw, wg);
      console_refresh(hw, co, co->tb.sk_id);
    }
    break;
    case EV_MOUSEWHEEL:                          // transmit to scrool bar slider
      if (wg->i_wg)
      {
        widget_t *sl = wg->i_wg->i_wg;
        sl->ev_proc(hw, sl);
      }
    break;
    case EV_DESTROY:
      W_FREE(co->tb.mem);
    break;
    default:;
  }
}

static void wg_console_out_size_proc(struct widget_t *wg)
{
  wg_console_out_t *co = (wg_console_out_t *)wg;
  co->vis_ch.x = wg->c_size.x / co->font->dx;
  co->vis_ch.y = wg->c_size.y / LN_DY;
  if (co->tb.n_lf >= co->vis_ch.y)
  {
    int sk_end = (co->tb.n_lf + 1) - co->vis_ch.y;     // end position
    seek_line_id(co, sk_end);
  }
  update_sbar_v(NULL, co, false);
}

widget_t *wg_init_console_out(hwin_t *hw, int buff_size)
{
  C_NEW(co, wg_console_out_t);
  if (co)
  {
    if (buff_size < 25*80)                       // min size required
      buff_size = 25*80;

    co->tb.mem = (char *)W_MALLOC(buff_size);
    if (co->tb.mem)
    {
      co->tb.mem_end = co->tb.mem + buff_size;
      co->tb.rd = co->tb.mem;
      co->tb.wr = co->tb.mem;
      *co->tb.wr = 0;
      co->font = win_font_list[wgs.co_out.text_font_id];
      co->wg.sz_proc = wg_console_out_size_proc;
      // co->mutex_id = win_get_mutex_id();

      init_dr_obj(&co->wg, e_obj_none, NULL, COL_ND, NULL);
      wg_init(&co->wg, wg_console_out_ev_proc, WG_SHOW|WG_ENABLE|WG_ENFOCUS, &wgs.co_out.e_frm);
      
      return win_add_widget(hw, wg_add_child(&co->wg, wg_get_scroll_bar_v(0, 0, LN_DY, sbarv_cb), WG_VSB));
    }
    W_FREE(co);
  }
  return &wg_void;
}

// scroll bar call back
static void sbarv_cb(hwin_t *hw, widget_t *wg_sl, int move_ofs)
{
  int sk_id;
  widget_t *wg = wg_sl->wg_parent->wg_parent;
  GET_WG(co, console_out);

  sk_id = move_ofs / LN_DY;
  if (sk_id != co->tb.sk_id)
  {
    console_refresh(hw, co, sk_id);
    console_blit(hw, co);
  }
}
