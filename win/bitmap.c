// bitmap drawing functions
#include "win.h"

// transparency mask in rgb 888
#define GMASK32 ((0x7f << 16) + (0x7f << 8) + 0x7f)

// address of a pixel
#define PIX_ADDR(x, y) (bm->pix_ptr + y*bm->l_size + x)

// test if cursor select pos + size region
bool vec_select(const vec2i *mc, const vec2i *pos, const vec2i *size)
{
  return    (mc->x >= pos->x) && (mc->x < (pos->x + size->x))
         && (mc->y >= pos->y) && (mc->y < (pos->y + size->y));
}

// ensure values between min/max (inclusive)
void vec_clip_min_max(vec2i *v, const vec2i *min, const vec2i *max)
{
  if      (v->x < min->x) v->x = min->x;
  else if (v->x > max->x) v->x = max->x;
  
  if      (v->y < min->y) v->y = min->y;
  else if (v->y > max->y) v->y = max->y;
}

// adjust coordinates to size, return false if completly clipped.
bool vec_clip_rect(vec2i *size, int *x, int *y, int *dx, int *dy)
{
  if (*x < 0) { *dx += *x; *x = 0; }
  if (*y < 0) { *dy += *y; *y = 0; }
  if ((*x + *dx) > size->x) *dx = size->x - *x;
  if ((*y + *dy) > size->y) *dy = size->y - *y;
  return ((*dx > 0) && (*dy > 0));
}

// create bitmap, a memory block of correct size must be passed (Size = dx*dy*2 bytes)
void bm_init(bitmap_t *bm, int dx, int dy, int line_dx, pix_t *pixel_mem)
{
  bm->pix_ptr = pixel_mem;
  bm->size.x = dx;
  bm->size.y = dy;
  bm->l_size = line_dx;
}

// init child bitmap as sub bitmap of parent
void bm_init_child(bitmap_t *child, const bitmap_t *bm, int x, int y, int dx, int dy)
{
  child->pix_ptr = PIX_ADDR(x, y);
  child->size.x = dx;
  child->size.y = dy;
  child->l_size = bm->l_size;
}

// get pixel address (warning: no clip check)
pix_t *bm_get_pix_addr(bitmap_t *bm, int x, int y)
{ 
  return PIX_ADDR(x, y);
}

// put single pixel
void bm_put_pixel(bitmap_t *bm, int x, int y, pix_t color)
{
  *PIX_ADDR(x, y) = color;
}

// draw horizontal line
void bm_draw_line_h(bitmap_t *bm, int x, int y, int len, pix_t color)
{
  if ((y < 0) || (y >= bm->size.y))
    return;
  if (x < 0) { len += x; x = 0; };
  if ((x + len) > bm->size.x)
    len = bm->size.x - x;
  if (len > 0)
  {
    pix_t *p = PIX_ADDR(x, y);
    pix_t *p_end = p + len;
    while (p < p_end)
      *p++ = color;
  }
}

// draw vertical line
void bm_draw_line_v(bitmap_t *bm, int x, int y, int len, pix_t color)
{
  if ((x < 0) || (x >= bm->size.x))
    return;
  if (y < 0) { len += y; y = 0; };
  if ((y + len) > bm->size.y)
    len = bm->size.y - y;
  if (len > 0)
  {
    pix_t *p = PIX_ADDR(x, y);
    pix_t *p_end = p + len*bm->l_size;
    while (p < p_end)
    {
      *p = color;
      p += bm->l_size;
    }
  }
}

// draw empty rectangle
void bm_draw_rect_shadow(bitmap_t *bm, int x, int y, int dx, int dy, pix_t col_tl, pix_t col_br)
{
  bm_draw_line_h(bm, x, y, dx-1, col_tl);               // top
  bm_draw_line_v(bm, x, y+1, dy-1, col_tl);             // left
  bm_draw_line_h(bm, x+1, y+dy-1, dx-1, col_br);        // bottom
  bm_draw_line_v(bm, x+dx-1, y, dy-1, col_br);          // right
}

// draw empty rectangle of with
void bm_draw_rect_width(bitmap_t *bm, int x, int y, int dx, int dy, int width, pix_t color)
{
  while (width--)
  {
    bm_draw_rect_shadow(bm, x++, y++, dx, dy, color, color);
    dx-=2;
    dy-=2;
  }
}

// draw empty rectangle of with and colors defined by array
void bm_draw_rect_shadow_width(bitmap_t *bm, int x, int y, int dx, int dy, int width, const pix_t *col_tl, const pix_t *col_br)
{
  while (width--)
  {
    bm_draw_rect_shadow(bm, x++, y++, dx, dy, *col_tl++, *col_br++);
    dx-=2;
    dy-=2;
  }
}

// fill full bitmap with color
void bm_paint(bitmap_t *bm, pix_t color)
{
  if (bm->size.x == bm->l_size)
  {
    pix_t *p = bm->pix_ptr;
    pix_t *p_end = bm->pix_ptr + bm->size.x*bm->size.y;
    while (p < p_end)
      *p++ = color;
  }
  else
  {
    pix_t *p_line = bm->pix_ptr;
    pix_t *p_end = bm->pix_ptr + bm->l_size*bm->size.y;
    while (p_line < p_end)
    {
      pix_t *p = p_line;
      pix_t *p_eol = p_line + bm->size.x;
      while (p < p_eol)
        *p++ = color;
      p_line += bm->l_size;
    }
  }
}

// draw filled rectangle
void bm_paint_rect(bitmap_t *bm, int x, int y, int dx, int dy, pix_t color)
{
  // clipping
  if (vec_clip_rect(&bm->size, &x, &y, &dx, &dy))
  {
    pix_t *p_line = PIX_ADDR(x, y);
    pix_t *p_end = p_line + bm->l_size*dy;
    while (p_line < p_end)
    {
      pix_t *p = p_line;
      pix_t *p_eol = p_line + dx;
      while (p < p_eol)
        *p++ = color;
      p_line += bm->l_size;
    }
  }
}

// draw bar with color interpolation (specicic for window title bar)
void bm_paint_rect_c2(bitmap_t *bm, int x, int y, int dx, int dy, pix_t c0, pix_t c1)
{
  if (dx && dy)
  {
    // get color componants shifted 16
    #define R_16(rgb) (int)( rgb        & 0xff0000)
    #define G_16(rgb) (int)((rgb <<  8) & 0xff0000)
    #define B_16(rgb) (int)((rgb << 16) & 0xff0000)

    int r = R_16(c0);
    int g = G_16(c0);
    int b = B_16(c0);
    int dr = (R_16(c1) - r) / dx;
    int dg = (G_16(c1) - g) / dx;
    int db = (B_16(c1) - b) / dx;

    pix_t *p0 = PIX_ADDR(x, y);
    pix_t *l0 = p0;
    pix_t *p = p0;
    pix_t *p_end = p0 + dx;
    uint8_t *c = (uint8_t *)&c0;

    while (p < p_end)
    {
      c[0] = (b >> 16); b += db;
      c[1] = (g >> 16); g += dg;
      c[2] = (r >> 16); r += dr;
      *p++ = *(pix_t *)c;
    }

    // clone 1st line dy-1 times
    while (--dy)
    {
      pix_t *d = l0;
      p0 += bm->l_size;
      p = p0;
      p_end = p0 + dx;
      while (p < p_end)
        *p++ = *d++;
    }
  }
}

// fill rect with x color defined in col.
// warning: no clipping check.
void bm_paint_rect_clone_color_no_clip(bitmap_t *bm, int x, int y, int dx, int dy, pix_t *col)
{
  pix_t *p0 = PIX_ADDR(x, y);
  for (; dy; dy--, p0 += bm->l_size)
  {
    pix_t *p = p0;
    pix_t *p_end = p0 + dx;
    pix_t *c = col;
    while (p < p_end)
      *p++ = *c++;
  }
}

// copy img bitmap to bm at position x,y
void bm_copy_img(bitmap_t *bm, int x, int y, const bitmap_t *img)
{
  int dx = img->size.x;
  int dy = img->size.y;
  if (vec_clip_rect(&bm->size, &x, &y, &dx, &dy))
  {
    pix_t *s = img->pix_ptr;
    pix_t *d = bm->pix_ptr + y*bm->l_size + x;

    // copy
    while (dy)
    {
      pix_t *_s = s;
      pix_t *_s_end = s + dx;
      pix_t *_d = d;
      while (_s < _s_end)
        *_d++ = *_s++;
      s += img->l_size;
      d += bm->l_size;
      dy--;
    }
  }
}

// do scroll up in bitmap. warning: no clip check
void bm_rect_scroll_up(bitmap_t *bm, vec2i *pos, int size_x, int sc_dy, int clr_dy, pix_t clr_color)
{
  pix_t *d = bm->pix_ptr + pos->y*bm->l_size + pos->x;  // dest
  pix_t *d_end = d + (sc_dy - clr_dy)*bm->l_size;
  pix_t *l_end = d + size_x;
  pix_t *s = d + clr_dy*bm->l_size;

  // copy
  while (d < d_end)
  {
    pix_t *_d = d;
    pix_t *_s = s;
    while (_d < l_end)
      *_d++ = *_s++;
    d += bm->l_size;
    s += bm->l_size;
    l_end += bm->l_size;
  }
  // clear last line
  d_end += clr_dy*bm->l_size;
  while (d < d_end)
  {
    pix_t *_d = d;
    while (_d < l_end)
      *_d++ = clr_color;
    d += bm->l_size;
    l_end += bm->l_size;
  }
}

// ----------------------------------------------
// images load

#include <stdlib.h>

// decode tga rle datas into allocated buffer
static pix_t *tga_rle_decode(int dx, int dy, const uint8_t *col_map, const uint8_t *col_idx, const uint8_t *data_end, int *idx_size)
{
  int pix_cnt = dx * dy;
  pix_t *pix32 = (pix_t *)W_MALLOC(pix_cnt * sizeof(pix_t));
  if (pix32)
  {
    pix_t *pix32_end = pix32 + pix_cnt;
    // rle decode
    int l_bytes = dx * 4;
    uint8_t *d_end = (uint8_t *)pix32_end;
    uint8_t *l_end = d_end;
    uint8_t *d = d_end - l_bytes;
    const uint8_t *s = col_idx;
    
    while (d >= (uint8_t *)pix32) 
    {
      int raw = (*s & 0x80) == 0;
      int rep = (*s & 0x7f) + 1;
      int i;
      W_ASSERT(s <= data_end);
      s++;
      for (i=0; i<rep; i++)
      {
        const uint8_t *col = &col_map[*s * 3];
        d[0] = col[0];
        d[1] = col[1];
        d[2] = col[2];
        d[3] = 0;
        d += 4;
        if (d == l_end)                          // y flip
        {
          l_end -= l_bytes;
          d = l_end - l_bytes;
        }
        if (raw)
          s++;
      }
      if (!raw)
        s++;
    }
    W_ASSERT((d + l_bytes) == (uint8_t *)pix32);
    *idx_size = s - col_idx;
  }
  return pix32;
}

// init bitmap from tga 8 bits rle encoded image resource
bool bm_init_from_res(bitmap_t *bm, const res_img8c_t *res)
{
  int idx_size;
  const uint8_t *col_map = res->dat;
  const uint8_t *col_idx = col_map + 256*3;
  const uint8_t *data_end = col_idx + res->idx_len;
  pix_t *pix = tga_rle_decode(res->img_size.x, res->img_size.y, col_map, col_idx, data_end, &idx_size);
  W_ASSERT(idx_size == res->idx_len);
  if (!pix)                                      // alloc failed
  {
    bm->pix_ptr = NULL;                          // ensure safe output if load fail
    bm->size.x = 0;
    bm->size.y = 0;
    return false;
  }
  bm_init(bm, res->img_size.x, res->img_size.y, res->img_size.x, pix);
  return true;
}

#if 0
// ----------------------------------------------------------------
// util code used to load image and save it as res_img8c_t resource

static void export_res_rle8(const char *name, int dx, int dy, const uint8_t *pal, const uint8_t *col_idx, int idx_len);

#include <stdio.h>

static uint8_t *load_file(const char *file_name, int *len)
{
  uint8_t *buff = NULL;
  FILE *f = fopen(file_name, "rb");
  if (f)
  {
    int f_size;
    fseek(f, 0, SEEK_END);
    f_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buff = W_MALLOC(f_size);
    if (buff)
    {
      if (fread(buff, f_size, 1, f))
        *len = f_size;
      else
      {
        W_FREE(buff);
        buff = NULL;
      }
    }
    fclose(f);
  }
  return buff;
}

struct tga_header_t
{
  uint8_t numiden;
  uint8_t colormaptype;
  uint8_t imagetype;
  uint8_t cmapspec[5];
  uint8_t origx[2];
  uint8_t origy[2];
  uint8_t width[2];
  uint8_t height[2];
  uint8_t bpp;
  uint8_t imagedes;
};

#define TGA_RLEMAP 9

pix_t *decode_tga8rle(const char *fdata, int fdata_size, vec2i *img_size)
{
  const struct tga_header_t *hdr = (struct tga_header_t *)fdata;
  if (    (hdr->colormaptype == 1)               // use color map
       && (hdr->imagetype == TGA_RLEMAP)         // 256 colors rle encoded
       && (hdr->cmapspec[0] == 0)                // lsb 1st entry color map
       && (hdr->cmapspec[1] == 0)                // msb 1st entry color map
       && (hdr->cmapspec[2] == 0)                // lsb color map size
       && (hdr->cmapspec[3] == 1)                // msb color map size (256)
       && (hdr->cmapspec[4] == 24)               // RGB888 palette
       && (hdr->bpp == 8))                       // 8 bit index encoded
  {
    const uint8_t *col_map = fdata + sizeof(struct tga_header_t) + hdr->numiden;
    const uint8_t *col_idx = col_map + 3*256;
    int dx = hdr->width[0]  + (hdr->width[1] << 8);
    int dy = hdr->height[0] + (hdr->height[1] << 8);
    int idx_size;
    pix_t *pix32 = tga_rle_decode(dx, dy, col_map, col_idx, fdata + fdata_size, &idx_size);
    if (pix32)
    {
      img_size->x = dx;
      img_size->y = dy;
#if 0
      export_res_rle8("epr_res8c.txt", dx, dy, col_map, col_idx, idx_size);
#endif
      return pix32;
    }
  }
  return NULL;
}

// load tga 8 bits rle encoded and uncompressed image to bitmap
bool bm_load_tga8c(bitmap_t *bm, const char *file_name)
{
  uint8_t *f_data;
  int f_len;
  bm->pix_ptr = NULL;                            // ensure safe output if load fail
  bm->size.x = 0;
  bm->size.y = 0;
    
  f_data = load_file(file_name, &f_len);
  if (f_data)
  {
    vec2i img_size;
    pix_t *pix = decode_tga8rle(f_data, f_len, &img_size);
    if (pix)
      bm_init(bm, img_size.x, img_size.y, img_size.x, pix);
    W_FREE(f_data);
  }
  return bm->pix_ptr != NULL;
}

// export datas to insert into code
static void export_res_rle8(const char *name, int dx, int dy, const uint8_t *pal, const uint8_t *col_idx, int idx_len)
{
  FILE *f = fopen(name, "wb");
  if (f)
  {
    int i, ln = 0;
    fprintf(f, "const res_img8c_t dat_rle8 =\n  { { %d, %d }, %d, {\n  ", dx, dy, idx_len);
    for (i=0; i<256*3; i++)
    {
      fprintf(f, "0x%.2x,", pal[i]);
      if (++ln == 32) { ln = 0; fprintf(f, "\n  "); }
    }
    fprintf(f, "\n  ");
    for (i=0; i<idx_len; i++)
    {
      fprintf(f, "0x%.2x,", col_idx[i]);
      if (++ln == 32) { ln = 0; fprintf(f, "\n  "); }
    }
    fprintf(f, "\n} };");
    fclose(f);
  }
}

#endif
