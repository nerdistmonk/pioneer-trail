#include "scores.h"
#include "tui.h"
#include <sys/stat.h>
#include <sys/types.h>

static void get_scores_filepath(char *dest, size_t dest_len) {
    char dir[400];
    const char *xdg_data = getenv("XDG_DATA_HOME");
    if (xdg_data && *xdg_data) {
        snprintf(dir, sizeof(dir), "%s/pioneer-trail", xdg_data);
    } else {
        const char *home = getenv("HOME");
        snprintf(dir, sizeof(dir), "%s/.local/share/pioneer-trail", home ? home : ".");
    }
    mkdir(dir, 0755);
    snprintf(dest, dest_len, "%s/highscores.txt", dir);
}

static int load_high_scores(HighScore scores[MAX_SCORES]) {
    char path[512];
    get_scores_filepath(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    int count = 0;
    while (count < MAX_SCORES && fscanf(f, "%31s %d %d %15s %15s",
           scores[count].name, &scores[count].score, &scores[count].miles,
           scores[count].outcome, scores[count].date) == 5) {
        count++;
    }
    fclose(f);
    return count;
}

static void save_high_scores(const HighScore scores[MAX_SCORES], int count) {
    char path[512];
    get_scores_filepath(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return;

    for (int i = 0; i < count; i++) {
        fprintf(f, "%s %d %d %s %s\n",
                scores[i].name, scores[i].score, scores[i].miles,
                scores[i].outcome, scores[i].date);
    }
    fclose(f);
}

void add_high_score(const char *name, int score, int miles, const char *outcome) {
    HighScore scores[MAX_SCORES + 1];
    int count = load_high_scores(scores);

    HighScore entry;
    snprintf(entry.name, sizeof(entry.name), "%s", name);
    for (char *p = entry.name; *p; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') *p = '_';
    }

    entry.score = score;
    entry.miles = miles;
    snprintf(entry.outcome, sizeof(entry.outcome), "%s", outcome);
    for (char *p = entry.outcome; *p; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') *p = '_';
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(entry.date, sizeof(entry.date), "%Y-%m-%d", t);

    int insert_idx = count;
    for (int i = 0; i < count; i++) {
        if (score > scores[i].score) {
            insert_idx = i;
            break;
        }
    }

    if (insert_idx < MAX_SCORES) {
        for (int i = (count < MAX_SCORES ? count : MAX_SCORES - 1); i > insert_idx; i--) {
            scores[i] = scores[i - 1];
        }
        scores[insert_idx] = entry;
        if (count < MAX_SCORES) count++;
        save_high_scores(scores, count);
    }
}

void show_leaderboard(void) {
    HighScore scores[MAX_SCORES];
    int count = load_high_scores(scores);

    werase(w_main);
    wattron(w_main, COLOR_PAIR(5) | A_BOLD);
    wprintw(w_main, "\n               --- PIONEER TRAIL LEADERBOARD ---\n\n");
    wattroff(w_main, COLOR_PAIR(5) | A_BOLD);

    wprintw(w_main, " %-4s %-16s %-8s %-8s %-10s %-10s\n", "RANK", "NAME", "SCORE", "MILES", "OUTCOME", "DATE");
    wprintw(w_main, " --------------------------------------------------------------\n");

    if (count == 0) {
        wprintw(w_main, " No pioneers have registered a score yet.\n");
    } else {
        for (int i = 0; i < count; i++) {
            char display_name[32];
            memcpy(display_name, scores[i].name, sizeof(display_name));
            display_name[sizeof(display_name) - 1] = '\0';
            for (char *p = display_name; *p; p++) {
                if (*p == '_') *p = ' ';
            }
            wprintw(w_main, " %-4d %-16s %-8d %-8d %-10s %-10s\n",
                    i + 1, display_name, scores[i].score, scores[i].miles,
                    scores[i].outcome, scores[i].date);
        }
    }
    wrefresh(w_main);
    wait_key("Leaderboard");
}