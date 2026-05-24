import os
import glob
import shutil

debug = True

if not debug:
    DEBUG_FLAGS = " -Wl,-subsystem,windows"
else:
    DEBUG_FLAGS = " -g"

COMMON_INCLUDES = "-I./libs/freetype/include -I./libs/glad/include -I./libs/glfw/include -I./libs/headeronly/include -I./libs/lua/lua-5.4.7/include -I./libs/openal/include -I./core"
COMMON_LIBS = "-L./lib -lopengl32 -lglfw3 -lfreetype -lgdi32"
COMMON_STATIC = "-static-libgcc -static-libstdc++"

def make_project():
    os.makedirs("bin", exist_ok=True)
    
    for filename in os.listdir("lib"):
        if filename.endswith((".dll", ".so")):
            src = os.path.join("lib", filename)
            dst = os.path.join("bin", filename)
            shutil.copy2(src, dst)
            print(f"Скопирован: {filename}")
    
    assets_path = "assets"
    bin_path = "bin"
    
    if os.path.exists(assets_path):
        for item in os.listdir(assets_path):
            src = os.path.join(assets_path, item)
            dst = os.path.join(bin_path, item)
            
            if os.path.isfile(src):
                shutil.copy2(src, dst)
                print(f"Скопирован файл: {item}")
            elif os.path.isdir(src):
                shutil.copytree(src, dst, dirs_exist_ok=True)
                print(f"Скопирована папка: {item}")
    else:
        print(f"Предупреждение: папка {assets_path} не существует")

def find_cpp_files(directory):
    cpp_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp'):
                cpp_files.append(os.path.join(root, file).replace('\\', '/'))
    return cpp_files

def compile_game():
    core_cpp_files = find_cpp_files('./core')
    game_files = "src/game.cpp libs/glad/src/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 -Oz -s "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-llua54 -lopenal32 "
        f"-o bin/game.exe "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

def compile_release():
    core_cpp_files = find_cpp_files('./core')
    game_files = "src/game.cpp libs/glad/src/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 -Oz -s "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-llua54 -lopenal32 "
        f"-o bin/release.exe "
        f"-Wl,-subsystem,windows "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

if __name__ == '__main__':
    make_project()
    compile_game()
    compile_release()
