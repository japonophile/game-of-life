# The Game of Life written in C89

C89 implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

## Overview

Just for fun, experiment with Conway's Game of Life, and try to understand the meaning of the Universe.

![Conway's Game of Life](/game-of-life.gif)

See a **live demo** [here](https://japonophile.github.io/game-of-life/). This is a link to the Clojurescript demo (TODO: add capture of the C89 demo)

## Why C89?

I saw Eskil Steenberg's talk at [BSC](https://bettersoftwareconference.com/) ["You should finish your software"](https://www.youtube.com/watch?v=EGLoKbBn-VI), and thought it would be fun to write a program in C89.

## How to Use

Build the program and run it on the command line, by specifying the grid size.
```
make
./gol 20 20
```

## Development environment setup

You need make and a C89 compiler, which should exist in almost any environment.  However, this code was developed on Mac OS and probably does not work on Windows (due to specificities of I/O among other).

## License

Copyright © 2020-2025 Antoine Choppin

Distributed under the Eclipse Public License either version 2.0 or (at your option) any later version.
