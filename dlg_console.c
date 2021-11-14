#include <stdio.h>
#include "win/widget/widget.h"
#include "dlg_console.h"

// frame messages console
struct dlg_console_t
{
  hwin_t *hw;
  widget_t *frm;
  widget_t *console_out;
  widget_t *bt_clear;
  widget_t *bt_copy;
  widget_t *bt_help;
  widget_t *bt_exit;
};

static struct dlg_console_t console = { 0 };

static void cons_bt_cb(hwin_t *hwin, widget_t *wg);

void dlg_console_init_widgets(hwin_t *hw, const font_t *font, pix_t color, pix_t aa_color)
{
  console.hw = hw;
  console.frm = wg_init_frame_ex(hw, "Messages", font, color, aa_color);

#if 1
  // change console default style
  wgs.co_out.text_bk_color = COL_RGB(219, 217, 207);
  wgs.co_out.text_color    = COL_RGB(0, 0, 0);
#endif

  // wgs.co_out.text_aa_color = COL_RGB(0, 0, 0);
  wgs.co_out.text_aa_color = font_get_aa_color(wgs.co_out.text_color, wgs.co_out.text_bk_color, 12);

  console.console_out       = wg_init_console_out(hw, 128000);
  console.bt_clear          = wg_init_text_button(hw, "Clear", cons_bt_cb);
  console.bt_clear->help_text =
    "Clear console content.";
  console.bt_copy           = wg_init_text_button(hw, "Copy", cons_bt_cb);
  console.bt_copy->help_text =
    "Copy console content to clipboard.";
  console.bt_help           = wg_init_text_button(hw, "Help", cons_bt_cb);
  console.bt_help->help_text =
    "Display help informations.";
  console.bt_exit           = wg_init_text_button(hw, "Exit", cons_bt_cb);
  console.bt_exit->help_text =
    "Exit program.";
}

// place widgets
void dlg_console_place_widget(vec2i *pos, vec2i *size)
{
  #define CONS_BT_SIZE 40
  #define CONS_BT_MARGIN 6
  wg_set_pos_size(console.frm, pos->x, pos->y, size->x, size->y);

  pos->x += wg_place_margin.frm.dx_push;
  pos->y += wg_place_margin.frm.dy_push;
  size->x -= wg_place_margin.frm.dx_push + CONS_BT_SIZE + CONS_BT_MARGIN*2 + 2;
  size->y -= wg_place_margin.frm.dy_push + wg_place_margin.frm.dy_pop_frm + 2;
  wg_set_pos_size(console.console_out, pos->x, pos->y, size->x, size->y);

  pos->x += size->x + CONS_BT_MARGIN;
  wg_set_pos_size(console.bt_clear, pos->x, pos->y, CONS_BT_SIZE, wg_place_margin.wg_dy.but);
  pos->y += wg_place_margin.wg_dy.but + 6;
  wg_set_pos_size(console.bt_copy,  pos->x, pos->y, CONS_BT_SIZE, wg_place_margin.wg_dy.but);
  pos->y += wg_place_margin.wg_dy.but + 20;
  wg_set_pos_size(console.bt_help,  pos->x, pos->y, CONS_BT_SIZE, wg_place_margin.wg_dy.but);
  pos->y += wg_place_margin.wg_dy.but + 6;
  wg_set_pos_size(console.bt_exit,  pos->x, pos->y, CONS_BT_SIZE, wg_place_margin.wg_dy.but);
}

// ----------------------------------------------
// help message

static char help_msg[1024];

static void show_help_win(hwin_t *hw)
{
  sprintf(help_msg,
  "PDC view v1.0\n\n"

  "This program allow to find configurations that can produce parametric\n"
  "down conversion of type 1 and 2.\n\n"
  "It can also display obtained parametric conversion cones.\n\n"
  "Help:\n\n"
  "It is provided by tooltips on most controls of the interface.\n"
  "(Appears after 1s when the mouse cursor is immobilized over a control).\n\n"
  "Crystals:\n"
  "Exact references of crystals can be found as comments in source code 'crystals.c'\n\n"
  "More information is available on github here: www.github.com/pierrel55/PDC_view\n\n"
  "%s\n\n",  guiXP_version);
  wg_message_box(help_msg, "Help", msg_box_ok, hw, NULL);
}

// buttons cb
static void cons_bt_cb(hwin_t *hw, widget_t *wg)
{
  if      (wg == console.bt_clear)  wg_console_clear(hw, console.console_out);
  else if (wg == console.bt_copy)   wg_console_copy_to_clipboard(hw, console.console_out);
  else if (wg == console.bt_help)   show_help_win(hw);
  else if (wg == console.bt_exit)
    win_close(hw);
}

// ----------------------------------------------
// print

#include <stdarg.h>

// print on console using console widget
void co_printf(const char *fmt, ...)
{
  if (console.hw && console.frm->c_size.x)       // ensure is initialized
  {
    char str[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(str, sizeof(str), fmt, args);
    wg_console_print(console.hw, console.console_out, str);
    va_end(args);
  }
}
