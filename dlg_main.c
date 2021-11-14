// init dialog window and widgets
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "win/widget/widget.h"
#include "PDC/pdc.h"
#include "dlg_main.h"
#include "dlg_ctrl.h"
#include "dlg_graph.h"

#define MFRAME_BOLD                              // if defined, use bold text on group frames

#ifdef MFRAME_BOLD                               // use bold drawing (convert tahoma9)
  #define FRM_EX win_font_list[fnt_vthm9], wgs.frame.text_color, bold_aa_color
#else
  #define FRM_EX win_font_list[wgs.frame.text_font_id], wgs.frame.text_color, wgs.frame.text_aa_color
#endif

struct dlg_init_wg_t dlg = { 0 };

// init widget list, define help
static void init_widget_list(hwin_t *hw)
{
#ifdef MFRAME_BOLD                        // init bold font and add 1 pixel space between chars.
  pix_t bold_aa_color = font_gen_bold(win_font_list[fnt_vthm9], wgs.frame.text_aa_color, win_font_list[fnt_vthm9], 1);
#endif

  // -----------------------------------
  // messages console
  dlg_console_init_widgets(hw, FRM_EX);

  // -----------------------------------
  // frame graph
  dlg.graph.frm = wg_init_frame_ex(hw, "Graph", FRM_EX);
  dlg.graph.wg_graph = wg_init_graph(hw, NULL);
  wg_graph_set_click_cb(dlg.graph.wg_graph, wg_graph_click_cb);

  // -----------------------------------
  // crystal frame
  dlg.crystal.frm = wg_init_frame_ex(hw, "Crystal type", FRM_EX);
  dlg.crystal.cbo_type = wg_init_combo_box(hw, get_crystal_names_list(), ct_list_size, pdc.crystal_type, 4, 2, 80, cb_combo_select_crystal);
  dlg.crystal.eds_temp = wgb_ledns_init(hw, "Crystal Temperature (°C)", pdc.crystal_temp, -40, 100, 1, 2, 0, cb_edit_num_value_changed,
                         "Crystal temperature in degree Celcius");
  dlg.crystal.lbl_lambda_range = wg_init_text(hw, get_crystal_lambda_range_str());
  dlg.crystal.lbl_lambda_range->help_text =
    "Crystal transparency lambda range.";
  dlg.crystal.bt_draw_n_lambda = wg_init_text_button(hw, "Draw n(lambda)", cb_text_bt);
  dlg.crystal.bt_draw_n_lambda->help_text =
    "Draw crystal indices n_x(lambda), n_y(lambda), n_z(lambda)";

  // -----------------------------------
  // phase matching frame
  dlg.phase_m.frm = wg_init_frame_ex(hw, "Phase matching", FRM_EX);
  dlg.phase_m.eds_lambda_pump = wgb_lednsc_init(hw, "Lambda pump (nm)", pdc.lambda_pump_nm, 150.0f, 3000.0f, 100.0f, 0, 0,
                                laser_std_lambda, N_STD_LAMBDA, cb_edit_num_value_changed,
    "Define lambda for pump in nanometers.");
  dlg.phase_m.lbl_indices_pump = wg_init_text(hw, get_pump_crystal_indices_str());
  dlg.phase_m.lbl_indices_pump->help_text = "Crystal indices for lamda pump.";

  dlg.phase_m.frm_n_graph = wg_init_frame(hw, "Indices graph");
  dlg.phase_m.bt_draw_n_slow = wg_init_text_button(hw, "n Slow", cb_text_bt);
  dlg.phase_m.bt_draw_n_slow->help_text =
    "Draw n_slow(theta, phi)\n"
    "y axis : theta [0..PI]\nx axis : phi [0..PI]";
  dlg.phase_m.bt_draw_n_fast = wg_init_text_button(hw, "n Fast", cb_text_bt);
  dlg.phase_m.bt_draw_n_fast->help_text =
    "Draw n_fast(theta, phi)\n"
    "y axis : theta [0..PI]\nx axis : phi [0..PI]";
  dlg.phase_m.bt_draw_n_slow_minus_n_fast = wg_init_text_button(hw, "(S-F)", cb_text_bt);
  dlg.phase_m.bt_draw_n_slow_minus_n_fast->help_text =
    "Draw n_slow - n_fast\n"
    "White color show region where difference is high.\n"
    "y axis : theta [0..PI]\nx axis : phi [0..PI]";
  dlg.phase_m.bt_draw_n_diff_avg = wg_init_text_button(hw, "((S-F)-a)²", cb_text_bt);
  dlg.phase_m.bt_draw_n_diff_avg->help_text =
    "Draw ((n_slow - n_fast) - average(n_slow - n_fast)) squared.\n"
    "Average is sum of (n_slow - n_fast) / (graph pixels count).\n"
    "y axis : theta [0..PI]\nx axis : phi [0..PI]";

  dlg.phase_m.frm_lambda = wg_init_frame(hw, "Lambda signal/idler");
  dlg.phase_m.ckb_lamda_dbl = wg_init_check_box(hw, "Set lambda signal to 2 * Lamda pump.", cb_check_box_changed);
  dlg.phase_m.ckb_lamda_dbl->help_text =
    "Auto define signal lamda wavelength as 2 * lambda pump.";
  dlg.phase_m.eds_lambda_signal = wgb_ledns_init(hw, "Lambda signal (nm)", pdc.lambda_signal_nm, 250.0f, 3000.0f, 100.0f, 0, 0, cb_edit_num_value_changed,
    "Define lambda for signal in nanometers.");
  dlg.phase_m.lbl_lambda_idler = wg_init_text(hw, get_lambda_idler_str());
  dlg.phase_m.lbl_lambda_idler->help_text = "Auto adjusted lambda idler value to keep w pump = w signal + w idler";

  dlg.phase_m.lbl_indices_signal_name = wg_init_text(hw, "Signal indices");
  dlg.phase_m.lbl_indices_signal_name->help_text = "Crystal indices for lamda signal.";
  dlg.phase_m.lbl_indices_signal = wg_init_text(hw, get_signal_crystal_indices_str());
  dlg.phase_m.lbl_indices_signal->help_text = dlg.phase_m.lbl_indices_signal_name->help_text;

  dlg.phase_m.lbl_indices_idler_name = wg_init_text(hw, "Idler indices");
  dlg.phase_m.lbl_indices_idler_name->help_text = "Crystal indices for lamda idler.";
  dlg.phase_m.lbl_indices_idler = wg_init_text(hw, get_idler_crystal_indices_str());
  dlg.phase_m.lbl_indices_idler->help_text = dlg.phase_m.lbl_indices_idler_name->help_text;

  dlg.phase_m.frm_angles = wg_init_frame(hw, "Angles signal");
  dlg.phase_m.eds_theta_signal = wgb_ledns_init(hw, "Theta signal (deg.)", 3.0f, 0.0f, 45.0f, 0.5f, 2, 0, cb_edit_num_value_changed,
    "Configure theta signal in degree. (detector location)");
  dlg.phase_m.frm_phi_signal = wg_init_frame(hw, "Phi signal");
  dlg.phase_m.eds_phi_signal = wgb_ledns_init(hw, "Phi signal (deg.)", 0, 0, 360.0f, 45.0f, 2, 0, cb_edit_num_value_changed,
    "Configure phi signal in degree. (detector location)");
  dlg.phase_m.ckb_phi_signal_opt = wg_init_check_box(hw, "Phi not fixed, optimized.", cb_check_box_changed);
  dlg.phase_m.ckb_phi_signal_opt->help_text = 
    "If checked, adjust phi signal to produce best phase matching.\n"
    "note: This allow to show where phi can affect phase matching.\n"
    "note: This option have effect only with biaxial crystal or with type 2 PM.";

  dlg.phase_m.frm_type = wg_init_frame(hw, "Matching type");
  dlg.phase_m.bt_radio_t1 = wg_init_radio_button(hw, "Type 1", cb_radio_button_changed);
  dlg.phase_m.bt_radio_t1->help_text = 
    "Configure type 1 phase matching:\n"
    "n pump = fast\nn signal = slow\nn idler = slow";
  dlg.phase_m.bt_radio_t2 = wg_init_radio_button(hw, "Type 2", cb_radio_button_changed);
  dlg.phase_m.bt_radio_t2->help_text =
    "Configure type 2 phase matching:\n"
    "n pump = fast\nn signal = fast\nn idler = slow";

  dlg.phase_m.frm_graph = wg_init_frame(hw, "PM graph");
  dlg.phase_m.bt_draw_pm = wg_init_text_button(hw, "Draw PM", cb_text_bt);
  dlg.phase_m.bt_draw_pm->help_text =
    "Draw phase matching graph.\n"
    "Color intensity is defined by wave vector difference between k_pump and k_signal + k_idler\n"
    "Black area show regions where phase missmatch is minimal.";
  dlg.phase_m.frm_filters = wg_init_frame(hw, "Display filters");
  dlg.phase_m.lbl_contrast = wg_init_text(hw, dlg.phase_m.lbl_contrast_str);
  dlg.phase_m.lbl_contrast->help_text = 
    "Apply contrast filter using n times square root applyed on phase matching result.\n"
    "This allow to keep only regions where phase matchig error is very close to 0";
  dlg.phase_m.spin_contrast = wg_init_spin_button_ud(hw, cb_spin_button_up, cb_spin_button_down);
  dlg.phase_m.ckb_grid =  wg_init_check_box(hw, "Grid", cb_check_box_changed);
  dlg.phase_m.ckb_grid->help_text = 
    "Show/Hide units grid.";
  dlg.phase_m.ckb_revert = wg_init_check_box(hw, "Rev.", cb_check_box_changed);
  dlg.phase_m.ckb_revert->help_text = 
    "Revert black/white color.";
  dlg.phase_m.ckb_scale = wg_init_check_box(hw, "Scale", cb_check_box_changed);
  dlg.phase_m.ckb_scale->help_text = 
    "Define graph min found PM error level as 0 reference for intensity scaling.\n"
    "If unchecked : color intensity = PM error * scale\n"
    "If checked   : color intensity = (PM error - min) * scale\n"
    "min: Min PM value found on hole graph.\n\n"
    "Warning: This mode allow to display small variations but may show\n"
    "phase matching where it cannot occur really.\n"
    "(if value for 'min' is too high to produce PM.)";

  dlg.phase_m.frm_zoom = wg_init_frame(hw, "Graph zoom");
  dlg.phase_m.frm_zoom->help_text =
    "Allow to zoom on phase matching graph.\n"
    "The position to zoom can be defined with a mouse click on the graph\n"
    "and displayed by 'Pos:' label.\n"
    "note: When max scale reached (x40), a mouse click define only new zoom target position.";
  dlg.phase_m.bt_zoom_plus = wg_init_text_button(hw, "Zoon +", cb_text_bt);
  dlg.phase_m.bt_zoom_plus->help_text =
    "Zoom in on PM graph";
  dlg.phase_m.bt_zoom_minus = wg_init_text_button(hw, "Zoon  -", cb_text_bt);
  dlg.phase_m.bt_zoom_minus->help_text =
    "Zoom out on PM graph";
  dlg.phase_m.bt_zoom_x1 = wg_init_text_button(hw, "Z reset", cb_text_bt);
  dlg.phase_m.bt_zoom_x1->help_text =
    "Reset zoom scale to 1:1";
  dlg.phase_m.bt_zoom_center = wg_init_text_button(hw, "Z Center", cb_text_bt);
  dlg.phase_m.bt_zoom_center->help_text =
    "Zoom center on selected graph position (no effect with scale 1:1)";
  dlg.phase_m.lbl_zoom_select = wg_init_text(hw, dlg.phase_m.lbl_zoom_select_str);
  dlg.phase_m.lbl_zoom_select->help_text =
    "Display zoom target position.";
  dlg.phase_m.ckb_cpy_pump = wg_init_check_box(hw, "Set pump.", cb_check_box_changed);
  dlg.phase_m.ckb_cpy_pump->help_text = 
    "If enabled, copy mouse selected position to pump angles in PCD cone frame.";

  // frame projection screen (pdc cones)
  dlg.proj_scr.frm_pdc_cones = wg_init_frame_ex(hw, "PDC Cones", FRM_EX);
  dlg.proj_scr.frm_crystal_cut = wg_init_frame(hw, "Pump angles");
  dlg.proj_scr.eds_beam_theta = wgb_ledns_init(hw, "Theta (deg)", 90.0f, 0.0f, 180.0f, 0.5f, 2, 0, cb_edit_num_value_changed,
    "Define pump theta angle in crystal coordinates.");
  dlg.proj_scr.eds_beam_phi = wgb_ledns_init(hw, "Phi (deg)", 0.0f, 0.0f, 180.0f, 0.5f, 2, 0, cb_edit_num_value_changed,
    "Define pump phi angle in crystal coordinates.");
  dlg.proj_scr.bt_cc_auto_find = wg_init_text_button(hw, "Search", cb_text_bt);
  dlg.proj_scr.bt_cc_auto_find->help_text = 
    "Global search a theta/phy value for configured phase-matching parameters that\n"
    "produce the minimal phase-matching error.\n"
    "note: The minimum found maybe not unique. Several angle configurations\n"
    "may produce same phase matching error. (with epsilon differences)";
  dlg.proj_scr.bt_cc_adjust = wg_init_text_button(hw, "Loc. Adjust", cb_text_bt);
  dlg.proj_scr.bt_cc_adjust->help_text = 
    "Adjust locally (+/- 0.5 deg) selected Theta/Phi to reduce phase matching error.\n"
    "This allow to keep focused on a specific pump theta/phi configuration.";
  dlg.proj_scr.bt_cc_zoom = wg_init_text_button(hw, "PM zoom", cb_text_bt);
  dlg.proj_scr.bt_cc_zoom->help_text = "Display phase match graph zoom centered on selected pump angles.";

  dlg.proj_scr.frm_proj_screen = wg_init_frame(hw, "Projection screen");
  dlg.proj_scr.frm_proj_screen->help_text =
    "Projection screen allow to display signal/idler cones.";

  dlg.proj_scr.eds_proj_scr_fov = wgb_ledns_init(hw, "Horizontal FOV (deg)", 10.0f, 1.0f, 45.0f, 1.0f, 1, 0, cb_edit_num_value_changed,
    "Define screen horizontal field of view in degree.\n"
    "Change value to change zoom level. (max 45 deg.)");

  dlg.proj_scr.bt_draw_cones = wg_init_text_button(hw, "Draw PDC cones", cb_text_bt);
  dlg.proj_scr.bt_draw_cones->help_text =
    "Draw ignal/idler cones.";

  dlg.proj_scr.ckb_show_signal = wg_init_check_box(hw, "Show Signal cone", cb_check_box_changed);
  dlg.proj_scr.ckb_show_signal->help_text = 
    "Show/hide signal cone.";
  dlg.proj_scr.ckb_show_idler = wg_init_check_box(hw, "Show Idler cone", cb_check_box_changed);
  dlg.proj_scr.ckb_show_signal->help_text = 
    "Show/hide idler cone.";

  dlg.proj_scr.frm_filters = wg_init_frame(hw, "Display filters");
  dlg.proj_scr.lbl_contrast = wg_init_text(hw, dlg.proj_scr.lbl_contrast_str);
  dlg.proj_scr.lbl_contrast->help_text = 
    "Apply contrast filter using n times square root applyed on drawing intensity result.\n";
  dlg.proj_scr.spin_contrast = wg_init_spin_button_ud(hw, cb_spin_button_up, cb_spin_button_down);
  dlg.proj_scr.ckb_grid =  wg_init_check_box(hw, "Grid", cb_check_box_changed);
  dlg.proj_scr.ckb_grid->help_text = 
    "Show/Hide units grid.";
  dlg.proj_scr.ckb_revert = wg_init_check_box(hw, "Rev.", cb_check_box_changed);
  dlg.proj_scr.ckb_revert->help_text = 
    "Revert black/white color.";
  dlg.proj_scr.ckb_colorize = wg_init_check_box(hw, "Col.", cb_check_box_changed);
  dlg.proj_scr.ckb_colorize->help_text = 
    "Colorize conversion cones:\n"
    "Signal: Colored in yellow (red + green)\n"
    "Idler: Colored in cyan (blue + gree)\n"
    "When cones overlap, resulting color is green.\n"
    "note: These are arbitrary colors that do not reflect photons wavelength.";
  dlg.proj_scr.ckb_scale = wg_init_check_box(hw, "Scale", cb_check_box_changed);
  dlg.proj_scr.ckb_scale->help_text = 
    "scale intensity using a non 0 value as minimum.\n"
    "(Same effect as the phase-matching filter)";

  // define some startup states
  wg_set_state_init(dlg.phase_m.ckb_lamda_dbl, true);
  wgb_ledns_enable_init(dlg.phase_m.eds_lambda_signal, false);
  wg_set_state_init(dlg.phase_m.bt_radio_t1, true);
  wg_set_state_init(dlg.phase_m.ckb_grid, true);
  wg_set_state_init(dlg.phase_m.ckb_cpy_pump, true);
  wg_set_state_init(dlg.proj_scr.ckb_show_signal, true);
  wg_set_state_init(dlg.proj_scr.ckb_show_idler, true);
  wg_set_state_init(dlg.proj_scr.ckb_revert, true);
}

// -----------------------------------------------------------
// define position of widgets

// ------------------------------------
// place widgets blocks

static void place_widget_block_graph(vec2i *pos, vec2i *size)
{
  wg_set_pos_size(dlg.graph.frm, pos->x, pos->y, size->x, size->y);
  pos->x += wg_place_margin.frm.dx_push;
  pos->y += wg_place_margin.frm.dy_push;
  size->x -= 2*wg_place_margin.frm.dx_push;
  size->y -= wg_place_margin.frm.dy_push + 2*wg_place_margin.frm.dy_pop_frm;
  wg_set_pos_size(dlg.graph.wg_graph, pos->x, pos->y, size->x, size->y);
}

static void place_widget_block_control(vec2i *pos, vec2i *size)
{
  wl_init(pos, size->x, false);  

  // frame crystal
  wl_push_frm(dlg.crystal.frm);
    wl_place(dlg.crystal.cbo_type, wg_place_margin.wg_dy.cbo);
    wlb_place_ledns(dlg.crystal.eds_temp, 100);
    wl_place_h_first(dlg.crystal.lbl_lambda_range, 170, wg_place_margin.wg_dy.but);
    wl_place_h_next(dlg.crystal.bt_draw_n_lambda, 10, 90, wg_place_margin.wg_dy.but, true);
  wl_pop_frm();

  // frame phase matching
  wl_push_frm(dlg.phase_m.frm);
    wlb_place_lednsc(dlg.phase_m.eds_lambda_pump, 100);
    wl_place(dlg.phase_m.lbl_indices_pump, 0);

    wl_push_frm(dlg.phase_m.frm_n_graph);
      wl_place_h_first(dlg.phase_m.bt_draw_n_slow, 50, wg_place_margin.wg_dy.but);
      wl_place_h_next(dlg.phase_m.bt_draw_n_fast, 16, 50, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.phase_m.bt_draw_n_slow_minus_n_fast, 16, 50, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.phase_m.bt_draw_n_diff_avg, 16, 60, wg_place_margin.wg_dy.but, true);
    wl_pop_frm();

    wl_push_frm(dlg.phase_m.frm_lambda);
      wl_place(dlg.phase_m.ckb_lamda_dbl, 0);
      wlb_place_ledns(dlg.phase_m.eds_lambda_signal, 100);
      wl_place(dlg.phase_m.lbl_lambda_idler, 0);

      wl_place_h_first(dlg.phase_m.lbl_indices_signal_name, 65, 0);
      wl_place_h_next(dlg.phase_m.lbl_indices_signal, 5, 0, 0, true);
      wl_place_h_first(dlg.phase_m.lbl_indices_idler_name, 65, 0);
      wl_place_h_next(dlg.phase_m.lbl_indices_idler, 5, 0, 0, true);
    wl_pop_frm();

    wl_push_frm(dlg.phase_m.frm_angles);
    wlb_place_ledns(dlg.phase_m.eds_theta_signal, 100);
      wl_push_frm(dlg.phase_m.frm_phi_signal);
        wlb_place_ledns(dlg.phase_m.eds_phi_signal, 100);
        wl_place(dlg.phase_m.ckb_phi_signal_opt, 0);
      wl_pop_frm();
    wl_pop_frm();

    wl_push_frm(dlg.phase_m.frm_type);
      wl_place_2h_center(dlg.phase_m.bt_radio_t1, 0, dlg.phase_m.bt_radio_t2, 0, 0);
    wl_pop_frm();

    wl_push_frm(dlg.phase_m.frm_graph);
    wl_place(dlg.phase_m.bt_draw_pm, wg_place_margin.wg_dy.but);
      wl_push_frm(dlg.phase_m.frm_filters);
        wl_place_h_first(dlg.phase_m.lbl_contrast, 40, wg_place_margin.wg_dy.but);
        wl_place_h_next(dlg.phase_m.spin_contrast, 6, 16, wg_place_margin.wg_dy.but, false);
        wl_place_h_next(dlg.phase_m.ckb_grid, 18, 0, wg_place_margin.wg_dy.but, false);
        wl_place_h_next(dlg.phase_m.ckb_revert, 10, 0, wg_place_margin.wg_dy.but, false);
        wl_place_h_next(dlg.phase_m.ckb_scale, 10, 0, wg_place_margin.wg_dy.but, true);
      wl_pop_frm();

      wl_push_frm(dlg.phase_m.frm_zoom);
        #define LZOOM_WY 22
        wl_place_h_first(dlg.phase_m.bt_zoom_plus, 50, LZOOM_WY);
        wl_place_h_next(dlg.phase_m.bt_zoom_minus, 10, 50, LZOOM_WY,  false);
        wl_place_h_next(dlg.phase_m.bt_zoom_x1, 14, 50, LZOOM_WY,  false);
        wl_place_h_next(dlg.phase_m.bt_zoom_center, 12, 55, LZOOM_WY,  true);
        
        wl_place_h_first(dlg.phase_m.lbl_zoom_select, 150, 0);
        wl_place_h_next(dlg.phase_m.ckb_cpy_pump, 8, 0, 0, true);
      wl_pop_frm();
    wl_pop_frm();
  wl_pop_frm();

  // frame projection screen (pdc cones)
  wl_push_frm(dlg.proj_scr.frm_pdc_cones);
    wl_push_frm(dlg.proj_scr.frm_crystal_cut);
      wlb_place_ledns(dlg.proj_scr.eds_beam_theta, 100);
      wlb_place_ledns(dlg.proj_scr.eds_beam_phi, 100);
      wl_place_h_first(dlg.proj_scr.bt_cc_auto_find, 70, wg_place_margin.wg_dy.but);
      wl_place_h_next(dlg.proj_scr.bt_cc_adjust, 25, 70, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.proj_scr.bt_cc_zoom, 25, 70, wg_place_margin.wg_dy.but, true);
    wl_pop_frm();
  wl_pop_frm();

  wl_push_frm(dlg.proj_scr.frm_proj_screen);
    wlb_place_ledns(dlg.proj_scr.eds_proj_scr_fov, 100);
    wl_place(dlg.proj_scr.bt_draw_cones, wg_place_margin.wg_dy.but);
    wl_place(dlg.proj_scr.ckb_show_signal, 0);
    wl_place(dlg.proj_scr.ckb_show_idler, 0);

    wl_push_frm(dlg.proj_scr.frm_filters);
      wl_place_h_first(dlg.proj_scr.lbl_contrast, 40, wg_place_margin.wg_dy.but);
      wl_place_h_next(dlg.proj_scr.spin_contrast, 5, 16, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.proj_scr.ckb_grid, 10, 0, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.proj_scr.ckb_revert, 4, 0, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.proj_scr.ckb_colorize, 2, 0, wg_place_margin.wg_dy.but, false);
      wl_place_h_next(dlg.proj_scr.ckb_scale, 4, 0, wg_place_margin.wg_dy.but, true);
    wl_pop_frm();
  wl_pop_frm();
}

// place widget blocks in client bitmap, and widgets in blocks
static void resize_dialog(bitmap_t *cli_bm)
{
  int a_x1, a_y1;                                // align positions
  struct wg_block_rec_t graph;
  struct wg_block_rec_t console;
  struct wg_block_rec_t control;

  vec2i *cli_size = &cli_bm->size;

  wg_place_blocks_init();                        // init block place statics

  // -------------------------------
  // block positions without margin
  a_x1 = cli_size->x - 300;                      // x1 align position
  a_y1 = a_x1 + GR_LEGEND_WY;                    // use square graph + legend area

  graph.pos.x = 0;
  graph.pos.y = 0;

  console.pos.x = 0;
  console.pos.y = a_y1;

  control.pos.x = a_x1;
  control.pos.y = 0;

  // -------------------------------
  // blocks size without margin
  graph.size.x = a_x1;
  graph.size.y = a_y1;

  console.size.x = a_x1;
  console.size.y = cli_size->y - a_y1;

  control.size.x = cli_size->x - a_x1;
  control.size.y = cli_size->y;

  // ----------------------------------------
  // margin between window borders and blocks
  wg_def_block_margin(&graph, cli_size);
  wg_def_block_margin(&console, cli_size);
  wg_def_block_margin(&control, cli_size);
  
  // -------------------------------
  // place widget blocks
  place_widget_block_graph(&graph.pos, &graph.size);
  dlg_console_place_widget(&console.pos, &console.size);
  place_widget_block_control(&control.pos, &control.size);
}

static void dlg_states_init(void);
static void dlg_ctrl_init(void);
static void dlg_ctrl_exit(void);

// window message proc
static void win_msg_proc(hwin_t *hw)
{
  switch (hw->ev.type)
  {
    // mouse messages
    case EV_RBUTTONDOWN:
    break;
    case EV_CREATE:                              // window creation
      dlg_states_init();
      init_widget_list(hw);                      // init list of widgets
      init_graph_base();
    break;
    case EV_CLOSE:
      win_wait_thread_end();
    break;
    case EV_DESTROY:                             // window destroyed
      dlg_ctrl_exit();
    break;
    case EV_SIZE:                                // window sized/resized
      if (hw->show_mode != show_minimized)       // then client y size is 0
        resize_dialog(&hw->cli_bm);
      dlg.graph.disp.e_displayed = e_graph_undef;
    break;
    case EV_PAINT:                               // paint required. bitmap is valid in this message
      bm_paint(&hw->cli_bm, wgs.clear_bk_color); // clear widget area background
    break;
    default:;
  }
  widget_dispatch_events(hw);                    // send events to widgets
}

// min/max dialog client window size for correct display
static const struct
{
  vec2i ini;                                     // create initial size
  vec2i min;
  vec2i max;
} dlg_size = { { 900, 956 }, { 900, 956 }, { 1054, 1150 } };

// init application main window
bool win_app_main(char *cmd_line)
{
  const vec2i *scr_size = win_get_screen_size(); // screen size
  vec2i cli_size = dlg_size.ini;

  // clip to screen
  vec_clip_min_max(&cli_size, &dlg_size.min, scr_size);

  font_init_aa();                                // init font anti aliasing
  if (win_create("PDC View", NULL, &cli_size, &dlg_size.min, &dlg_size.max, win_msg_proc, NULL, NULL))
  {
    dlg_ctrl_init();
    return true;
  }
  return false;
}

// -----------------------------------------------

// startup set initial dialog states
static void dlg_states_init(void)
{
#ifdef _DEBUG
  _controlfp(_EM_INEXACT, _MCW_EM);              // produce exception if FP error
#endif
  // initial config
  pdc.crystal_type   = ct_BBO_1;
  pdc.crystal_temp   = 20.0;                     // temperature (celcius)
  pdc.lambda_pump_nm = 405;
  pdc.lambda_signal_nm = 810;
  pdc.type1 = true;
  get_crystal_index(pdc.crystal_type, 0, 0, NULL, &pdc.crystal_info);
  pdc_init();
  strcpy(dlg.phase_m.lbl_contrast_str, "Cont.: 2");
  strcpy(dlg.phase_m.lbl_zoom_select_str, "Pos: Theta 0.0   Phi 0.0");
  strcpy(dlg.proj_scr.lbl_contrast_str, "Cont.: 3");
  dlg.phase_m.zoom.scale = 1;
  dlg.phase_m.zoom.gv.an_range = 180.0f;
}

// some startup init
static void dlg_ctrl_init(void)
{
  pdc_init_search();
}

// some exit clean up (allocs)
static void dlg_ctrl_exit(void)
{
}

// display message if internal error, should never occur
void PDC_error(const char *err_msg)
{
  co_printf("PDC code internal error '%s'.\n Please contact developer.\n", err_msg);
}

