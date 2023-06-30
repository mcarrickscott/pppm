# pppm- The Poor Programmer's Password Manager

PPPM is a stateless password manager, targeted at programmers. Some programming experience required!

PPPM requires the Qt development environment.

For a description of PPPM see doc/poor.pdf

# Build instructions

- Start Qt Creator and create a new project
- Select the default Qt Widgets Application. Use the CMake build system.
- Change the Classname to PassMan
- Do not select Translation File or make a kit section
- Copy all files from src/qt to the project folder. Say Yes to All to allow Qt Creator access to the new files
- Edit the automatically generated CMakeLists.txt file. Add sha3.cpp and sha3.h to the PROJECT_SOURCES
- Build and run the application on your desktop

Follow standard Qt practices to build for your other devices.

