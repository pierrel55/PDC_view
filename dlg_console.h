// console widget block
void dlg_console_init_widgets(hwin_t *hw, const font_t *font, pix_t color, pix_t aa_color);
void dlg_console_place_widget(vec2i *pos, vec2i *size);

// console print
void co_printf(const char *fmt, ...);
