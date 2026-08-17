add_library(creation_engineer_language_host STATIC
    AppLanguagePolicy.cpp
    AppLanguagePolicy.h
)

target_include_directories(creation_engineer_language_host PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(CREATION_ENGINEER_ENABLE_SCRIPTING)
    target_include_directories(creation_engineer_language_host PRIVATE "${CREATION_ENGINEER_LLVM_ROOT}/include")
    target_compile_definitions(creation_engineer_language_host PUBLIC CREATION_ENGINEER_ENABLE_SCRIPTING=1)
else()
    target_compile_definitions(creation_engineer_language_host PUBLIC CREATION_ENGINEER_ENABLE_SCRIPTING=0)
endif()

