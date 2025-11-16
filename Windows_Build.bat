:: Clone raylib
::      Uncomment this line to get raylib, then comment it out
:: git clone https://github.com/raysan5/raylib.git external/raylib

:: Clone raygui
::      Uncomment this line to get raygui, then comment it out
:: git clone https://github.com/raysan5/raygui.git external/raygui

:: Build
mkdir build
cd build
cmake ..
cmake --build .