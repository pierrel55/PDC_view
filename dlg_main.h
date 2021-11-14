#include "dlg_console.h"               // common block

// grid/display view range
struct gr_view_t
{
  float an_range;                      // range
  float theta;                         // start theta
  float phi;                           // start phi
};

// current displayed graph type
enum e_graph_display
{
  e_graph_undef = 0,                   // not phase match or cone projection
  e_graph_phase_match,
  e_graph_proj_scr,
};

// define widgets list for main dialog
struct dlg_init_wg_t
{
  // frame graph
  struct
  {
    widget_t *frm;
    widget_t *wg_graph;

    // current displayed graph states
    struct gr_disp_t
    {
      enum e_graph_display e_displayed;
      struct gr_view_t gv;
    } disp;
  } graph;

  // frame crystal
  struct
  {
    widget_t *frm;
    const char *crystal_names_list[ct_list_size];
    widget_t *cbo_type;
    wledns_t *eds_temp;
    char crystal_lambda_range_str[100];
    widget_t *lbl_lambda_range;
    widget_t *bt_draw_n_lambda;
  } crystal;

  // frame phase matching
  struct
  {
    widget_t *frm;
    wlednsc_t *eds_lambda_pump;
    char crystal_indices_pump_str[100];
    widget_t *lbl_indices_pump;
    
    widget_t *frm_n_graph;
    widget_t *bt_draw_n_slow;
    widget_t *bt_draw_n_fast;
    widget_t *bt_draw_n_slow_minus_n_fast;
    widget_t *bt_draw_n_diff_avg;

    widget_t *frm_lambda;
    widget_t *ckb_lamda_dbl;
    wledns_t *eds_lambda_signal;
    char lambda_idler_str[100];
    widget_t *lbl_lambda_idler;
    widget_t *lbl_indices_signal_name;
    widget_t *lbl_indices_idler_name;
    char crystal_indices_signal_str[100];
    widget_t *lbl_indices_signal;
    char crystal_indices_idler_str[100];
    widget_t *lbl_indices_idler;

    widget_t *frm_angles;
    wledns_t *eds_theta_signal;
    widget_t *frm_phi_signal;
    wledns_t *eds_phi_signal;
    widget_t *ckb_phi_signal_opt;

    widget_t *frm_type;
    widget_t *bt_radio_t1;
    widget_t *bt_radio_t2;

    widget_t *frm_graph;
    widget_t *bt_draw_pm;

    widget_t *frm_filters;
    char lbl_contrast_str[16];  // "Cont.: 2"
    widget_t *lbl_contrast;
    widget_t *spin_contrast;
    widget_t *ckb_grid;
    widget_t *ckb_revert;
    widget_t *ckb_scale;

    // zoom
    widget_t *frm_zoom;
    widget_t *bt_zoom_plus;
    widget_t *bt_zoom_minus;
    widget_t *bt_zoom_x1;
    widget_t *bt_zoom_center;
    struct
    {
      int scale;                     // zoom scale
      struct gr_view_t gv;           // view range and origin offset
      struct
      {
        float theta;
        float phi;
      } click;                       // pm graph clicked pos
    } zoom;

    widget_t *ckb_cpy_pump;
    char lbl_zoom_select_str[100];   // click select position
    widget_t *lbl_zoom_select;
  } phase_m;

  // frame projection screen (pdc cones)
  struct
  {
    widget_t *frm_pdc_cones;
    widget_t *frm_crystal_cut;
    wledns_t *eds_beam_theta;
    wledns_t *eds_beam_phi;
    widget_t *bt_cc_auto_find;
    widget_t *bt_cc_adjust;
    widget_t *bt_cc_zoom;

    widget_t *frm_proj_screen;
    wledns_t *eds_proj_scr_fov;
    widget_t *bt_draw_cones;
    widget_t *ckb_show_signal;
    widget_t *ckb_show_idler;
    widget_t *frm_filters;
    char lbl_contrast_str[16];  // "Cont.: 2"
    widget_t *lbl_contrast;
    widget_t *spin_contrast;
    widget_t *ckb_grid;
    widget_t *ckb_revert;
    widget_t *ckb_colorize;
    widget_t *ckb_scale;
  } proj_scr;

  // datas
};

extern struct dlg_init_wg_t dlg;
