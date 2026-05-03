import os
import glob

debug = True

if not debug:
    DEBUG_FLAGS = " -Wl,-subsystem,windows"
else:
    DEBUG_FLAGS = " -g"

COMMON_INCLUDES = "-I./include -I./core"
COMMON_LIBS = "-L./lib -lgdi32 -lopengl32 -lglfw3 -lfreetype"
COMMON_STATIC = "-static-libgcc -static-libstdc++"

def find_cpp_files(directory):
    cpp_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp'):
                cpp_files.append(os.path.join(root, file).replace('\\', '/'))
    return cpp_files

def compile_game():
    core_cpp_files = find_cpp_files('./core')
    game_files = "src/game.cpp include/glad/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-llua54 -lopenal32 "
        f"-o bin/versions/game.exe "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

def compile_release():
    core_cpp_files = find_cpp_files('./core')
    game_files = "src/game.cpp include/glad/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-llua54 -lopenal32 "
        f"-o bin/versions/release.exe "
        f"-Wl,-subsystem,windows "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

compile_game()
compile_release()
