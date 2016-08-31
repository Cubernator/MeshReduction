"MeshReduction" - A tool for mesh simplification.
Created by Dennis Heinze
at Trier University of Applied Sciences

Project page on GitHub: https://github.com/Cubernator/MeshReduction


====== BUILD INSTRUCTIONS ======

This application was developed and tested using Qt 5.6.1 and MinGW32 4.9 on Microsoft Windows
Make sure Qt is properly configured to use the Mingw compiler!

-- USING THE COMMAND LINE --

1. Create a build directory outside the project tree
2. Change the working directory to this build directory
3. Call "qmake <path-to-project-dir>\MeshReduction\MeshReduction.pro -spec win32-g++"
4. Call "<path-to-mingw>\bin\mingw32-make"

See the instructions below if you want to run the built executable


-- USING QT CREATOR (GUI) --

1. Open the "MeshReduction.pro" project file with Qt Creator
2. On the "Configure Project" screen, make sure only the Kit "Desktop Qt 5.6.1 MinGW 32bit" is checked
3. Click "Configure Project"
4. Select Project Configuration in the bottom left (Debug/Release)
5. Press "Ctrl+R" or click "Run"


Note that if you build this project using Qt Creator, it should be able to run the executable out of the box
(but only if it is launched using Qt Creator's GUI).
If you want to use the built executable on it's own, you have to copy all necessary DLLs to the output directory first.
These are:

- Qt5Core.dll
- Qt5Gui.dll
- Qt5Widgets.dll
- libgcc_s_dw2-1.dll
- libstdc++-6.dll
- libwinpthread-1.dll

which can be found in Qt's bin directory (Use the corresponding debug variants of the DLLs for debug builds).
Additionally, from the "<project-dir>\MeshReduction\bin" directory, copy these as well:

- libassimp.dll
- libzlib.dll

In case the executable still wont run, a pre-built binary can be found on the download section on GitHub.