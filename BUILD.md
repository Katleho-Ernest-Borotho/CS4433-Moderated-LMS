# Build instructions

Development uses MSYS2 UCRT64 with C++17, Qt 6, SQLite, libsodium, CMake, Ninja and GoogleTest.

Run the following commands from the repository folder in MSYS2 UCRT64:

    cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake --build /c/dev/cs4433-lms-build
    ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
    /c/dev/cs4433-lms-build/ModeratedLMS.exe

Qt Creator can also open CMakeLists.txt with the CS4433 Qt 6 UCRT64 kit. Use a build directory outside the OneDrive folder.

Current state: the desktop window, four user classes, password hashing wrapper, SQLite connection wrapper and initial unit tests are present. Login and the main LMS features are still to be implemented.
