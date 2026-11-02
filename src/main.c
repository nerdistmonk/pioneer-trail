#include "common.h"
#include "tui.h"
#include "setup.h"
#include "events.h"
#include "scores.h"

void game_over_death(GameState *g, const char *cause);
static void victory_screen(GameState *g);
static double calculate_trail_progress(GameState *g);

void check_party_vital_status(GameState *g) {
    if (g->party[0].health == HEALTH_DEAD) {
        game_over_death(g, "EXPEDITION LEADER HAS PERISHED");
    }
    if (get_living_member_count(g) == 0) {
        game_over_death(g, "ALL PARTY MEMBERS HAVE PERISHED");
    }
}

void game_over_death(GameState *g, const char *cause) {
    char buf[64];
    werase(w_main);
    update_hud(g);
    update_art_panel(g, SCENE_DEATH, 0);

    int final_score = calculate_score(g, 0);

    wattron(w_main, COLOR_PAIR(3) | A_BOLD);
    wprintw(w_main, "\n===============================================================\n");
    wprintw(w_main, "                    TRAGEDY STRIKES THE EXPEDITION             \n");
    wprintw(w_main, "===============================================================\n\n");
    wattroff(w_main, COLOR_PAIR(3) | A_BOLD);

    wprintw(w_main, "  CAUSE OF DEATH : %s\n", cause);
    wprintw(w_main, "  TRAIL DISTANCE : %.0f / 2040 miles (%.1f%% complete)\n",
            g->miles, (g->miles / TOTAL_TRAIL_MILES) * 100.0);
    wprintw(w_main, "  FINAL TURN     : Turn %d (Monday, %s 1847)\n",
            g->turn + 1, (g->turn >= 0 && g->turn < 20) ? DATES[g->turn] : "Late Winter");
    wprintw(w_main, "  RESOURCES LEFT : Food: %d lbs | Ammo: %d | Misc: $%d | Cash: $%d\n",
            (int)g->food, (int)g->ammo, (int)g->misc, (int)g->cash);
    wprintw(w_main, "  FINAL SCORE    : %d points\n", final_score);
    wprintw(w_main, "---------------------------------------------------------------\n\n");
    wrefresh(w_main);

    prompt_input("Would you like a minister? (YES/NO)", buf, sizeof(buf));
    prompt_input("Would you like a fancy funeral? (YES/NO)", buf, sizeof(buf));
    prompt_input("Inform next of kin? (YES/NO)", buf, sizeof(buf));

    for (char *p = buf; *p; ++p) *p = toupper((unsigned char)*p);
    if (strcmp(buf, "YES") == 0) {
        wprintw(w_main, "\nThat will be $4.50 for the telegraph charge.\n");
    } else {
        wprintw(w_main, "\nBut Aunt Sadie in St. Louis was really worried about you!\n");
    }
    wprintw(w_main, "\nSincerely,\nTHE OREGON CITY CHAMBER OF COMMERCE\n\n");
    wrefresh(w_main);

    prompt_input("Enter your name/initials for the headstone", buf, sizeof(buf));
    if (!buf[0]) snprintf(buf, sizeof(buf), "%s", g->party[0].name);
    add_high_score(buf, final_score, (int)g->miles, "PERISHED");

    show_leaderboard();
    teardown_tui();
    exit(0);
}

static void victory_screen(GameState *g) {
    char buf[64];
    werase(w_main);
    update_hud(g);
    update_art_panel(g, SCENE_VICTORY, 0);

    double delta_miles = g->miles - g->prev_miles;
    double frac = 1.0;
    if (delta_miles > 0.0) {
        frac = (TOTAL_TRAIL_MILES - g->prev_miles) / delta_miles;
    }
    if (frac < 0.0) frac = 0.0;
    if (frac > 1.0) frac = 1.0;

    int total_days = (g->turn * 14) + (int)(frac * 14.0);

    struct tm arrival_tm = {0};
    arrival_tm.tm_year = 1847 - 1900;
    arrival_tm.tm_mon  = 2;
    arrival_tm.tm_mday = 29 + total_days;
    mktime(&arrival_tm);

    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%A, %B %d, %Y", &arrival_tm);

    int final_score = calculate_score(g, 1);

    wattron(w_main, COLOR_PAIR(4) | A_BOLD);
    wprintw(w_main, "\n***************************************************************\n");
    wprintw(w_main, "               YOU FINALLY ARRIVED AT OREGON CITY!             \n");
    wprintw(w_main, "            AFTER 2040 LONG MILES --- A REAL PIONEER!          \n");
    wprintw(w_main, "***************************************************************\n\n");
    wattroff(w_main, COLOR_PAIR(4) | A_BOLD);

    wprintw(w_main, "  PRESIDENT JAMES K. POLK SENDS YOU HIS HEARTIEST CONGRATULATIONS\n");
    wprintw(w_main, "  AND WISHES YOU A PROSPEROUS LIFE AHEAD AT YOUR NEW HOME!\n\n");
    wprintw(w_main, "  TRAIL PERFORMANCE:\n");
    wprintw(w_main, "    Total Distance  : 2040 miles (100.0%%)\n");
    wprintw(w_main, "    Arrival Date    : %s\n", date_buf);
    wprintw(w_main, "    Party Survived  : %d of %d pioneers\n",
            get_living_member_count(g), g->party_count);
    wprintw(w_main, "    Remaining Cash  : $%d\n", (int)g->cash);
    wprintw(w_main, "    Provisions Left : Food: %d lbs | Ammo: %d | Misc: $%d | Clothes: $%d\n",
            (int)g->food, (int)g->ammo, (int)g->misc, (int)g->clothing);
    wprintw(w_main, "    FINAL SCORE     : %d POINTS\n", final_score);
    wprintw(w_main, "---------------------------------------------------------------\n");
    wrefresh(w_main);

    prompt_input("Enter your name for the Pioneers Roll of Honor", buf, sizeof(buf));
    if (!buf[0]) snprintf(buf, sizeof(buf), "%s", g->party[0].name);
    add_high_score(buf, final_score, 2040, "ARRIVED");

    show_leaderboard();
    teardown_tui();
    exit(0);
}

static double calculate_trail_progress(GameState *g) {
    double oxen_factor = (g->animals - 200.0) / 4.0;
    if (oxen_factor < 0.0) oxen_factor = 0.0;
    double base = 140.0 + oxen_factor + 15.0 * rnd();

    double cond_roll = rnd();
    if (g->turn <= 3) {
        if (cond_roll < 0.40) {
            wprintw(w_main, "Trail Condition: Spring mud and swollen creeks slow wagon wheels.\n");
            base *= 0.80;
        } else {
            wprintw(w_main, "Trail Condition: Mild spring weather and good prairie grass.\n");
        }
    } else if (g->miles < 950.0) {
        if (cond_roll < 0.25) {
            wprintw(w_main, "Trail Condition: Prairie windstorms and deep sandy ruts.\n");
            base *= 0.85;
        } else if (cond_roll > 0.80) {
            wprintw(w_main, "Trail Condition: Clear skies and firm ground allow good time!\n");
            base *= 1.15;
        }
    } else if (g->miles < 1600.0) {
        if (cond_roll < 0.45) {
            wprintw(w_main, "Trail Condition: Steep rocky ascents and choking alkali dust.\n");
            base *= 0.78;
        } else {
            wprintw(w_main, "Trail Condition: Dry mountain road; steady hauling.\n");
            base *= 0.95;
        }
    } else {
        if (cond_roll < 0.50) {
            wprintw(w_main, "Trail Condition: Mountain grades, fallen timber, and icy trails.\n");
            base *= 0.75;
        }
    }

    if (g->pace == PACE_STRENUOUS) {
        base *= 1.25;
        if (rnd() < 0.20) {
            wprintw(w_main, "Strenuous pace exhausted the team; an ox collapsed!\n");
            g->animals -= 15.0;
        }
    } else if (g->pace == PACE_GRUELING) {
        base *= 1.50;
        if (rnd() < 0.35) {
            wprintw(w_main, "Grueling pace causes severe fatigue and illness!\n");
            handle_illness(g);
        }
    }
    return base;
}

int main(void) {
    srand((unsigned)time(NULL));
    init_tui();
    draw_frame();

    GameState g = {0};
    g.pace = PACE_STEADY;
    g.fort_available = 0;

    char buf[64];
    prompt_input("Do you need instructions? (YES/NO)", buf, sizeof(buf));
    for (char *p = buf; *p; ++p) *p = toupper((unsigned char)*p);

    if (strcmp(buf, "YES") == 0) {
        wprintw(w_main, "THIS PROGRAM SIMULATES THE 1847 PIONEER TREK FROM\n");
        wprintw(w_main, "INDEPENDENCE, MISSOURI TO OREGON CITY'S WILLAMETTE VALLEY.\n");
        wprintw(w_main, "YOUR EXPEDITION WILL COVER 2040 MILES ACROSS PLAINS,\n");
        wprintw(w_main, "FORTS, DESERTS, AND THE ROCKY MOUNTAINS.\n\n");
        wrefresh(w_main);
        wait_key("Continue to Setup");
    }

    werase(w_main);
    select_skill(&g);
    setup_party(&g);
    setup_initial_purchases(&g);

    for (g.turn = 0; g.turn < 20; g.turn++) {
        werase(w_main);
        update_hud(&g);

        if (g.miles >= 950.0) {
            update_art_panel(&g, SCENE_MOUNTAINS, g.turn);
        } else {
            update_art_panel(&g, SCENE_PLAINS, g.turn);
        }

        check_party_vital_status(&g);

        int sick_count = 0;
        for (int i = 0; i < g.party_count; i++) {
            if (g.party[i].health != HEALTH_DEAD && (g.party[i].is_sick || g.party[i].is_injured)) {
                sick_count++;
            }
        }

        if (sick_count > 0) {
            double bill = sick_count * 20.0;
            if (g.cash >= bill) {
                g.cash -= bill;
                wprintw(w_main, "Doctor treated %d sick/injured member(s) ($%.0f paid in cash).\n", sick_count, bill);
                for (int i = 0; i < g.party_count; i++) {
                    if (g.party[i].health != HEALTH_DEAD) {
                        g.party[i].is_sick = 0;
                        g.party[i].is_injured = 0;
                        if (g.party[i].health > HEALTH_GOOD) g.party[i].health--;
                    }
                }
            } else if (g.misc >= 5.0 * sick_count) {
                g.misc -= 5.0 * sick_count;
                wprintw(w_main, "Short on cash; used emergency medical supplies to care for party.\n");
                for (int i = 0; i < g.party_count; i++) {
                    if (g.party[i].health != HEALTH_DEAD) {
                        g.party[i].is_sick = 0;
                        g.party[i].is_injured = 0;
                    }
                }
            } else {
                wattron(w_main, COLOR_PAIR(3));
                wprintw(w_main, "Cannot afford medical attention for all patients! Health worsens.\n");
                wattroff(w_main, COLOR_PAIR(3));
                for (int i = 0; i < g.party_count; i++) {
                    if (g.party[i].health != HEALTH_DEAD && (g.party[i].is_sick || g.party[i].is_injured)) {
                        if (g.party[i].health < HEALTH_DEAD) g.party[i].health++;
                        if (g.party[i].health == HEALTH_DEAD) {
                            wprintw(w_main, "%s succumbed to illness and died!\n", g.party[i].name);
                        }
                    }
                }
                check_party_vital_status(&g);
            }
        }

        int living = get_living_member_count(&g);
        if (g.food < living * 13.0) {
            wattron(w_main, COLOR_PAIR(3));
            wprintw(w_main, "Warning: Provisions running dangerously low! Hunt or visit a post soon.\n");
            wattroff(w_main, COLOR_PAIR(3));
        }

        while (1) {
            prompt_input("Choose Pace: (1) Steady (2) Strenuous (3) Grueling", buf, sizeof(buf));
            int p = atoi(buf);
            if (p >= 1 && p <= 3) {
                g.pace = (TravelPace)p;
                break;
            }
        }
        update_hud(&g);

        int action = 3;
        while (1) {
            if (g.fort_available) {
                prompt_input("(1) Stop at Fort  (2) Hunt  (3) Continue", buf, sizeof(buf));
                action = atoi(buf);
                if (action >= 1 && action <= 3) break;
            } else {
                prompt_input("(1) Hunt  (2) Continue", buf, sizeof(buf));
                int choice = atoi(buf);
                if (choice == 1) { action = 2; break; }
                if (choice == 2) { action = 3; break; }
            }
        }
        g.fort_available = !g.fort_available;

        if (action == 1) {
            do_fort_trade(&g);
        } else if (action == 2) {
            do_hunt(&g);
        }

        if (g.food <= 0.0) {
            g.miles += 30.0 + 10.0 * rnd();
            game_over_death(&g, "STARVATION (OUT OF PROVISIONS)");
        }

        living = get_living_member_count(&g);
        while (1) {
            char p[128];
            snprintf(p, sizeof(p), "Rations: (1) Poor (%dlbs) (2) Moderate (%dlbs) (3) Well (%dlbs)",
                     living * 13, living * 18, living * 23);
            prompt_input(p, buf, sizeof(buf));
            g.eating_choice = atoi(buf);
            if (g.eating_choice >= 1 && g.eating_choice <= 3) {
                double per_person = (g.eating_choice == 3) ? 23.0 : (g.eating_choice == 2) ? 18.0 : 13.0;
                double total_ration = living * per_person;
                if (g.food >= total_ration) {
                    g.food -= total_ration;
                    break;
                } else if (g.food > 0.0) {
                    wprintw(w_main, "Not enough food for full rations; party stretches remaining %.0f lbs.\n", g.food);
                    g.food = 0.0;
                    break;
                } else {
                    break;
                }
            }
        }

        g.prev_miles = g.miles;
        double miles_this_turn = calculate_trail_progress(&g);
        g.miles += miles_this_turn;
        wprintw(w_main, "Expedition traveled %.0f miles this fortnight.\n", miles_this_turn);

        handle_landmarks_and_rivers(&g);
        check_party_vital_status(&g);

        handle_riders(&g);
        check_party_vital_status(&g);

        handle_random_event(&g);
        check_party_vital_status(&g);

        clamp_supplies(&g);
        update_hud(&g);
        update_art_panel(&g, (g.miles >= 950.0 ? SCENE_MOUNTAINS : SCENE_PLAINS), g.turn);
        wrefresh(w_main);

        if (!g.columbia_done && g.miles >= 1850.0) {
            g.columbia_done = 1;
            handle_finale_columbia_river(&g);
            check_party_vital_status(&g);
            g.prev_miles = TOTAL_TRAIL_MILES - miles_this_turn;
            g.miles = TOTAL_TRAIL_MILES;
        }

        if (g.miles >= TOTAL_TRAIL_MILES) {
            victory_screen(&g);
        }

        wait_key("End of Turn");
    }

    game_over_death(&g, "BLIZZARD (YOU WERE ON THE TRAIL TOO LONG)");
    return 0;
}