// util functions to place/size widgets using relative placement stack

#include "widget.h"
#include "wg_priv.h"

#define MAX_LOC_STK 32

struct wl_t                            // widget location
{
  vec2i pos;                           // current x, y position
  int wx;                              // available size x
  widget_t *push_wg;                   // widget size to ajust on pop
};

static struct
{
  struct wl_t loc[MAX_LOC_STK];
  int stk_size;
  int push_frm_dx;                     // x decal added after push
  int push_frm_dy;                     // y decal added after push
  int pop_frm_dy;                      // y decal added before and after pop
  int place_dy;                        // y margin added after w_place call
  bool get_size;                       // get size only, do not place
} loc_stk = { 0 };

// ----------------------------------------------
// size utility. size must be defined before pos

// init widget client area
static void set_wg_client(widget_t *wg)
{
  if (wg->flags & (WG_CFIXED | WG_CFRAME))
  {
    //if (wg->flags & WG_CFIXED)
    //{
    //  wg->c_pos.x = wg->e_pos.x + ((wg->e_size.x - wg->c_size.x) >> 1);
    //  wg->c_pos.y = wg->e_pos.y + ((wg->e_size.y - wg->c_size.y) >> 1);
    //}
  }
  else
  {
    wg->c_pos = wg->e_pos;
    wg->c_size = wg->e_size;
    if (wg->e_frm)                               // if external frame, reduce client size
    {
      int wf2 = wg->e_frm->width << 1;
      wg->c_pos.x += wg->e_frm->width;
      wg->c_pos.y += wg->e_frm->width;
      wg->c_size.x -= wf2;
      wg->c_size.y -= wf2;
    }
  }
}

// set i_wg e_size/e_pos. adjust wg size
static void set_wg_i_size_pos(widget_t *i_wg, widget_t *wg)
{
  W_ASSERT(!(wg->flags & WG_CFIXED));            // attempt to resize fixed client area
  if (i_wg->flags & (WG_ILEFT|WG_IRIGHT))
  {
    int h = wg->c_size.x >> 1;
    if (i_wg->e_size.x < wg->c_size.y)
      i_wg->e_size.x = wg->c_size.y;
    if ((i_wg->e_size.x > h) || (i_wg->flags & WG_ICENTER))
      i_wg->e_size.x = h;

    if (i_wg->flags & WG_ILEFT)                  // init left widget
    {
      i_wg->e_size.y = wg->c_size.y;
      i_wg->e_pos = wg->c_pos;
      wg->c_pos.x  += i_wg->e_size.x;
      wg->c_size.x -= i_wg->e_size.x;
    }
    else                                         // init right widget
    {
      i_wg->e_size.y = wg->c_size.y;
      wg->c_size.x -= i_wg->e_size.x;
      i_wg->e_pos.x = wg->c_pos.x + wg->c_size.x;
      i_wg->e_pos.y = wg->c_pos.y;
    }
    set_wg_client(i_wg);
  }
  else
  if (i_wg->flags & (WG_IUP|WG_IDOWN))
  {
    int h = wg->c_size.y >> 1;
    if (i_wg->e_size.y < wg->c_size.x)
      i_wg->e_size.y = wg->c_size.x;
    if ((i_wg->e_size.y > h) || (i_wg->flags & WG_ICENTER))
      i_wg->e_size.y = h;

    if (i_wg->flags & WG_IUP)                    // init up widget
    {
      i_wg->e_size.x = wg->c_size.x;
      i_wg->e_pos = wg->c_pos;
      wg->c_pos.y  += i_wg->e_size.y;
      wg->c_size.y -= i_wg->e_size.y;
    }
    else                                         // init bottom widget
    {
      i_wg->e_size.x = wg->c_size.x;
      wg->c_size.y -= i_wg->e_size.y;
      i_wg->e_pos.y = wg->c_pos.y + wg->c_size.y;
      i_wg->e_pos.x = wg->c_pos.x;
    }
    set_wg_client(i_wg);
  }
  else
  if (i_wg->flags & (WG_VSB|WG_VSB_S|WG_VSB_D))  // vertical scroll bar
  {
    int w = wgs.slider.bt_width;
    i_wg->e_size.x = w;
    i_wg->c_size.x = w;
    if (i_wg->flags & WG_VSB)                    // sub object root (up button)
    {
      wg->c_size.x -= w;                         // reduce parent client size
      i_wg->e_pos.x = wg->c_pos.x + wg->c_size.x;
      i_wg->e_pos.y = wg->c_pos.y;
      i_wg->e_size.y = wg->c_size.y;             // include slider + down button
      i_wg->c_size.y = w;
    }
    else
    if (i_wg->flags & WG_VSB_S)                  // vertical scroll bar up slider
    {
      i_wg->wg_parent->c_size.y = w;             // window scroll bar have no WG_VSB

      i_wg->e_pos.x = wg->c_pos.x;
      i_wg->e_pos.y = wg->c_pos.y + w;
      i_wg->e_size.y = wg->e_size.y - w;
      i_wg->c_size.y = wg->e_size.y - 2*w;
    }
    else                                         // vertical scroll bar down button
    {
      W_ASSERT(i_wg->flags & WG_VSB_D);
      i_wg->e_pos.x = wg->c_pos.x;
      i_wg->e_pos.y = wg->c_pos.y + wg->c_size.y;
      i_wg->e_size.y = w;
      i_wg->c_size.y = w;
    }
    i_wg->c_pos = i_wg->e_pos;
  }
}

// place inserted sub widgets
static void wg_dispatch_ev_size(widget_t *wg, widget_t *i_wg)
{
  if (i_wg->next)                                // parent same level sub widget
    wg_dispatch_ev_size(wg, i_wg->next);
  set_wg_i_size_pos(i_wg, wg);                   // insert sub widget in widget
  if (i_wg->i_wg)                                // if sub widget contain sub, insert
    wg_dispatch_ev_size(i_wg, i_wg->i_wg);
  if (i_wg->sz_proc)
    i_wg->sz_proc(i_wg);
}

// set size and dispatch events
void wg_set_size(widget_t *wg, int size_x, int size_y)
{
  if (!loc_stk.get_size)                         // is size evaluation, do not dispatch events
  {
    wg->e_size.x = size_x;
    wg->e_size.y = size_y;
    set_wg_client(wg);
    if (wg->i_wg)
      wg_dispatch_ev_size(wg, wg->i_wg);
    if (wg->sz_proc)
      wg->sz_proc(wg);
  }
}

// ----------------------------------------------
// position utility using loc_stk stack

// move widget and childs
static void wg_move_dx(widget_t *wg, int dx)
{
  if (wg)
  {
    wg_move_dx(wg->next, dx);
    wg_move_dx(wg->i_wg, dx);
    wg->e_pos.x += dx;
    wg->c_pos.x += dx;
  }
}

void wg_set_pos_x(widget_t *wg, int x)
{
  int dx = x - wg->e_pos.x;
  if (dx)
  {
    wg_move_dx(wg->i_wg, dx);
    wg->e_pos.x = x;
    wg->c_pos.x += dx;
  }
}

static void wg_move_dy(widget_t *wg, int dy)
{
  if (wg)
  {
    wg_move_dy(wg->next, dy);
    wg_move_dy(wg->i_wg, dy);
    wg->e_pos.y += dy;
    wg->c_pos.y += dy;
  }
}

void wg_set_pos_y(widget_t *wg, int y)
{
  int dy = y - wg->e_pos.y;
  if (dy)
  {
    wg_move_dy(wg->i_wg, dy);
    wg->e_pos.y = y;
    wg->c_pos.y += dy;
  }
}

void wg_set_pos(widget_t *wg, int x, int y)
{
  wg_set_pos_x(wg, x);
  wg_set_pos_y(wg, y);
}

void wg_align_left(widget_t *wg, int l_margin)
{
  wg_set_pos_x(wg, l_margin);
}

void wg_align_right(widget_t *wg, int r_margin, int x_width)
{
  wg_set_pos_x(wg, x_width - r_margin - wg->e_size.x);
}

void wg_center(widget_t *wg, int x_width)
{
  wg_set_pos_x(wg, (x_width - wg->e_size.x) >> 1);
}

void wg_center_between(widget_t *wg, int l_margin, int r_margin, int x_width)
{
  wg_set_pos_x(wg, ((x_width - (l_margin + r_margin)) - wg->e_size.x) >> 1);
}

// set e_size and position
void wg_set_pos_size(widget_t *wg, int pos_x, int pos_y, int size_x, int size_y)
{
  if (!loc_stk.get_size)
  {
    wg_set_pos(wg, pos_x, pos_y);
    wg_set_size(wg, size_x, size_y);
  }
}

// ----------------------------------------------

// define origine position and available w width
void wl_init(vec2i *pos_ini, int max_wx, bool get_size)
{
  struct wl_t *loc = &loc_stk.loc[0];
  W_ASSERT(!get_size || !(pos_ini->x || pos_ini->y));  // must be 0 if get_size true
  loc->pos = *pos_ini;
  loc->wx = max_wx;
  loc->push_wg = NULL;
  loc_stk.stk_size = 0;
  loc_stk.get_size = get_size;
}

// define position decal for wl_push
void wl_set_push_frm(int push_frm_dx, int push_frm_dy, int pop_frm_dy)
{
  loc_stk.push_frm_dx = push_frm_dx;
  loc_stk.push_frm_dy = push_frm_dy;
  loc_stk.pop_frm_dy = pop_frm_dy;
}

// return current placement pos
vec2i *wl_pos(void)
{
  return &loc_stk.loc[loc_stk.stk_size].pos;
}

// return current x size
int *wl_wx(void)
{
  return &loc_stk.loc[loc_stk.stk_size].wx;
}

// return current placement size
void wl_get_size(vec2i *size)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  size->x = loc->wx;
  size->y = loc->pos.y;
}

int wl_size_x(void)
{
  return loc_stk.loc[loc_stk.stk_size].wx;
}

// push and move
void wl_push(int dx, int dy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  W_ASSERT(loc_stk.stk_size < (MAX_LOC_STK-1));

  loc[1].pos.x = loc->pos.x + dx;
  loc[1].pos.y = loc->pos.y + dy;
  loc[1].wx = loc->wx - 2*dx;
  loc_stk.stk_size++;
}

// pop and add y margin
void wl_pop(int y_margin)
{
  int y = loc_stk.loc[loc_stk.stk_size].pos.y;
  W_ASSERT(loc_stk.stk_size > 0);
  loc_stk.loc[--loc_stk.stk_size].pos.y = y + y_margin;
}

// push frame origin and decal
void wl_push_frm(widget_t *wg)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  W_ASSERT((loc_stk.stk_size < (MAX_LOC_STK-1)) && !loc->push_wg);
  loc->push_wg = wg;
  loc[1].pos.x = loc->pos.x + loc_stk.push_frm_dx;
  loc[1].pos.y = loc->pos.y + loc_stk.push_frm_dy;
  loc[1].wx = loc->wx - 2*loc_stk.push_frm_dx;
  loc_stk.stk_size++;
}

// pop and set frame position/size
void wl_pop_frm(void)
{
  struct wl_t *loc = &loc_stk.loc[--loc_stk.stk_size];
  W_ASSERT((loc_stk.stk_size >= 0) && loc->push_wg);
  if (!loc_stk.get_size)
    wg_set_pos_size(loc->push_wg, loc->pos.x, loc->pos.y, loc->wx, loc_stk.pop_frm_dy + loc[1].pos.y - loc->pos.y);
  loc->push_wg = NULL;
  loc->pos.y = loc[1].pos.y + 2*loc_stk.pop_frm_dy;
}

// y margin added after w_place call
void wl_place_set_y_margin(int y_margin)
{
  loc_stk.place_dy = y_margin;
}

// wy: define new y size (if > to min size)
void wl_place(widget_t *wg, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = wg->e_size.y;                    // keep default y size
  if (!loc_stk.get_size)
    wg_set_pos_size(wg, loc->pos.x, loc->pos.y, loc->wx, wy);
  loc->pos.y += wy + loc_stk.place_dy;
}

// do not resize widget, but center it on available x width
void wl_place_center(widget_t *wg, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = wg->e_size.y;                    // keep default y size
  if (!loc_stk.get_size)
  {
    int x = loc->pos.x + ((loc->wx - wg->e_size.x) >> 1);
    wg_set_pos_size(wg, x, loc->pos.y, wg->e_size.x, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally with dx0 x decal
void wl_place_2h(widget_t *wg0, int dx0, widget_t *wg1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    int w = (loc->wx - dx0)/2;
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w, wy);
    wg_set_pos_size(wg1, loc->pos.x + w + dx0, loc->pos.y, w, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally, use w0 for first, place w1 with dx0 offset, adjust w1 size
void wl_place_2h_w0_dx0(widget_t *wg0, int w0, int dx0, widget_t *wg1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w0, wy);
    dx0 += w0;
    wg_set_pos_size(wg1, loc->pos.x + dx0, loc->pos.y, loc->wx - dx0, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally, use w0 for first, place w1 with dx0 offset, use w1 size
void wl_place_2h_w0_dx0_w1(widget_t *wg0, int w0, int dx0, widget_t *wg1, int w1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    int w1_max;
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w0, wy);
    dx0 += w0;
    w1_max = loc->wx - dx0;
    if (!w1)
      w1 = wg1->e_size.x;
    if (w1 > w1_max)
      w1 = w1_max;
    wg_set_pos_size(wg1, loc->pos.x + dx0, loc->pos.y, w1, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally, use w1 for second
void wl_place_2h_w1(widget_t *wg0, widget_t *wg1, int w1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, wg0->e_size.x, wy);
    wg_set_pos_size(wg1, loc->pos.x + loc->wx - w1, loc->pos.y, w1, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally, use w1 for second, center w0
void wl_place_2h_w1_c0(widget_t *wg0, widget_t *wg1, int w1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    int w0 = loc->wx - w1;
    int dx0 = (w0 - wg0->e_size.x) >> 1;
    wg_set_pos_size(wg0, dx0 + loc->pos.x, loc->pos.y, wg0->e_size.x, wy);
    wg_set_pos_size(wg1, loc->pos.x + w0, loc->pos.y, w1, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widgets horizontally, add spc between wg0 a wg1, use w1 width for wg1
void wl_place_2h_spc_r(widget_t *wg0, int spc, widget_t *wg1, int w1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    int w0 = loc->wx - w1 - spc;
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w0, wy);
    wg_set_pos_size(wg1, loc->pos.x + w0 + spc, loc->pos.y, w1, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 2 widget centered
void wl_place_2h_center(widget_t *wg0, int w0, widget_t *wg1, int w1, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = MAX(wg0->e_size.y, wg1->e_size.y);
  if (!loc_stk.get_size)
  {
    int dw;
    if (!w0) w0 = wg0->e_size.x;
    if (!w1) w1 = wg1->e_size.x;
    dw = (loc->wx - (w0 + w1))/3;
    wg_set_pos_size(wg0, loc->pos.x + dw, loc->pos.y, w0, wy);
    wg_set_pos_size(wg1, loc->pos.x + w0 + 2*dw, loc->pos.y, w1, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 3 widget centered with x margin between 1/2 and 2/3
void wl_place_3h_center(widget_t *wg0, widget_t *wg1, widget_t *wg2, int w_ma, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy)
  {
    wy = MAX(wg0->e_size.y, wg1->e_size.y);
    if (wg2->e_size.y > wy)
      wy = wg2->e_size.y;
  }
  if (!loc_stk.get_size)
  {
    int w_wg = (loc->wx - 2*w_ma)/3;
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w_wg, wy);
    wg_set_pos_size(wg1, loc->pos.x + w_wg + w_ma, loc->pos.y, w_wg, wy);
    wg_set_pos_size(wg2, loc->pos.x + loc->wx - w_wg, loc->pos.y, w_wg, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// place 4 widget centered with x margin between 1/2 and 2/3 and 3/4
// additionnal increase size can be passed for last widget size
void wl_place_4h_center(widget_t *wg0, widget_t *wg1, widget_t *wg2, widget_t *wg3, int w_ma, int wg3_w_inc, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy)
  {
    wy = wg0->e_size.y;
    if (wg1->e_size.y > wy) wy = wg1->e_size.y;
    if (wg2->e_size.y > wy) wy = wg2->e_size.y;
    if (wg3->e_size.y > wy) wy = wg3->e_size.y;
  }
  if (!loc_stk.get_size)
  {
    int w_wg = ((loc->wx - wg3_w_inc) - 3*w_ma)/4;
    wg_set_pos_size(wg0, loc->pos.x, loc->pos.y, w_wg, wy);
    wg_set_pos_size(wg1, loc->pos.x + w_wg + w_ma, loc->pos.y, w_wg, wy);
    wg_set_pos_size(wg2, loc->pos.x + (w_wg + w_ma)*2, loc->pos.y, w_wg, wy);
    wg_set_pos_size(wg3, loc->pos.x + (w_wg + w_ma)*3, loc->pos.y, w_wg + wg3_w_inc, wy);
  }
  loc->pos.y += wy + loc_stk.place_dy;
}

// ---------------------------------------------
// horizontal placements

void wl_place_h_first(widget_t *wg, int wx, int wy)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = wg->e_size.y;
  loc[1].pos.y = wy;
  if (!loc_stk.get_size)
  {
    if (!wx) wx = wg->e_size.x;
    loc[1].pos.x = loc->pos.x;
    loc[1].wx = loc->wx;
    wg_set_pos_size(wg, loc->pos.x, loc->pos.y, wx, wy);
    loc->pos.x += wx;
    loc->wx -= wx;
  }
}

void wl_place_h_next(widget_t *wg, int x_margin, int wx, int wy, bool is_last)
{
  struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
  if (!wy) wy = wg->e_size.y;
  if (wy > loc[1].pos.y)
    loc[1].pos.y = wy;
  if (!loc_stk.get_size)
  {
    if (!wx)
    {
      if (is_last)
        wx = loc->wx - x_margin;
      else
        wx = wg->e_size.x;
    }
    loc->pos.x += x_margin;
    wg_set_pos_size(wg, loc->pos.x, loc->pos.y, wx, wy);
    loc->pos.x += wx;
    loc->wx -= x_margin + wx;
  }
  if (is_last)
  {
    // restore values
    struct wl_t *loc = &loc_stk.loc[loc_stk.stk_size];
    loc->pos.x = loc[1].pos.x;
    loc->pos.y += loc[1].pos.y + loc_stk.place_dy;
    loc->wx = loc[1].wx;
  }
}
