#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <ncurses.h>

#define TOTAL_TRAIL_MILES 2040.0
#define MAX_SCORES 10
#define MAX_PARTY 5

typedef enum {
    HEALTH_GOOD = 0,
    HEALTH_FAIR,
    HEALTH_POOR,
    HEALTH_CRITICAL,
    HEALTH_DEAD
} MemberHealth;

typedef struct {
    char name[16];
    MemberHealth health;
    int is_sick;
    int is_injured;
} PartyMember;

typedef enum {
    PACE_STEADY = 1,
    PACE_STRENUOUS = 2,
    PACE_GRUELING = 3
} TravelPace;

typedef enum {
    SCENE_PLAINS = 0,
    SCENE_RIVER,
    SCENE_FORT,
    SCENE_LANDMARK,
    SCENE_MOUNTAINS,
    SCENE_VICTORY,
    SCENE_DEATH
} ArtScene;

typedef struct {
    double animals;       /* $ spent on oxen / animal health & speed */
    double ammo;          /* Bullet count */
    double clothing;      /* $ clothing */
    double misc;          /* $ misc supplies / medicine */
    double cash;          /* Cash on hand */
    double food;          /* Food in lbs */
    double miles;         /* Total miles traveled */
    double prev_miles;    /* Miles up to previous turn */
    
    int turn;             /* Turn index (0-19) */
    int skill;            /* Shooting skill level (1-5) */
    int eating_choice;    /* Eating level (1: poorly, 2: moderately, 3: well) */
    TravelPace pace;      /* Travel pace */
    int fort_available;   /* Alternating fort presence */
    int south_pass_flag;  /* Cleared South Pass */
    int blue_mtn_flag;    /* Cleared Blue Mountains */

    /* Party status */
    PartyMember party[MAX_PARTY];
    int party_count;

    /* Landmark & finale trigger flags */
    int kansas_river_done;
    int ft_kearny_done;
    int chimney_rock_done;
    int ft_laramie_done;
    int indep_rock_done;
    int green_river_done;
    int ft_hall_done;
    int snake_river_done;
    int ft_boise_done;
    int sublette_taken;
    int columbia_done;
} GameState;

typedef struct {
    char name[32];
    int score;
    int miles;
    char outcome[16];
    char date[16];
} HighScore;

extern WINDOW *w_hdr, *w_hud, *w_main, *w_art, *w_cmd;
extern const char *DATES[];
extern int art_visible;

double rnd(void);
void clamp_supplies(GameState *g);
int calculate_score(const GameState *g, int survived);
int get_living_member_count(const GameState *g);
int get_random_living_member(const GameState *g);
void check_party_vital_status(GameState *g);

#endif /* COMMON_H */