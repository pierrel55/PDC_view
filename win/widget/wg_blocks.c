// util functions to place/size widgets blocks

#include "widget.h"

// base
const struct wg_place_margin_t wg_place_margin_ref = { { 22, 22, 22, 22 }, {  8, 16, 4, 4 }, { 10, 8, 8,  8 } };
// small client area, minimal margin
const struct wg_place_margin_t wg_place_margin_min = { { 19, 19, 18, 18 }, {  4, 13, 3, 3 }, {  8, 6, 5,  4 } };
// large client area, maximal margin
const struct wg_place_margin_t wg_place_margin_max = { { 23, 23, 23, 23 }, {  8, 18, 6, 6 }, { 12, 8, 8, 10 } };

// active margin, default init is wg_place_margin_ref
struct wg_place_margin_t wg_place_margin = { { 22, 22, 22, 22 }, {  8, 16, 4, 4 }, { 10, 8, 8,  8 } };

// some inits before use
void wg_place_blocks_init(void)
{
  // define move offsets for wl_push_frm and wl_place
  wl_set_push_frm(wg_place_margin.frm.dx_push, wg_place_margin.frm.dy_push, wg_place_margin.frm.dy_pop_frm);
  wl_place_set_y_margin(wg_place_margin.frm.dy_place);
}

// define margin between widget blocks and client area borders
void wg_def_block_margin(struct wg_block_rec_t *br, vec2i *cli_size)
{
  if ((br->pos.x + br->size.x) == cli_size->x)
    br->size.x -= wg_place_margin.blk.x_lr;
  else
    br->size.x -= wg_place_margin.blk.dxy;

  if (br->pos.x == 0)
  {
    br->pos.x += wg_place_margin.blk.x_lr;
    br->size.x -= wg_place_margin.blk.x_lr;
  }

  if ((br->pos.y + br->size.y) == cli_size->y)
    br->size.y -= wg_place_margin.blk.y_bottom;
  else
    br->size.y -= wg_place_margin.blk.dxy;

  if (br->pos.y == 0)
  {
    br->pos.y += wg_place_margin.blk.y_up;
    br->size.y -= wg_place_margin.blk.y_up;
  }
}
