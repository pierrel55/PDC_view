// window manager.
// manage stack of empty windows (no frame) on a desktop (screen)

#include <string.h>
#include <stdlib.h>
#include "screen.h"

#define POS_MIN_VIS_LR 8                         // min left/righ visible margin pixels when set position
#define POS_MIN_VIS_BOTTOM 64                    // min bottom margin pixels when set position (status bar)
#define SIZE_MINIMIZED_X 160                     // x size when window is minimized
#define MAX_BORDER_WIDTH 5                       // window borders max width

// title bar style
struct title_bar_style_t
{
  int size_y;                                    // title bar y size
  enum e_font_id title_font_id;                  // font used to draw title bar name
  struct
  {
    pix_t text;                                  // text color
    pix_t background;                            // background color
  } text_color[2];                               // color with no focus/focus
  pix_t bar_gradient[2];                         // title bar background right color
  bool bk_gradient;                              // use color gradient for background (XP style)
};

struct help_bubble_style_t                       // help style
{
  enum e_font_id text_font_id;                   // font
  pix_t t_color;                                 // text color
  pix_t aa_color;                                // text aa color
  pix_t bk_color;                                // background color
};

// borders style
struct border_style_t
{
  int width;
  pix_t color_tl[MAX_BORDER_WIDTH];
  pix_t color_br[MAX_BORDER_WIDTH];
};

// window style config (decorations)
struct win_style_t
{
  const struct border_style_t *border[MAX_BORDER_TYPE]; // frame border list
  struct title_bar_style_t title_bar;
  struct help_bubble_style_t h_bubble;
};

static struct win_style_t w_style = { 0 };

// ------------------------------------------------
// datas for default style buttons (16 * 14 pixels)
// pressed and unpressed bitmap for each button
// note: y size must be constant for all buttons

static const pix_t bt_close_u_pic[] =
{
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x404040,
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,
};

static const pix_t bt_close_p_pic[] =
{
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0xffffff,
  0x404040,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
};

static const pix_t bt_maxi_u_pic[] =
{
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x404040,
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,
};

static const pix_t bt_maxi_p_pic[] =
{
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0xffffff,
  0x404040,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
};

static const pix_t bt_mini_u_pic[] =
{
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x404040,
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,
};

static const pix_t bt_mini_p_pic[] =
{
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0xffffff,
  0x404040,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
};

static const pix_t bt_rest_u_pic[] =
{
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x808080,0x404040,
  0xffffff,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x404040,
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,
};

static const pix_t bt_rest_p_pic[] =
{
  0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0x404040,0xffffff,
  0x404040,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0x808080,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0x808080,0xc8d0d4,0xc8d0d4,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0x404040,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xc8d0d4,0xffffff,
  0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,
};

// title bar buttons images
struct tb_buttons_img_t
{
  struct bt_img_t bt_close;
  struct bt_img_t bt_maximize;
  struct bt_img_t bt_minimize;
  struct bt_img_t bt_restore;
};

static struct tb_buttons_img_t frame_tb_buttons_img;

static bool frame_init_tb_buttons(enum e_win_style style)
{
  // note, override const for button datas, do not try to modify bitmap
  bm_init(&frame_tb_buttons_img.bt_close.bm_u   , 16, 14, 16, (pix_t *)bt_close_u_pic);
  bm_init(&frame_tb_buttons_img.bt_close.bm_p   , 16, 14, 16, (pix_t *)bt_close_p_pic);
  bm_init(&frame_tb_buttons_img.bt_maximize.bm_u, 16, 14, 16, (pix_t *)bt_maxi_u_pic);
  bm_init(&frame_tb_buttons_img.bt_maximize.bm_p, 16, 14, 16, (pix_t *)bt_maxi_p_pic);
  bm_init(&frame_tb_buttons_img.bt_minimize.bm_u, 16, 14, 16, (pix_t *)bt_mini_u_pic);
  bm_init(&frame_tb_buttons_img.bt_minimize.bm_p, 16, 14, 16, (pix_t *)bt_mini_p_pic);
  bm_init(&frame_tb_buttons_img.bt_restore.bm_u , 16, 14, 16, (pix_t *)bt_rest_u_pic);
  bm_init(&frame_tb_buttons_img.bt_restore.bm_p , 16, 14, 16, (pix_t *)bt_rest_p_pic);
  return true;
}

static void frame_free_tb_buttons(void)
{
}

// ----------------------------------------------
// style init

// --------------
// borders styles

static const struct border_style_t border_style_w0 =
{
  0,
  { COL_RGB(0, 0, 0) },
  { COL_RGB(0, 0, 0) },
};

static const struct border_style_t border_style_w1 =
{
  1,
  { COL_RGB(200, 200, 200) },
  { COL_RGB(200, 200, 200) },
};

static const struct border_style_t border_style_w1_xp =
{
  1,
  { COL_RGB(0, 0, 0) },
  { COL_RGB(0, 0, 0) },
};

#if 0
// unused
static const struct border_style_t border_style_w2 =
{
  2,
  { COL_RGB(200, 200, 200), COL_RGB(240, 240, 240) },
  { COL_RGB(200, 200, 200), COL_RGB(140, 140, 140) },
};
#endif

static const struct border_style_t border_style_w2_xp =
{
  2,
  { COL_RGB(212, 208, 200), COL_RGB(255, 255, 255) },
  { COL_RGB( 64,  64,  64), COL_RGB(128, 128, 128) },
};

static const struct border_style_t border_style_w3 =
{
  3,
  { COL_RGB(212, 208, 200), COL_RGB(255, 255, 255), COL_RGB(212, 208, 200) },
  { COL_RGB( 64,  64,  64), COL_RGB(128, 128, 128), COL_RGB(212, 208, 200) },
};

static const struct border_style_t border_style_w4 =
{
  4,
  { COL_RGB(212, 208, 200), COL_RGB(255, 255, 255), COL_RGB(212, 208, 200), COL_RGB(212, 208, 200) },
  { COL_RGB( 64,  64,  64), COL_RGB(128, 128, 128), COL_RGB(212, 208, 200), COL_RGB(212, 208, 200) },
};

// ----------------
// title bar styles

static const struct title_bar_style_t tb_style_base =
{
  20,                                  // y size of title bar
  fnt_fbold7,                          // font used to draw title bar name
  {
    { // color without focus
      COL_RGB(200, 200, 200),          // no focus title text color
      COL_RGB(128, 128, 128)           // no focus title background color
    },
    { // color if have focus
      COL_RGB(255, 255, 255),          // focus title text color
      COL_RGB( 10,  36, 106)           // focus title background color
    },
  },
  {
    COL_RGB(192, 192, 192),            // bar gradient no focus right color
    COL_RGB(166, 202, 240),            // bar gradient focus right color
  },
  false                                // no color gradient
};

static const struct title_bar_style_t tb_style_XP =
{
  18,                                  // y size of title bar
  fnt_fbold7,                          // font used to draw title bar name
  {
    { // color without focus
      COL_RGB(200, 200, 200),          // no focus title text color
      COL_RGB(128, 128, 128)           // no focus title background color
    },
    { // color if have focus
      COL_RGB(255, 255, 255),          // focus title text color
      COL_RGB( 10,  36, 106)           // focus title background color
    },
  },
  {
    COL_RGB(192, 192, 192),            // bar gradient no focus right color
    COL_RGB(166, 202, 240),            // bar gradient focus right color
  },
  true                                 // use color gradient
};

static const struct help_bubble_style_t h_bubble_xp =
{
  fnt_vthm8,                           // text font
  COL_RGB(0, 0, 0),                    // text
  COL_RGB(230, 230, 205),              // text aa
  COL_RGB(255, 255, 225),              // text back color
};

// init windows manager
bool init_win_style(enum e_win_style win_style)
{
  const struct border_style_t **bs;

  // border style
  bs = w_style.border;
  if (win_style == win_style_base)
  {
    bs[win_border_resize] = &border_style_w4;    // window mouse resize border (default)
    bs[win_border_fixed]  = &border_style_w3;    // window fixed size border
    bs[win_border_pu1]    = &border_style_w1;    // menu popup1 (combo box)
    bs[win_border_pu2]    = &border_style_w3;    // menu popup2 (context right click)
    bs[win_border_none]   = &border_style_w0;    // no border (define to avoid NULL check)
    w_style.title_bar = tb_style_base;
  }
  else
  {
    bs[win_border_resize] = &border_style_w4;
    bs[win_border_fixed]  = &border_style_w3;
    bs[win_border_pu1]    = &border_style_w1_xp;
    bs[win_border_pu2]    = &border_style_w2_xp;
    bs[win_border_none]   = &border_style_w0;
    w_style.title_bar = tb_style_XP;
  }

  w_style.h_bubble = h_bubble_xp;

  // init buttons
  return frame_init_tb_buttons(win_style);
}

// free allocated resources
void close_win_style(void)
{
  frame_free_tb_buttons();
}

// ----------------------------------------------
// window frame ant title bar flags

// mouse resizeable
#define WF_RESIZE_X          (1 << 0)            // mouse resizeable in x direction
#define WF_RESIZE_Y          (1 << 1)            // mouse resizeable in y direction
#define WF_RESIZE   (WF_RESIZE_X | WF_RESIZE_Y)  // mouse resizeable in x and y directions

// title bar
#define WF_TITLE_BAR         (1 << 2)            // window will have title bar

// title bar buttons
#define WF_BUT_CLOSE         (1 << 3)            // add close button
#define WF_BUT_MAXIMIZE      (1 << 4)            // add maximize button
#define WF_BUT_MINIMIZE      (1 << 5)            // add minimize button

// add button in title bar
static void tb_add_button(struct title_bar_t *tb, int *pos_r, struct bt_img_t *img)
{
  struct button_t *bt = &tb->bt_list[tb->bt_count++];
  *pos_r += img->bm_u.size.x;
  bt->pos_r = *pos_r;
  bt->img = img;
}

// init frame constant parameters
static void init_win_frame(win_t *w, int flags, enum e_win_border_type border_type)
{
  const struct border_style_t *bs;

  if (flags & WF_CTRL_POPUP)
    flags &= ~(WF_RESIZE | WF_BUT_MAXIMIZE | WF_BUT_MINIMIZE);

  if (flags & WF_RESIZE)
    border_type = win_border_resize;

  // if buttons added, force title bar
  if (flags & (WF_BUT_CLOSE | WF_BUT_MAXIMIZE | WF_BUT_MINIMIZE))
    flags |= WF_TITLE_BAR;

  // set
  w->flags = flags;
  w->border_type = border_type;

  bs = w_style.border[border_type];
  w->cli_ofs.x = bs->width;
  w->cli_ofs.y = bs->width;

  if (flags & WF_TITLE_BAR)
  {
    struct title_bar_t *tb = &w->title_bar;
    tb->size_y = w_style.title_bar.size_y;       // min size
    tb->bt_count = 0;

    // init title bar buttons constants
    if (flags & (WF_BUT_CLOSE | WF_BUT_MAXIMIZE | WF_BUT_MINIMIZE))
    {
      int pos_r = w->cli_ofs.x + 2;              // right offset
      int wy = frame_tb_buttons_img.bt_close.bm_u.size.y;  // button y size
      if ((wy + 4) > tb->size_y)                 // increase bar with if too small
        tb->size_y = wy + 4;
      tb->bt_pos_y = w->cli_ofs.x + ((tb->size_y - wy) >> 1);

      if (flags & WF_BUT_CLOSE)
      {
        tb_add_button(tb, &pos_r, &frame_tb_buttons_img.bt_close);
        pos_r += 2;                              // isolate close button from others
      }
      if (flags & WF_BUT_MAXIMIZE)
        tb_add_button(tb, &pos_r, &frame_tb_buttons_img.bt_maximize);
      if (flags & WF_BUT_MINIMIZE)
        tb_add_button(tb, &pos_r, &frame_tb_buttons_img.bt_minimize);
    }
    w->cli_ofs.y += tb->size_y;
  }
}

// draw window border
static void draw_win_borders(win_t *w)
{
  const struct border_style_t *bs = w_style.border[w->border_type];
  bitmap_t *bm = &w->win_bm;

  if (bs->width)
    bm_draw_rect_shadow_width(bm, 0, 0, bm->size.x, bm->size.y, bs->width, bs->color_tl, bs->color_br);
}

// draw title bar, set buttons positions
static void draw_win_title_bar(win_t *w)
{
  bool have_focus = (w->flags & WF_CTRL_FOCUS) != 0;
  struct title_bar_t *tb = &w->title_bar;
  bitmap_t *bm = &w->win_bm;

  if (tb->size_y)
  {
    int i, tb_size_x, x_bt;
    struct title_bar_style_t *tbs = &w_style.title_bar;
    const font_t *title_fnt = win_font_list[tbs->title_font_id];

    // paint title bar background
    tb_size_x = bm->size.x - (w->cli_ofs.x << 1);
    if (tbs->bk_gradient)
      bm_paint_rect_c2(bm, w->cli_ofs.x,
                           w->cli_ofs.x,
                           tb_size_x,
                           tb->size_y,
                           tbs->text_color[have_focus].background,
                           tbs->bar_gradient[have_focus]);
    else
      bm_paint_rect(bm,    w->cli_ofs.x,
                           w->cli_ofs.x,
                           tb_size_x,
                           tb->size_y,
                           tbs->text_color[have_focus].background);

    // draw buttons
    x_bt = bm->size.x - w->cli_ofs.x;            // x button default
    for (i=0; i<tb->bt_count; i++)
    {
      struct button_t *bt = &tb->bt_list[i];
      x_bt = bm->size.x - bt->pos_r;
      bm_copy_img(bm, x_bt, tb->bt_pos_y, &bt->img->bm_u);
    }

    // draw title bar name
    if (tb->title_str[0])
      bm_draw_string_truncate(bm,
        w->cli_ofs.x + 4,
        w->cli_ofs.x + ((tb->size_y - title_fnt->dy + 1) >> 1),
        tb->title_str,
        tbs->text_color[have_focus].text,
        0,
        win_font_list[tbs->title_font_id],
        x_bt - w->cli_ofs.x - 8);
  }
}

// set title bar text
static void rename_title_bar(win_t *w, const char *str)
{
  struct title_bar_t *tb = &w->title_bar;
  if (!str)
    tb->title_str[0] = 0;
  else
  {
    int l = strlen(str);
    int l_max = sizeof(tb->title_str) - 4;
    if (l < l_max)
      strcpy(tb->title_str, str);
    else
    {
      memcpy(tb->title_str, str, l_max);
      strcpy(tb->title_str + l_max, "...");
    }
  }
}

// ------------------------------------------------
// refresh and blit window title bar

static void draw_blit_title_bar(win_t *w)
{
  draw_win_title_bar(w);
  scr_win_blt_rect(w,
                   w->cli_ofs.x,
                   w->cli_ofs.x,
                   w->win_bm.size.x - (w->cli_ofs.x << 1),
                   w->title_bar.size_y);
}

// ----------------------------------------------
// flash window
// warning: pause user idle_proc if defined.

#define WT scr.win_top
#define HW(w) ((hwin_t *)(w))
#define WIN(hw) ((win_t *)(hw))
#define GET_W(hw) win_t *w = WIN(hw)

// ------------------------------------------------
// windows list.

// send event to current window
#define EV_SEND(ev_type) w->ev_proc((w->ev.type = ev_type, (hwin_t *)w))
#define EV_PROC w->ev_proc((hwin_t *)w)

// add in list on top
static void win_list_push(win_t *w)
{
  w->next = WT;
  WT = w;
}

// remove element
static void win_list_pop(void)
{
  W_ASSERT(WT);
  WT = WT->next;
}

// destroy window
static void win_destroy(win_t *w)
{
  EV_SEND(EV_HIDE);
  EV_SEND(EV_DESTROY);
  win_list_pop();                                // ignore screen messages
  scr_win_destroy(w);
  W_FREE(w);
  scr.tmr.dis_time_ctr = 0;
  scr.tmr.mouse_time_ctr = 0;
}

// ------------------------------------------------
// focus control

// close opened popup
static void close_popup(void)
{
  while (WT && (WT->flags & WF_CTRL_POPUP))
    win_destroy(WT);
}

// kill focus
static void topwin_kill_focus(void)
{
  close_popup();
  if (WT)
  {
    win_t *w = WT;
    EV_SEND(EV_KILLFOCUS);
    w->flags &= ~WF_CTRL_FOCUS;
    draw_blit_title_bar(w);
    win_draw_text_cursor(HW(w), false);
  }
}

// set focus
static void topwin_set_focus(void)
{
  if (WT)
  {
    win_t *w = WT;
    EV_SEND(EV_SETFOCUS);
    w->flags |= WF_CTRL_FOCUS;
    draw_blit_title_bar(w);
  }
}

// application focus
void app_activate(bool active)
{
  if (active)
    topwin_set_focus();
  else
    topwin_kill_focus();
  scr.app_active = active;
}

// ------------------------------------------------
// window close/destroy

// close window, return parent
bool win_close(hwin_t *hw)
{
  GET_W(hw);
  if (w->flags & WF_CTRL_POPUP)
  {
    close_popup();
    return true;
  }
  w->ev.close_cancel = 0;
  EV_SEND(EV_CLOSE);
  if (w->ev.close_cancel)
    return false;

  win_destroy(w);
  if (scr.app_active)
    topwin_set_focus();
  return true;
}

// close application
bool app_close_query(void)
{
  while (WT)
    if (!win_close(HW(WT)))
      return false;
  return true;
}

// ----------------------------------------------

// check/adjust window sizes user arguments. return false if errors found.
static bool win_set_sizes(const vec2i *req_size, const vec2i *req_min_size, const vec2i *req_max_size, const vec2i *desktop_size,
                          vec2i *res_cli_size, vec2i *res_min_size, vec2i *res_max_size, bool *fixed_style)
{
  vec2i cli_min = { CLI_MIN_SIZE_X, CLI_MIN_SIZE_Y };
  *fixed_style = false;                          // default

  // check arguments coherency
  if (req_min_size && req_max_size)
  {
    if ((req_min_size->x > req_max_size->x) || (req_min_size->y > req_max_size->y))
      return false;

    *fixed_style =    (req_min_size->x == req_max_size->x)
                   && (req_min_size->y == req_max_size->y);
  }

  if (req_size)
  {
    if ( req_min_size
         && ((req_size->x < req_min_size->x) || (req_size->y < req_min_size->y)))
     return false;

    if ( req_max_size
         && ((req_size->x > req_max_size->x) || (req_size->y > req_max_size->y)))
     return false;
  }

  // set values and clip
  if (req_min_size)
  {
    *res_min_size = *req_min_size;
    vec_clip_min_max(res_min_size, &cli_min, desktop_size);
  }
  else
    *res_min_size = cli_min;

  if (req_max_size)
  {
    *res_max_size = *req_max_size;
    vec_clip_min_max(res_max_size, res_min_size, desktop_size);
  }
  else
    *res_max_size = *desktop_size;

  if (req_size)
    *res_cli_size = *req_size;
  else
  if (*fixed_style)
    *res_cli_size = *res_min_size;
  else                                           // define average size based on desktop
  {
    res_cli_size->x = desktop_size->x >> 1;
    res_cli_size->y = desktop_size->y >> 1;
  }

  vec_clip_min_max(res_cli_size, res_min_size, res_max_size);

  // check results
  if (req_size && ((res_cli_size->x != req_size->x) || (res_cli_size->y != req_size->y)))
    return false;

  if (*fixed_style && ((res_cli_size->x != req_min_size->x) || (res_cli_size->y != req_min_size->y)))
    return false;

  return true;
}

// window is resized
static bool win_move_resize(win_t *w, const vec2i *win_pos, const vec2i *win_size)
{
  scr_win_move_resize(w, win_pos->x, win_pos->y, win_size->x, win_size->y);
  if (w->win_bm.size.x)                          // 0 if bitmap resize fail or hw minimized
  {
    bm_init_child(&w->cli_bm, &w->win_bm,
      w->cli_ofs.x,
      w->cli_ofs.y,
      w->win_bm.size.x - (w->cli_ofs.x * 2),
      w->win_bm.size.y - (w->cli_ofs.y + w->cli_ofs.x));
    EV_SEND(EV_SIZE);
    if (w->show_mode != show_minimized)
      EV_SEND(EV_PAINT);
    draw_win_title_bar(w);
    draw_win_borders(w);
    return true;
  }
  return false;
}

// adjust hw position to ensure visible on screen
static bool screen_clip_win_pos(const vec2i *win_size, vec2i *win_pos)
{
  vec2i ini_pos, min_pos, max_pos;
  ini_pos = *win_pos;
  min_pos.x = POS_MIN_VIS_LR - win_size->x;
  min_pos.y = 0;
  max_pos.x = scr.size.x - POS_MIN_VIS_LR;
  max_pos.y = scr.size.y - POS_MIN_VIS_BOTTOM;
  vec_clip_min_max(win_pos, &min_pos, &max_pos);
  return (win_pos->x != ini_pos.x) || (win_pos->y != ini_pos.y);  // test if was adjusted
}

// set mouse cursor macro, avoid function call if not required
#define SET_CURSOR(shape) ((shape == scr.last_cursor) ? (void)0 : scr_set_cursor(shape))

// check parameters and create window
static hwin_t *win_create_ex(win_ev_proc_t ev_proc,
                             const vec2i *win_pos, const vec2i *cli_size, const vec2i *min_size,  const vec2i *max_size,
                             int flags, enum e_win_border_type border_type, const char *name, hwin_t *parent, void *user_ptr)
{
  win_t *w;
  win_t *w_parent = (win_t *)parent;
  vec2i frame_w, w_pos, w_size;
  W_ASSERT(ev_proc);                             // check defined

  if (!ev_proc)                                  // check defined
    return NULL;

  if (!w_parent && (flags & WF_CTRL_POPUP))
    return NULL;

  if (!(flags & WF_CTRL_POPUP))
    topwin_kill_focus();
  else
  if (w_parent)
    win_draw_text_cursor(HW(w_parent), false);

  // alloc memory
  w = (win_t *)(W_CALLOC(1, scr_win_sizeof()));
  if (!w)
    return NULL;

  w->user_ptr = user_ptr;
  w->ev_proc = ev_proc;
  w->min_size = *min_size;
  w->max_size = *max_size;

  win_list_push(w);                              // put on top

  init_win_frame(w, flags | WF_CTRL_FOCUS, border_type);
  rename_title_bar(w, name);

  frame_w.x = w->cli_ofs.x * 2;
  frame_w.y = w->cli_ofs.y + w->cli_ofs.x;

  // adjust resize min/max
  w->min_size.x += frame_w.x;
  w->min_size.y += frame_w.y;
  w->max_size.x += frame_w.x;
  w->max_size.y += frame_w.y;

  // define window size
  w_size.x = cli_size->x + frame_w.x;
  w_size.y = cli_size->y + frame_w.y;

  // define window pos
  if (win_pos)
    w_pos = *win_pos;
  else
  if (w_parent)
  {
    w_pos.x = w_parent->win_pos.x + ((w_parent->win_bm.size.x - w_size.x) >> 1);
    w_pos.y = w_parent->win_pos.y + ((w_parent->win_bm.size.y - w_size.y) >> 1);
  }
  else
  {
    w_pos.x = (scr.size.x - w_size.x) >> 1;
    w_pos.y = (scr.size.y - w_size.y) >> 1;
  }
  screen_clip_win_pos(&w_size, &w_pos);          // ensure visible buttons position on screen

  // query user for create
  w->ev.exit_code = 0;
  EV_SEND(EV_CREATE);
  if (!w->ev.exit_code)                          // user init success
  {
    if (scr_win_create(w, name))                 // create window
    {
      if (win_move_resize(w, &w_pos, &w_size))   // set position/size
      {
        scr_win_show(w, true);                   // show window
        EV_SEND(EV_SHOW);
        topwin_set_focus();
        scr.last_cursor = -1;                    // force cursor init
        SET_CURSOR(MC_ARROW);
        return HW(w);
      }
      scr_win_destroy(w);
    }
  }

  EV_SEND(EV_DESTROY);                           // screen failed to create window
  win_list_pop();
  W_FREE(w);
  topwin_set_focus();                            // restore focus
  return NULL;
}

// simple window create with title bar
hwin_t *win_create(const char *name,
                   const vec2i *win_pos,
                   const vec2i *cli_req_size,
                   const vec2i *cli_min_size,
                   const vec2i *cli_max_size,
                   win_ev_proc_t ev_proc, hwin_t *parent, void *user_ptr)
{
  vec2i res_cli_size, res_min_size, res_max_size;
  enum e_win_border_type border_type;
  bool fixed_style;
  int flags;

  close_popup();                                 // get non poput parent

  // check/adjust arguments
  if (!win_set_sizes(cli_req_size, cli_min_size, cli_max_size, &scr.size,
                     &res_cli_size, &res_min_size, &res_max_size, &fixed_style))
    return NULL;

  // define w->flags, get frame size, window size and position
  if (fixed_style)                               // select sizeable or not style
  {
    flags = WF_TITLE_BAR | WF_BUT_CLOSE;
    border_type = win_border_fixed;
  }
  else
  {
    flags = WF_TITLE_BAR | WF_RESIZE | WF_BUT_CLOSE;
    if (!cli_max_size)
      flags |= WF_BUT_MAXIMIZE;
    border_type = win_border_resize;
  }

  if (!parent)
    flags |= WF_BUT_MINIMIZE;

  return win_create_ex(ev_proc, win_pos, &res_cli_size, &res_min_size, &res_max_size, flags, border_type, name, parent, user_ptr);
}

// non sizeable, no menu bar and buttons
hwin_t *win_create_popup(vec2i *win_pos,
                         const vec2i *cli_size,
                         enum e_win_border_type border_type,
                         win_ev_proc_t ev_proc, hwin_t *parent, void *user_ptr)
{
  W_ASSERT(win_pos && cli_size);
  if (!parent)
    return NULL;                                 // popup must have parent

  return win_create_ex(ev_proc, win_pos, cli_size, cli_size, cli_size, WF_CTRL_POPUP, border_type, "popup", parent, user_ptr);
}

// ----------------------------------------------
// events control

// selected border mask
#define SEL_BORDER_L (1 << 0)
#define SEL_BORDER_R (1 << 1)
#define SEL_BORDER_T (1 << 2)
#define SEL_BORDER_B (1 << 3)

#define CORNER_SEL_PIX 8                         // size to select corners
#define B_SEL(m, cur) return(*border_sel_msk = m, cur)

// define mouse selected border mask and return mouse cursor to set
static enum e_mouse_cursor def_border_select_cursor(win_t *w, vec2i *mc, int *border_sel_msk)
{
  if ((w->flags & WF_RESIZE) == WF_RESIZE)       // x && y resize-able
  {
    // top left or bottom left corner selection
    if (mc->x < CORNER_SEL_PIX)
    {
      if (mc->y < CORNER_SEL_PIX)
        B_SEL(SEL_BORDER_T | SEL_BORDER_L, MC_SIZE_TL);

      if (mc->y >= (w->win_bm.size.y - CORNER_SEL_PIX))
        B_SEL(SEL_BORDER_B | SEL_BORDER_L, MC_SIZE_BL);
    }

    // top right or bottom right corner selection
    if (mc->x >= (w->win_bm.size.x - CORNER_SEL_PIX))
    {
      if (mc->y < CORNER_SEL_PIX)
        B_SEL(SEL_BORDER_T | SEL_BORDER_R, MC_SIZE_TR);

      if (mc->y >= (w->win_bm.size.y - CORNER_SEL_PIX))
        B_SEL(SEL_BORDER_B | SEL_BORDER_R, MC_SIZE_BR);
    }
  }

  // left or right border selection
  if (w->flags & WF_RESIZE_X)
  {
    if (mc->x < w->cli_ofs.x)
      B_SEL(SEL_BORDER_L, MC_SIZEX);
    if (mc->x >= (w->cli_ofs.x + w->cli_bm.size.x))
      B_SEL(SEL_BORDER_R, MC_SIZEX);
  }

  // top or bottom selection
  if (w->flags & WF_RESIZE_Y)
  {
    if (mc->y < w->cli_ofs.x)
      B_SEL(SEL_BORDER_T, MC_SIZEY);
    if (mc->y >= (w->cli_ofs.y + w->cli_bm.size.y))
      B_SEL(SEL_BORDER_B, MC_SIZEY);
  }

  B_SEL(0, MC_ARROW);
}

// refresh button image on title bar
static void draw_title_bar_bt_control(win_t *w, bool pressed)
{
  bitmap_t *bt_bm = pressed ? &scr.bt_ctrl->img->bm_p : &scr.bt_ctrl->img->bm_u;
  bm_copy_img(&w->win_bm, scr.bt_pos.x, scr.bt_pos.y, bt_bm);
  scr_win_blt_rect(w, scr.bt_pos.x, scr.bt_pos.y, bt_bm->size.x, bt_bm->size.y);
  scr.bt_pressed = pressed;
}

// change show mode, update bouton image
static void win_set_show_mode(win_t *w, struct button_t *bt_update, enum e_show_mode show_mode)
{
  vec2i w_pos, w_size;
  if (show_mode == w->show_mode)
    return;                                      // same or undefined mode

  if (show_mode == show_normal)
  {
    init_win_frame(w, w->restore.flags, w->restore.border_type);
    w_pos = w->restore.win_pos;
    w_size = w->restore.win_size;
  }
  else
  {
    if (w->show_mode == show_normal)             // save normal init mode
    {
      w->restore.win_pos = w->win_pos;
      w->restore.win_size = w->win_bm.size;
      w->restore.flags = w->flags;
      w->restore.border_type = w->border_type;
    }

    if (show_mode == show_minimized)
    {
      // use fin borders, disable mouse resize
      init_win_frame(w, w->restore.flags & ~WF_RESIZE, win_border_fixed);
      w_pos = w->restore.win_pos;
      w_size.x = SIZE_MINIMIZED_X;
      w_size.y = w->cli_ofs.y + w->cli_ofs.x;    // 0 size client area
      bt_update->img = &frame_tb_buttons_img.bt_restore;
    }
    else  // maximized or fullscreen
    {
      w_pos.x = 0;
      w_pos.y = 0;
      w_size = scr.size;
      if (show_mode == show_maximized)
      {
        // no borders, disable mouse resize
        init_win_frame(w, w->restore.flags & ~WF_RESIZE, win_border_none);
        bt_update->img = &frame_tb_buttons_img.bt_restore;
      }
      else
      {
        // full screen, no buttons and no borders (must be restored without use of button)
        init_win_frame(w, w->restore.flags & ~(WF_RESIZE | WF_TITLE_BAR | WF_BUT_CLOSE | WF_BUT_MAXIMIZE | WF_BUT_MINIMIZE), win_border_none);
      }
    }
  }

  w->show_mode = show_mode;
  win_move_resize(w, &w_pos, &w_size);
}

// mouse control stop on EV_LBUTTONUP
static void win_ctrl_mouse_capture_move_terminate(win_t *w)
{
  enum e_win_ctrl e_ctrl = scr.e_ctrl;           // backup
  scr.e_ctrl = e_win_ctrl_none;                  // clear
  SET_CURSOR(MC_ARROW);

  if (e_ctrl == e_win_ctrl_client)
  {
    w->ev.mouse_pos.x -= w->cli_ofs.x;           // adjust to client area
    w->ev.mouse_pos.y -= w->cli_ofs.y;
    EV_PROC;
  }
  else
  if (e_ctrl == e_win_ctrl_moving)
  {
    if (screen_clip_win_pos(&w->win_bm.size, &w->win_pos))
      scr_win_move(w, w->win_pos.x, w->win_pos.y);
  }
  else
  if (e_ctrl == e_win_ctrl_button)               // button activated on release
  {
    if (scr.bt_pressed)
    {
      // activate button functions
      if (scr.bt_ctrl->img == &frame_tb_buttons_img.bt_close)
      {
        if (!win_close(HW(w)))
          draw_title_bar_bt_control(w, false);   // close canceled, refresh button
      }
      else
      if (scr.bt_ctrl->img == &frame_tb_buttons_img.bt_maximize)
        win_set_show_mode(w, scr.bt_ctrl, show_maximized);
      else
      if (scr.bt_ctrl->img == &frame_tb_buttons_img.bt_minimize)
        win_set_show_mode(w, scr.bt_ctrl, show_minimized);
      else
      if (scr.bt_ctrl->img == &frame_tb_buttons_img.bt_restore)
        win_set_show_mode(w, scr.bt_ctrl, show_normal);
    }
  }
}

// control window move/size/title bar buttons on mouse mouve
static void win_ctrl_mouse_capture_move_update(win_t *w)
{
  vec2i mc_s;
  scr_get_cursor_pos(&mc_s);                     // get screen mouse cursor position
  switch (scr.e_ctrl)
  {
    case e_win_ctrl_moving:
    {
      vec2i wp_new;                              // new window position
      wp_new.x = mc_s.x - scr.mouse_ofs.x;
      wp_new.y = mc_s.y - scr.mouse_ofs.y;

      // hysteresis to unglue maximized window
      if (w->show_mode == show_maximized)
      {
        #define HYST_UNGLUE 3
        int dx = wp_new.x >= w->win_pos.x ? wp_new.x - w->win_pos.x : w->win_pos.x - wp_new.x;
        int dy = wp_new.y >= w->win_pos.y ? wp_new.y - w->win_pos.y : w->win_pos.y - wp_new.y;
        if ((dx > HYST_UNGLUE) || (dy > HYST_UNGLUE))
        {
          if (mc_s.x < w->restore.win_size.x)
            scr.mouse_ofs.x = mc_s.x;            // left align window
          else
          if ((scr.size.x - mc_s.x) < w->restore.win_size.x) // right align window
            scr.mouse_ofs.x = mc_s.x - (scr.size.x - w->restore.win_size.x);
          else
            scr.mouse_ofs.x = w->restore.win_size.x >> 1;    // center window on cursor

          w->restore.win_pos.x = mc_s.x - scr.mouse_ofs.x;
          w->restore.win_pos.y = mc_s.y - scr.mouse_ofs.y;
          win_set_show_mode(w, NULL, show_normal);
        }
        return;
      }

      if ((wp_new.x != w->win_pos.x) || (wp_new.y != w->win_pos.y))
        scr_win_move(w, wp_new.x, wp_new.y);
    }
    break;
    case e_win_ctrl_sizing:
    {
      vec2i win_pos = scr.pos_ini;
      vec2i win_size = scr.size_ini;
      int msk = scr.border_sel_msk;
      int dx = mc_s.x - scr.mouse_ofs.x;
      int dy = mc_s.y - scr.mouse_ofs.y;

      if (scr.sizing)        // previous resize not completed (slow machine or VM) wait
        return;

      if (msk & SEL_BORDER_L)
      {
        int wx = win_size.x - dx;
        if (wx < w->min_size.x)
        {
          win_pos.x = scr.pos_ini.x + scr.size_ini.x - w->min_size.x;
          win_size.x = w->min_size.x;
        }
        else
        if (wx > w->max_size.x)
        {
          win_pos.x = scr.pos_ini.x + scr.size_ini.x - w->max_size.x;
          win_size.x = w->max_size.x;
        }
        else
        {
          win_pos.x += dx;
          win_size.x = wx;
        }
      }
      else
      if (msk & SEL_BORDER_R)
      {
        int wx = win_size.x + dx;
        if (wx < w->min_size.x)
          win_size.x = w->min_size.x;
        else
        if (wx > w->max_size.x)
          win_size.x = w->max_size.x;
        else
          win_size.x = wx;
      }

      if (msk & SEL_BORDER_T)
      {
        int wy = win_size.y - dy;
        if (wy < w->min_size.y)
        {
          win_pos.y = scr.pos_ini.y + scr.size_ini.y - w->min_size.y;
          win_size.y = w->min_size.y;
        }
        else
        if (wy > w->max_size.y)
        {
          win_pos.y = scr.pos_ini.y + scr.size_ini.y - w->max_size.y;
          win_size.y = w->max_size.y;
        }
        else
        {
          win_pos.y += dy;
          win_size.y = wy;
        }
      }
      else
      if (msk & SEL_BORDER_B)
      {
        int wy = win_size.y + dy;
        if (wy < w->min_size.y)
          win_size.y = w->min_size.y;
        else
        if (wy > w->max_size.y)
          win_size.y = w->max_size.y;
        else
          win_size.y = wy;
      }

      // check if resize required
      if ((win_size.x != w->win_bm.size.x) || (win_size.y != w->win_bm.size.y))
      {
        scr.sizing = true;
        win_move_resize(w, &win_pos, &win_size);
      }
    }
    break;
    case e_win_ctrl_button:
    {
      // refresh button state
      bool pressed = vec_select(&w->ev.mouse_pos, &scr.bt_pos, &scr.bt_ctrl->img->bm_u.size);
      if (pressed != scr.bt_pressed)
        draw_title_bar_bt_control(w, pressed);
    }
    break;
    case e_win_ctrl_client:
      w->ev.mouse_pos.x -= w->cli_ofs.x;         // adjust to client area
      w->ev.mouse_pos.y -= w->cli_ofs.y;
      EV_PROC;
    break;
    default:
      W_ASSERT(0);                               // should never occur
  }
}

// blink title bar
static void flash_window(win_t *w)
{
  int i;
  for (i=0; i<(400/50); i++)
  {
    w->flags ^= WF_CTRL_FOCUS;
    draw_blit_title_bar(w);
    sleep_ms(50);
  }
}

// EV_LBUTTONDOWN / EV_MBUTTONDOWN / EV_RBUTTONDOWN
void app_mouse_set_capture(win_t *w)
{
  w->ev.gain_focus = false;                      // default
  if (w != WT)                                   // change window focus
  {
    close_popup();
    if (w != WT)
    {
      flash_window(WT);
      return;
    }
    w->ev.gain_focus = true;
  }

  if (w->ev.type == EV_LBUTTONDOWN)
  {
    // resize
    if (w->flags & (WF_RESIZE_X | WF_RESIZE_Y))
    {
      if (scr.last_cursor != MC_ARROW)           // border selection
      {
        scr.pos_ini = w->win_pos;
        scr.size_ini = w->win_bm.size;
        scr_get_cursor_pos(&scr.mouse_ofs);
        scr.e_ctrl = e_win_ctrl_sizing;
        scr.sizing = false;
        return;
      }
    }

    // title bar selected
    if (w->ev.mouse_pos.y < w->cli_ofs.y)
    {
      int i;
      struct title_bar_t *tb = &w->title_bar;
      scr.bt_pos.y = tb->bt_pos_y;

      // trig title bar buttons
      for (i=0; i<tb->bt_count; i++)
      {
        struct button_t *bt = &tb->bt_list[i];
        scr.bt_pos.x = w->win_bm.size.x - bt->pos_r;
        if (vec_select(&w->ev.mouse_pos, &scr.bt_pos, &bt->img->bm_u.size))
        {
          scr.e_ctrl = e_win_ctrl_button;
          scr.bt_ctrl = bt;
          draw_title_bar_bt_control(w, true);
          return;
        }
      }

      // if no button selected, move window
      scr.mouse_ofs = w->ev.mouse_pos;
      scr.e_ctrl = e_win_ctrl_moving;
      return;
    }
  }

  // client area
  SET_CURSOR(MC_ARROW);                          // debug todo set client cursor
  w->ev.mouse_pos.x -= w->cli_ofs.x;             // adjust to client area
  w->ev.mouse_pos.y -= w->cli_ofs.y;
  scr.e_ctrl = e_win_ctrl_client;
  EV_PROC;
}

// EV_LBUTTONUP / EV_MBUTTONUP / EV_RBUTTONUP
void app_mouse_release_capture(win_t *w)
{
  if (scr.e_ctrl != e_win_ctrl_none)
    win_ctrl_mouse_capture_move_terminate(w);
  else
  {
    // double click 2nd button up
    w->ev.mouse_pos.x -= w->cli_ofs.x;           // adjust to client area
    w->ev.mouse_pos.y -= w->cli_ofs.y;
    EV_PROC;
  }
}

// EV_MOUSEMOVE
void app_mouse_move(win_t *w)
{
  if (scr.e_ctrl != e_win_ctrl_none)
  {
    win_ctrl_mouse_capture_move_update(w);
    return;
  }

  // resize start control
  if (    (w->flags & (WF_RESIZE_X | WF_RESIZE_Y))
       && ((w == WT) || ((w == WT->next) && (WT->flags & WF_CTRL_POPUP))))
  {
    vec2i *mc = &w->ev.mouse_pos;
    int sel_msk;
    enum e_mouse_cursor cursor = def_border_select_cursor(w, mc, &sel_msk);
    if (cursor != MC_ARROW)
    {
      scr.border_sel_msk = sel_msk;
      SET_CURSOR(cursor);
      return;
    }
#if 1
    // option: add hysteresis on previous borders selections
    #define HYST_BSELECT 2                       // hysteresis in pixels
    if (    (scr.last_cursor != MC_ARROW)
         && (      (mc->x < (w->cli_ofs.x + HYST_BSELECT))
               ||  (mc->y < (w->cli_ofs.x + HYST_BSELECT))
               ||  (mc->x >= (w->win_bm.size.x - w->cli_ofs.x - HYST_BSELECT))
               ||  (mc->y >= (w->win_bm.size.y - w->cli_ofs.x - HYST_BSELECT))))
    {
      return;
    }
#endif
    scr.border_sel_msk = 0;
  }

  SET_CURSOR(MC_ARROW);                        // todo set client cursor

  if (WT->flags & WF_CTRL_HELP)
  {
    if (    (w != scr.tmr.mouse_w)
         || !vec_select(&w->ev.mouse_pos, &scr.tmr.clip_pos, &scr.tmr.clip_size))
      close_popup();
    return;
  }

  if ((w == WT) && vec_select(&w->ev.mouse_pos, &w->cli_ofs, &w->cli_bm.size))
  {
    // client area
    scr.notify_leave = true;
    w->ev.mouse_pos.x -= w->cli_ofs.x;         // adjust to client area
    w->ev.mouse_pos.y -= w->cli_ofs.y;
    EV_PROC;
  }
  else
  if (scr.notify_leave)
  {
    scr.notify_leave = false;
    EV_SEND(EV_LEAVE);                         // EV_LEAVE emulate in local
  }
}

// EV_LBUTTONDBLCLK / EV_RBUTTONDBLCLK / EV_MBUTTONDBLCLK / EV_MOUSEWHEEL
void app_mouse_event_other(win_t *w)
{
  // title bar double click
  if ((w->ev.type == EV_LBUTTONDBLCLK) && (w->ev.mouse_pos.y < w->cli_ofs.y))
  {
    // find restore or maximize button
    struct title_bar_t *tb = &w->title_bar;
    struct button_t *bt_maximize = NULL;
    struct button_t *bt_restore = NULL;
    int i, x_min = 0;
    for (i=0; i<tb->bt_count; i++)
    {
      struct button_t *bt = &tb->bt_list[i];
      x_min = bt->pos_r;
      if (bt->img == &frame_tb_buttons_img.bt_maximize)
        bt_maximize = bt;
      else
      if (bt->img == &frame_tb_buttons_img.bt_restore)
        bt_restore = bt;
    }
    if (w->ev.mouse_pos.x > (w->win_bm.size.x - x_min - 4))  // 4: margin
      return;                                    // double click on title bar button is ignored
    if (bt_restore)
      win_set_show_mode(w, bt_restore, show_normal);
    else
    if (bt_maximize)
      win_set_show_mode(w, bt_maximize, show_maximized);
  }
  else
  {
    w->ev.mouse_pos.x -= w->cli_ofs.x;           // adjust to client area
    w->ev.mouse_pos.y -= w->cli_ofs.y;
    EV_PROC;
  }
}

// --------------------------------------
// text cursor

// init
void win_init_text_cursor(hwin_t *hw, int y_size, int color, int bk_color)
{
  GET_W(hw);
  struct text_cursor_t *tc = &w->t_cursor;
  if (tc->vis)
    win_draw_text_cursor(hw, false);
  tc->show = false;
  tc->conf.y_size = y_size;
  tc->conf.color = color;
  tc->conf.bk_color = bk_color;
}

// draw and blit cursor
void win_draw_text_cursor(hwin_t *hw, bool vis)
{
  GET_W(hw);
  struct text_cursor_t *tc = &w->t_cursor;
  if (vis != tc->vis)
  {
    bm_draw_line_v(&w->cli_bm, tc->pos.x, tc->pos.y, tc->conf.y_size, vis ? tc->conf.color : tc->conf.bk_color);
    win_cli_blit_rect(hw, tc->pos.x, tc->pos.y, 1, tc->conf.y_size);
    tc->vis = vis;
  }
}

void win_move_text_cursor(hwin_t *hw, int x, int y)
{
  GET_W(hw);
  struct text_cursor_t *tc = &w->t_cursor;
  if (tc->vis)
    win_draw_text_cursor(hw, false);
  tc->pos.x = x;
  tc->pos.y = y;
  if (tc->show)
    win_draw_text_cursor(hw, true);
}

// ----------------------------------------------
// create help window

#define HB_LRMARGIN 2                            // text margin left right
#define HB_UDMARGIN 2                            // text margin up down
#define HB_LINE_DY 0                             // line dy

static void win_help_paint(win_t *w)
{
  bitmap_t *bm = &w->cli_bm;
  vec2i t_pos = { HB_LRMARGIN, HB_UDMARGIN };

  // clear background
  bm_paint(&w->cli_bm, w_style.h_bubble.bk_color);

  // draw text
  font_draw_text_rect(bm, (const char *)w->user_ptr,
                      win_font_list[w_style.h_bubble.text_font_id],
                      w_style.h_bubble.t_color,
                      w_style.h_bubble.aa_color,
                      &t_pos, HB_LINE_DY, &bm->size);
}

static void win_help_event_proc(hwin_t *hw)
{
  switch (hw->ev.type)
  {
    case EV_PAINT:
      win_help_paint(WIN(hw));
    break;
    case EV_KEYPRESSED:
      close_popup();
    break;
    default:;
  }
}

#define WG_HELP_SHIFT_X 12                       // x shift position to display help (mouse relative)
#define WG_HELP_SHIFT_Y 16                       // y shift position to display help (mouse relative)

void win_help_open(const char *text, vec2i *clip_pos, vec2i *clip_size, hwin_t *parent)
{
  vec2i t_size, c_size, w_pos;
  win_t *pa = WIN(parent);
  W_ASSERT(text && parent);
  if (!parent)
    return;
  
  scr.help_clip_pos = *clip_pos;
  scr.help_clip_size = *clip_size;

  font_eval_text_rect(text, win_font_list[w_style.h_bubble.text_font_id], HB_LINE_DY, &t_size);
  c_size.x = 2*HB_LRMARGIN + t_size.x;
  c_size.y = 2*HB_UDMARGIN + t_size.y;

  w_pos.x = pa->win_pos.x + pa->cli_ofs.x + pa->ev.mouse_pos.x + WG_HELP_SHIFT_X;
  w_pos.y = pa->win_pos.y + pa->cli_ofs.y + pa->ev.mouse_pos.y + WG_HELP_SHIFT_Y;

  // ensure visible
  if ((w_pos.x + c_size.x) > scr.size.x)
    w_pos.x = scr.size.x - c_size.x;
  if ((w_pos.y + c_size.y) > scr.size.y)
    w_pos.y -= (WG_HELP_SHIFT_Y + WG_HELP_SHIFT_Y/2 + c_size.y);

  if (win_create_popup(&w_pos, &c_size, win_border_pu1, win_help_event_proc, parent, (void *)text))
  {
    WT->flags |= WF_CTRL_HELP;
    scr.tmr.clip_pos.x = pa->cli_ofs.x + clip_pos->x;
    scr.tmr.clip_pos.y = pa->cli_ofs.y + clip_pos->y;
    scr.tmr.clip_size = *clip_size;
  }
}

// suspend help activation until mouse move
void win_help_suspend(void)
{
  scr.tmr.dis_time_ctr = HELP_DIS_CNT;
  scr.tmr.mouse_time_ctr = HELP_TRG_CNT;
}

void app_tmr_update(win_t *w)
{
  EV_SEND(EV_2HZ);                               // about 2 hertz event
  if (     scr.app_active                        // app active
       && (!scr.win_top->next)                   // no child opened
       && (scr.e_ctrl == e_win_ctrl_none))       // no captured controls
  {
    if (w->t_cursor.show)                        // cursor blink
      EV_SEND(EV_BLINK);
    else
    if (scr.tmr.dis_time_ctr > 0)                // help popup
      scr.tmr.dis_time_ctr--;
    else
    if (scr.tmr.mouse_time_ctr < HELP_TRG_CNT)
    {
      scr.tmr.mouse_time_ctr++;
      if ((scr.tmr.mouse_time_ctr == HELP_TRG_CNT) && (scr.tmr.mouse_w == w))
      {
        EV_SEND(EV_HELP);
      }
    }
  }
}

// ----------------------------------------------
// blits

// window client blits
void win_blit_all(hwin_t *hw)
{
  GET_W(hw);
  scr_win_blt_rect(w, w->cli_ofs.x, w->cli_ofs.y, w->cli_bm.size.x, w->cli_bm.size.y);
}

#if 0
// debug blit, affect bitmap to show blit regions
#define BLIT_DBG
  
static int blt_x_line = 0;
void blt_debug(win_t *w, int x, int y, int dx, int dy)
{
  // affect bitmap
  bitmap_t *bm = &w->cli_bm;
  pix_t *p_line = BM_PIX_ADDR(bm, x, y);
  int i;
  W_ASSERT((x >= 0) && (dx <= bm->size.x) && (y >= 0) && (dy <= bm->size.y));
  for (i=0; i<dy; i++, p_line += bm->l_size)
  {
    pix_t *p = p_line + (blt_x_line & 7);
    pix_t *p_end = p_line + dx;
    for (;p < p_end; p+=8)
      *p = 0;
  }
  blt_x_line++;
  scr_win_blt_rect(w, w->cli_ofs.x + x, w->cli_ofs.y + y, dx, dy);
}

#endif

void win_cli_blit_rect(hwin_t *hw, int x, int y, int dx, int dy)
{
  GET_W(hw);
  if (vec_clip_rect(&w->cli_bm.size, &x, &y, &dx, &dy))    // clipping, do not blit into frame
#ifdef BLIT_DBG
    blt_debug(w, x, y, dx, dy);
#else
    scr_win_blt_rect(w, w->cli_ofs.x + x, w->cli_ofs.y + y, dx, dy);
#endif
}

// ----------------------------------------------
// utils

// return window having focus
hwin_t *win_get_winfocus(void)
{
  return (hwin_t *)scr.win_top;
}

// return size of screen containing window
const vec2i *win_get_screen_size(void)
{
  return &scr.size;
}

// return client origin position on screen
void win_get_cli_pos(hwin_t *hw, vec2i *scr_pos)
{
  GET_W(hw);
  scr_pos->x = w->win_pos.x + w->cli_ofs.x;
  scr_pos->y = w->win_pos.y + w->cli_ofs.y;
}

// return with of border
int win_get_border_width(enum e_win_border_type border_type)
{
  return w_style.border[border_type]->width;
}

int win_set_timer(hwin_t *hw, int delay_ms)
{
  return scr_set_timer(WIN(hw), delay_ms);
}

void win_kill_timer(hwin_t *hw, int timer_id)
{
  scr_kill_timer(WIN(hw), timer_id);
}

// return true if mouse cursor captured by client area
bool win_mouse_captured(void)
{
  return scr.e_ctrl == e_win_ctrl_client;
}

// window resize in progress
bool win_ctrl_resize(void)
{
  return scr.e_ctrl == e_win_ctrl_sizing;
}
