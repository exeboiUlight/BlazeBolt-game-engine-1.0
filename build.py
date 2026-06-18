import os
import glob
import shutil
import sys

target = None

# Parse command line arguments
for arg in sys.argv[1:]:
    if arg == "--release":
        debug = False
    elif arg == "--linux":
        target = "linux"
    elif arg == "--windows":
        target = "windows"
    elif arg.startswith("--target="):
        target = arg.split("=", 1)[1]

# Auto-detect platform if not specified
if target is None:
    target = "linux" if sys.platform == "linux" else "windows"

# Platform-specific settings
if target == "linux":
    DEBUG_FLAGS1 = DEBUG_FLAGS2 = " -g"
    COMMON_INCLUDES = "-I./include -I./core"
    COMMON_LIBS = "-L./lib -lGL -lglfw -lfreetype -llua5.4 -lopenal"
    COMMON_STATIC = "-static-libgcc -static-libstdc++ -flto -ffunction-sections -fdata-sections -Wl,--gc-sections"
    EXT = ""
    SIZE_OPT = "-Os"
else:
    DEBUG_FLAGS1 = " -g"
    DEBUG_FLAGS2 = " -Wl,-subsystem,windows"
    COMMON_INCLUDES = "-I./include -I./core"
    COMMON_LIBS = "-L./lib -lopengl32 -lglfw3 -lfreetype -lgdi32 -llua54 -lopenal32 -lws2_32"
    COMMON_STATIC = "-static-libgcc -static-libstdc++ -flto -ffunction-sections -fdata-sections -Wl,--gc-sections"
    EXT = ".exe"
    SIZE_OPT = "-Oz"

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
    game_files = "src/game.cpp src/stb_image_impl.cpp include/glad/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS1} {game_files} "
        f"-O2 {SIZE_OPT} -s "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-o bin/game{EXT} "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

def compile_release():
    core_cpp_files = find_cpp_files('./core')
    game_files = "src/game.cpp src/stb_image_impl.cpp include/glad/glad.c " + " ".join(core_cpp_files)
    cmd = (
        f"g++{DEBUG_FLAGS2} {game_files} "
        f"-O2 {SIZE_OPT} -s "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-o bin/release{EXT} "
        f"{COMMON_STATIC}"
    )
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    os.system(cmd)

def compile_cs_lib():
    if target == "windows":
        core_cpp_files = find_cpp_files('./core')
        game_files = "include/glad/glad.c src/stb_image_impl.cpp " + " ".join(core_cpp_files)
        cmd = (
            f"g++{DEBUG_FLAGS2} {game_files} -shared "
            f"-O2 {SIZE_OPT} -s "
            f"{COMMON_INCLUDES} "
            f"{COMMON_LIBS} "
            f"-o bin/BlazeBolt.dll "
            f"{COMMON_STATIC}"
        )
        print("\nCompiling dll file...")
        print(f"Found cpp files: {len(core_cpp_files)}")
        for f in core_cpp_files:
            print(f"  {f}")
        os.system(cmd)
    
    if target == "linux":
        core_cpp_files = find_cpp_files('./core')
        game_files = "include/glad/glad.c src/stb_image_impl.cpp " + " ".join(core_cpp_files)
        cmd = (
            f"g++ -fPIC{DEBUG_FLAGS2} {game_files} -shared "
            f"-O2 {SIZE_OPT} -s "
            f"{COMMON_INCLUDES} "
            f"{COMMON_LIBS} "
            f"-o bin/BlazeBolt.so "
            f"{COMMON_STATIC}"
        )
        print("\nCompiling dll file...")
        print(f"Found cpp files: {len(core_cpp_files)}")
        for f in core_cpp_files:
            print(f"  {f}")
        os.system(cmd)

if __name__ == '__main__':
    make_project()
    compile_game()
    compile_release()
    # compile_cs_lib()
