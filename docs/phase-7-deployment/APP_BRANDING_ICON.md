# Custom Moderated LMS application icon

The user-supplied black-and-blue orb-and-ring emblem is now the LMS application icon. The art was **not redrawn**. Only blank background margins were cropped before creating a multi-resolution Windows ICO (16, 32 and 48 px).

- **Icon asset:** [ModeratedLMS_app_icon.ico](../../assets/ModeratedLMS_app_icon.ico)
- **Qt window/taskbar icon:** embedded by `qt_add_resources` and loaded by `src/main.cpp` with `QIcon(":/assets/ModeratedLMS_app_icon.ico")`
- **Windows EXE/Explorer icon:** compiled into `ModeratedLMS.exe` with a generated Windows resource script
- **No changes** to the academic assessment model, database schema, testing rules or user records

## Update and rebuild on the development laptop

First **close the LMS**, then in **MSYS2 UCRT64**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase8-cr1-class-extensions
git pull --ff-only origin feature/phase8-cr1-class-extensions

cmake -S . -B /c/dev/cs4433-lms-v11 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-v11 --target ModeratedLMS
/c/dev/cs4433-lms-v11/ModeratedLMS.exe
```

To prepare a **new** portable Windows Release folder, run `bash scripts/package_windows_ucrt64.sh`. The release script creates a new timestamped folder, retaining earlier tested packages. If the shell shows Qt DLL errors in the development build, launch from MSYS2 UCRT64.

**Release provenance:** this is a cosmetic update on the feature branch only. The already published `v1.0` and `v1.1` GitHub pre-release ZIPs and their tags still contain the earlier icon-less binaries. Do not claim those archives have the new icon. Do not reassign an existing version tag to this later commit.

If the taskbar icon remains cached, close old LMS instances and restart Windows Explorer or reopen the rebuilt EXE; do not delete application data.
