# The Game of Life

My implementations of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

## Overview

Just for fun, experiment with Conway's Game of Life, and try to understand the meaning of the Universe.

![Conway's Game of Life](/cljs/game-of-life.gif)

See a **live demo** [here](https://japonophile.github.io/game-of-life/).

Note about the starting pattern in the demo.  According to [LifeWiki](https://www.conwaylife.com/w/index.php?title=Block-laying_switch_engine), the pattern should create a single block-laying switch engine: a configuration that leaves behind two-by-two still life blocks as its translates itself across the game's universe.  However, since this simulation environment is finite (really?) the simulation kind of stops after a few hundred steps.

## How it works

I implemented the Game of Life in different languages:
- [Clojurescript](/cljs)
- [C89](/c89) (WIP)

See each subfolder for details about each implementation.

## License

Copyright © 2020-2025 Antoine Choppin

Distributed under the Eclipse Public License either version 2.0 or (at your option) any later version.
