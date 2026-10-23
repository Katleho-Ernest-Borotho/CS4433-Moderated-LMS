# Build instructions

Development uses MSYS2 UCRT64 with C++17, Qt 6, SQLite, libsodium, CMake, Ninja and GoogleTest.

Run these commands from the repository folder in MSYS2 UCRT64:

    cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build /c/dev/cs4433-lms-build
    ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
    /c/dev/cs4433-lms-build/ModeratedLMS.exe

Qt Creator can also open CMakeLists.txt with the CS4433 Qt 6 UCRT64 kit. Use a build directory outside the OneDrive folder.

On its first run, the application creates a SQLite database in the Windows local application-data folder and installs the Week 3 database schema. Later runs use the existing database. No login or academic functions are implemented yet.
