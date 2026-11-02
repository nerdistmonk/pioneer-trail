#include "setup.h"
#include "tui.h"

void select_skill(GameState *g) {
    char buf[16];
    wprintw(w_main, "How good a shot are you with your rifle?\n");
    wprintw(w_main, " (1) Ace Marksman\n (2) Good Shot\n (3) Fair to Middlin'\n");
    wprintw(w_main, " (4) Need More Practice\n (5) Shaky Knees\n\n");
    wrefresh(w_main);

    prompt_input("Select skill (1-5)", buf, sizeof(buf));
    int s = atoi(buf);
    if (s < 1 || s > 5) s = 3;
    g->skill = s;
}

void setup_party(GameState *g) {
    char buf[16];
    werase(w_main);
    wprintw(w_main, "========================================================\n");
    wprintw(w_main, "               EXPEDITION PARTY ROSTER                  \n");
    wprintw(w_main, "========================================================\n\n");
    wprintw(w_main, "Choose expedition composition:\n");
    wprintw(w_main, " (1) Custom Wagon Party (Leader + 4 Companions)\n");
    wprintw(w_main, " (2) Standard Trail Family (Default 5 members)\n");
    wprintw(w_main, " (3) Solo Trailblazer (Just you)\n\n");
    wrefresh(w_main);

    int mode = 2;
    while (1) {
        prompt_input("Select option (1-3)", buf, sizeof(buf));
        mode = atoi(buf);
        if (mode >= 1 && mode <= 3) break;
    }

    if (mode == 1) {
        g->party_count = 5;
        prompt_input("Enter Wagon Leader name", buf, sizeof(buf));
        snprintf(g->party[0].name, sizeof(g->party[0].name), "%s", buf[0] ? buf : "Leader");
        g->party[0].health = HEALTH_GOOD;

        for (int i = 1; i < 5; i++) {
            char p[32];
            snprintf(p, sizeof(p), "Enter Companion #%d name", i);
            prompt_input(p, buf, sizeof(buf));
            if (!buf[0]) snprintf(buf, sizeof(buf), "Member_%d", i);
            snprintf(g->party[i].name, sizeof(g->party[i].name), "%s", buf);
            g->party[i].health = HEALTH_GOOD;
        }
    } else if (mode == 2) {
        g->party_count = 5;
        prompt_input("Enter your name", buf, sizeof(buf));
        snprintf(g->party[0].name, sizeof(g->party[0].name), "%s", buf[0] ? buf : "Leader");
        g->party[0].health = HEALTH_GOOD;

        const char *defs[] = {"Mary", "John", "Sarah", "James"};
        for (int i = 1; i < 5; i++) {
            snprintf(g->party[i].name, sizeof(g->party[i].name), "%s", defs[i-1]);
            g->party[i].health = HEALTH_GOOD;
        }
    } else {
        g->party_count = 1;
        prompt_input("Enter your name", buf, sizeof(buf));
        snprintf(g->party[0].name, sizeof(g->party[0].name), "%s", buf[0] ? buf : "Scout");
        g->party[0].health = HEALTH_GOOD;
    }
}

void setup_initial_purchases(GameState *g) {
    char buf[64];
    while (1) {
        werase(w_main);
        double budget = 700.0;

        wprintw(w_main, "========================================================\n");
        wprintw(w_main, "            INDEPENDENCE GENERAL OUTFITTERS             \n");
        wprintw(w_main, "========================================================\n");
        wprintw(w_main, "You saved $900 for the expedition. Wagon cost: $200.\n");
        wprintw(w_main, "Total spending money available: $%.0f\n\n", budget);
        wprintw(w_main, "Provisions: Flour, bacon, and dry beans are $0.20 / lb.\n\n");
        wrefresh(w_main);

        double a = 0, f_spend = 0, b = 0, c = 0, m1 = 0;

        while (1) {
            char p[64];
            snprintf(p, sizeof(p), "Spend on Oxen Team ($200-$300) [Available: $%.0f]", budget);
            prompt_input(p, buf, sizeof(buf));
            a = atof(buf);
            if (a < 200.0) {
                wprintw(w_main, "  > $%.0f is not enough! A proper team costs at least $200.\n", a);
            } else if (a > 300.0) {
                wprintw(w_main, "  > $%.0f is too much! Keep team purchases under $300.\n", a);
            } else if (a > budget) {
                wprintw(w_main, "  > You don't have that much cash!\n");
            } else {
                budget -= a;
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "  [Oxen purchased: $%.0f | Remaining Cash: $%.0f]\n\n", a, budget);
                wattroff(w_main, COLOR_PAIR(4));
                wrefresh(w_main);
                break;
            }
            wrefresh(w_main);
        }

        while (1) {
            char p[64];
            snprintf(p, sizeof(p), "Spend on Food Provisions [Available: $%.0f]", budget);
            prompt_input(p, buf, sizeof(buf));
            f_spend = atof(buf);
            if (f_spend < 0.0) {
                wprintw(w_main, "  > Impossible amount.\n");
            } else if (f_spend > budget) {
                wprintw(w_main, "  > You only have $%.0f remaining!\n", budget);
            } else {
                budget -= f_spend;
                double lbs = f_spend * 5.0;
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "  [Provisions: %.0f lbs ($%.0f) | Remaining Cash: $%.0f]\n\n", lbs, f_spend, budget);
                wattroff(w_main, COLOR_PAIR(4));
                wrefresh(w_main);
                break;
            }
            wrefresh(w_main);
        }

        while (1) {
            char p[64];
            snprintf(p, sizeof(p), "Spend on Ammo ($1 = 50 bullets) [Available: $%.0f]", budget);
            prompt_input(p, buf, sizeof(buf));
            b = atof(buf);
            if (b < 0.0) {
                wprintw(w_main, "  > Impossible amount.\n");
            } else if (b > budget) {
                wprintw(w_main, "  > You only have $%.0f remaining!\n", budget);
            } else {
                budget -= b;
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "  [Ammo purchased: %.0f bullets ($%.0f) | Remaining Cash: $%.0f]\n\n", b * 50.0, b, budget);
                wattroff(w_main, COLOR_PAIR(4));
                wrefresh(w_main);
                break;
            }
            wrefresh(w_main);
        }

        while (1) {
            char p[64];
            snprintf(p, sizeof(p), "Spend on Clothing [Available: $%.0f]", budget);
            prompt_input(p, buf, sizeof(buf));
            c = atof(buf);
            if (c < 0.0) {
                wprintw(w_main, "  > Impossible amount.\n");
            } else if (c > budget) {
                wprintw(w_main, "  > You only have $%.0f remaining!\n", budget);
            } else {
                budget -= c;
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "  [Clothing purchased: $%.0f | Remaining Cash: $%.0f]\n\n", c, budget);
                wattroff(w_main, COLOR_PAIR(4));
                wrefresh(w_main);
                break;
            }
            wrefresh(w_main);
        }

        while (1) {
            char p[64];
            snprintf(p, sizeof(p), "Spend on Misc Supplies [Available: $%.0f]", budget);
            prompt_input(p, buf, sizeof(buf));
            m1 = atof(buf);
            if (m1 < 0.0) {
                wprintw(w_main, "  > Impossible amount.\n");
            } else if (m1 > budget) {
                wprintw(w_main, "  > You only have $%.0f remaining!\n", budget);
            } else {
                budget -= m1;
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "  [Supplies purchased: $%.0f | Remaining Cash: $%.0f]\n\n", m1, budget);
                wattroff(w_main, COLOR_PAIR(4));
                wrefresh(w_main);
                break;
            }
            wrefresh(w_main);
        }

        g->animals  = a;
        g->food     = f_spend * 5.0;
        g->ammo     = b * 50.0;
        g->clothing = c;
        g->misc     = m1;
        g->cash     = budget;
        break;
    }

    wprintw(w_main, "--------------------------------------------------------\n");
    wprintw(w_main, "All provisions secured. Ready to start the trek with $%.0f in reserve.\n", g->cash);
    wrefresh(w_main);
    wait_key("Hit the Trail");
}

void do_fort_trade(GameState *g) {
    char buf[32];
    wprintw(w_main, "\n--- Fort Trading Post (Prices higher: 2/3 yield) ---\n");
    wrefresh(w_main);

    const char *items[] = {"Food Provisions", "Ammunition", "Clothing", "Misc Supplies"};
    for (int i = 0; i < 4; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Spend on %s (Cash: $%.0f)", items[i], g->cash);
        prompt_input(prompt, buf, sizeof(buf));
        double spend = atof(buf);
        if (spend <= 0.0) {
            spend = 0.0;
        } else if (spend > g->cash) {
            wprintw(w_main, "You don't have that much! Missed your chance on %s.\n", items[i]);
            spend = 0.0;
        }

        if (spend > 0.0) {
            g->cash -= spend;
            if (i == 0) {
                double lbs_bought = (2.0 / 3.0) * spend * 5.0;
                g->food += lbs_bought;
                wprintw(w_main, "  Purchased %.0f lbs of food.\n", lbs_bought);
            }
            if (i == 1) g->ammo += (int)((2.0 / 3.0) * spend * 50.0);
            if (i == 2) g->clothing += (2.0 / 3.0) * spend;
            if (i == 3) g->misc += (2.0 / 3.0) * spend;
            update_hud(g);
        }
    }

    if (g->miles >= 45.0) {
        g->miles -= 45.0;
    } else {
        g->miles = 0.0;
    }
    clamp_supplies(g);
}