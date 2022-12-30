# Another image viewer
An image viewer implementation using C++17 and SDL2. It is heavily inspired on sxiv.

## Features
<div>
<img src="image0.png" width="50%" height="50%" alt="Grid view mode">
<p> Grid view mode</p>
</div>

<div>
<img src="image1.png" width="50%" height="50%" alt="Image view mode">
<p> Image view mode</p>
</div>

<div>
<img src="image2.gif" width="50%" height="50%" alt="Image continuum view mode" animated>
<p> Image continuum view mode </p>
</div>

- Keyboard input only. Vim-like commands.
- Rendering of gif animations.
- Continuum view mode. You are able to scroll from top to bottom to see the images in a continuum way.
- When the program is closed, it will resume to the same image position when opened again with similar arguments. The way it works is that when you close the image viewer, is saves the path of the current image to a cache file. When you open it again and one of the input filenames matches a string in the cache, it moves the cursor to that image.
- Option to write to standard output the current image filename on exit, so it is posible to use this program in a way similar to dmenu, but for images.


## Usage
The command is "aiv". You can insert as command line arguments the filenames or directories for it to search images. You can also input the filenames or directories by pipeline.

## Requirements to compile
clang 14.00+, meson, Make, Linux system.

## Keyboard input:
### General:
- f: toggle fullscreen
- n: next image
- "SPACE": next image
- p: previous image
- g: go to first image
- G: go to last image
- "ENTER": Toggle between grid view and image view
- b: toggle bottom bar information
- q: exit the image viewer
	
### Grid image mode
- +: Zoom up grid
- -: Zoom down grid
- j: move down
- k: move up
- h: move left
- l: move right
### Image viewer mode
- +: Zoom up image
- -: Zoom down image
- j: move down
- k: move up
- h: move left
- l: move right
- e: fit image to the width of the window
- E: fit image to the height of the window
- <: rotate left
- \>: rotate right
- c: Toggle between single image view to continuum view of multiple images in a vertical line from top to bottom.
	
## Commands
- make: builds the project
- make install: builds and copies the executable to $(HOME)/.local/bin/aiv
- make test: builds and executes the tests

## TODO
- Change "cacheFilenames.hpp" so it also works on windows.
- Be able to insert system commands inside the image viewer. It would allow to perform arbitrary operations on selected images.
- Key bindings so the user is able to execute arbitrary commands.
