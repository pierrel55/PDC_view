// graph widget
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "widget.h"
#include "wg_priv.h"

// background/axis/legend colors
struct wg_graph_color_conf_t
{
  pix_t gr_bk_color;          // gr_bk_color
  pix_t gr_axis_color;        // gr_axis_color
  pix_t txt_legend_color;     // txt_legend_color
  pix_t txt_legend_aa;        // txt_legend_aa
};

// some base colors
static const struct wg_graph_color_conf_t color_conf_list[2] =
{
  { // white style
    COL_RGB(255, 255, 255),
    COL_RGB(  0,   0,   0),
    COL_RGB(  0,   0,   0),
    COL_RGB(225, 225, 225)
  },
  { // black style
    COL_RGB( 32,  32,  32),
    COL_RGB(140, 140, 140),
    COL_RGB(225, 225, 225),
    COL_RGB(  0,   0,   0)
  }
};

typedef struct
{
  widget_t wg;
  float x0;
  float x1;
  float y0;
  float y1;
  float x_units;
  float y_units;

  // axis draw config
  const struct wg_graph_axis_list_t *x_axis_list;
  const struct wg_graph_axis_list_t *y_axis_list;

  // curves list
  const struct wg_graph_curve_t * const *curve_list;

  // draw scale
  float dr_scale_x;
  float dr_scale_y;
  bool y_clamp;              // if set, y values out of graph clamped to visible limits

  // color id
  int col_id;   
  bool enable_aa;

  // post draw call back (user add text on graph)
  wg_graph_pos_draw_cb_t pos_draw_cb;
  wg_graph_click_cb_t click_cb;
} wg_graph_t;

static void draw_graph(hwin_t *hw, wg_graph_t *gr);

static void wg_graph_ev_proc(hwin_t *hw, widget_t *wg)
{
  wg_graph_t *gr = (wg_graph_t *)wg;
  switch (hw->ev.type)
  {
    case EV_PAINT:
    {
      wg_draw_e_frame(hw, wg);
      draw_graph(hw, gr);
    }
    break;
    case EV_LBUTTONDOWN:
      if (gr->click_cb)
        gr->click_cb(hw, wg, hw->ev.mouse_pos.x - wg->c_pos.x, hw->ev.mouse_pos.y - wg->c_pos.y);
    break;
    default:;
  }
}

widget_t *wg_init_graph(hwin_t *hw, wg_graph_pos_draw_cb_t pos_draw_cb)
{
  C_NEW(gr, wg_graph_t);
  if (gr)
  {
    init_dr_obj(&gr->wg, e_obj_none, NULL, COL_ND, NULL);
    wg_init(&gr->wg, wg_graph_ev_proc, WG_SHOW, &wgs.co_out.e_frm);
    gr->pos_draw_cb = pos_draw_cb;
    // define base init to avoid / 0 if wg_config_graph() not done before draw.
    gr->x1 = 1;
    gr->y1 = 1;
    gr->x_units = 0.1f;
    gr->y_units = 0.1f;
    return win_add_widget(hw, &gr->wg);
  }
  return &wg_void;
}

void wg_config_graph(widget_t *wg,
                     float x0, float x1, float y0, float y1,
                     float x_units, float y_units,
                     const struct wg_graph_axis_list_t *x_axis_list,
                     const struct wg_graph_axis_list_t *y_axis_list,
                     enum wg_graph_color_id color_id)
{
  GET_WG(gr, graph);
  gr->x0 = x0;
  gr->x1 = x1;
  gr->y0 = y0;
  gr->y1 = y1;
  gr->x_units = x_units;
  gr->y_units = y_units;

  gr->x_axis_list = x_axis_list;
  gr->y_axis_list = y_axis_list;

  gr->col_id = color_id;
  gr->enable_aa = true;                      // default
}

void wg_graph_def_curve_list(widget_t *wg, const struct wg_graph_curve_t * const *curve_list)
{
  GET_WG(gr, graph);
  gr->curve_list = curve_list;
}

void wg_graph_refresh(hwin_t *hw, widget_t *wg)
{
  GET_WG(gr, graph);
  draw_graph(hw, gr);
  win_cli_blit_rect(hw, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
}

void wg_graph_select_color_id(hwin_t *hw, widget_t *wg, enum wg_graph_color_id color_id, bool refresh)
{
  GET_WG(gr, graph);
  gr->col_id = color_id;
  if (refresh)
    wg_graph_refresh(hw, wg);
}

void wg_graph_enable_aa(widget_t *wg, bool enable)
{
  GET_WG(gr, graph);
  gr->enable_aa = enable;
}

void wg_graph_enable_clamp(widget_t *wg, bool enable)
{
  GET_WG(gr, graph);
  gr->y_clamp = enable;
}

// define a call back if graph client area clicked
void wg_graph_set_click_cb(widget_t *wg, wg_graph_click_cb_t click_cb)
{
  GET_WG(gr, graph);
  gr->click_cb = click_cb;
  if (click_cb)
    wg->flags |= (WG_ENFOCUS | WG_ENABLE);       // required to receive event
  else
    wg->flags &= ~(WG_ENFOCUS | WG_ENABLE);
}

// ----------------------------------------------
// drawing

#define X_BM(x_gr) (int)((x_gr - gr->x0)*gr->dr_scale_x)
#define Y_BM(y_gr) (bm->size.y - (int)((y_gr - gr->y0)*gr->dr_scale_y))

static void graph_draw_grid(wg_graph_t *gr, bitmap_t *bm)
{
  double x_gr, y_gr;
  double dx = gr->x_units / 4.0;
  double dy = gr->y_units / 4.0;
  double x0_s = (int)(gr->x0 / dx)*dx;
  double y0_s = (int)(gr->y0 / dy)*dy;
  double x0_l = (int)(gr->x0 / gr->x_units)*gr->x_units;
  double y0_l = (int)(gr->y0 / gr->y_units)*gr->y_units;
  pix_t grid_color = color_conf_list[gr->col_id].gr_axis_color;

  for (y_gr = y0_l; y_gr < gr->y1; y_gr += gr->y_units)
  {
    int y = Y_BM(y_gr);
    for (x_gr = x0_s; x_gr < gr->x1; x_gr += dx)
      bm_put_pixel(bm, X_BM(x_gr), y, grid_color);
  }

  for (x_gr = x0_l; x_gr < gr->x1; x_gr += gr->x_units)
  {
    int x = X_BM(x_gr);
    for (y_gr = y0_s; y_gr < gr->y1; y_gr += dy)
      bm_put_pixel(bm, x, Y_BM(y_gr), grid_color);
  }

  if (gr->x_axis_list)
  {
    double dy_a = gr->y_units / 8.0;
    double y0_a = (int)(gr->y0 / dy_a)*dy_a;
    int i;
    for (i=0; i<gr->x_axis_list->count; i++)
    {
      double y;
      int x = X_BM(gr->x_axis_list->pos[i]);
      for (y = y0_a; y < gr->y1; y += dy_a)
        bm_put_pixel(bm, x, Y_BM(y), grid_color);
    }
  }

  if (gr->y_axis_list)
  {
    double dx_a = gr->x_units / 8.0;
    double x0_a = (int)(gr->x0 / dx_a)*dx_a;
    int i;
    for (i=0; i<gr->y_axis_list->count; i++)
    {
      double x;
      int y = Y_BM(gr->y_axis_list->pos[i]);
      for (x = x0_a; x < gr->x1; x += dx_a)
        bm_put_pixel(bm, X_BM(x), y, grid_color);
    }
  }
}

// draw graph units
static void graph_draw_units(wg_graph_t *gr, bitmap_t *bm)
{
  char legend[256];
  char dx_scale_str[256];
  char dy_scale_str[256];
  int dx_scale_pos = -1;
  int dy_scale_pos = -1;

  font_t *f = win_font_list[fnt_vthm8];
  const struct wg_graph_color_conf_t *col = &color_conf_list[gr->col_id];

  if (gr->x_axis_list && gr->x_axis_list->fmt_units)
  {
    int x, w, i;
    for (i=0; i<gr->x_axis_list->count; i++)
    {
      double xa = gr->x_axis_list->pos[i];       // x axis value
      sprintf(legend, gr->x_axis_list->fmt_units, xa);
      w = font_get_string_width(legend, f);
      x = X_BM(xa) - w/2;                        // center
      if (x < 0)
        x = 0;
      else
      if ((x + w) > bm->size.x)
        x = bm->size.x - w;
      bm_draw_string(bm, x, bm->size.y - f->dy - 4, legend, col->txt_legend_color, col->txt_legend_aa, f, -1);
    }
    // dx scale
    sprintf(dx_scale_str, "dx: ");
    sprintf(dx_scale_str+4, gr->x_axis_list->fmt_units, gr->x_units);
    w = font_get_string_width(dx_scale_str, f);
    dx_scale_pos = bm->size.x - w - 4;
  }

  if (gr->y_axis_list && gr->y_axis_list->fmt_units)
  {
    int y, w, i;
    for (i=0; i<gr->y_axis_list->count; i++)
    {
      double ya = gr->y_axis_list->pos[i];       // y axis value
      sprintf(legend, gr->y_axis_list->fmt_units, ya);
      y = Y_BM(ya) - f->dy/2;
      if (y < 0)
        y = 0;
      bm_draw_string(bm, 4, y, legend, col->txt_legend_color, col->txt_legend_aa, f, -1);
    }
    // dy scale
    sprintf(dy_scale_str, "dy: ");
    sprintf(dy_scale_str+4, gr->y_axis_list->fmt_units, gr->y_units);
    w = font_get_string_width(dy_scale_str, f);
    dy_scale_pos = bm->size.x - w - 4;
  }

  // align both scale positions
  if ((dx_scale_pos >= 0) && (dy_scale_pos >= 0))
  {
    if (dx_scale_pos < dy_scale_pos)
      dy_scale_pos = dx_scale_pos;
    else
    if (dy_scale_pos < dx_scale_pos)
      dx_scale_pos = dy_scale_pos;
  }

  if (dx_scale_pos >= 0)
    bm_draw_string(bm, dx_scale_pos, 4, dx_scale_str, col->txt_legend_color, col->txt_legend_aa, f, -1);
  if (dy_scale_pos >= 0)
    bm_draw_string(bm, dy_scale_pos, 4 + f->dy, dy_scale_str, col->txt_legend_color, col->txt_legend_aa, f, -1);
}

// draw single curve
static void draw_curve(wg_graph_t *gr, bitmap_t *bm, const struct wg_graph_curve_t *crv)
{
  float x = crv->x0;
  float dx = ((crv->x1 - crv->x0)/(crv->data_count - 1))*crv->draw_stride;
  const float *y_data = crv->y_data;
  int float_stride = crv->data_stride_bytes / sizeof(float);
  int n_data = crv->data_count / crv->draw_stride;
  int y_data_stride = float_stride * crv->draw_stride;

  // round buffer
  const float *y_data_limit = crv->rb_limit ? crv->rb_limit : crv->y_data + (n_data * y_data_stride);
  int rb_dec = crv->rb_sizeof / sizeof(float);

  // colors
  pix_t crv_col = crv->color[gr->col_id];
  pix_t aa_col = crv->aa_color[gr->col_id];

  W_ASSERT(crv->data_stride_bytes == float_stride * sizeof(float));     // ensure stride is aligned on float
  W_ASSERT(!(crv->rb_sizeof % crv->data_stride_bytes));                 // ensure rotate buffer data size aligned on stride

  // test if fifo mode is used and loop to stack begin if end reached
  #define RB_CHECK if (y_data >= y_data_limit) y_data -= rb_dec

  // big square pixels
  if (crv->pix_w > 1)
  {
    int n;
    for (n=0; n < n_data; n++, y_data += y_data_stride, x += dx)
    {
      pix_t *p;
      int dot_w = crv->pix_w;
      int dw = dot_w >> 1;

      int x0, y0, x1, y1, wx, wy;
      
      x0 = X_BM(x) - dw;
      if (x0 >= bm->size.x)
        break;
      if (x0 < 0)
        x0 = 0;
      x1 = x0 + dot_w;
      if (x1 > bm->size.x)
        x1 = bm->size.x;
      wx = x1 - x0;
      if (wx <= 0)
        continue;

      RB_CHECK;
      y0 = Y_BM(*y_data) - dw;
      if (y0 >= bm->size.y)          // not visible
      {
        if (!gr->y_clamp)
          continue;
        // y0 = bm->size.y - dw;     // half dot
        y0 = bm->size.y - dot_w;     // full dot
      }
      y1 = y0 + dot_w;
      if (y1 <= 0)                   // not visible
      {
        if (!gr->y_clamp)
          continue;
        // y1 = dw;                  // half dot
        y1 = dot_w;                  // full dot
      }
      if (y0 < 0)
        y0 = 0;
      if (y1 > bm->size.y)
        y1 = bm->size.y;
      wy = y1 - y0;
      if (wy <= 0)
        continue;

      p = BM_PIX_ADDR(bm, x0, y0);
      while (wy--)
      {
        pix_t *pix = p;
        pix_t *p_end = p + wx;
        while (pix < p_end)
          *pix++ = crv_col;
        p += bm->l_size;
      }
    }
    return;
  }

  if (!aa_col || !gr->enable_aa)
  {
    int n;
    for (n=0; n < n_data; n++, y_data += y_data_stride, x += dx)
    {
      int x_bm, y_bm;

      x_bm = X_BM(x);
      if (x_bm < 0)
        continue;
      if (x_bm >= bm->size.x)
        break;

      RB_CHECK;
      y_bm = Y_BM(*y_data);
      if (y_bm < 0) { if (!gr->y_clamp) continue; y_bm = 0; }
      if (y_bm >= bm->size.y) { if (!gr->y_clamp) continue; y_bm = bm->size.y - 1; }
      *BM_PIX_ADDR(bm, x_bm, y_bm) = crv_col;
    }
  }
  else
  {
    int x_max = bm->size.x-1;
    int y_max = bm->size.y-1;
    int n;
    pix_t bk_color = color_conf_list[gr->col_id].gr_bk_color;
    if (aa_col == -1)                   // light background
    {
#if 0
      int r = ((crv_col &     0xff) + (bk_color &     0xff)*7)/8;
      int g = ((crv_col &   0xff00) + (bk_color &   0xff00)*7)/8;
      int b = ((crv_col & 0xff0000) + (bk_color & 0xff0000)*7)/8;
      aa_col = (r & 0xff) | (g & 0xff00) | (b & 0xff0000);
#else
      // aa_col = ((crv_col & 0xf8f8f8) + 7*(bk_color & 0xf8f8f8)) >> 3;  // auto defined
      aa_col = ((crv_col & 0xf0f0f0) + 15*(bk_color & 0xf0f0f0)) >> 4;  // auto defined (light background)
#endif
    }
    else
    if (aa_col == -2)                  // dark background
      aa_col = ((crv_col & 0xf0f0f0)*6 + 10*(bk_color & 0xf0f0f0)) >> 4;  // auto defined (dark background)

    for (n=0; n < n_data; n++, y_data += y_data_stride, x += dx)
    {
      pix_t *p;
      int x_bm, y_bm;

      x_bm = X_BM(x);
      if (x_bm <= 0)
        continue;
      if (x_bm >= x_max)
        break;

      RB_CHECK;
      y_bm = Y_BM(*y_data);
      if (y_bm <= 0) { if (!gr->y_clamp) continue; y_bm = 1; }
      if (y_bm >= y_max) { if (!gr->y_clamp) continue; y_bm = y_max - 1; }

      p = BM_PIX_ADDR(bm, x_bm, y_bm);
      *p = crv_col;
      if (p[-1] == bk_color) p[-1] = aa_col;
      if (p[ 1] == bk_color) p[ 1] = aa_col;
      if (p[-bm->l_size] == bk_color) p[-bm->l_size] = aa_col;
      if (p[ bm->l_size] == bk_color) p[ bm->l_size] = aa_col;
#if 0
      if (p[-1-bm->l_size] == bk_color) p[-1-bm->l_size] = aa_col;
      if (p[ 1-bm->l_size] == bk_color) p[ 1-bm->l_size] = aa_col;
      if (p[-1+bm->l_size] == bk_color) p[-1+bm->l_size] = aa_col;
      if (p[ 1+bm->l_size] == bk_color) p[ 1+bm->l_size] = aa_col;
#endif
    }
  }
}

// draw name of curves at top left (warning, no clipping, ensure large enough client size)
static void graph_draw_curve_names(hwin_t *hw, wg_graph_t *gr)
{
  const struct wg_graph_curve_t * const *crv;
  font_t *f = win_font_list[fnt_vthm8];
  const struct wg_graph_color_conf_t *col = &color_conf_list[gr->col_id];

  bitmap_t *bm = &hw->cli_bm;
  int x = gr->wg.c_pos.x;
  int y = gr->wg.c_pos.y;

  #define SQ_W 8          // with of colored square
  
  for (crv = gr->curve_list; *crv; crv++)
  {
    if ((*crv)->show && (*crv)->data_count && (*crv)->name)
    {
      bm_paint_rect(bm, x + 4, y + (f->dy - SQ_W)/2, SQ_W, SQ_W, (*crv)->color[gr->col_id]);
      bm_draw_string(bm, x + SQ_W + 8, y, (*crv)->name, col->txt_legend_color, col->txt_legend_aa, f, -1);
      y += f->dy + 2;
    }
  }
}

// draw full graph
static void draw_graph(hwin_t *hw, wg_graph_t *gr)
{
  bitmap_t bm;
  widget_t *wg = &gr->wg;
  bm_init_child(&bm, &hw->cli_bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
  bm_paint(&bm, color_conf_list[gr->col_id].gr_bk_color);

  // update if changed
  gr->dr_scale_x = (float)(wg->c_size.x)/(gr->x1 - gr->x0);
  gr->dr_scale_y = (float)(wg->c_size.y)/(gr->y1 - gr->y0);

  graph_draw_grid(gr, &bm);

  if (gr->curve_list)
  {
    const struct wg_graph_curve_t * const *crv;
    for (crv = gr->curve_list; *crv; crv++)
      if ((*crv)->show && (*crv)->data_count)
        draw_curve(gr, &bm, *crv);

    graph_draw_curve_names(hw, gr);
  }

  graph_draw_units(gr, &bm);

  if (gr->pos_draw_cb)
    gr->pos_draw_cb(hw, wg, &bm);
}
