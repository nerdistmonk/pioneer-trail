#include "events.h"
#include "tui.h"

extern void game_over_death(GameState *g, const char *cause);

static const char *SKILL_NAMES[] = {
    "", "Ace Marksman", "Good Shot", "Fair to Middlin'", "Need Practice", "Shaky Knees"
};

double do_shooting(GameState *g) {
    static const char *words[] = {"BANG", "BLAM", "POW", "WHAM"};
    int s6 = rand() % 4;
    const char *target = words[s6];

    char input[32] = {0};
    int input_len = 0;
    double leeway = (double)g->skill - 1.0;

    flushinp();

    struct timespec t_start, t_now;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    wtimeout(w_cmd, 50);
    curs_set(1);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &t_now);
        double elapsed = (t_now.tv_sec - t_start.tv_sec) + 
                         (t_now.tv_nsec - t_start.tv_nsec) / 1e9;

        if (elapsed >= 9.0) {
            werase(w_cmd);
            box(w_cmd, 0, 0);
            wattron(w_cmd, COLOR_PAIR(3) | A_BOLD);
            mvwprintw(w_cmd, 1, 2, "TOO SLOW! [9.0s elapsed]");
            wattroff(w_cmd, COLOR_PAIR(3) | A_BOLD);
            wrefresh(w_cmd);
            napms(400);
            curs_set(0);
            wtimeout(w_cmd, -1);
            return 9.0;
        }

        werase(w_cmd);
        box(w_cmd, 0, 0);

        wattron(w_cmd, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w_cmd, 1, 2, "TYPE '%s': ", target);
        wattroff(w_cmd, COLOR_PAIR(3) | A_BOLD);

        wprintw(w_cmd, "%s", input);

        wattron(w_cmd, COLOR_PAIR(2));
        mvwprintw(w_cmd, 1, 40, "[Time: %.1fs | %s (-%.0fs)]",
                  elapsed, SKILL_NAMES[g->skill], leeway);
        wattroff(w_cmd, COLOR_PAIR(2));

        wrefresh(w_cmd);

        int ch = wgetch(w_cmd);
        if (ch == ERR) continue;

        if (ch == '\n' || ch == '\r') {
            break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (input_len > 0) input[--input_len] = '\0';
        } else if (isprint(ch) && input_len < 31) {
            input[input_len++] = (char)toupper((unsigned char)ch);
            input[input_len] = '\0';
        }
    }

    curs_set(0);
    wtimeout(w_cmd, -1);

    clock_gettime(CLOCK_MONOTONIC, &t_now);
    double elapsed = (t_now.tv_sec - t_start.tv_sec) + 
                     (t_now.tv_nsec - t_start.tv_nsec) / 1e9;

    double b1 = elapsed - leeway;
    if (b1 < 0.0) b1 = 0.0;

    if (strcmp(input, target) != 0) {
        wattron(w_main, COLOR_PAIR(3));
        wprintw(w_main, "Misfire! You typed '%s' instead of '%s'.\n", input, target);
        wattroff(w_main, COLOR_PAIR(3));
        b1 = 9.0;
    } else {
        wattron(w_main, COLOR_PAIR(4));
        wprintw(w_main, "Reaction time: %.2fs (Adjusted score: %.2fs)\n", elapsed, b1);
        wattroff(w_main, COLOR_PAIR(4));
    }
    return b1;
}

void do_hunt(GameState *g) {
    if (g->ammo < 40.0) {
        wprintw(w_main, "\nTough luck - you need at least 40 bullets to go hunting.\n");
        return;
    }
    g->miles -= 45.0;
    double b1 = do_shooting(g);

    if (b1 <= 1.0) {
        double meat = 260.0 + rnd() * 40.0;
        wattron(w_main, COLOR_PAIR(4) | A_BOLD);
        wprintw(w_main, "\nRIGHT BETWEEN THE EYES! You bagged a massive bison! (+%.0f lbs of meat)\n", meat);
        wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
        g->food += meat;
        g->ammo -= (10.0 + rnd() * 4.0);
    } else if (100.0 * rnd() < 13.0 * b1) {
        wprintw(w_main, "\nYou missed - and your dinner got away...\n");
        g->ammo -= (10.0 + 2.0 * b1);
    } else {
        double meat = 180.0 - 10.0 * b1;
        wprintw(w_main, "\nNice shot! Bagged an antelope! (+%.0f lbs of meat)\n", meat);
        g->food += meat;
        g->ammo -= (10.0 + 3.0 * b1);
    }
    clamp_supplies(g);
}

void handle_illness(GameState *g) {
    int target = get_random_living_member(g);
    double r = 100.0 * rnd();

    if (r < 10.0 + 35.0 * (g->eating_choice - 1)) {
        wprintw(w_main, "\nMild sickness for %s - medicine used.\n", g->party[target].name);
        g->miles -= 5.0;
        g->misc -= 2.0;
        if (g->party[target].health < HEALTH_POOR) g->party[target].health++;
    } else if (r < 100.0 - (40.0 / pow(4.0, g->eating_choice - 1))) {
        wprintw(w_main, "\nBad illness strikes %s!\n", g->party[target].name);
        g->miles -= 5.0;
        g->misc -= 5.0;
        g->party[target].is_sick = 1;
        if (g->party[target].health < HEALTH_CRITICAL) g->party[target].health++;
    } else {
        wattron(w_main, COLOR_PAIR(3));
        wprintw(w_main, "\nSevere illness! %s is in critical condition!\n", g->party[target].name);
        wattroff(w_main, COLOR_PAIR(3));
        g->misc -= 10.0;
        g->party[target].is_sick = 1;
        g->party[target].health = HEALTH_CRITICAL;
    }
    clamp_supplies(g);
}

void handle_riders(GameState *g) {
    double m_scaled = (g->miles / 100.0) - 4.0;
    double p_attack = ((m_scaled * m_scaled + 72.0) / (m_scaled * m_scaled + 12.0) - 1.0) / 10.0;
    p_attack *= 0.40;

    if (rnd() > p_attack) return;

    wattron(w_main, COLOR_PAIR(3) | A_BOLD);
    wprintw(w_main, "\nRIDERS SPOTTED AHEAD!\n");
    wattroff(w_main, COLOR_PAIR(3) | A_BOLD);

    int hostile = (rnd() < 0.8) ? 1 : 0;
    if (hostile) {
        wprintw(w_main, "They look hostile!\n");
    } else {
        wprintw(w_main, "They do not look hostile.\n");
    }

    if (rnd() < 0.20) hostile = !hostile;

    wprintw(w_main, "Tactics: (1) Run  (2) Attack  (3) Continue  (4) Circle Wagons\n");
    wrefresh(w_main);

    char buf[16];
    int tactic = 3;
    while (1) {
        prompt_input("Select tactic (1-4)", buf, sizeof(buf));
        tactic = atoi(buf);
        if (tactic >= 1 && tactic <= 4) break;
    }

    if (hostile) {
        if (tactic == 1) {
            wprintw(w_main, "You gallop away! Oxen worn down, supplies jostled loose.\n");
            g->miles += 20.0;
            g->misc -= 15.0;
            g->animals -= 40.0;
            clamp_supplies(g);
            return;
        } else if (tactic == 2) {
            double b1 = do_shooting(g);
            g->ammo -= (b1 * 40.0 + 80.0);
            if (b1 <= 1.0) {
                wprintw(w_main, "Nice shooting - you drove them off!\n");
            } else if (b1 <= 4.0) {
                wprintw(w_main, "Kinda slow with your Colt .45 - took some hits.\n");
            } else {
                int victim = get_random_living_member(g);
                wprintw(w_main, "Lousy shot - %s took a bullet! Need a doctor.\n", g->party[victim].name);
                g->party[victim].is_injured = 1;
                g->party[victim].health = HEALTH_CRITICAL;
            }
        } else if (tactic == 3) {
            if (rnd() > 0.80) {
                wprintw(w_main, "They passed without attacking.\n");
            } else {
                wprintw(w_main, "They raided your wagon as you moved!\n");
                g->ammo -= (g->ammo >= 150.0) ? 150.0 : g->ammo;
                g->misc -= 15.0;
            }
        } else {
            double b1 = do_shooting(g);
            g->ammo -= (b1 * 30.0 + 80.0);
            g->miles -= 25.0;
            if (b1 <= 1.0) {
                wprintw(w_main, "Drove them off from behind the wagons!\n");
            } else if (b1 <= 4.0) {
                wprintw(w_main, "Slow draw - you held them off but expended ammunition.\n");
            } else {
                wprintw(w_main, "They overwhelmed your wagon circle and took supplies.\n");
                g->misc -= 10.0;
            }
        }

        clamp_supplies(g);
        if (g->ammo <= 0.0) {
            game_over_death(g, "MASSACRED BY RAIDERS (RAN OUT OF AMMO)");
        }
    } else {
        if (tactic == 1) {
            g->miles += 15.0;
            g->animals -= 10.0;
            wprintw(w_main, "They waved as you sped past.\n");
        } else if (tactic == 2) {
            wprintw(w_main, "You opened fire on friendly travelers! Ammunition wasted.\n");
            g->ammo -= (g->ammo >= 100.0) ? 100.0 : g->ammo;
            g->miles -= 5.0;
        } else if (tactic == 3) {
            wprintw(w_main, "Friendly riders tipped their hats as they passed.\n");
        } else {
            wprintw(w_main, "You circled wagons for nothing - time lost.\n");
            g->miles -= 20.0;
        }
    }
    clamp_supplies(g);
}

void handle_river_crossing(GameState *g, const char *river_name, double depth_feet) {
    update_art_panel(g, SCENE_RIVER, 0);
    wattron(w_main, COLOR_PAIR(6) | A_BOLD);
    wprintw(w_main, "\n=======================================================\n");
    wprintw(w_main, "           RIVER CROSSING: %s (Depth: %.1f ft)         \n", river_name, depth_feet);
    wprintw(w_main, "=======================================================\n\n");
    wattroff(w_main, COLOR_PAIR(6) | A_BOLD);

    wprintw(w_main, "Choose how to cross:\n");
    wprintw(w_main, " (1) Ford the river (drive oxen across)\n");
    wprintw(w_main, " (2) Caulk the wagon and float across\n");
    wprintw(w_main, " (3) Take ferry / Hire Native guide ($6.00)\n\n");
    wrefresh(w_main);

    char buf[16];
    int choice = 1;
    while (1) {
        prompt_input("Select river crossing tactic (1-3)", buf, sizeof(buf));
        choice = atoi(buf);
        if (choice >= 1 && choice <= 3) break;
    }

    if (choice == 1) {
        if (depth_feet > 3.0) {
            wattron(w_main, COLOR_PAIR(3));
            wprintw(w_main, "Water is too deep! Flour and bacon sacks are swamped!\n");
            wattroff(w_main, COLOR_PAIR(3));
            g->food -= 120.0 + 40.0 * rnd();
            g->clothing -= 20.0;
            g->ammo -= 50.0;
            g->misc -= 10.0;

            if (rnd() < 0.20) {
                int victim = get_random_living_member(g);
                wprintw(w_main, "Tragedy! %s was swept away by the river and drowned!\n", g->party[victim].name);
                g->party[victim].health = HEALTH_DEAD;
                check_party_vital_status(g);
            }
            if (rnd() < 0.25) {
                wprintw(w_main, "An ox drowned during the crossing!\n");
                g->animals -= 30.0;
            }
        } else {
            wattron(w_main, COLOR_PAIR(4));
            wprintw(w_main, "Shallow waters! You forded the river successfully!\n");
            wattroff(w_main, COLOR_PAIR(4));
        }
    } else if (choice == 2) {
        if (rnd() < 0.20) {
            wattron(w_main, COLOR_PAIR(3));
            wprintw(w_main, "Wagon overturned in the current! Heavy provisions sunk!\n");
            wattroff(w_main, COLOR_PAIR(3));
            g->food -= 180.0;
            g->clothing -= 25.0;
            g->ammo -= 100.0;
            if (rnd() < 0.15) {
                int victim = get_random_living_member(g);
                wprintw(w_main, "%s was trapped beneath the overturned wagon and drowned!\n", g->party[victim].name);
                g->party[victim].health = HEALTH_DEAD;
                check_party_vital_status(g);
            }
        } else {
            wattron(w_main, COLOR_PAIR(4));
            wprintw(w_main, "Wagon floated cleanly to the opposite bank!\n");
            wattroff(w_main, COLOR_PAIR(4));
        }
    } else {
        if (g->cash >= 6.0) {
            g->cash -= 6.0;
            wattron(w_main, COLOR_PAIR(4));
            wprintw(w_main, "Ferry/Guide safely navigated your family across ($6 paid).\n");
            wattroff(w_main, COLOR_PAIR(4));
        } else {
            wattron(w_main, COLOR_PAIR(3));
            wprintw(w_main, "Not enough cash! Forced to caulk and float the wagon.\n");
            wattroff(w_main, COLOR_PAIR(3));
            if (rnd() < 0.20) {
                wattron(w_main, COLOR_PAIR(3));
                wprintw(w_main, "Wagon overturned in the current! Heavy provisions sunk!\n");
                wattroff(w_main, COLOR_PAIR(3));
                g->food -= 180.0;
                g->clothing -= 25.0;
                g->ammo -= 100.0;
                if (rnd() < 0.15) {
                    int victim = get_random_living_member(g);
                    wprintw(w_main, "%s was trapped beneath the overturned wagon and drowned!\n", g->party[victim].name);
                    g->party[victim].health = HEALTH_DEAD;
                    check_party_vital_status(g);
                }
            } else {
                wattron(w_main, COLOR_PAIR(4));
                wprintw(w_main, "Wagon floated cleanly to the opposite bank!\n");
                wattroff(w_main, COLOR_PAIR(4));
            }
        }
    }
    clamp_supplies(g);
    update_hud(g);
    wait_key("River Crossed");
}

void handle_landmarks_and_rivers(GameState *g) {
    if (!g->kansas_river_done && g->miles >= 102.0) {
        g->kansas_river_done = 1;
        handle_river_crossing(g, "KANSAS RIVER", 4.5);
    }
    if (!g->ft_kearny_done && g->miles >= 319.0) {
        g->ft_kearny_done = 1;
        update_art_panel(g, SCENE_FORT, 0);
        wprintw(w_main, "\n>>> ARRIVED AT FORT KEARNY (NEBRASKA) <<<\n");
        wprintw(w_main, "The first military post protecting the Oregon Trail.\n");
        wrefresh(w_main);
        wait_key("Fort Kearny");
    }
    if (!g->chimney_rock_done && g->miles >= 554.0) {
        g->chimney_rock_done = 1;
        update_art_panel(g, SCENE_LANDMARK, 0);
        wattron(w_main, COLOR_PAIR(5) | A_BOLD);
        wprintw(w_main, "\n>>> CHIMNEY ROCK IN VIEW (Mile 554) <<<\n");
        wattroff(w_main, COLOR_PAIR(5) | A_BOLD);
        wprintw(w_main, "The towering spire rises 300 feet over the North Platte.\n");
        wrefresh(w_main);
        wait_key("Chimney Rock");
    }
    if (!g->ft_laramie_done && g->miles >= 640.0) {
        g->ft_laramie_done = 1;
        update_art_panel(g, SCENE_FORT, 0);
        wprintw(w_main, "\n>>> ARRIVED AT FORT LARAMIE (WYOMING) <<<\n");
        wprintw(w_main, "Major fur trading stronghold and haven before the Rockies.\n");
        wrefresh(w_main);
        wait_key("Fort Laramie");
    }
    if (!g->indep_rock_done && g->miles >= 830.0) {
        g->indep_rock_done = 1;
        update_art_panel(g, SCENE_LANDMARK, 0);
        wattron(w_main, COLOR_PAIR(5) | A_BOLD);
        wprintw(w_main, "\n>>> INDEPENDENCE ROCK (Mile 830) <<<\n");
        wattroff(w_main, COLOR_PAIR(5) | A_BOLD);
        wprintw(w_main, "'Register of the Desert' - pioneers carve names on granite.\n");
        wrefresh(w_main);
        wait_key("Independence Rock");
    }
    if (!g->sublette_taken && g->miles >= 980.0) {
        g->sublette_taken = 1;
        wprintw(w_main, "\n=== ROUTE CHOICE: SUBLETTE CUTOFF ===\n");
        wprintw(w_main, "Take the cutoff? Saves 50 miles, but crosses 45 miles of waterless desert.\n");
        char buf[16];
        prompt_input("Take cutoff? (1) Yes (Shorter/Dry) (2) No (Standard Trail)", buf, sizeof(buf));
        if (atoi(buf) == 1) {
            wprintw(w_main, "You took the cutoff! 50 miles saved, but oxen stressed.\n");
            g->miles += 50.0;
            g->animals -= 25.0;
        }
    }
    if (!g->green_river_done && g->miles >= 1100.0) {
        g->green_river_done = 1;
        handle_river_crossing(g, "GREEN RIVER", 6.0);
    }
    if (!g->ft_hall_done && g->miles >= 1288.0) {
        g->ft_hall_done = 1;
        update_art_panel(g, SCENE_FORT, 0);
        wprintw(w_main, "\n>>> ARRIVED AT FORT HALL (IDAHO) <<<\n");
        wprintw(w_main, "Where the California and Oregon trails diverge.\n");
        wrefresh(w_main);
        wait_key("Fort Hall");
    }
    if (!g->snake_river_done && g->miles >= 1530.0) {
        g->snake_river_done = 1;
        handle_river_crossing(g, "SNAKE RIVER", 5.2);
    }
    if (!g->ft_boise_done && g->miles >= 1642.0) {
        g->ft_boise_done = 1;
        update_art_panel(g, SCENE_FORT, 0);
        wprintw(w_main, "\n>>> ARRIVED AT FORT BOISE <<<\n");
        wprintw(w_main, "Final supply fort before tackling the Blue Mountains.\n");
        wrefresh(w_main);
        wait_key("Fort Boise");
    }
}

void handle_finale_columbia_river(GameState *g) {
    werase(w_main);
    update_hud(g);
    update_art_panel(g, SCENE_RIVER, 0);

    wattron(w_main, COLOR_PAIR(1) | A_BOLD);
    wprintw(w_main, "\n=======================================================\n");
    wprintw(w_main, "        THE DALLES: THE FINAL PASSAGE TO OREGON        \n");
    wprintw(w_main, "=======================================================\n\n");
    wattroff(w_main, COLOR_PAIR(1) | A_BOLD);

    wprintw(w_main, "You have reached The Dalles on the Columbia River (Mile 1850).\n");
    wprintw(w_main, "Choose your final leg into Oregon City:\n");
    wprintw(w_main, " (1) Raft the treacherous Columbia River rapids\n");
    wprintw(w_main, " (2) Take Barlow Toll Road around Mt. Hood ($8.00 toll)\n\n");
    wrefresh(w_main);

    char buf[16];
    int choice = 1;
    while (1) {
        prompt_input("Select route (1 or 2)", buf, sizeof(buf));
        choice = atoi(buf);
        if (choice == 1 || choice == 2) break;
    }

    if (choice == 1) {
        wprintw(w_main, "\nYou launch your raft into the roaring Columbia River Gorge...\n");
        wrefresh(w_main);
        double b1 = do_shooting(g);
        if (b1 <= 2.0) {
            wattron(w_main, COLOR_PAIR(4) | A_BOLD);
            wprintw(w_main, "Superb steering! You shot through the cascades safely!\n");
            wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
        } else {
            wattron(w_main, COLOR_PAIR(3) | A_BOLD);
            wprintw(w_main, "The raft crashed into the rocks! Flour barrels swept away!\n");
            wattroff(w_main, COLOR_PAIR(3) | A_BOLD);
            g->food -= 150.0;
            g->clothing -= 30.0;
            g->ammo -= 100.0;
            if (rnd() < 0.25) {
                int victim = get_random_living_member(g);
                wprintw(w_main, "%s was thrown into the rapids and lost!\n", g->party[victim].name);
                g->party[victim].health = HEALTH_DEAD;
                check_party_vital_status(g);
            }
        }
    } else {
        if (g->cash >= 8.0) {
            g->cash -= 8.0;
            wprintw(w_main, "Paid $8.00 toll. Wagon hauled over the rugged Mt. Hood passes.\n");
        } else {
            wprintw(w_main, "Short on cash! Traded supplies to pay Samuel Barlow's toll.\n");
            g->misc -= 10.0;
        }
        g->animals -= 20.0;
    }
    clamp_supplies(g);
    wait_key("Approach Oregon City");
}

void handle_random_event(GameState *g) {
    double event_roll = 100.0 * rnd();
    if (event_roll <= 6.0) {
        wprintw(w_main, "\nWagon breaks down - lose time and supplies fixing it.\n");
        g->miles -= 15.0 + 5.0 * rnd();
        g->misc -= 8.0;
    } else if (event_roll <= 11.0) {
        wprintw(w_main, "\nOx injures leg - slows you down for the rest of trip.\n");
        g->miles -= 25.0;
        g->animals -= 20.0;
    } else if (event_roll <= 13.0) {
        int victim = get_random_living_member(g);
        wprintw(w_main, "\nBad luck - %s broke an arm!\n", g->party[victim].name);
        g->party[victim].is_injured = 1;
        if (g->party[victim].health < HEALTH_CRITICAL) g->party[victim].health++;
        g->miles -= 5.0 + 4.0 * rnd();
        g->misc -= 2.0 + 3.0 * rnd();
    } else if (event_roll <= 15.0) {
        wprintw(w_main, "\nOx wanders off - spend time looking for it.\n");
        g->miles -= 17.0;
    } else if (event_roll <= 17.0) {
        int victim = get_random_living_member(g);
        wprintw(w_main, "\n%s gets lost in the brush - spend time searching.\n", g->party[victim].name);
        g->miles -= 10.0;
    } else if (event_roll <= 22.0) {
        wprintw(w_main, "\nUnsafe water - time lost looking for clean spring.\n");
        g->miles -= 10.0 * rnd() - 2.0;
    } else if (event_roll <= 32.0) {
        if (g->miles > 950.0) {
            if (g->clothing < 22.0 + 4.0 * rnd()) {
                wprintw(w_main, "\nCold mountain weather and not enough clothing!\n");
                handle_illness(g);
            }
        } else {
            wprintw(w_main, "\nHeavy rains - time and supplies lost.\n");
            g->food -= 40.0;
            double ammo_loss = (g->ammo > 100.0) ? 50.0 : g->ammo * 0.25;
            g->ammo -= ammo_loss;
            g->misc -= 15.0;
            g->miles -= 10.0 * rnd() - 5.0;
        }
    } else if (event_roll <= 35.0) {
        wattron(w_main, COLOR_PAIR(3) | A_BOLD);
        wprintw(w_main, "\nBANDITS ATTACK!\n");
        wattroff(w_main, COLOR_PAIR(3) | A_BOLD);
        if (g->ammo <= 0.0) {
            wprintw(w_main, "Out of ammunition! Bandits ransacked your wagon!\n");
            g->misc -= 15.0;
            g->food -= 50.0;
            g->cash -= 20.0;
        } else {
            double b1 = do_shooting(g);
            g->ammo -= (20.0 * b1 + 10.0);
            clamp_supplies(g);

            if (b1 <= 1.0) {
                wattron(w_main, COLOR_PAIR(4) | A_BOLD);
                wprintw(w_main, "Quickest draw outside of Dodge City! You got 'em!\n");
                wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
            } else if (b1 <= 4.0) {
                wprintw(w_main, "Kinda slow with your Colt .45, but you drove them off!\n");
            } else {
                int victim = get_random_living_member(g);
                wattron(w_main, COLOR_PAIR(3));
                wprintw(w_main, "%s was shot in the leg and bandits took an ox!\n", g->party[victim].name);
                wattroff(w_main, COLOR_PAIR(3));
                g->party[victim].is_injured = 1;
                g->party[victim].health = HEALTH_CRITICAL;
                g->misc -= 5.0;
                g->animals -= 20.0;
            }
        }
    } else if (event_roll <= 37.0) {
        wprintw(w_main, "\nFire in wagon - food and supplies damaged!\n");
        g->food -= 120.0;
        double ammo_loss = (g->ammo > 100.0) ? 60.0 : g->ammo * 0.30;
        g->ammo -= ammo_loss;
        g->misc -= 3.0 + 8.0 * rnd();
        g->miles -= 15.0;
    } else if (event_roll <= 42.0) {
        wprintw(w_main, "\nLost in heavy fog - time lost.\n");
        g->miles -= 10.0 + 5.0 * rnd();
    } else if (event_roll <= 44.0) {
        int victim = get_random_living_member(g);
        wprintw(w_main, "\nPoisonous prairie rattlesnake bit %s!\n", g->party[victim].name);
        g->ammo -= 10.0;
        if (g->misc < 2.0) {
            wattron(w_main, COLOR_PAIR(3));
            wprintw(w_main, "No medicine available! %s succumbed to the venom!\n", g->party[victim].name);
            wattroff(w_main, COLOR_PAIR(3));
            g->party[victim].health = HEALTH_DEAD;
            check_party_vital_status(g);
        } else {
            g->misc -= 5.0;
            wprintw(w_main, "Emergency antivenom and treatment administered.\n");
            if (g->party[victim].health < HEALTH_POOR) g->party[victim].health++;
        }
        clamp_supplies(g);
    } else if (event_roll <= 54.0) {
        wprintw(w_main, "\nWagon swamped fording creek - lost flour and clothes.\n");
        g->food -= 100.0;
        g->clothing -= 20.0;
        g->miles -= 20.0 + 20.0 * rnd();
    } else if (event_roll <= 64.0) {
        wattron(w_main, COLOR_PAIR(3));
        wprintw(w_main, "\nWILD ANIMALS ATTACK!\n");
        wattroff(w_main, COLOR_PAIR(3));
        double b1 = do_shooting(g);

        if (b1 <= 1.0) {
            wattron(w_main, COLOR_PAIR(4) | A_BOLD);
            wprintw(w_main, "Dead eye! One shot dropped the lead beast!\n");
            wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
            g->ammo -= 10.0;
        } else if (b1 <= 3.0) {
            wprintw(w_main, "Nice shooting pardner - drove them off without damage.\n");
            g->ammo -= (15.0 + 5.0 * b1);
        } else {
            wprintw(w_main, "Slow on the draw - wolves tore through the provision sacks!\n");
            g->clothing -= b1 * 4.0;
            g->food -= b1 * 35.0;
            g->ammo -= (20.0 * b1 + 10.0);
        }
    } else if (event_roll <= 69.0) {
        wprintw(w_main, "\nHail storm - supplies damaged.\n");
        g->miles -= 5.0 + 10.0 * rnd();
        double ammo_loss = (g->ammo > 100.0) ? 40.0 : g->ammo * 0.20;
        g->ammo -= ammo_loss;
        g->misc -= 4.0 + 3.0 * rnd();
    } else if (event_roll <= 95.0) {
        handle_illness(g);
    } else {
        wattron(w_main, COLOR_PAIR(4));
        wprintw(w_main, "\nHelpful Indians show you where to find edible roots & berries (+60 lbs food).\n");
        wattroff(w_main, COLOR_PAIR(4));
        g->food += 60.0;
    }

    if (g->miles > 950.0) {
        if (rnd() * 10.0 > 9.0 - (pow((g->miles / 100.0 - 15.0), 2) + 72.0) / (pow((g->miles / 100.0 - 15.0), 2) + 12.0)) {
            wprintw(w_main, "\nRugged mountain terrain slows the trail.\n");
            g->miles -= 45.0 + rnd() / 0.02;
        }

        if (!g->south_pass_flag && g->miles >= 950.0) {
            g->south_pass_flag = 1;
            if (rnd() < 0.80) {
                wattron(w_main, COLOR_PAIR(4) | A_BOLD);
                wprintw(w_main, "\nYOU MADE IT SAFELY THROUGH SOUTH PASS--NO SNOW!\n");
                wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
            } else {
                wprintw(w_main, "\nBlizzard strikes as you cross South Pass!\n");
                g->food -= 80.0;
                g->misc -= 10.0;
                g->ammo -= 50.0;
                g->miles -= 30.0 + 40.0 * rnd();
                if (g->clothing < 26.0 + 4.0 * rnd()) {
                    wprintw(w_main, "Not enough warm clothes for this mountain blizzard!\n");
                    handle_illness(g);
                }
            }
        }

        if (!g->blue_mtn_flag && g->miles >= 1700.0) {
            g->blue_mtn_flag = 1;
            if (rnd() < 0.70) {
                wattron(w_main, COLOR_PAIR(4) | A_BOLD);
                wprintw(w_main, "\nCLEARED THE BLUE MOUNTAINS SAFELY!\n");
                wattroff(w_main, COLOR_PAIR(4) | A_BOLD);
            } else {
                wprintw(w_main, "\nEarly blizzard in the Blue Mountains!\n");
                g->food -= 80.0;
                g->misc -= 10.0;
                g->ammo -= 50.0;
                g->miles -= 30.0 + 40.0 * rnd();
                if (g->clothing < 26.0 + 4.0 * rnd()) {
                    wprintw(w_main, "Freezing mountain blizzard causes sickness!\n");
                    handle_illness(g);
                }
            }
        }
    }
}