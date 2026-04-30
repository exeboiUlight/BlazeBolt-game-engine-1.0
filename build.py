import os

debug = True

if not debug:
    DEBUG_FLAGS = " -Wl,-subsystem,windows"
else:
    DEBUG_FLAGS = " -g"

def compile_game():
    game_files = [
        "src/game.cpp",
        "include/glad/glad.c"
    ]
    game_files_str = " ".join(game_files)
    compile_game_cmd = (
        f"g++{DEBUG_FLAGS} {game_files_str} "
        f"-O2 "
        f"-I./include "
        f"-I./core "
        f"-L./lib "
        f"-llua54 "
        f"-lgdi32 "
        f"-lopengl32 "
        f"-lglfw3 "
        f"-lopenal32 "
        f"-lfreetype "
        f"-o bin/versions/game.exe "
        f"-static-libgcc "
        f"-static-libstdc++"
    )
    print("\nCompiling game...")
    os.system(compile_game_cmd)

compile_game()