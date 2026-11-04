# Build and test

## Development environment

Build on Windows using **MSYS2 UCRT64**. Required components: GCC with C++17 support, CMake, Ninja, Qt 6 Widgets, SQLite, libsodium, pkg-config and GoogleTest.

From the MSYS2 **UCRT64** terminal:

```bash
pacman -Syu
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-libsodium mingw-w64-ucrt-x86_64-gtest git
```

If the system upgrade requests a restart of the terminal, reopen UCRT64 before proceeding.

## Compile, test and launch

Inside a fresh checkout of this repository:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/ModeratedLMS.exe
```

CMake places `lms_schema.sql` beside the executable in the build folder. The SQLite database and attachments are stored in Windows Local AppData, **not** in the source checkout. Use a separate Windows user profile for test records.

The recorded v1.1 baseline passed **128 GoogleTest/CTest cases**. Always rerun the tests after local changes.

## Portable Windows package

In MSYS2 UCRT64, run:

```bash
bash scripts/package_windows_ucrt64.sh
```

The script creates a timestamped portable Release folder containing the application, standalone Restore tool, schema, Qt plugins, runtime DLLs and provenance metadata. Keep the entire folder intact and test it outside the development environment.

For people who only want to run the LMS, use the [downloadable v1.1 ZIP](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/ModeratedLMS_v1.1_windows_ucrt64.zip) instead.

## Optional CMake settings

| Setting | Purpose |
| --- | --- |
| `-DBUILD_TESTING=OFF` | Build without automated tests |
| `-DLMS_ENABLE_COVERAGE=ON` | Enable GNU code-coverage instrumentation |
| `-DLMS_ENABLE_NFR_BENCHMARK=ON` | Build the isolated 800-account login benchmark |
| `-DLMS_ENABLE_LOCAL_ADMIN_RECOVERY=ON` | Compile the authorised local Administrator recovery utility |

Use `ctest --test-dir build -N` to list the discovered tests.

## Data protection

Do not delete or replace `lms.sqlite3` to upgrade the application. **Create and verify a full backup before any migration or restore.** Stop the LMS before using `ModeratedLMSRestore.exe`. See [backup and restore operations](OPERATIONS_BACKUP_REPORTS.md), the [role manual](docs/phase-7-deployment/ROLE_USER_MANUAL_V11.md), and the [release procedure](docs/RELEASE_GUIDE.md).
