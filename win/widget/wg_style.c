#include "widget.h"

// default font and color

// ----------------------------------------------
// combo box menu

static const pix_t cb_menu_frame_tl1[1] = { COL_RGB(0, 0, 0) };
static const pix_t cb_menu_frame_br1[1] = { COL_RGB(0, 0, 0) };

// ----------------------------------------------
// widget frame

static const pix_t wg_frame_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB(255, 255, 255) };
static const pix_t wg_frame_br2[2] = { COL_RGB(255, 255, 255), COL_RGB(128, 128, 128) };

// ----------------------------------------------
// text button unpressed/pressed

static const pix_t wg_tb_u_tl2[2] = { COL_RGB(255, 255, 255), COL_RGB(212, 208, 200) };
static const pix_t wg_tb_u_br2[2] = { COL_RGB( 64,  64,  64), COL_RGB(128, 128, 128) };
static const pix_t wg_tb_p_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB(  0,   0,   0) };
static const pix_t wg_tb_p_br2[2] = { COL_RGB(128, 128, 128), COL_RGB(  0,   0,   0) };

// ----------------------------------------------
// control button unpressed/pressed

static const pix_t wg_ctrlb_u_tl2[2] = { COL_RGB(212, 208, 200), COL_RGB(255, 255, 255) };
static const pix_t wg_ctrlb_u_br2[2] = { COL_RGB( 64,  64,  64), COL_RGB(128, 128, 128) };
static const pix_t wg_ctrlb_p_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB(212, 208, 200) };
static const pix_t wg_ctrlb_p_br2[2] = { COL_RGB(128, 128, 128), COL_RGB(212, 208, 200) };

// ----------------------------------------------
// combo box area (pressed style)

static const pix_t wg_cbbox_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB( 64,  64,  64) };
static const pix_t wg_cbbox_br2[2] = { COL_RGB(255, 255, 255), COL_RGB(212, 208, 200) };

// ----------------------------------------------
// edit box area (pressed style)

static const pix_t wg_edbox_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB( 64,  64,  64) };
static const pix_t wg_edbox_br2[2] = { COL_RGB(255, 255, 255), COL_RGB(212, 208, 200) };

// ----------------------------------------------
// progress bar

static const pix_t wg_progress_tl1[1] = { COL_RGB(128, 128, 128) };
static const pix_t wg_progress_br1[1] = { COL_RGB(255, 255, 255) };

// ----------------------------------------------
// console out

//static const pix_t wg_co_out_tl2[2] = { COL_RGB(128, 128, 128), COL_RGB( 64,  64,  64) };
//static const pix_t wg_co_out_br2[2] = { COL_RGB(255, 255, 255), COL_RGB(212, 208, 200) };
static const pix_t wg_co_out_tl2[2] = { COL_RGB(110, 110, 110), COL_RGB(110, 110, 110) };
static const pix_t wg_co_out_br2[2] = { COL_RGB(110, 110, 110), COL_RGB(110, 110, 110) };

// ----------------------------------------------
// define default style

text_color_t tu_color =                          // no selection text color
{
  COL_RGB(0, 0, 0),
  COL_RGB(180, 180, 180),
  COL_RGB(255, 255, 255),
};

text_color_t ts_color =                          // selection text color
{
  COL_RGB(255, 255, 255),                        // selected text color
  COL_RGB(20, 50, 120),                          // selected text aa color 
  COL_RGB(10, 36, 106)                           // selected text bk color
};

struct wg_style_t wgs =
{
  COL_RGB(212, 208, 200),                        // clear background color
  COL_RGB(255, 255, 255),                        // enabled text back color
  COL_RGB(180, 180, 180),                        // anti aliasing color
  COL_RGB(128, 128, 128),                        // text disable color
  COL_RGB(255, 255, 255),                        // text shifted disable color
  COL_RGB(0, 0, 0),                              // control buttons char color

  // ------------------------
  // text_select
  {
    COL_RGB(255, 255, 255),                      // selected text color
    COL_RGB(20, 50, 120),                        // selected text aa color 
    COL_RGB(10, 36, 106)                         // selected text bk color
  },

  // ------------------------
  // msg_box
  {
    fnt_vthm8,                                   // message box font
    {
      COL_RGB(0, 0, 0),                          // text
      COL_RGB(225, 225, 225),                    // text aa
      COL_RGB(255, 255, 255),                    // text back color
    },
  },

  // ------------------------
  // menu_combo
  {
    fnt_vthm8,                                   // context menu font
    {                                            // unselect color
      COL_RGB(0, 0, 0),                          // t_color;
      COL_RGB(225, 225, 225),                    // aa_color
      COL_RGB(255, 255, 255),                    // bk_color
    },
    {                                            // select color
      COL_RGB(255, 255, 255),                    // t_color;
      COL_RGB(20, 50, 120),                      // aa_color
      COL_RGB(10, 36, 106)                       // bk_color 
    },
    {
      COL_RGB(128, 128, 128),                    // horizontal line color
      COL_RGB(255, 255, 255)
    },
  },

  // ------------------------
  // menu_ctx
  {
    fnt_vthm8,                                   // context menu font
    {                                            // unselect color
      COL_RGB(0, 0, 0),                          // t_color;
      COL_RGB(180, 180, 180),                    // aa_color
      COL_RGB(212, 208, 200),                    // bk_color
    },
    {                                            // select color
      COL_RGB(255, 255, 255),                    // t_color;
      COL_RGB(20, 50, 120),                      // aa_color
      COL_RGB(10, 36, 106)                       // bk_color 
    },
    {
      COL_RGB(128, 128, 128),                    // horizontal line color
      COL_RGB(255, 255, 255)
    },
  },

  { 2, wg_cbbox_tl2, wg_cbbox_br2 },             // check box frame

  // ------------------------
  // slider (scroll bar)
  {
    COL_RGB(230, 230, 230),                      // sliders back color (scroll bar)
    { 2, wg_ctrlb_u_tl2, wg_ctrlb_u_br2 },       // button frame
    16,                                          // button width
  },
  // ------------------------
  // combo box menu
  {
    fnt_vthm8,
    COL_RGB(0, 0, 0),
    0,
    { 1, cb_menu_frame_tl1, cb_menu_frame_br1 }
  },
  // ------------------------
  // wg_text
  {
    fnt_vthm8,                                   // default wg_text font
    COL_RGB(0, 0, 0),                            // default wg_text color
    COL_RGB(180, 180, 180),
  },
  // ------------------------
  // wg_frame
  {
    fnt_vthm8,
    COL_RGB(0, 0, 0),
    COL_RGB(180, 180, 180),
    { 2, wg_frame_tl2, wg_frame_br2 }            // note: use custom draw code
  },
  // ------------------------
  // wg_text_button
  {
    fnt_vthm8,                                   // text_font_id
    COL_RGB(0, 0, 0),
    COL_RGB(180, 180, 180),
    { { 2, wg_tb_u_tl2, wg_tb_u_br2 },
      { 2, wg_tb_p_tl2, wg_tb_p_br2 } }
  },
  // ------------------------
  // wg_combo_box
  {
    fnt_vthm8,                                   // text_font_id
    COL_RGB(0, 0, 0),
    COL_RGB(225, 225, 225),
    COL_RGB(255, 255, 255),                      // clear back color
    { 2, wg_cbbox_tl2, wg_cbbox_br2 },
    { { 2, wg_ctrlb_u_tl2, wg_ctrlb_u_br2 },
      { 2, wg_ctrlb_p_tl2, wg_ctrlb_p_br2 } }
  },
  // ------------------------
  // wg_edit_box
  {
    fnt_vthm8,                                   // text_font_id
    COL_RGB(0, 0, 0),
    COL_RGB(225, 225, 225),
    COL_RGB(255, 255, 255),                      // clear back color
    COL_RGB(0, 0, 0),                            // cursor color
    COL_RGB(0, 0, 64),                           // text select back color
    { 2, wg_edbox_tl2, wg_edbox_br2 }
  },
  // ------------------------
  // wg_progress_bar
  {
    COL_RGB(20, 50, 120),                        // fill color
    { 1, wg_progress_tl1, wg_progress_br1 }
  },
  // ------------------------
  // console_out
  {
    fnt_fnorm7,                                  // text_font_id (must be fixed width)
    // fnt_fbold7,
    COL_RGB(210, 210, 210),                      // text color
    COL_RGB(70, 70, 70),                         // text aa color
    COL_RGB(0, 0, 0),                            // text bk color
    { 2, wg_co_out_tl2, wg_co_out_br2 }
  },
};
