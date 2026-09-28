# Graphics API

Graphics functions are available after adding `use DLC:graphics` to a CuffScript file. Coordinates are in pixels, with the origin at the top-left of the window. Color values use RGB components from 0 to 255.

## Window and frame

- `window(width, height, title)` creates the window and initializes audio. Call it before drawing or using window input.
- `should_close()` returns true after a quit event has been processed.
- `poll()` processes events, updates mouse state, and records the time since the previous poll.
- `dt()` returns that elapsed time in seconds.
- `present()` displays the completed frame.

The runtime polls and presents each frame. Define optional `on_load()`, `on_update(dt)`, and `on_draw()` functions for setup, per-frame updates, and drawing. Call `window(...)` during setup, commonly from `on_load()`. The runtime calls `poll()` before the callbacks and `present()` after drawing.

## Drawing

- `clear(r, g, b)` fills the window with a color.
- `rect(x, y, width, height, r, g, b)` draws a filled rectangle.
- `line(x1, y1, x2, y2, r, g, b)` draws a line between two points.
- `circle(center_x, center_y, radius, r, g, b)` draws a filled circle.

## Images and sprites

- `image_load(path)` loads an image and returns its numeric ID.
- `image_draw(id, x, y)` draws the image at its original size.
- `image_width(id)` and `image_height(id)` return its dimensions in pixels.
- `image_draw_ex(id, x, y, scale, rotation)` draws a scaled and rotated image. Rotation is in degrees.
- `sprite_draw(id, source_x, source_y, source_width, source_height, x, y, scale, rotation)` draws a source rectangle from an image as a scaled and rotated sprite.

For web builds, image files must be available in the virtual filesystem. The current build preloads the demo script at `/demo.cuff`; add preload options for any other assets your script uses.

## Input

- `key_down(name)` returns whether the named keyboard key is held.
- `mouse_x()` and `mouse_y()` return the current mouse position in window coordinates.
- `mouse_down(button)` returns whether a mouse button is held. SDL button numbers are `1` for left, `2` for middle, and `3` for right.

Input state is updated by `poll()`.

## Collision

These helpers return a boolean and do not need a window:

- `rect_overlap(x1, y1, width1, height1, x2, y2, width2, height2)` checks whether two rectangles overlap. Touching edges do not count as overlap.
- `circle_overlap(x1, y1, radius1, x2, y2, radius2)` checks whether two circles overlap. Touching edges count as overlap.
- `point_in_rect(point_x, point_y, x, y, width, height)` checks whether a point is inside or on the edge of a rectangle.

## Audio

- `sound_load(path)` loads a sound effect and returns its numeric ID.
- `sound_play(id)` plays a loaded sound effect once.
- `music_play(path)` loads and plays music in a loop, replacing any current music.
- `music_stop()` stops the current music.

## Build a desktop executable

The repository does not currently provide a Make target for the native graphics executable. Build `graphics/desktop_main.cpp` directly and define `CUFF_ENABLE_GRAPHICS` to enable the graphics DLC.

### Linux with GCC or Clang

Install a C++ compiler, `pkg-config`, and SDL2 development packages. On Debian or Ubuntu:

```sh
sudo apt install build-essential clang pkg-config libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev
```

Build with GCC:

```sh
g++ -std=c++17 -O2 -DCUFF_ENABLE_GRAPHICS graphics/desktop_main.cpp -o c2d-graphics $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer)
```

Use `clang++` in place of `g++` to build with Clang. Run the executable with a script path, for example `./c2d-graphics graphics/windows_runtime/demo.cuff`.

### Windows with MinGW-w64 GCC or Clang

Install MSYS2 and use its UCRT64 shell for GCC or CLANG64 shell for Clang. Install the matching compiler, `pkgconf`, and SDL2, SDL2_image, and SDL2_mixer development packages with `pacman`. For example, in UCRT64:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_mixer
```

Then build from that shell:

```sh
g++ -std=c++17 -O2 -DCUFF_ENABLE_GRAPHICS graphics/desktop_main.cpp -o c2d-graphics.exe $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer) -lws2_32
```

For Clang, install the corresponding `mingw-w64-clang-x86_64-*` packages in CLANG64 and use `clang++` instead of `g++`. Keep the SDL runtime DLLs available on `PATH` when running the executable.

### Windows with MSVC

Install the SDL2 ports using vcpkg:

```powershell
vcpkg install sdl2:x64-windows sdl2-image:x64-windows sdl2-mixer:x64-windows
```

From a Visual Studio Developer PowerShell, compile with the vcpkg include and library directories:

```powershell
cl /std:c++17 /EHsc /O2 /DCUFF_ENABLE_GRAPHICS /I"$env:VCPKG_ROOT\installed\x64-windows\include" graphics\desktop_main.cpp /Fe:c2d-graphics.exe /link /LIBPATH:"$env:VCPKG_ROOT\installed\x64-windows\lib" SDL2main.lib SDL2.lib SDL2_image.lib SDL2_mixer.lib ws2_32.lib
```

Keep the vcpkg `x64-windows\bin` directory on `PATH` when running the executable.

## Build and run for the web

Install and activate the Emscripten SDK, then make sure `em++` is available in the current shell. From the repository root, build the web demo:

```sh
make web
```

The build writes `c2d.js`, `c2d.wasm`, and `c2d.data` to `web/`. Serve the page over HTTP rather than opening the HTML file directly:

```sh
make web-serve
```

Open `http://localhost:8000` in a browser.

## Run the browser test

Install the Node dependencies and Chromium for Playwright:

```sh
npm install
npx playwright install --with-deps chromium
```

Then run:

```sh
make SHELL=/bin/bash web-test
```

The test builds the web demo, starts a local server, and checks the page output and a canvas pixel. In this dev container, setting `SHELL=/bin/bash` avoids a permission error when Make runs Node through its default shell.