import os

debug = True

if not debug:
    DEBUG = " -Wl,-subsystem,windows"
else:
    DEBUG=""

# compile editor
os.system(f"g++{DEBUG} -O2 src/editor.cpp include/glad/glad.c -I./include -I./core -L./lib -llibtcc -lgdi32 -lopengl32 -lglfw3 -lopenal32 -lfreetype -o bin/editor.exe -static-libgcc -static-libstdc++")
