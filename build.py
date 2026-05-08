import os
import glob
import shutil
import platform

debug = True

IS_WINDOWS = platform.system() == "Windows"
IS_LINUX = platform.system() == "Linux"

if IS_WINDOWS:
    if not debug:
        DEBUG_FLAGS = " -Wl,-subsystem,windows"
    else:
        DEBUG_FLAGS = " -g"
    EXE_EXT = ".exe"
    STATIC_FLAGS = "-static-libgcc -static-libstdc++"
    LIBS = "-lgdi32 -lopengl32 -lglfw3 -lfreetype -llua54 -lopenal32"
else:
    if not debug:
        DEBUG_FLAGS = " -O2"
    else:
        DEBUG_FLAGS = " -g"
    EXE_EXT = ""
    STATIC_FLAGS = ""
    LIBS = "-lglfw -lfreetype -llua5.4 -lopenal -lGL -lpthread -ldl"

COMMON_INCLUDES = "-I./include -I./core"
COMMON_LIBS = f"-L./lib {LIBS}"
COMMON_STATIC = STATIC_FLAGS

def make_project():
    os.makedirs("bin", exist_ok=True)
    
    if os.path.exists("lib"):
        for filename in os.listdir("lib"):
            if IS_WINDOWS and filename.endswith(".dll"):
                src = os.path.join("lib", filename)
                dst = os.path.join("bin", filename)
                shutil.copy2(src, dst)
                print(f"Скопирован: {filename}")
            elif IS_LINUX and filename.endswith(".so"):
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
                if os.path.exists(dst):
                    shutil.rmtree(dst)
                shutil.copytree(src, dst)
                print(f"Скопирована папка: {item}")
    else:
        print(f"Предупреждение: папка {assets_path} не существует")

def find_cpp_files(directory):
    cpp_files = []
    if not os.path.exists(directory):
        return cpp_files
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp'):
                cpp_files.append(os.path.join(root, file))
    return cpp_files

def compile_game():
    core_cpp_files = find_cpp_files('./core')
    glad_file = "include/glad/glad.c" if os.path.exists("include/glad/glad.c") else ""
    game_files = f"src/game.cpp {glad_file} " + " ".join(core_cpp_files)
    
    output_name = f"bin/game{EXE_EXT}"
    
    if IS_WINDOWS and not debug:
        subsystem_flag = "-Wl,-subsystem,windows"
    else:
        subsystem_flag = ""
    
    cmd = (
        f"g++{DEBUG_FLAGS} {game_files} "
        f"-O2 "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-o {output_name} "
        f"{subsystem_flag} "
        f"{COMMON_STATIC}"
    )
    
    print("\nCompiling game...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    print(f"\nCommand: {cmd}\n")
    
    result = os.system(cmd)
    if result == 0:
        print(f"✅ Successfully compiled to {output_name}")
    else:
        print(f"❌ Compilation failed with error code {result}")
    return result

def compile_release():
    core_cpp_files = find_cpp_files('./core')
    glad_file = "include/glad/glad.c" if os.path.exists("include/glad/glad.c") else ""
    game_files = f"src/game.cpp {glad_file} " + " ".join(core_cpp_files)
    
    output_name = f"bin/release{EXE_EXT}"
    
    if IS_WINDOWS:
        release_flags = "-O3 -Wl,-subsystem,windows"
    else:
        release_flags = "-O3"
    
    cmd = (
        f"g++ {release_flags} {game_files} "
        f"{COMMON_INCLUDES} "
        f"{COMMON_LIBS} "
        f"-o {output_name} "
        f"{COMMON_STATIC}"
    )
    
    print("\nCompiling release version...")
    print(f"Found cpp files: {len(core_cpp_files)}")
    for f in core_cpp_files:
        print(f"  {f}")
    print(f"\nCommand: {cmd}\n")
    
    result = os.system(cmd)
    if result == 0:
        print(f"✅ Successfully compiled to {output_name}")
    else:
        print(f"❌ Compilation failed with error code {result}")
    return result

def print_help():
    print("""
Доступные команды:
  python build.py        - Полная сборка (копирование ассетов + компиляция)
  python build.py game   - Только компиляция game
  python build.py release- Только компиляция release
  python build.py assets  - Только копирование ассетов
  python build.py clean   - Очистка bin директории
    """)

def clean():
    if os.path.exists("bin"):
        shutil.rmtree("bin")
        print("✅ Очищена папка bin")
    else:
        print("Папка bin не существует")

if __name__ == '__main__':
    import sys
    
    print(f"🖥️  Операционная система: {platform.system()}")
    
    if len(sys.argv) > 1:
        command = sys.argv[1].lower()
        if command == "game":
            compile_game()
        elif command == "release":
            compile_release()
        elif command == "assets":
            make_project()
        elif command == "clean":
            clean()
        else:
            print_help()
    else:
        make_project()
        compile_game()
        compile_release()