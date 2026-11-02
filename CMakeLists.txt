cmake_minimum_required(VERSION 3.21)
project(ModeratedLMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Separate, opt-in build for measured Phase 5 business-logic coverage.
option(LMS_ENABLE_COVERAGE "Compile tests with GCC coverage instrumentation" OFF)
if(LMS_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Coverage instrumentation requires GCC in MSYS2 UCRT64")
    endif()
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

configure_file(schema/lms_schema.sql ${CMAKE_CURRENT_BINARY_DIR}/lms_schema.sql COPYONLY)

add_library(lms_core
    src/domain/User.cpp
    src/domain/LatePenaltyPolicy.cpp
    src/persistence/SqliteDatabase.cpp
    src/persistence/BackupManager.cpp
    src/persistence/BackupRestore.cpp
    src/persistence/SQLiteRecordRepository.cpp
    src/persistence/SQLiteAcademicRepository.cpp
    src/persistence/SQLiteAssignmentsRepository.cpp
    src/persistence/SQLiteSubmissionRepository.cpp
    src/persistence/SQLiteAssessmentRepository.cpp
    src/persistence/SQLiteModerationRepository.cpp
    src/persistence/LocalSubmissionContentStore.cpp
    src/security/SodiumPasswordHasher.cpp
    src/application/AuthenticationService.cpp
    src/application/BootstrapService.cpp
    src/application/AcademicManagementService.cpp
    src/application/AdminReportingService.cpp
    src/application/SubmissionManagementService.cpp
    src/application/MarkingService.cpp
    src/application/ModerationService.cpp
)
target_include_directories(lms_core PUBLIC include)
target_link_libraries(lms_core PUBLIC SQLite::SQLite3 PkgConfig::SODIUM)

qt_add_executable(ModeratedLMS
    src/main.cpp
    src/ui/LoginWindow.cpp
    src/ui/AcademicPanel.cpp
    src/ui/AssignmentPanel.cpp
    src/ui/MarkingPanel.cpp
    src/ui/ResultsPanel.cpp
    src/ui/MaintenancePanel.cpp
)
target_link_libraries(ModeratedLMS PRIVATE Qt6::Widgets lms_core)

# Deliberately OFF in normal builds and distribution packages.
# Enable only for an authorised local administrator recovery on the development PC.
option(LMS_ENABLE_LOCAL_ADMIN_RECOVERY "Build the local admin recovery utility" OFF)
if(LMS_ENABLE_LOCAL_ADMIN_RECOVERY)
    qt_add_executable(ModeratedLMSAdminRecovery src/tools/AdminRecovery.cpp)
    target_link_libraries(ModeratedLMSAdminRecovery PRIVATE Qt6::Widgets lms_core)
endif()

# A restore tool can never share a database connection with the running LMS.
qt_add_executable(ModeratedLMSRestore src/tools/RestoreBackup.cpp)
target_link_libraries(ModeratedLMSRestore PRIVATE Qt6::Widgets lms_core)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(lms_tests tests/core_tests.cpp)
    target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(lms_tests)
endif()
