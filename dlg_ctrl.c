// contain all call-backs called when gui configuration is changed
// dispatch event to application functions
#include <stdio.h>
#include <math.h>
#include "win/widget/widget.h"
#include "PDC/pdc.h"
#include "dlg_main.h"
#include "dlg_graph.h"
#include "dlg_ctrl.h"

// ----------------------------------------------
// datas for inits or update

// UVA 400 nm - 320 nm
// UVB 320 nm - 290 nm
// UVC 290 nm - 100 nm

// some lamba for commercial laser diodes (except UV)
const char *laser_std_lambda[N_STD_LAMBDA+1] = 
{
  "275 nm  UV(C)",
  "295 nm  UV(B)",
  "365 nm  UV(A)",
  "405 nm  blue near UV",
  "450 nm  blue",
  "488 nm  blue-green",
  "520 nm  green",
  "635 nm  light red",
  "650 nm  red",
  "780 nm  dark red",
  "820 nm  IR",
  "850 nm  IR",
  "905 nm  IR",
  "942 nm  IR",
  "1310 nm  IR",
  NULL
};

// define crystal names list
const char **get_crystal_names_list(void)
{
  enum e_crystal_type ct;
  for (ct = 0; ct < ct_list_size; ct++)
  {
    struct crystal_info_t inf;
    get_crystal_index(ct, 0, 0, NULL, &inf);
    dlg.crystal.crystal_names_list[ct] = inf.name;
  }
  return dlg.crystal.crystal_names_list;
}

// define crystal lambda range string
const char *get_crystal_lambda_range_str(void)
{
  get_crystal_index(pdc.crystal_type, 0, 0, NULL, &pdc.crystal_info);
  sprintf(dlg.crystal.crystal_lambda_range_str, "Lambda range: %d .. %d nm", 
    (int)pdc.crystal_info.min_lambda_nm, (int)pdc.crystal_info.max_lambda_nm);
  return dlg.crystal.crystal_lambda_range_str;
}

// define indice string
static char *def_ind_str(vec3 *n2, const char *name, char *str)
{
  if (n2->x == n2->y)
    sprintf(str, "%s x = y = %.4f  z = %.4f", name, sqrt(n2->x), sqrt(n2->z));
  else
    sprintf(str, "%s x = %.4f  y = %.4f  z = %.4f", name, sqrt(n2->x), sqrt(n2->y), sqrt(n2->z));
  return str;
}

const char *get_pump_crystal_indices_str(void)
{
  if (st.pump.ind.ind2.x == 1)
  {
    if (pdc.lambda_pump_nm > pdc.crystal_info.max_lambda_nm)
      return "Pump indices : ERROR: lambda too long.";
    return "Pump indices : ERROR: lambda too short.";
  }
  return def_ind_str(&st.pump.ind.ind2, "Pump indices : ", dlg.phase_m.crystal_indices_pump_str);
}

const char *get_signal_crystal_indices_str(void)
{
  if (st.signal.ind.ind2.x == 1)
    return ": ERROR: Invalid configuration.";
  return def_ind_str(&st.signal.ind.ind2, ":", dlg.phase_m.crystal_indices_signal_str);
}

const char *get_idler_crystal_indices_str(void)
{
  if (st.idler.ind.ind2.x == 1)
    return ": ERROR: Invalid configuration.";
  return def_ind_str(&st.idler.ind.ind2, ":", dlg.phase_m.crystal_indices_idler_str);
}

// define lambda idler string
const char *get_lambda_idler_str(void)
{
  if (!pdc.conf_valid)
    return "Lambda idler = ERROR: Invalid configuration.";
  sprintf(dlg.phase_m.lambda_idler_str, "Lambda idler = %.0f nm", pdc.lambda_idler_nm);
  return dlg.phase_m.lambda_idler_str;
}

// ----------------------------------------------
// update function with logic

// update crystal indices labels
static void update_indices_labels(hwin_t *hw)
{
  wg_text_update(hw, dlg.phase_m.lbl_indices_pump, get_pump_crystal_indices_str());
  wg_text_update(hw, dlg.phase_m.lbl_indices_signal, get_signal_crystal_indices_str());
  wg_text_update(hw, dlg.phase_m.lbl_indices_idler, get_idler_crystal_indices_str());
}

static void update_pdc_gui(hwin_t *hw)
{
  if (wg_get_state(dlg.phase_m.ckb_lamda_dbl))         // lambda signal = 2 * lambda pump checked
    pdc.lambda_signal_nm = 2*pdc.lambda_pump_nm;
  pdc_init();
  wg_edit_box_num_set_value(hw, dlg.phase_m.eds_lambda_signal->ed, pdc.lambda_signal_nm, false);
  wg_text_update(hw, dlg.phase_m.lbl_lambda_idler, get_lambda_idler_str());
  update_indices_labels(hw);
}

// ------------------------------------
// phase matching graph zoom

// zoom center on click position
static void update_zoom_center(hwin_t *hw)
{
  float theta_ofs = dlg.phase_m.zoom.click.theta - dlg.phase_m.zoom.gv.an_range*0.5f;
  float phi_ofs = dlg.phase_m.zoom.click.phi - dlg.phase_m.zoom.gv.an_range*0.5f;

  if (theta_ofs < 1.0f)
    theta_ofs = 0;
  else
  if ((theta_ofs + dlg.phase_m.zoom.gv.an_range) > 179.0f)
    theta_ofs = 180.0f - dlg.phase_m.zoom.gv.an_range;

  if (phi_ofs < 1.0f)
    phi_ofs = 0;
  else
  if ((phi_ofs + dlg.phase_m.zoom.gv.an_range) > 179.0f)
    phi_ofs = 180.0f - dlg.phase_m.zoom.gv.an_range;

  dlg.phase_m.zoom.gv.theta = floorf(theta_ofs);
  dlg.phase_m.zoom.gv.phi = floorf(phi_ofs);

  draw_graph_phase_match(hw);
}

// set a scale
static void set_zoom_scale(int z)
{
  dlg.phase_m.zoom.scale = z;
  dlg.phase_m.zoom.gv.an_range = 180.0f / z;
}

// update pdc graph zoom
static void update_zoom_scale(hwin_t *hw, int dz)
{
  // levels are 1, 2, 4, 10, 20, 40
  int z, z0 = dlg.phase_m.zoom.scale;
  if (dz > 0)
    z = (z0 == 1) ? 2 : (z0 == 2) ? 4 : (z0 == 4) ? 10 : (z0 == 10) ? 20 : (z0 == 20) ? 40 : 40;
  else
  if (dz < 0)
    z = (z0 == 1) ? 1 : (z0 == 2) ? 1 : (z0 == 4) ? 2 : (z0 == 10) ? 4 : (z0 == 20) ? 10 : 20;
  else
    z = 1;

  set_zoom_scale(z);
  update_zoom_center(hw);
}

// update clicked position. refresh only label string
static void update_zoom_click_pos(hwin_t *hw, float theta, float phi)
{
  dlg.phase_m.zoom.click.theta = theta;          // save
  dlg.phase_m.zoom.click.phi = phi;
  sprintf(dlg.phase_m.lbl_zoom_select_str, "Pos: Theta %.1f   Phi %.1f", theta, phi);
  wg_text_update(hw, dlg.phase_m.lbl_zoom_select, dlg.phase_m.lbl_zoom_select_str);
}

// ------------------------------------
// widgets call backs

// combo select crystal action
void cb_combo_select_crystal(hwin_t *hw, int sel_id)
{
  pdc.crystal_type = sel_id;
  pdc.crystal_temp = 20.0;                       // reset to default temp.
  wg_edit_box_num_set_value(hw, dlg.crystal.eds_temp->ed, pdc.crystal_temp, false);
  wg_text_update(hw, dlg.crystal.lbl_lambda_range, get_crystal_lambda_range_str());
  set_zoom_scale(1);
  dlg.phase_m.zoom.gv.theta = 0;
  dlg.phase_m.zoom.gv.phi = 0;
  update_zoom_click_pos(hw, 0, 0);
  update_pdc_gui(hw);
  if (!pdc.conf_valid)
    co_printf("INFO: Some configured lambda are out of crystal transparency range.\n");
}

// graph clicked cb, acquire theta/phi on phase matching graph
void wg_graph_click_cb(hwin_t *hw, widget_t *wg, int x, int y)
{
  if (dlg.graph.disp.e_displayed == e_graph_phase_match) // graph displayed
  {
    int gy = wg->c_size.y - GR_LEGEND_WY;        // graph drawing y size
    if (y < gy)                                  // not legend selected
    {
      float d_theta = (dlg.graph.disp.gv.an_range * (gy - 1 - y))/(gy - 1);
      float theta = dlg.graph.disp.gv.theta + d_theta;
      float d_phi = (dlg.graph.disp.gv.an_range * x)/(wg->c_size.x - 1);
      float phi = dlg.graph.disp.gv.phi + d_phi;
      
      // zoom clicked pos
      update_zoom_click_pos(hw, theta, phi);
      
      // pump coordinates
      if (wg_get_state(dlg.phase_m.ckb_cpy_pump))  // option enabled
      {
        wg_edit_box_num_set_value(hw, dlg.proj_scr.eds_beam_theta->ed, theta, false);
        wg_edit_box_num_set_value(hw, dlg.proj_scr.eds_beam_phi->ed, phi, false);
      }
    }
  }
}

// text button pressed
void cb_text_bt(hwin_t *hw, widget_t *wg)
{
  if (wg == dlg.crystal.bt_draw_n_lambda)
    draw_graph_n_lambda(hw);
  else
  if (wg == dlg.phase_m.bt_draw_n_slow)
    draw_graph_ind_sf(hw, e_ind_s, wg->dr_obj.text->str);
  else
  if (wg == dlg.phase_m.bt_draw_n_fast)
    draw_graph_ind_sf(hw, e_ind_f, wg->dr_obj.text->str);
  else
  if (wg == dlg.phase_m.bt_draw_n_slow_minus_n_fast) 
    draw_graph_ind_sf(hw, e_ind_sf_diff, wg->dr_obj.text->str);
  else
  if (wg == dlg.phase_m.bt_draw_n_diff_avg)
    draw_graph_ind_sf(hw, e_ind_sf_diff_avg, wg->dr_obj.text->str);
  else
  if (wg == dlg.phase_m.bt_draw_pm)
    draw_graph_phase_match(hw);
  else
  if (wg == dlg.phase_m.bt_zoom_plus)
    update_zoom_scale(hw, 1);
  else
  if (wg == dlg.phase_m.bt_zoom_minus)
    update_zoom_scale(hw, -1);
  else
  if (wg == dlg.phase_m.bt_zoom_x1)
    update_zoom_scale(hw, 0);
  else
  if (wg == dlg.phase_m.bt_zoom_center)
    update_zoom_center(hw);
  else
  if (    (wg == dlg.proj_scr.bt_cc_auto_find)
       || (wg == dlg.proj_scr.bt_cc_adjust))
  {
    if (check_pm_config(hw))
    {
      float best_theta_pump = 0;
      float best_phi_pump = 0;
      double theta_signal = DEG_TO_RAD(wg_edit_box_num_get_value(dlg.phase_m.eds_theta_signal->ed));
      double phi_signal = 0;
      bool adjust_phi_signal = wg_get_state(dlg.phase_m.ckb_phi_signal_opt);
      if (!adjust_phi_signal)
        phi_signal = DEG_TO_RAD(wg_edit_box_num_get_value(dlg.phase_m.eds_phi_signal->ed));
      
      // note: return result angles in degree
      if (wg == dlg.proj_scr.bt_cc_auto_find)
        pdc_search_pm(theta_signal, phi_signal, adjust_phi_signal, &best_theta_pump, &best_phi_pump);
      else
      {
        // local adjust current value
        double e;
        best_theta_pump = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_theta->ed);
        best_phi_pump = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_phi->ed);
        e = pdc_adjust_pm_local(theta_signal, phi_signal, adjust_phi_signal, &best_theta_pump, &best_phi_pump);
        co_printf("INFO: k_diff result for local pm adjust = %.5f\n", e);
      }

      wg_edit_box_num_set_value(hw, dlg.proj_scr.eds_beam_theta->ed, best_theta_pump, false);
      wg_edit_box_num_set_value(hw, dlg.proj_scr.eds_beam_phi->ed, best_phi_pump, false);
    }
  }
  else
  if (wg == dlg.proj_scr.bt_cc_zoom)
  {
    // zoom on crystal cut angles
    float theta = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_theta->ed);
    float phi = wg_edit_box_num_get_value(dlg.proj_scr.eds_beam_phi->ed);

    set_zoom_scale(10);                        // scale * 10, 18 deg
    update_zoom_click_pos(hw, theta, phi);     // simulate click
    update_zoom_center(hw);
  }
  else
  if (wg == dlg.proj_scr.bt_draw_cones)
    draw_graph_proj_scr(hw);
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

// check box changed
void cb_check_box_changed(hwin_t *hw, widget_t *wg)
{
  bool checked = wg_get_state(wg);
  if (wg == dlg.phase_m.ckb_lamda_dbl)
  {
    if (checked)
      pdc.lambda_signal_nm = pdc.lambda_pump_nm*2;
    wgb_ledns_enable(hw, dlg.phase_m.eds_lambda_signal, !checked);
    update_pdc_gui(hw);
  }
  else
  if (wg == dlg.phase_m.ckb_phi_signal_opt)
    wgb_ledns_enable(hw, dlg.phase_m.eds_phi_signal, !checked);
  else
  if (wg == dlg.phase_m.ckb_grid)
    draw_graph_phase_match_update_filters(hw, false);
  else
  if (wg == dlg.phase_m.ckb_revert)
    draw_graph_phase_match_update_filters(hw, true);
  else
  if (wg == dlg.phase_m.ckb_scale)
    draw_graph_phase_match_update_filters(hw, true);
  else
  if (wg == dlg.phase_m.ckb_cpy_pump)
  {
  }
  else
  if (wg == dlg.proj_scr.ckb_show_signal)
    draw_graph_proj_scr_update_filters(hw, false);
  else
  if (wg == dlg.proj_scr.ckb_show_idler)
    draw_graph_proj_scr_update_filters(hw, false);
  else
  if (wg == dlg.proj_scr.ckb_grid)
    draw_graph_proj_scr_update_filters(hw, false);
  else
  if (wg == dlg.proj_scr.ckb_revert)
    draw_graph_proj_scr_update_filters(hw, true);
  else
  if (wg == dlg.proj_scr.ckb_scale)
    draw_graph_proj_scr_update_filters(hw, true);
  else
  if (wg == dlg.proj_scr.ckb_colorize)
    draw_graph_proj_scr_update_filters(hw, false);
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

// radio button changed
void cb_radio_button_changed(hwin_t *hw, widget_t *wg)
{
  bool checked = wg_get_state(wg);
  if (wg == dlg.phase_m.bt_radio_t1)
  {
    wg_set_state(hw, dlg.phase_m.bt_radio_t2, !checked);
    pdc.type1 = true;
    pdc_init_type();
  }
  else
  if (wg == dlg.phase_m.bt_radio_t2)
  {
    wg_set_state(hw, dlg.phase_m.bt_radio_t1, !checked);
    pdc.type1 = false;
    pdc_init_type();
  }
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

// spin button up pressed
void cb_spin_button_up(hwin_t *hw, widget_t *wg)
{
  if (wg == dlg.phase_m.spin_contrast)
  {
    char *l = &dlg.phase_m.lbl_contrast_str[7];
    if (*l < '3')
    {
      (*l)++;
      wg_text_update(hw, dlg.phase_m.lbl_contrast, dlg.phase_m.lbl_contrast_str);
      draw_graph_phase_match_update_filters(hw, true);
    }
  }
  else
  if (wg == dlg.proj_scr.spin_contrast)
  {
    char *l = &dlg.proj_scr.lbl_contrast_str[7];
    if (*l < '3')
    {
      (*l)++;
      wg_text_update(hw, dlg.proj_scr.lbl_contrast, dlg.proj_scr.lbl_contrast_str);
      draw_graph_proj_scr_update_filters(hw, true);
    }
  }
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

// spin button down pressed
void cb_spin_button_down(hwin_t *hw, widget_t *wg)
{
  wg = wg->wg_parent;                    // down button is child of up
  if (wg == dlg.phase_m.spin_contrast)
  {
    char *l = &dlg.phase_m.lbl_contrast_str[7];
    if (*l > '0')
    {
      (*l)--;
      wg_text_update(hw, dlg.phase_m.lbl_contrast, dlg.phase_m.lbl_contrast_str);
      draw_graph_phase_match_update_filters(hw, true);
    }
  }
  else
  if (wg == dlg.proj_scr.spin_contrast)
  {
    char *l = &dlg.proj_scr.lbl_contrast_str[7];
    if (*l > '0')
    {
      (*l)--;
      wg_text_update(hw, dlg.proj_scr.lbl_contrast, dlg.proj_scr.lbl_contrast_str);
      draw_graph_proj_scr_update_filters(hw, true);
    }
  }
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

// numeric value changed in edit box
void cb_edit_num_value_changed(hwin_t *hw, widget_t *wg, float value)
{
  if (wg == dlg.crystal.eds_temp->ed)
  {
    pdc.crystal_temp = value;
#if 1
    // need to add temp coeff for crystal in crystal.c
    // inform user.
    if (!pdc.crystal_info.temp_en)
      co_printf("WARNING: temperature correction ignored for this crystal. (no datas).\n");
#endif
    update_pdc_gui(hw);
  }
  else
  if (wg == dlg.phase_m.eds_lambda_pump->ed)
  {
    pdc.lambda_pump_nm = value;
    update_pdc_gui(hw);
  }
  else
  if (wg == dlg.phase_m.eds_lambda_signal->ed)
  {
    pdc.lambda_signal_nm = value;
    update_pdc_gui(hw);
  }
  else
  if (wg == dlg.phase_m.eds_theta_signal->ed)
  {
  }
  else
  if (wg == dlg.phase_m.eds_phi_signal->ed)
  {
  }
  else
  if (wg == dlg.proj_scr.eds_beam_theta->ed)
  {
  }
  else
  if (wg == dlg.proj_scr.eds_beam_phi->ed)
  {
  }
  else
  if (wg == dlg.proj_scr.eds_proj_scr_fov->ed)
  {
  }
  else
  {
    W_ASSERT(0);        // bad ptr ?
  }
}

