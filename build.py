import os

debug = True

if not debug:
    DEBUG_FLAGS = " -Wl,-subsystem,windows"
else:
    DEBUG_FLAGS = " -g"

COMMON_INCLUDES = "-I./include -I./core"
COMMON_LIBS = "-L./lib -lgdi32 -lopengl32 -lglfw3 -lfreetype"
COMMON_STATIC = "-static-libgcc -static-libstdc++"

def compile_game():
    game_files = "src/game.cpp include/glad/glad.c"
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-llua54 -lopenal32 "
        f"-o bin/versions/game.exe "
        f"-Wl,-subsystem,windows "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    os.system(cmd)

compile_game()
# compile_editor()
