# The Game of Life in Clojurescript

Clojurescript implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).

## Overview

Just for fun, experiment with Conway's Game of Life, and try to understand the meaning of the Universe.

![Conway's Game of Life](/game-of-life.gif)

See a **live demo** [here](https://japonophile.github.io/game-of-life/).

## How to Use

You can create your own initial configuration by clicking on the canvas to toggle cells on or off, then press the "play" button and see what happens.
You can pause the simulation and go back in time to check what happened.

## Development environment setup

To get an interactive development environment run:

    npm install
    lein figwheel

and open your browser at [localhost:3449](http://localhost:3449/).
This will auto compile and send all changes to the browser without the
need to reload. After the compilation process is complete, you will
get a Browser Connected REPL. An easy way to try it is:

    (js/alert "Am I connected?")

and you should see an alert in the browser window.

To clean all compiled files:

    lein clean

To create a production build run:

    lein do clean, cljsbuild once min

And open your browser in `resources/public/index.html`. You will not
get live reloading, nor a REPL. 

## License

Copyright © 2020-2025 Antoine Choppin

Distributed under the Eclipse Public License either version 2.0 or (at your option) any later version.
