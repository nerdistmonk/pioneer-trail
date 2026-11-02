#ifndef EVENTS_H
#define EVENTS_H

#include "common.h"

double do_shooting(GameState *g);
void do_hunt(GameState *g);
void handle_illness(GameState *g);
void handle_riders(GameState *g);
void handle_random_event(GameState *g);
void handle_landmarks_and_rivers(GameState *g);
void handle_finale_columbia_river(GameState *g);

#endif /* EVENTS_H */