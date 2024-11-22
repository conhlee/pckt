# pckt
A command-line tool for packing and unpacking Godot PCK archives.  
Use it to extract files from `.pck` archives or create new `.pck` files from a directory.

## Usage
`pckt <unpack|pack> <input pck/dir> <output pck/dir>`


### Examples
- Extract files from `Game.pck` into the `game_files` directory:
  ```bash
  pckt unpack ./Game.pck ./game_files
  ```
- Create `Game.pck` from the contents of the `game_files` directory:
  ```bash
  pckt pack ./game_files ./Game.pck
  ```