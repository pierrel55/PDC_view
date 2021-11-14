#define GR_LEGEND_WY 50                // width of bottom legend in graph

void init_graph_base(void);

// n(lambda)
void draw_graph_n_lambda(hwin_t *hw);

// n(theta,phi)
enum e_graph_ind_sf
{
  e_ind_s = 0,
  e_ind_f,
  e_ind_sf_diff,
  e_ind_sf_diff_avg,
};

// check if config correct, display errors/warnings to console
bool check_pm_config(hwin_t *hw);

void draw_graph_ind_sf(hwin_t *hw, enum e_graph_ind_sf graph_ind_type, const char *legend_str);

void draw_graph_phase_match(hwin_t *hw);

void draw_graph_phase_match_update_filters(hwin_t *hw, bool upd_filters);

void draw_graph_proj_scr(hwin_t *hw);

void draw_graph_proj_scr_update_filters(hwin_t *hw, bool upd_filters);
