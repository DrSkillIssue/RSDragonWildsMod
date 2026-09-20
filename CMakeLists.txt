cmake_minimum_required(VERSION 3.28)
project(DragonwildsSpellBar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MOD_SOURCES
    src/Configuration.cpp
    src/SpellBarMod.cpp
    src/game/Player.cpp
    src/game/Spells.cpp
    src/game/SpellWheel.cpp
    src/game/Widgets.cpp
    src/spellbar/Actions.cpp
    src/spellbar/Cooldowns.cpp
    src/spellbar/Setup.cpp
    src/spellbar/SpellBar.cpp
    src/containers/Containers.cpp
)

set(MOD_FOLDER RSDragonwilds/Binaries/Win64/ue4ss/Mods/DragonwildsSpellBar)

include(FetchContent)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    FetchContent_Declare(
        ue4ss
        URL https://github.com/UE4SS-RE/RE-UE4SS/releases/download/experimental/UE4SS_v3.0.1-1135-gf6d5f942.zip
        URL_HASH SHA256=e96640054e0da71232ea63d105eeefb96de1ece609b113f0c6e617b83addbac2
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(ue4ss)

    find_program(LLVM_DLLTOOL_EXECUTABLE llvm-dlltool REQUIRED)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/UE4SS.lib"
        COMMAND "${LLVM_DLLTOOL_EXECUTABLE}" -m i386:x86-64 -d "${CMAKE_SOURCE_DIR}/cmake/UE4SS.def" -l "${CMAKE_BINARY_DIR}/UE4SS.lib"
        DEPENDS "${CMAKE_SOURCE_DIR}/cmake/UE4SS.def"
    )
    add_custom_target(ue4ss_import_library DEPENDS "${CMAKE_BINARY_DIR}/UE4SS.lib")

    add_library(DragonwildsSpellBar SHARED src/Main.cpp ${MOD_SOURCES})
    set_target_properties(DragonwildsSpellBar PROPERTIES OUTPUT_NAME main)
    target_compile_definitions(DragonwildsSpellBar PRIVATE _CRT_SECURE_NO_WARNINGS)
    add_dependencies(DragonwildsSpellBar ue4ss_import_library)
    target_link_libraries(DragonwildsSpellBar PRIVATE "${CMAKE_BINARY_DIR}/UE4SS.lib")
    add_custom_command(
        TARGET DragonwildsSpellBar POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -DDLL=$<TARGET_FILE:DragonwildsSpellBar> -P "${CMAKE_SOURCE_DIR}/cmake/CheckModExports.cmake"
    )

    install(TARGETS DragonwildsSpellBar RUNTIME DESTINATION ${MOD_FOLDER}/dlls)
    install(FILES
        package/enabled.txt
        package/mods.ini
        LICENSE
        THIRD-PARTY-NOTICES.txt
        DESTINATION ${MOD_FOLDER}
    )
    install(FILES ${ue4ss_SOURCE_DIR}/dwmapi.dll DESTINATION RSDragonwilds/Binaries/Win64)
    install(FILES
        ${ue4ss_SOURCE_DIR}/ue4ss/UE4SS.dll
        ${ue4ss_SOURCE_DIR}/ue4ss/UE4SS-settings.ini
        ${ue4ss_SOURCE_DIR}/ue4ss/LICENSE
        DESTINATION RSDragonwilds/Binaries/Win64/ue4ss
    )

    set(CPACK_GENERATOR ZIP)
    set(CPACK_PACKAGE_FILE_NAME DragonwildsSpellBar)
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    include(CPack)
else()
    enable_testing()
    find_program(LOCALEDEF_EXECUTABLE localedef REQUIRED)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0
        GIT_SHALLOW TRUE
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)

    add_executable(spellbar-tests
        tests/main.cpp
        tests/fake/Engine.cpp
        tests/fake/Fake.cpp
        tests/fake/Functions.cpp
        tests/fake/Game.cpp
        tests/fake/Ini.cpp
        tests/fake/Objects.cpp
        tests/actions_test.cpp
        tests/configuration_test.cpp
        tests/containers_test.cpp
        tests/cooldowns_test.cpp
        tests/manager_test.cpp
        tests/setup_test.cpp
        ${MOD_SOURCES}
    )
    target_include_directories(spellbar-tests PRIVATE tests/fake src)
    target_link_libraries(spellbar-tests PRIVATE GTest::gtest)
    target_compile_definitions(spellbar-tests PRIVATE DW_DEV)
    target_compile_options(spellbar-tests PRIVATE "-D__declspec(x)=" -UNDEBUG -g -Wall -Wextra
        $<$<CXX_COMPILER_ID:GNU>:-Wno-class-memaccess -Wno-mismatched-new-delete>
    )

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/locales/de_DE.UTF-8/LC_NUMERIC"
        COMMAND "${CMAKE_COMMAND}" -E make_directory locales
        COMMAND "${LOCALEDEF_EXECUTABLE}" --no-archive -i de_DE -f UTF-8 locales/de_DE.UTF-8
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    add_custom_target(de_DE_locale ALL DEPENDS "${CMAKE_BINARY_DIR}/locales/de_DE.UTF-8/LC_NUMERIC")

    gtest_discover_tests(spellbar-tests TEST_PREFIX C.)
    gtest_discover_tests(spellbar-tests TEST_PREFIX de_DE. EXTRA_ARGS de_DE.UTF-8
        PROPERTIES ENVIRONMENT "LOCPATH=${CMAKE_BINARY_DIR}/locales"
    )
endif()
