# Pioneer Trail (1847) — Expedition TUI Edition

A robust, modern terminal-based recreation of the classic 1847 covered wagon simulation, written in C11 using `ncurses`.

Guide your pioneer expedition across 2,040 grueling miles from Independence, Missouri to Oregon City's Willamette Valley. Manage party health, balance rations and travel pace, trade at frontier posts, ford treacherous rivers, survive mountain blizzards, and defend against bandits and predators.

---

## Features

- **Full Terminal User Interface (TUI):** Built on `ncurses` featuring split windows for status headers, live trail statistics, interactive commands, and ASCII art displays.
- **Dynamic ASCII Scene Panel:** Scales automatically for wider terminals (>= 108 columns) to render dynamic route landscapes (Plains, Forts, Mountain Passes, River Crossings, Landmarks) and an expedition roster.
- **Expedition Party Management:**
  - Choose between a custom 5-person expedition, standard pioneer family, or solo trailblazer.
  - Individual status tracking (`Good`, `Fair`, `Poor`, `Critical`, `DEAD`) with conditions for illnesses and injuries.
  - Proportional resource consumption—ration costs dynamically scale based on surviving members.
- **Authentic 1847 Route Mechanics:**
  - Historic landmarks and waypoints: Kansas River, Fort Kearny, Chimney Rock, Fort Laramie, Independence Rock, the Sublette Cutoff, Green River, Fort Hall, Snake River, and Fort Boise.
  - High-stakes finale at The Dalles: navigate the Columbia River rapids or pay the toll on Samuel Barlow's Mt. Hood road.
- **Interactive Mini-Games & Hazards:**
  - Timed rifle combat minigame for hunting bison and antelope or driving off hostile riders and predators (adjusted for marksman skill level).
  - Depth-based river crossings: ford shallow waters, caulk and float, or hire a local ferry/guide.
- **Persistent Leaderboard:** Saves high scores and trek outcomes to XDG-compliant storage (`~/.local/share/pioneer-trail/highscores.txt`).

---

## Requirements

- **Compiler:** GCC with C11 support (`-std=c11`)
- **Build System:** CMake 3.12 or newer
- **Libraries:** `ncurses` (including development headers, e.g., `libncurses-dev` on Debian/Ubuntu or `ncurses-devel` on Fedora/RHEL)
- **Math Library:** `libm` (handled automatically by CMake)

---

## Building and Installation

Just run 'build_release' and the binaries will be deposited into ".build"

---

## How to Play

### Setup & Equipment
1. **Shooting Skill:** Select your rifle proficiency (Ace Marksman down to Shaky Knees), which dictates your typing reaction window during shootouts and hunts.
2. **Expedition Composition:** Choose between a custom party, a default family, or a solo trek.
3. **Outfitting:** Spend your starting savings wisely at Independence General Outfitters on oxen, provisions (flour, bacon, beans), ammunition, warm clothing, and emergency medicine/misc supplies.

### On the Trail
- **Pace:** Choose between **Steady**, **Strenuous** (burns extra stamina/oxen health), or **Grueling** (fastest progress, but dramatically increases illness risks).
- **Rations:** Adjust rations to **Well**, **Moderate**, or **Poor**. Skimping preserves food but weakens the party against disease and injury.
- **Actions:** When available, stop at frontier trading posts to buy supplies, spend a day hunting for game, or push ahead.
- **Vital Status:** Take care of the wagon leader. If the expedition leader dies or all members perish, the journey ends.

---

## Project Structure

```text
.
├── CMakeLists.txt          # Build configuration and compiler flags
├── README.md               # Project documentation
├── COPYING                 # GNU General Public License v2 text
└── src/
    ├── common.h            # Core data types (GameState, PartyMember) and constants
    ├── tui.h / tui.c       # ncurses initialization, layout management, HUD, and art
    ├── setup.h / setup.c   # Character creation, party selection, and outfitting store
    ├── events.h / events.c # River crossings, landmark encounters, combat, and hunting
    ├── scores.h / scores.c # XDG high score file persistence and leaderboard display
    └── main.c              # Game loop, turn progression, and win/loss states
```

---

## Legal & Disclaimers

```text
Pioneer Trail is an independent, open-source educational simulation inspired 
by historical 19th-century accounts of American westward migration and classic 
1970s mainframe educational software (originally created by Don Rawitsch, Paul 
Dillenberger, and Bill Heinemann, published in Creative Computing).

All C source code, terminal user interface architecture, and ASCII art 
in this repository are original implementations. This project is not 
affiliated with, sponsored by, or endorsed by Houghton Mifflin Harcourt, MECC, 
or any owners of "The Oregon Trail" trademark.
```

---

## License

This program is free software; you can redistribute it and/or modify it under the terms of the **GNU General Public License version 2** as published by the Free Software Foundation.

See the [COPYING](license.md) file for full license details.
