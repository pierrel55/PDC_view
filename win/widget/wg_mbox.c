// message box
#include <string.h>
#include <stdlib.h>
#include "widget.h"

#define MB_MAX_BT 3                    // max buttons

static const struct
{
  int n_bt;
  const char *label[MB_MAX_BT];
  enum e_msg_box_res res[MB_MAX_BT];
} mb_inf[4] =
{
  { 1 , {  "OK"                     }, { msg_res_ok } },
  { 2 , {  "OK", "CANCEL"           }, { msg_res_ok, msg_res_cancel } },
  { 2 , { "YES",     "NO"           }, { msg_res_yes, msg_res_no } },
  { 3 , { "YES",     "NO", "CANCEL" }, { msg_res_yes, msg_res_no, msg_res_cancel } },
};

#define MB_LINE_DY 2                   // space between 2 lines

#define TXT_LRMARGIN 16                // text margin left right
#define TXT_UDMARGIN 25                // text margin up down
#define BT_LMARGIN   46                // button left margin
#define BT_DX_SPACE   8                // x space between buttons
#define BT_DY_SPACE  40                // reserved y space for buttons
#define BT_X_SIZE    80                // buttons x size
#define BT_Y_SIZE    22                // buttons y size

struct msg_box_t
{
  enum e_msg_box_type box_type;
  const char *text;
  vec2i msg_size;
  msg_box_resume_t resume_proc;
  hwin_t *resume_hw;
  widget_t *bt[MB_MAX_BT];
};

// send resume answer
static void mb_bt_pressed(hwin_t *hw, widget_t *wg)
{
  struct msg_box_t *mb = (struct msg_box_t *)hw->user_ptr;
  msg_box_resume_t resume_proc = mb->resume_proc;     // backup before to destroy window
  hwin_t *resume_hw = mb->resume_hw;
  enum e_msg_box_res res = msg_res_cancel;
  if (mb->resume_proc)
  {
    int bt_id = (wg == mb->bt[0]) ? 0 : (wg == mb->bt[1]) ? 1 : 2;
    res = mb_inf[mb->box_type].res[bt_id];
  }
  win_close(hw);
  if (resume_proc)
    resume_proc(resume_hw, res);
}

static bool init_msg_box(hwin_t *hw)
{
  struct msg_box_t *mb = (struct msg_box_t *)hw->user_ptr;
  int i, n_bt = mb_inf[mb->box_type].n_bt;
  for (i=0; i<n_bt; i++)
  {
    mb->bt[i] = wg_init_text_button(hw, mb_inf[mb->box_type].label[i], mb_bt_pressed);
    if (!mb->bt[i])
      return false;
  }
  return true;
}

// size/place buttons
static void size_msg_box(hwin_t *hw)
{
  struct msg_box_t *mb = (struct msg_box_t *)hw->user_ptr;
  const vec2i *cli_size = &hw->cli_bm.size;
  int i, n_bt = mb_inf[mb->box_type].n_bt;
  int x = cli_size->x - n_bt * (BT_DX_SPACE + BT_X_SIZE);
  int y = cli_size->y - BT_DY_SPACE + (BT_DY_SPACE - BT_Y_SIZE)/2;
  for (i=0; i<n_bt; i++)
  {
    widget_t *wg = mb->bt[i];
    wg_set_size(wg, BT_X_SIZE, BT_Y_SIZE);
    wg_set_pos_x(wg, x);
    wg_set_pos_y(wg, y);
    x += BT_X_SIZE + BT_DX_SPACE;
  }
}

// paint background and text
static void paint_msg_box(hwin_t *hw)
{
  struct msg_box_t *mb = (struct msg_box_t *)hw->user_ptr;
  vec2i t_pos = { TXT_LRMARGIN, TXT_UDMARGIN };
  bitmap_t *bm = &hw->cli_bm;

  // clear background
  bm_paint_rect(bm, 0, 0, bm->size.x, bm->size.y - BT_DY_SPACE, wgs.msg_box.msg_color.bk_color);
  bm_paint_rect(bm, 0, bm->size.y - BT_DY_SPACE, bm->size.x, BT_DY_SPACE, wgs.clear_bk_color);

  // draw text
  font_draw_text_rect(bm, mb->text,
                      win_font_list[wgs.msg_box.text_font_id],
                      wgs.msg_box.msg_color.t_color,
                      wgs.msg_box.msg_color.aa_color,
                      &t_pos, MB_LINE_DY, &mb->msg_size);
}

static void win_msg_box_msg_proc(hwin_t *hw)
{
  switch (hw->ev.type)
  {
    case EV_CREATE:
      init_msg_box(hw);
    break;
    case EV_SIZE:
      size_msg_box(hw);
    break;
    case EV_PAINT:
      paint_msg_box(hw);
    break;
    case EV_DESTROY:
      W_FREE(hw->user_ptr);
    break;
    default:;
  }
  widget_dispatch_events(hw);
}

hwin_t *wg_message_box(const char *message, const char *win_title, enum e_msg_box_type box_type, hwin_t *parent, msg_box_resume_t resume_proc)
{
  vec2i c_size, max_c_size, max_t_size, ct_size;
  const vec2i *scr_size;
  hwin_t *hw;
  C_NEW(mb, struct msg_box_t);
  if (!mb)
    return NULL;

  // evaluate min requested client size with one line of text.
  c_size.x = BT_LMARGIN + mb_inf[box_type].n_bt * (BT_X_SIZE+BT_DX_SPACE);
  c_size.y = TXT_UDMARGIN*2 + 20 + BT_DY_SPACE;

  // get max reasonable client size
  scr_size = win_get_screen_size();
  max_c_size.x = (scr_size->x*3)/4;
  max_c_size.y = (scr_size->y*3)/4;
  if ((max_c_size.x < c_size.x) || (max_c_size.y < c_size.y))  // screen too small ?
  {
    W_FREE(mb);
    return NULL;
  }

  mb->box_type = box_type;
  mb->text = message;
  mb->resume_proc = resume_proc;
  mb->resume_hw = parent;

  // get max text size
  max_t_size.x = max_c_size.x - 2*TXT_LRMARGIN;
  max_t_size.y = max_c_size.y - 2*TXT_UDMARGIN - BT_DY_SPACE;

  // get text message size
  font_eval_text_rect(message, win_font_list[wgs.msg_box.text_font_id], MB_LINE_DY, &mb->msg_size);

  // truncate if too big
  if (mb->msg_size.x > max_t_size.x)
    mb->msg_size.x = max_t_size.x;
  if (mb->msg_size.y > max_t_size.y)
    mb->msg_size.y = max_t_size.y;

  // define client size for text
  ct_size.x = mb->msg_size.x + 2*TXT_LRMARGIN;
  ct_size.y = mb->msg_size.y + 2*TXT_UDMARGIN + BT_DY_SPACE;

  // update final client size
  if (ct_size.x > c_size.x)
    c_size.x = ct_size.x;
  if (ct_size.y > c_size.y)
    c_size.y = ct_size.y;

  hw = win_create(win_title, NULL, &c_size, &c_size, &c_size, win_msg_box_msg_proc, parent, mb);
  if (!hw)
    W_FREE(mb);
  return hw;
}
