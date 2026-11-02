#ifndef TUI_H
#define TUI_H

#include "common.h"

void init_tui(void);
void teardown_tui(void);
void rebuild_tui_layout(void);
void draw_frame(void);
void update_hud(GameState *g);
void update_art_panel(GameState *g, ArtScene scene, int frame);
void prompt_input(const char *prompt, char *buffer, int max_len);
void wait_key(const char *msg);

#endif /* TUI_H */