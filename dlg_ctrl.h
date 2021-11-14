// ----------------------------------------------
// datas for inits or update

#define N_STD_LAMBDA 15
extern const char *laser_std_lambda[N_STD_LAMBDA+1];

// define names list for gui
const char **get_crystal_names_list(void);

// define crystal lambda range string
const char *get_crystal_lambda_range_str(void);

// define indice string
const char *get_pump_crystal_indices_str(void);
const char *get_signal_crystal_indices_str(void);
const char *get_idler_crystal_indices_str(void);

// define lambda idler string
const char *get_lambda_idler_str(void);

// ----------------------------------------------
// main dialog widget callbacks

// select crystal combo
void cb_combo_select_crystal(hwin_t *hw, int sel_id);

// text button pressed
void cb_text_bt(hwin_t *hw, widget_t *wg);

// check box changed
void cb_check_box_changed(hwin_t *hw, widget_t *wg);

// radio button changed
void cb_radio_button_changed(hwin_t *hw, widget_t *wg);

// spi button up pressed
void cb_spin_button_up(hwin_t *hw, widget_t *wg);

// spi button down pressed
void cb_spin_button_down(hwin_t *hw, widget_t *wg);

// numeric value changed in edit box
void cb_edit_num_value_changed(hwin_t *hw, widget_t *wg, float value);

// graph clicked cb
void wg_graph_click_cb(hwin_t *hw, widget_t *wg, int x, int y);
