cmake_minimum_required(VERSION 3.21)
project(ModeratedLMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

configure_file(schema/lms_schema.sql ${CMAKE_CURRENT_BINARY_DIR}/lms_schema.sql COPYONLY)

add_library(lms_core
    src/domain/User.cpp
    src/persistence/SqliteDatabase.cpp
    src/persistence/SQLiteRecordRepository.cpp
    src/persistence/SQLiteAcademicRepository.cpp
    src/security/SodiumPasswordHasher.cpp
    src/application/AuthenticationService.cpp
    src/application/BootstrapService.cpp
    src/application/AcademicManagementService.cpp
)
target_include_directories(lms_core PUBLIC include)
target_link_libraries(lms_core PUBLIC SQLite::SQLite3 PkgConfig::SODIUM)

qt_add_executable(ModeratedLMS
    src/main.cpp
    src/ui/LoginWindow.cpp
    src/ui/AcademicPanel.cpp
)
target_link_libraries(ModeratedLMS PRIVATE Qt6::Widgets lms_core)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(lms_tests tests/core_tests.cpp)
    target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(lms_tests)
endif()
