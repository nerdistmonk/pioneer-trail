#ifndef SCORES_H
#define SCORES_H

#include "common.h"

void add_high_score(const char *name, int score, int miles, const char *outcome);
void show_leaderboard(void);

#endif /* SCORES_H */