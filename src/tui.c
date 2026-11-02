#include "tui.h"

WINDOW *w_hdr = NULL;
WINDOW *w_hud = NULL;
WINDOW *w_main = NULL;
WINDOW *w_art = NULL;
WINDOW *w_cmd = NULL;

int art_visible = 0;

const char *DATES[] = {
    "MARCH 29", "APRIL 12", "APRIL 26", "MAY 10", "MAY 24",
    "JUNE 7", "JUNE 21", "JULY 5", "JULY 19", "AUGUST 2",
    "AUGUST 16", "AUGUST 31", "SEPTEMBER 13", "SEPTEMBER 27",
    "OCTOBER 11", "OCTOBER 25", "NOVEMBER 8", "NOVEMBER 22",
    "DECEMBER 6", "DECEMBER 20"
};

double rnd(void) {
    return (double)rand() / ((double)RAND_MAX + 1.0);
}

void clamp_supplies(GameState *g) {
    if (g->food < 0.0) g->food = 0.0;
    if (g->ammo < 0.0) g->ammo = 0.0;
    if (g->clothing < 0.0) g->clothing = 0.0;
    if (g->misc < 0.0) g->misc = 0.0;
    if (g->cash < 0.0) g->cash = 0.0;
    if (g->miles < 0.0) g->miles = 0.0;
}

int get_living_member_count(const GameState *g) {
    int count = 0;
    for (int i = 0; i < g->party_count; i++) {
        if (g->party[i].health != HEALTH_DEAD) count++;
    }
    return count;
}

int get_random_living_member(const GameState *g) {
    int living_indices[MAX_PARTY];
    int count = 0;
    for (int i = 0; i < g->party_count; i++) {
        if (g->party[i].health != HEALTH_DEAD) {
            living_indices[count++] = i;
        }
    }
    if (count == 0) return 0;
    return living_indices[rand() % count];
}

int calculate_score(const GameState *g, int survived) {
    int s = (int)g->miles;
    if (survived) {
        s += 1000;
        s += get_living_member_count(g) * 200;
        s += (int)(g->cash * 2.0);
        s += (int)(g->food * 0.20);
        s += (int)g->clothing;
        s += (int)g->misc;
        s += (int)(g->ammo * 0.02);
    }
    return (s > 0) ? s : 0;
}

void rebuild_tui_layout(void) {
    int my, mx;
    getmaxyx(stdscr, my, mx);

    if (w_hdr) delwin(w_hdr);
    if (w_hud) delwin(w_hud);
    if (w_main) delwin(w_main);
    if (w_art) delwin(w_art);
    if (w_cmd) delwin(w_cmd);

    int main_h = my - 10;
    w_hdr = newwin(3, mx, 0, 0);
    w_hud = newwin(4, mx, 3, 0);
    w_cmd = newwin(3, mx, my - 3, 0);

    if (mx >= 108) {
        art_visible = 1;
        int art_w = 42;
        int main_w = mx - art_w;
        w_main = newwin(main_h, main_w, 7, 0);
        w_art  = newwin(main_h, art_w, 7, main_w);
    } else {
        art_visible = 0;
        w_main = newwin(main_h, mx, 7, 0);
        w_art = NULL;
    }

    scrollok(w_main, TRUE);
}

void init_tui(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_GREEN, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
    }

    rebuild_tui_layout();
}

void teardown_tui(void) {
    if (w_hdr) delwin(w_hdr);
    if (w_hud) delwin(w_hud);
    if (w_main) delwin(w_main);
    if (w_art) delwin(w_art);
    if (w_cmd) delwin(w_cmd);
    endwin();
}

void draw_frame(void) {
    werase(w_hdr);
    box(w_hdr, 0, 0);
    wattron(w_hdr, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w_hdr, 1, 2, "PIONEER TRAIL (1847) - EXPEDITION TUI EDITION");
    wattroff(w_hdr, COLOR_PAIR(1) | A_BOLD);
    wrefresh(w_hdr);
}

void update_hud(GameState *g) {
    clamp_supplies(g);
    werase(w_hud);
    box(w_hud, 0, 0);

    const char *date_str = (g->turn >= 0 && g->turn < 20) ? DATES[g->turn] : "LATE WINTER";
    double pct = (g->miles / TOTAL_TRAIL_MILES) * 100.0;
    if (pct > 100.0) pct = 100.0;

    const char *pace_str = (g->pace == PACE_GRUELING) ? "Grueling" :
                           (g->pace == PACE_STRENUOUS) ? "Strenuous" : "Steady";

    wattron(w_hud, COLOR_PAIR(2));
    mvwprintw(w_hud, 1, 2, "DATE: MONDAY %s 1847 | MILES: %.0f / 2040 (%.1f%%) | PACE: %s",
              date_str, g->miles, pct, pace_str);
    mvwprintw(w_hud, 2, 2, "FOOD: %d lbs | AMMO: %d | CLOTHES: $%d | MISC: $%d | CASH: $%d | PARTY: %d/%d",
              (int)g->food, (int)g->ammo, (int)g->clothing, (int)g->misc, (int)g->cash,
              get_living_member_count(g), g->party_count);
    wattroff(w_hud, COLOR_PAIR(2));
    wrefresh(w_hud);
}

void update_art_panel(GameState *g, ArtScene scene, int frame) {
    if (!art_visible || !w_art) return;

    werase(w_art);
    box(w_art, 0, 0);
    wattron(w_art, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w_art, 1, 2, "[ TRAIL EXPEDITION VIEW ]");
    wattroff(w_art, COLOR_PAIR(1) | A_BOLD);

    int y = 2;
    switch (scene) {
        case SCENE_PLAINS:
            wattron(w_art, COLOR_PAIR(4));
            mvwprintw(w_art, y++, 2, "       /   \\_  _____     /    \\      ");
            wattroff(w_art, COLOR_PAIR(4));
            wattron(w_art, COLOR_PAIR(1));
            if (frame % 2 == 0) {
                mvwprintw(w_art, y++, 2, "        o-o=o=o=o_    ____           ");
                mvwprintw(w_art, y++, 2, "       (\\_/ \\_/  \\\\--[____]\\         ");
                mvwprintw(w_art, y++, 2, "        (o   o)   \\\\  (o)(o)         ");
            } else {
                mvwprintw(w_art, y++, 2, "         o-o=o=o=o_    ____          ");
                mvwprintw(w_art, y++, 2, "        (\\_/ \\_/  \\\\--[____]\\        ");
                mvwprintw(w_art, y++, 2, "        (/   \\)   \\\\  (x)(x)         ");
            }
            wattroff(w_art, COLOR_PAIR(1));
            break;

        case SCENE_RIVER:
            wattron(w_art, COLOR_PAIR(6));
            mvwprintw(w_art, y++, 2, "   ~~~~  ~~  RIVER CROSSING  ~~ ~~~  ");
            mvwprintw(w_art, y++, 2, "      ____                           ");
            mvwprintw(w_art, y++, 2, " ___/[____]\\__ ~~~~~~   ~~~~~        ");
            mvwprintw(w_art, y++, 2, " \\__________/  ~~~~~~~~~~~~~~~~~~~~~ ");
            wattroff(w_art, COLOR_PAIR(6));
            break;

        case SCENE_FORT:
            wattron(w_art, COLOR_PAIR(1));
            mvwprintw(w_art, y++, 2, "    +-+--+   [TRADING POST]   +-+--+ ");
            mvwprintw(w_art, y++, 2, "    |[]  |====================|  []| ");
            mvwprintw(w_art, y++, 2, "    +----+-----[ |    | ]-----+----+ ");
            wattroff(w_art, COLOR_PAIR(1));
            break;

        case SCENE_LANDMARK:
            wattron(w_art, COLOR_PAIR(5) | A_BOLD);
            mvwprintw(w_art, y++, 2, "             /\\      CHIMNEY ROCK    ");
            mvwprintw(w_art, y++, 2, "            /  \\                     ");
            mvwprintw(w_art, y++, 2, "         __/ /\\ \\__                  ");
            mvwprintw(w_art, y++, 2, "        /___/  \\___\\                 ");
            wattroff(w_art, COLOR_PAIR(5) | A_BOLD);
            break;

        case SCENE_MOUNTAINS:
            wattron(w_art, COLOR_PAIR(2) | A_BOLD);
            mvwprintw(w_art, y++, 2, "        /\\     /\\     ROCKY PASS     ");
            mvwprintw(w_art, y++, 2, "       /  \\   /  \\   /\\              ");
            mvwprintw(w_art, y++, 2, "      / /\\ \\_/ /\\ \\ /  \\             ");
            mvwprintw(w_art, y++, 2, "     /___/\\___/  \\_\\/___\\            ");
            wattroff(w_art, COLOR_PAIR(2) | A_BOLD);
            break;

        case SCENE_VICTORY:
            wattron(w_art, COLOR_PAIR(4) | A_BOLD);
            mvwprintw(w_art, y++, 2, "    *** OREGON CITY REACHED! ***     ");
            mvwprintw(w_art, y++, 2, "    *    WILLAMETTE VALLEY     *     ");
            mvwprintw(w_art, y++, 2, "           \\o/        \\o/            ");
            mvwprintw(w_art, y++, 2, "            |          |             ");
            wattroff(w_art, COLOR_PAIR(4) | A_BOLD);
            break;

        case SCENE_DEATH:
            wattron(w_art, COLOR_PAIR(3) | A_BOLD);
            mvwprintw(w_art, y++, 2, "             ______                  ");
            mvwprintw(w_art, y++, 2, "          .-'      '-.               ");
            mvwprintw(w_art, y++, 2, "         /  R. I. P.  \\              ");
            mvwprintw(w_art, y++, 2, "         |____________|              ");
            wattroff(w_art, COLOR_PAIR(3) | A_BOLD);
            break;
    }

    /* Start roster below the art */
    y = 7;
    wattron(w_art, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w_art, y++, 2, "--- EXPEDITION ROSTER ---");
    wattroff(w_art, COLOR_PAIR(1) | A_BOLD);

    int max_y = getmaxy(w_art) - 1;
    for (int i = 0; i < g->party_count && y < max_y; i++) {
        const char *h_str = "Good";
        int pair = 4;
        if (g->party[i].health == HEALTH_FAIR) { h_str = "Fair"; pair = 2; }
        else if (g->party[i].health == HEALTH_POOR) { h_str = "Poor"; pair = 1; }
        else if (g->party[i].health == HEALTH_CRITICAL) { h_str = "Crit"; pair = 3; }
        else if (g->party[i].health == HEALTH_DEAD) { h_str = "DEAD"; pair = 3; }

        char cond[16] = "";
        if (g->party[i].is_sick && g->party[i].health != HEALTH_DEAD) strcat(cond, "[Sick]");
        if (g->party[i].is_injured && g->party[i].health != HEALTH_DEAD) strcat(cond, "[Inj]");

        mvwprintw(w_art, y, 2, "%-10s", g->party[i].name);
        wattron(w_art, COLOR_PAIR(pair) | (pair == 3 ? A_BOLD : 0));
        wprintw(w_art, " %-4s", h_str);
        wattroff(w_art, COLOR_PAIR(pair) | (pair == 3 ? A_BOLD : 0));
        if (cond[0]) wprintw(w_art, " %s", cond);
        y++;
    }

    wrefresh(w_art);
}

void prompt_input(const char *prompt, char *buffer, int max_len) {
    werase(w_cmd);
    box(w_cmd, 0, 0);
    wattron(w_cmd, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w_cmd, 1, 2, "%s: ", prompt);
    wattroff(w_cmd, COLOR_PAIR(1) | A_BOLD);
    wrefresh(w_cmd);

    echo();
    curs_set(1);
    wgetnstr(w_cmd, buffer, max_len - 1);
    noecho();
    curs_set(0);

    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r' || buffer[len-1] == ' ')) {
        buffer[--len] = '\0';
    }
}

void wait_key(const char *msg) {
    werase(w_cmd);
    box(w_cmd, 0, 0);
    wattron(w_cmd, COLOR_PAIR(1));
    mvwprintw(w_cmd, 1, 2, "%s [Press any key...]", msg ? msg : "Continue");
    wattroff(w_cmd, COLOR_PAIR(1));
    wrefresh(w_cmd);
    flushinp();
    wgetch(w_cmd);
}