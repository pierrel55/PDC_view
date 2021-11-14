#include <string.h>
#include <stdio.h>
#include <math.h>
#include "win/widget/widget.h"
#include "PDC/pdc.h"
#include "dlg_main.h"
#include "dlg_graph.h"

// -----------------------------------------------------------
// graph 2D n crystal(lambda)
// -----------------------------------------------------------

#define GR_COLOR wg_graph_color_black

static struct wg_graph_axis_list_t axis_x = { 0 };
static struct wg_graph_axis_list_t axis_y = { 0 };

#define NP_NLAMDA 1024
static struct gr_ind_t
{
  float nx;
  float ny;
  float nz;
} ind_lambda[NP_NLAMDA];

// lambda range
#define LAMBDA_MIN_NM 150.0f
#define LAMBDA_MAX_NM 1250.0f

// indice range
#define IND_MIN 1.5f
#define IND_MAX 3.0f

#if 0
// some colors for curves
COL_RGB(200,   0,   0), COL_RGB(250,   0,   0)
COL_RGB( 50, 150, 200), COL_RGB( 70, 190, 255)
COL_RGB( 70,  70, 220), COL_RGB(100, 100, 250)
COL_RGB(200, 200,   0), COL_RGB(250, 250,   0)
COL_RGB(200,   0, 200), COL_RGB(250,   0, 250)
COL_RGB(  0, 200, 200), COL_RGB(  0, 200, 200)
COL_RGB(128, 128, 128), COL_RGB(180, 180, 180)
COL_RGB(  0, 200,   0), COL_RGB(  0, 250,   0)
COL_RGB(128, 128, 128), COL_RGB(180, 180, 180)
COL_RGB(128, 128, 128), COL_RGB(180, 180, 180)
COL_RGB(128, 128, 128), COL_RGB(180, 180, 180)
COL_RGB(128, 128, 128), COL_RGB(180, 180, 180)
#endif

// indices(lamda) curves 2D
static struct wg_graph_curve_t crv_n_x = { 0, "nX", false, { COL_RGB( 50, 150, 200), COL_RGB( 70, 190, 255) }, {-1, -2 }, 0, LAMBDA_MIN_NM, LAMBDA_MAX_NM, 0, sizeof(struct gr_ind_t), 1, &ind_lambda[0].nx, NULL, 0 };
static struct wg_graph_curve_t crv_n_y = { 0, "nY", false, { COL_RGB(  0, 200,   0), COL_RGB(  0, 250,   0) }, {-1, -2 }, 0, LAMBDA_MIN_NM, LAMBDA_MAX_NM, 0, sizeof(struct gr_ind_t), 1, &ind_lambda[0].ny, NULL, 0 };
static struct wg_graph_curve_t crv_n_z = { 0, "nZ", false, { COL_RGB(200,   0,   0), COL_RGB(250,   0,   0) }, {-1, -2 }, 0, LAMBDA_MIN_NM, LAMBDA_MAX_NM, 0, sizeof(struct gr_ind_t), 1, &ind_lambda[0].nz, NULL, 0 };

// note: declare in draw order
static const struct wg_graph_curve_t *curve_list[] =
{
  &crv_n_x,
  &crv_n_y,
  &crv_n_z,
  NULL                                 // define list end
};

// some base init
void init_graph_base(void)
{
  wg_config_graph(dlg.graph.wg_graph, 
                  0, 1,                // x0, x1
                  0, 1,                // y0, y1
                  0.1f, 0.1f,
                  NULL, NULL, GR_COLOR);
}

static int def_n_lambda(float lambda_min, float lambda_max)
{
  int n;
  int wx = dlg.graph.wg_graph->c_size.x;             // graph pixels size
  int np = (wx < NP_NLAMDA) ? wx : NP_NLAMDA;        // n to define, adjust to graph pixels size
  float l, dl = (LAMBDA_MAX_NM - LAMBDA_MIN_NM)/np;     // lambda variation / pixel
  struct gr_ind_t *ind = ind_lambda;

  for (n=0, l = lambda_min; (n<np) && (l < lambda_max); n++, ind++, l+=dl)
  {
    vec3 n2;
    get_crystal_index(pdc.crystal_type, l, pdc.crystal_temp, &n2, NULL);
    ind->nx = sqrtf(n2.x);
    ind->ny = sqrtf(n2.y);
    ind->nz = sqrtf(n2.z);
  }
  return n;
}

void draw_graph_n_lambda(hwin_t *hw)
{
  int nd;
  float lambda_min = (float)pdc.crystal_info.min_lambda_nm;
  float lambda_max = (float)pdc.crystal_info.max_lambda_nm;
  if (lambda_max > LAMBDA_MAX_NM)
    lambda_max = LAMBDA_MAX_NM;
  nd = def_n_lambda(lambda_min, lambda_max);

  // lambda in x
  axis_x.count = 2;
  axis_x.pos[0] = LAMBDA_MIN_NM + 50.0f;
  axis_x.pos[1] = LAMBDA_MAX_NM - 50.0f;
  axis_x.fmt_units = "%.0f nm";

  // indices in y
  axis_y.count = 2;
  axis_y.pos[0] = 1.0f;
  axis_y.pos[1] = 2.0f;
  axis_y.fmt_units = "%.1f";

  wg_config_graph(dlg.graph.wg_graph, 
                  LAMBDA_MIN_NM, LAMBDA_MAX_NM,    // x0, x1 range
                  0, IND_MAX,                      // y0, y1 range
                  100.0, 0.5f,                     // dx/dy grid
                  &axis_x, &axis_y, GR_COLOR);

  crv_n_x.show = true;
  crv_n_z.show = true;

  if (pdc.crystal_info.uniaxial)
  {
    crv_n_y.show = false;
    crv_n_x.name = "nX = nY";
  }
  else
  {
    crv_n_y.show = true;
    crv_n_x.name = "nX";
    crv_n_y.name = "nY";
  }

  crv_n_x.data_count = nd;
  crv_n_y.data_count = nd;
  crv_n_z.data_count = nd;
  crv_n_x.x0 = lambda_min;
  crv_n_y.x0 = lambda_min;
  crv_n_z.x0 = lambda_min;
  crv_n_x.x1 = lambda_max;
  crv_n_y.x1 = lambda_max;
  crv_n_z.x1 = lambda_max;

  wg_graph_def_curve_list(dlg.graph.wg_graph, curve_list);
  wg_graph_refresh(hw, dlg.graph.wg_graph);
  dlg.graph.disp.e_displayed = e_graph_undef;
}

// -----------------------------------------------------------
// graph 3D
// -----------------------------------------------------------

#define FGR_W_MAX 800                            // max graph pixel size

// floating point pixel
struct fgr_pix_t
{
  float i;                                       // original value
  float i_f;                                     // filtered value
};

#define MAX_ISCALE 10000                         // max intensity amplification scale

// floating point bitmap
struct fgr_t
{
  // size config
  int wx;                                        // displayed size
  int wy;

  // filters applyed on initial intensity
  int n_sqrt;                                    // non linear contrast
  bool revert;                                   // revert intensity
  bool min_scale;                                // scale intensity using i_max - i_min instead of i_max - 0

  // intensity datas (reserved to max graph size)
  float i_min;                                   // min intensity in pix before filters applyed
  float i_max;                                   // max intensity in pix before filters applyed
  struct fgr_pix_t pix[FGR_W_MAX*FGR_W_MAX];
};

// init size, clear mem
static struct fgr_t *fgr_init(struct fgr_t *fgr, bool clear_mem)
{
  fgr->wx = dlg.graph.wg_graph->c_size.x;        // graph pixels size
  fgr->wy = dlg.graph.wg_graph->c_size.y - GR_LEGEND_WY;

  // default clear filters
  fgr->n_sqrt = 0;
  fgr->revert = false;
  fgr->min_scale = false;

  if (clear_mem)                                 // required if use cummulative intensity or if pixels not drawn (screen projection)
    memset(fgr->pix, 0, fgr->wx*fgr->wy*sizeof(struct fgr_pix_t));
  return fgr;
}

// apply display filters
static void fgr_filter(struct fgr_t *fgr)
{
  struct fgr_pix_t *p, *p_end = fgr->pix + fgr->wy*fgr->wx;

  if (fgr->min_scale && (fgr->i_min > 0))
  {
    // scale applyed to (i - i_min) (show small variations)
    float min = fgr->i_min;
    float diff = fgr->i_max - min;
    float scale;

    if (diff < (1.0/MAX_ISCALE))
      scale = MAX_ISCALE;
    else
      scale = 1.0f/diff;

    for (p = fgr->pix; p < p_end; p++)
    {
      if (p->i >= min)
      {
        int j;
        float i_f = (p->i - min)*scale;          // scale
        for (j=0; j<fgr->n_sqrt; j++)            // contrast filters
          i_f = (float)sqrt(i_f);
        if (fgr->revert)                         // revert intensity
          i_f = 1.0f - i_f;
        p->i_f = i_f;
      }
    }
  }
  else
  {
    // scale applyed to (i - 0) normal mode
    float diff = fgr->i_max;
    float scale;

    if (diff < (1.0/MAX_ISCALE))
      scale = MAX_ISCALE;
    else
      scale = 1.0f/diff;

    for (p = fgr->pix; p < p_end; p++)
    {
      if (p->i > 0)
      {
        int j;
        float i_f = p->i*scale;
        for (j=0; j<fgr->n_sqrt; j++)
          i_f = (float)sqrt(i_f);
        if (fgr->revert)
          i_f = 1.0f - i_f;
        p->i_f = i_f;
      }
    }
  }
}

// ----------------------------------------------
// fgr0 specific

static struct fgr_t fgr0;

// def min/max
static void fgr_def_min_max(void)
{
  struct fgr_t *fgr = &fgr0;
  struct fgr_pix_t *p, *p_end = fgr->pix + fgr->wy*fgr->wx;
  float i_max = -1e6;
  float i_min =  1e6;

  // get min/max
  for (p = fgr->pix; p < p_end; p++)
  {
    if (p->i > i_max)
      i_max = p->i;
    if (p->i < i_min)
      i_min = p->i;
  }

  fgr->i_min = i_min;
  fgr->i_max = i_max;
}

// draw color interpolation depending of filter level
static pix_t *draw_h_line_interpolate(bitmap_t *bm, int x, int y, int w)
{
  struct fgr_t *fgr = &fgr0;
  pix_t *p0 = bm_get_pix_addr(bm, x, y);
  pix_t *p = p0;
  float i_f = 0;
  float d_if = 1.0f/w;
  int i;

  for (i=0; i<w; i++, i_f += d_if)
  {
    float i_ff = i_f;
    int j;
    for (j=0; j<fgr->n_sqrt; j++)
      i_ff = sqrtf(i_ff);
    if (fgr->revert)
      i_ff = (1.0f - i_ff);
    *p++ = (unsigned int)(i_ff * 255.99)*0x10101;
  }
  return p0;
}

// draw graph legend (3D mode)
static void draw_graph_legend(bitmap_t *bm, const char *legend_str)
{
  struct fgr_t *fgr = &fgr0;
  int yl = fgr->wy;
  int x, y, dx, dy, ws;
  char num_str[64];
  font_t *fnt = win_font_list[fnt_vthm8];
  pix_t *c0;
  pix_t ef_color = dlg.graph.wg_graph->e_frm->tl[0];
  bm_draw_line_h(bm, 0, yl    , bm->size.x, ef_color);
  bm_draw_line_h(bm, 0, yl + 1, bm->size.x, ef_color);
  bm_paint_rect(bm, 0, yl+2, bm->size.x, bm->size.y - (yl+2), wgs.clear_bk_color);

  dx = (bm->size.x*3)/5;
  dy = (GR_LEGEND_WY*3)/5;
  x = (bm->size.x - dx)/2;
  y = yl + 2 + ((GR_LEGEND_WY - dy)/2);
  bm_draw_rect_width(bm, x, y, dx, dy, 1, COL_RGB(0, 0, 0));
  x++;
  y++;
  dx -= 2;
  dy -= 2;

  // draw intensity reference
  c0 = draw_h_line_interpolate(bm, x, y, dx);
  bm_paint_rect_clone_color_no_clip(bm, x, y+1, dx, dy-1, c0);

  // string min max values
  y += (dy - fnt->dy)/2;
  sprintf(num_str, "%s %.3f", legend_str, fgr->i_min);
  ws = font_get_string_width(num_str, fnt);
  bm_draw_string(bm, x - ws - 6, y, num_str, wgs.text.text_color, wgs.text.text_aa_color, fnt, -1);
  sprintf(num_str, "%.3f", fgr->i_max);
  bm_draw_string(bm, x + dx + 6, y, num_str, wgs.text.text_color, wgs.text.text_aa_color, fnt, -1);
}

// draw units grid
static void draw_graph_angles_grid(hwin_t *hw, bitmap_t *bm, struct gr_view_t *gv)
{
  #define PM_DIV_H 9.0f   // coarse grid
  #define PM_DIV_L 5.0f   // fine grid

  struct fgr_t *fgr = &fgr0;
  const char *x_axis_name = "Phi";
  const char *y_axis_name = "Theta";
  const char *unit_fmt = (gv->an_range >= (PM_DIV_H-0.1)) ? "%.0f" : "%.1f";
  font_t *fnt = win_font_list[fnt_vthm8];

  int ws;
  char num_str[32];
  float x, y, x1, y1, theta, phi;
  float angle_range    = gv->an_range;
  float theta0         = gv->theta;
  float phi0           = gv->phi;

  float d_x = (fgr->wx-1) / (PM_DIV_H*PM_DIV_L);
  float d_y = (fgr->wy-1) / (PM_DIV_H*PM_DIV_L);
  float d_theta = angle_range / PM_DIV_H;
  float d_phi   = angle_range / PM_DIV_H;

  pix_t col_txt = COL_RGB(0, 0, 0);
  pix_t col_pix = COL_RGB(0, 0, 0);

  bm_draw_string(bm, 2, 0, y_axis_name, col_txt, 0, fnt, -1);
  sprintf(num_str, unit_fmt, theta0 + angle_range);
  bm_draw_string(bm, 2, fnt->dy-1, num_str, col_txt, 0, fnt, -1);
  for (y = d_y*PM_DIV_L, y1 = d_y*(PM_DIV_L*PM_DIV_H) - 1, theta = theta0 + angle_range - d_theta; y < y1; y += d_y*PM_DIV_L, theta -= d_theta)
  {
    int ys = (int)y;
    sprintf(num_str, unit_fmt, theta);
    bm_draw_string(bm, 2, ys - (fnt->dy >> 1), num_str, col_txt,  0, fnt, -1);
    for (x = d_x*(PM_DIV_L-3), x1 = d_x*(PM_DIV_L*PM_DIV_H - 1); x < x1; x += d_x)
      bm_put_pixel(bm, (int)x, ys, col_pix);
  }

  ws = font_get_string_width(x_axis_name, fnt);
  bm_draw_string(bm, fgr->wx - ws - 1, fgr->wy - 2*fnt->dy, x_axis_name, col_txt, 0, fnt, -1);
  sprintf(num_str, unit_fmt, phi0 + angle_range);
  ws = font_get_string_width(num_str, fnt);
  bm_draw_string(bm, fgr->wx - ws, fgr->wy - fnt->dy, num_str, col_txt, 0, fnt, -1);
  for (x = d_x*PM_DIV_L, x1 = d_x*(PM_DIV_L*PM_DIV_H) - 1, phi = phi0 + d_phi; x < x1; x += d_x*PM_DIV_L, phi += d_phi)
  {
    int xs = (int)x;
    sprintf(num_str, unit_fmt, phi);
    ws = font_get_string_width(num_str, fnt);
    bm_draw_string(bm, xs - (ws >> 1), fgr->wy - fnt->dy, num_str, col_txt,  0, fnt, -1);
    for (y = d_y, y1 = d_y*(PM_DIV_L*PM_DIV_H - 2); y < y1; y += d_y)
      bm_put_pixel(bm, xs, (int)y, col_pix);
  }

  sprintf(num_str, "Grid step %.1f deg.", d_theta);
  ws = font_get_string_width(num_str, fnt);
  bm_draw_string(bm, fgr->wx - ws - 1, 0, num_str, col_txt, 0, fnt, -1);
}

// copy graphic datas to graph bitmap
static void fgr_blit(hwin_t *hw, const char *legend_str, struct gr_view_t *gv)
{
  struct fgr_t *fgr = &fgr0;
  widget_t *wg = dlg.graph.wg_graph;
  bitmap_t bm;
  pix_t *pix_line;
  struct fgr_pix_t *p = fgr->pix;
  int y;

  bm_init_child(&bm, &hw->cli_bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);

  // graph part
  for (y=0, pix_line = bm.pix_ptr; y<fgr->wy; y++, pix_line += bm.l_size)
  {
    pix_t *pix = pix_line;
    pix_t *pix_end = pix_line + bm.size.x;
    for (;pix < pix_end; pix++, p++)
       *pix = (unsigned int)(p->i_f * 255.99)*0x10101;  // convert to grey scale pixel color
  }

  // grid
  if (gv)
    draw_graph_angles_grid(hw, &bm, gv);

  // 3D legend
  draw_graph_legend(&bm, legend_str);

  // blit
  win_cli_blit_rect(hw, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
}

// ----------------------------------------------
// slow/fast indices graph
// ----------------------------------------------

// ns - nf squared diff with average ns - nf
static void fgr_diff_avg(void)
{
  struct fgr_t *fgr = &fgr0;
  int np = fgr->wx * fgr->wy;
  struct fgr_pix_t *p, *p_end = fgr->pix + np;
  float avg; 
  double i_sum = 0;

  // define ns - nf average
  for (p = fgr->pix; p < p_end; p++)
    i_sum += p->i;
  avg = (float)(i_sum/np);

  // define squared diff with average
  for (p = fgr->pix; p < p_end; p++)
  {
    float d = p->i - avg;
    p->i = d*d;
  }
}

// display message if config not valid
bool check_pm_config(hwin_t *hw)
{
  if (pdc.conf_valid)
    return true;
  co_printf("ERROR: Cannot execute operation: Invalid PM configuration.\n");
  return false;
}

void draw_graph_ind_sf(hwin_t *hw, enum e_graph_ind_sf graph_ind_type, const char *legend_str)
{
  struct fgr_t *fgr = fgr_init(&fgr0, false);
  int x, y;
  double theta, phi, d_theta, d_phi;
  struct fgr_pix_t *p = fgr->pix;
  pho_t pho;

  if (st.pump.ind.ind2.x == 1)
  {
    co_printf("ERROR: Cannot execute operation: Invalid lambda pump.\n");
    return;
  }

  init_pho_lambda(&pho, pdc.lambda_pump_nm);

  d_theta = PI_RAD / (fgr->wy-1);             // include limits [0..PI] for symetrical graph drawing
  d_phi   = PI_RAD / (fgr->wx-1);

  for (y=0, theta=PI_RAD; y<fgr->wy; y++, theta -= d_theta)  // flip y (bitmap 0 is up)
  {
    sc_rotate(&pho.rot.theta, theta);
    for (x=0, phi=0; x<fgr->wx; x++, p++, phi += d_phi)
    {
      double nf, ns;
      sc_rotate(&pho.rot.phi, phi);
      
      // s in crystal
      pho.s.x = pho.rot.theta.s * pho.rot.phi.c;
      pho.s.y = pho.rot.theta.s * pho.rot.phi.s;
      pho.s.z = pho.rot.theta.c;

      // get fast index for s
      pho.fast = true;
      def_dir_index(&pho, &pho.s);
      nf = pho.n;

      // get slow index for s
      pho.fast = false;
      def_dir_index(&pho, &pho.s);
      ns = pho.n;

      switch (graph_ind_type)
      {
        case e_ind_s:
          p->i = (float)ns;
        break;
        case e_ind_f:
          p->i = (float)nf;
        break;
        case e_ind_sf_diff:
        case e_ind_sf_diff_avg:
          p->i = (float)(ns - nf);
        break;
      }
    }
  }

  dlg.graph.disp.e_displayed = e_graph_undef;    // set type before blit
  if (graph_ind_type == e_ind_sf_diff_avg)
    fgr_diff_avg();
  fgr_def_min_max();
  fgr->min_scale = true;
  fgr_filter(fgr);
  fgr_blit(hw, legend_str, NULL);
}

// ----------------------------------------------
// phase matching graph
// ----------------------------------------------

#define OPT_MATCH_TRIGO                          // optimize for speed

#ifdef OPT_MATCH_TRIGO
#define SC_PM_MAX 1024                           // must be >= max graph x size in pixels
static sc_t sc_pm[1024];

static void def_sc_pm(int wx, double angle_range, double phi0, double d_phi)
{
  double phi;
  sc_t *sc = sc_pm;
  sc_t *sc_end = sc_pm + wx;
  W_ASSERT(wx < SC_PM_MAX);
  for (phi=phi0; sc<sc_end; phi += d_phi, sc++)
    sc_rotate(sc, phi);
}
#endif // OPT_MATCH_TRIGO

// match for fixed theta signal value
static void def_graph_match_range(void)
{
  struct fgr_t *fgr = fgr_init(&fgr0, false);
  // get gui config
  double theta_signal = DEG_TO_RAD(wg_edit_box_num_get_value(dlg.phase_m.eds_theta_signal->ed));
  double angle_range  = DEG_TO_RAD(dlg.phase_m.zoom.gv.an_range);
  double theta0       = DEG_TO_RAD(dlg.phase_m.zoom.gv.theta);
  double phi0         = DEG_TO_RAD(dlg.phase_m.zoom.gv.phi);
  bool phi_signal_adj = wg_get_state(dlg.phase_m.ckb_phi_signal_opt);

  int x, y;
  double theta;
  double d_theta = angle_range / (fgr->wy-1);    // include limits [0..PI] for symetrical graph drawing
  double d_phi   = angle_range / (fgr->wx-1);
  struct fgr_pix_t *p = fgr->pix;

  // set fixed values
  pdc_set_theta_signal(theta_signal);            // fixed
  if (!phi_signal_adj)
  {
    double phi_signal = DEG_TO_RAD(wg_edit_box_num_get_value(dlg.phase_m.eds_phi_signal->ed));
    pdc_set_phi(phi_signal);
  }

#ifndef OPT_MATCH_TRIGO
  // ref code
  for (y=0, theta=theta0 + angle_range; y<fgr->wy; y++, theta -= d_theta)  // flip y (bitmap 0 is up)
  {
    double phi;
    int i_phi;
    for (x=0, phi=phi0; x<fgr->wx; x++, phi += d_phi, p++)
    {
      double e;
      pump_rotate(theta, phi);
      if (!phi_signal_adj)
      {
        pho_update(&st.signal);
        pdc_set_theta_idler();
        pho_update(&st.idler);
        e = get_k_diff2();
      }
      else
        e = get_min_pm_error_adjust_phi(&i_phi, x == 0);
      p->i = (float)e;
    }
  }
#else
  // speed optimized
  def_sc_pm(fgr->wx, angle_range, phi0, d_phi);
  for (y=0, theta=theta0 + angle_range; y<fgr->wy; y++, theta -= d_theta)  // flip y (bitmap 0 is up)
  {
    int i_phi;
    sc_t *sc = sc_pm;
    sc_rotate(&st.pump.rot.theta, theta);
    for (x=0; x<fgr->wx; x++, p++, sc++)
    {
      double e;
      st.pump.rot.phi = *sc;
      pump_update();
      if (!phi_signal_adj)
      {
        pho_update(&st.signal);
        pdc_set_theta_idler();
        pho_update(&st.idler);
        e = get_k_diff2();
      }
      else
        e = get_min_pm_error_adjust_phi(&i_phi, x == 0);
      p->i = (float)e;
    }
  }
#endif

  fgr_def_min_max();

  // set display data states
  dlg.graph.disp.e_displayed = e_graph_phase_match;
  dlg.graph.disp.gv = dlg.phase_m.zoom.gv;
}

// update display filter for graph_update if displayed
void draw_graph_phase_match_update_filters(hwin_t *hw, bool upd_filters)
{
  struct fgr_t *fgr = &fgr0;
  struct gr_view_t *gv;
  if (dlg.graph.disp.e_displayed != e_graph_phase_match)
    return;

  fgr->min_scale = wg_get_state(dlg.phase_m.ckb_scale);
  fgr->n_sqrt = dlg.phase_m.lbl_contrast_str[7] - '0';
  fgr->revert = wg_get_state(dlg.phase_m.ckb_revert);
  gv = wg_get_state(dlg.phase_m.ckb_grid) ? &dlg.phase_m.zoom.gv : NULL;
  if (upd_filters)
    fgr_filter(fgr);
  fgr_blit(hw, "k diff. :", gv);
}

// draw phase matching graph
void draw_graph_phase_match(hwin_t *hw)
{
  if (!check_pm_config(hw))
    return;

  def_graph_match_range();

  // update screen
  draw_graph_phase_match_update_filters(hw, true);
}

// ----------------------------------------------
// PDC cones graph
// ----------------------------------------------

static struct fgr_t fgr1;                        // separate screen for idler

// draw units grid
static void draw_proj_screen_grid(hwin_t *hw, bitmap_t *bm, struct gr_view_t *gv)
{
  struct fgr_t *fgr = &fgr0;
#if 0
  const char *x_axis_name = "Phi";
  const char *y_axis_name = "Theta";
#endif
  const char *unit_fmt = "%.1f";
  font_t *fnt = win_font_list[fnt_vthm8];

  int ws, l_id;
  char num_str[32];
  float x, y, x1, y1, theta, phi;
  float angle_range    = gv->an_range;
  float theta0         = gv->theta;
  float phi0           = gv->phi;

  #define PS_DIV_H 10.0f  // coarse grid
  #define PS_DIV_L 5.0f   // fine grid

  float d_x = (fgr->wx-1) / (PS_DIV_H*PS_DIV_L);
  float d_y = (fgr->wy-1) / (PS_DIV_H*PS_DIV_L);
  float d_theta = angle_range / PS_DIV_H;
  float d_phi   = angle_range / PS_DIV_H;

  pix_t col_txt = COL_RGB(255, 255, 255);
  pix_t col_pix = COL_RGB(255, 255, 255);

#if 0
  bm_draw_string(bm, 2, 0, y_axis_name, col_txt, 0, fnt, -1);
  sprintf(num_str, unit_fmt, theta0 + angle_range);
  bm_draw_string(bm, 2, fnt->dy-1, num_str, col_txt, 0, fnt, -1);
#else
  sprintf(num_str, unit_fmt, theta0 + angle_range);
  bm_draw_string(bm, 2, 0, num_str, col_txt, 0, fnt, -1);
#endif
  for (y = d_y*PS_DIV_L, y1 = d_y*(PS_DIV_L*PS_DIV_H) - 1, theta = theta0 + angle_range - d_theta, l_id = 0; y < y1; y += d_y*PS_DIV_L, theta -= d_theta, l_id++)
  {
    int ys = (int)y;
    float d_xx = (l_id == ((int)(PS_DIV_H/2)-1)) ? d_x*0.5f : d_x;
    sprintf(num_str, unit_fmt, theta);
    bm_draw_string(bm, 2, ys - (fnt->dy >> 1), num_str, col_txt,  0, fnt, -1);
    for (x = d_x*(PS_DIV_L-2), x1 = d_x*(PS_DIV_L*PS_DIV_H - 0); x < x1; x += d_xx)
      bm_put_pixel(bm, (int)x, ys, col_pix);
  }

#if 0
  ws = font_get_string_width(x_axis_name, fnt);
  bm_draw_string(bm, fgr->wx - ws - 1, fgr->wy - 2*fnt->dy, x_axis_name, col_txt, 0, fnt, -1);
#endif
  sprintf(num_str, unit_fmt, phi0 + angle_range);
  ws = font_get_string_width(num_str, fnt);
  bm_draw_string(bm, fgr->wx - ws, fgr->wy - fnt->dy, num_str, col_txt, 0, fnt, -1);
  for (x = d_x*PS_DIV_L, x1 = d_x*(PS_DIV_L*PS_DIV_H) - 1, phi = phi0 + d_phi, l_id = 0; x < x1; x += d_x*PS_DIV_L, phi += d_phi, l_id++)
  {
    int xs = (int)x;
    float d_yy = (l_id == ((int)(PS_DIV_H/2)-1)) ? d_y*0.5f : d_y;
    sprintf(num_str, unit_fmt, phi);
    ws = font_get_string_width(num_str, fnt);
    bm_draw_string(bm, xs - (ws >> 1), fgr->wy - fnt->dy, num_str, col_txt,  0, fnt, -1);
    for (y = d_y, y1 = d_y*(PS_DIV_L*PS_DIV_H - 2); y < y1; y += d_yy)
      bm_put_pixel(bm, xs, (int)y, col_pix);
  }

  sprintf(num_str, "Grid step %.1f deg.", d_theta);
  ws = font_get_string_width(num_str, fnt);
  bm_draw_string(bm, fgr->wx - ws - 1, 0, num_str, col_txt, 0, fnt, -1);
}

// copy graphic datas to graph bitmap
static void fgr_blit_merge(hwin_t *hw, int i_col_s, int i_col_i, struct gr_view_t *gv)
{
  struct fgr_t *fgr = &fgr0;                     // destination
  struct fgr_pix_t *p0 = fgr0.pix;
  struct fgr_pix_t *p1 = fgr1.pix;

  widget_t *wg = dlg.graph.wg_graph;
  bitmap_t bm;
  pix_t *pix_line;
  int y;

  bm_init_child(&bm, &hw->cli_bm, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
  pix_line = bm.pix_ptr;

  if (i_col_s && !i_col_i)
  {
    for (y=0, pix_line = bm.pix_ptr; y<fgr->wy; y++, pix_line += bm.l_size)
    {
      pix_t *pix = pix_line;
      pix_t *pix_end = pix_line + bm.size.x;
      for (;pix < pix_end; pix++, p0++)
         *pix = (unsigned int)(p0->i_f * 255.99)*i_col_s;
    }
  }
  else
  if (!i_col_s && i_col_i)
  {
    for (y=0, pix_line = bm.pix_ptr; y<fgr->wy; y++, pix_line += bm.l_size)
    {
      pix_t *pix = pix_line;
      pix_t *pix_end = pix_line + bm.size.x;
      for (;pix < pix_end; pix++, p0++, p1++)
        if (p1->i)
          *pix = (unsigned int)(p1->i_f * 255.99)*i_col_i;
        else
          *pix = 0;
    }
  }
  else
  if (i_col_s && i_col_i)
  {
    for (y=0, pix_line = bm.pix_ptr; y<fgr->wy; y++, pix_line += bm.l_size)
    {
      pix_t *pix = pix_line;
      pix_t *pix_end = pix_line + bm.size.x;
      for (;pix < pix_end; pix++, p0++, p1++)
        if (p1->i)
        {
          unsigned int is = (unsigned int)(p0->i_f * 255.99)*i_col_s;
          unsigned int ii = (unsigned int)(p1->i_f * 255.99)*i_col_i;
          *pix = ((is & 0xfefefe) >> 1) + ((ii & 0xfefefe) >> 1);
        }
        else
          *pix = 0;
    }
  }
  else  // no draw !
    bm_paint(&bm, COL_RGB(0, 0, 64));

  // grid
  if (gv)
    draw_proj_screen_grid(hw, &bm, gv);

  // 3D legend
  draw_graph_legend(&bm, "k diff. :");

  // blit
  win_cli_blit_rect(hw, wg->c_pos.x, wg->c_pos.y, wg->c_size.x, wg->c_size.y);
}

// interpolate using neighbour unexposed pixels on projection screen (occur with idler)
static void fgr1_interpolate(void)
{
  struct fgr_t *fgr = &fgr1;
  int wx = fgr->wx;
  struct fgr_pix_t *fp_end = fgr->pix + fgr->wx*(fgr->wx-1);
  struct fgr_pix_t *fp = fgr->pix + wx;
  struct fgr_pix_t *fp_u = fp - wx;
  struct fgr_pix_t *fp_l = fp - 1;
  struct fgr_pix_t *fp_r = fp + 1;
  struct fgr_pix_t *fp_d = fp + wx;

  for (; fp < fp_end; fp++, fp_u++, fp_l++, fp_r++, fp_d++)
  {
    if (fp->i)                                   // projection on this point
      fp->i_f = fp->i;                           // note: i_f used temporary
    else
    {
      int n = 0;
      float avg = 0;
      if (fp_u->i) { avg  = fp_u->i; n = 1; }
      if (fp_l->i) { avg += fp_l->i; n++; }
      if (fp_r->i) { avg += fp_r->i; n++; }
      if (fp_d->i) { avg += fp_d->i; n++; }
      if (n)
        fp->i_f = avg / n;
      else
      {
        if (fp_u[-1].i) { avg  = fp_u[-1].i; n = 1; }
        if (fp_u[ 1].i) { avg += fp_u[ 1].i; n++; }
        if (fp_d[-1].i) { avg += fp_d[-1].i; n++; }
        if (fp_d[ 1].i) { avg += fp_d[ 1].i; n++; }
        fp->i_f = (n > 0) ? avg / n : 0;
      }
    }
  }

  // copy back i_f => i
  for (fp = fgr->pix + wx; fp < fp_end; fp++)
    fp->i = fp->i_f;
}

// ---------------------------------------------

// projection screen parameters
struct ps_t
{
  double fov;                                    // horizontal fov (not half)
  double d_scr;                                  // screen distance (in pixels units)
  double cx;                                     // screen center offset
  double cy;
  double k_fov;                                  // fov/wx
};

static struct ps_t ps;

static void scr_project_init(int scr_wx, int scr_wy, double fov) // define horizontal fov, aspect ratio 1:1
{
  ps.fov = fov;                                  // save
  ps.d_scr = ((float)scr_wx)/(2*tanf(fov/2));    // screen distance in pixels units
  ps.cx = scr_wx*0.5 + 0.5;
  ps.cy = scr_wy*0.5 + 0.5;
  ps.k_fov = fov / scr_wx;
}

// projection use s direction vector (normalized)
static void scr_project_s(struct fgr_t *fgr, vec3 *s, float i)
{
  double kz = ps.d_scr/s->z;                     // x/z => tan
  int x = (int)(ps.cx + kz*s->x);
  if ((unsigned int)x < (unsigned int)fgr->wx)
  {
    int y = (int)(ps.cy + kz*s->y);
    if ((unsigned int)y < (unsigned int)fgr->wy)
    {
      struct fgr_pix_t *p = fgr->pix + (y*fgr->wx) + x;
      if ((!p->i) || (i < p->i))                 // get min k diff
        p->i = i;
    }
  }
}

// project signal/idler on screen
static void def_graph_proj_scr(void)
{
  // clear bitmap for cummulative or max intensity projection
  struct fgr_t *fgr_s = fgr_init(&fgr0, false);
  struct fgr_t *fgr_i = fgr_init(&fgr1, true);

  // get settings from gui
  float scr_fov_deg    = wg_edit_box_num_get_value(dlg.proj_scr.eds_proj_scr_fov->ed);
  float theta_pump_deg = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_theta->ed);
  float phi_pump_deg   = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_phi->ed);

  double scr_fov    = DEG_TO_RAD(scr_fov_deg);
  double theta_pump = DEG_TO_RAD(theta_pump_deg);
  double phi_pump   = DEG_TO_RAD(phi_pump_deg);

  struct fgr_pix_t *p = fgr_s->pix;
  int wx = fgr_s->wx;
  int wy = fgr_s->wy;
  int xs, ys;

  // use normalized local screen for -1..1 range
  double d_scr = 1.0 / tan(scr_fov/2);
  double dw = 2.0 / wx;
  double y = (-wy*dw)/2;                         // fov defined with wx, adjust y initial
  double d_scr2 = d_scr*d_scr;

  // init screens signal/idler
  scr_project_init(wx, wy, scr_fov);
  pump_rotate(theta_pump, phi_pump);             // main beam orientation
  
  for (ys=0; ys<wy; ys++, y+=dw)
  {
    double x, y2 = y * y;
    for (xs=0, x=-1.0; xs<wx; xs++, x+=dw, p++)
    {
      float diff;
      double x2 = x * x;
#if 0
      // use angles
      double theta = atan2(sqrt(x2 + y2), d_scr);
      double phi = atan2(y, x);
      pdc_set_phi(phi);
      pdc_set_theta_signal(theta);
#else
      // optim, use base trigo to avoid use of atan
      double cos_theta, sin_theta, cos_phi, sin_phi;
      double x2_plus_y2 = x2 + y2;
      if (x2_plus_y2 > 0.0000001)                // at least 1 pixel radius
      {
        double i_h2;
        double h1 = sqrt(x2_plus_y2);
        double i_h1 = 1.0 / h1;
        cos_phi = x*i_h1;
        sin_phi = y*i_h1;
        i_h2 = 1.0 / sqrt(d_scr2 + x2_plus_y2);
        cos_theta = d_scr * i_h2;
        sin_theta = h1 * i_h2;
      }
      else
      {
        cos_phi = 1.0;
        sin_phi = 0.0;
        cos_theta = 1.0;
        sin_theta = 0.0;
      }

      // set signal
      st.signal.rot.theta.s = sin_theta;
      st.signal.rot.theta.c = cos_theta;
      st.signal.rot.phi.s = sin_phi;
      st.signal.rot.phi.c = cos_phi;

      // set phi idler to signal opposite phi
      st.idler.rot.phi.s = -sin_phi;   // = sin(phi + PI)
      st.idler.rot.phi.c = -cos_phi;   // = cos(phi + PI)
#endif

      pho_update(&st.signal);
      pdc_set_theta_idler();                     // depend of signal, require up to date signal
      pho_update(&st.idler);

      diff = (float)get_k_diff2();

#if 1
      // projet signal
      p->i = diff;                               // direct copy, avoid projection
#else
      // projet signal
      scr_project_s(fgr_s, &st.signal.s, diff);  // use projection (require fgr clear before)
#endif
      // projet idler
      scr_project_s(fgr_i, &st.idler.s, diff);

#if 0
      // check projection identity (angle to pixel)
      {
        vec3 *s = &st.signal.s;
        double kz = ps.d_scr/s->z;
        int xe = (int)(ps.cx + kz*s->x);
        int ye = (int)(ps.cy + kz*s->y);
        if ((abs(xe - xs) > 1) || (abs(ye - ys) > 1))
          __asm int 3;
      }
#endif
    }
  }

  fgr1_interpolate();
  fgr_def_min_max();
  fgr_i->i_min = fgr_s->i_min;
  fgr_i->i_max = fgr_s->i_max;

  // set display states
  dlg.graph.disp.e_displayed = e_graph_proj_scr;
  dlg.graph.disp.gv.an_range = scr_fov_deg;
  dlg.graph.disp.gv.theta = - scr_fov_deg*0.5f;
  dlg.graph.disp.gv.phi   = - scr_fov_deg*0.5f;
}

// update display filter for graph update if displayed
void draw_graph_proj_scr_update_filters(hwin_t *hw, bool upd_filters)
{
  struct gr_view_t *gv;
  bool show_signal, show_idler, colorize;
  int i_col_s = 0;
  int i_col_i = 0;

  if (dlg.graph.disp.e_displayed != e_graph_proj_scr)
    return;

  gv = wg_get_state(dlg.proj_scr.ckb_grid) ? &dlg.graph.disp.gv : NULL;

  show_signal = wg_get_state(dlg.proj_scr.ckb_show_signal);
  show_idler = wg_get_state(dlg.proj_scr.ckb_show_idler);
  colorize = wg_get_state(dlg.proj_scr.ckb_colorize);

  fgr0.min_scale = wg_get_state(dlg.proj_scr.ckb_scale);
  fgr0.n_sqrt = dlg.proj_scr.lbl_contrast_str[7] - '0';
  fgr0.revert = wg_get_state(dlg.proj_scr.ckb_revert);

  fgr1.min_scale = fgr0.min_scale;
  fgr1.n_sqrt  = fgr0.n_sqrt;
  fgr1.revert  = fgr0.revert;

  #define ICOL_GREY   0x10101
#if 1
  #define ICOL_SIGNAL 0x10100
  #define ICOL_IDLER  0x00101
#endif
#if 0
  #define ICOL_SIGNAL 0x10001
  #define ICOL_IDLER  0x00101
#endif
#if 0
  #define ICOL_SIGNAL 0x10001
  #define ICOL_IDLER  0x10100
#endif
#if 0
  #define ICOL_SIGNAL 0x10001
  #define ICOL_IDLER  0x10100
#endif

  if (show_signal)
  {
    if (upd_filters)
      fgr_filter(&fgr0);
    i_col_s = colorize ? ICOL_SIGNAL : ICOL_GREY;
  }

  if (show_idler)
  {
    if (upd_filters)
      fgr_filter(&fgr1);
    i_col_i = colorize ? ICOL_IDLER : ICOL_GREY;
  }

  // copy graphic datas to graph bitmap
  fgr_blit_merge(hw, i_col_s, i_col_i, gv);
}

// draw projection screen graph
void draw_graph_proj_scr(hwin_t *hw)
{
  if (!check_pm_config(hw))
    return;

  // draw projection
  def_graph_proj_scr();

  // update screen
  draw_graph_proj_scr_update_filters(hw, true);
}
