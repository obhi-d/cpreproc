cmake_minimum_required(VERSION 3.1)

project(ppr VERSION 0.0.1 LANGUAGES CXX)


option(PPR_BUILD_TESTS      "Build the unit tests when BUILD_TESTING is enabled." ON)
option(PPR_BUILD_STATIC     "Build as static lib."                                ON)
option(PPR_USE_LIBCPP_CLANG "Use libc++ for clang"                                ON)
set   (PPR_BISON            "bison"                                               CACHE STRING "Bison execuatable")
set   (PPR_FLEX             "flex"                                                CACHE STRING "Flex executable"  )

##
## CONFIGURATION
##
set(PPR_TARGET_NAME               ${PROJECT_NAME})
set(PPR_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_CXX_STANDARD 20)

add_custom_command( 
	OUTPUT 
		"${CMAKE_CURRENT_BINARY_DIR}/parse.ppr.cpp" 
	 COMMAND ${PPR_BISON} -o ${CMAKE_CURRENT_BINARY_DIR}/parse.ppr.cpp --no-lines ${PROJECT_SOURCE_DIR}/src/ppr_eval.yy
	 DEPENDS 
		"${PROJECT_SOURCE_DIR}/src/ppr_eval.yy"
 )

add_custom_command( 
	OUTPUT 
		"${CMAKE_CURRENT_BINARY_DIR}/lex.ppr.cpp" 
	 COMMAND ${PPR_FLEX} -o ${CMAKE_CURRENT_BINARY_DIR}/lex.ppr.cpp -L "${PROJECT_SOURCE_DIR}/src/ppr_tokenizer.l"
	 DEPENDS 
		"${PROJECT_SOURCE_DIR}/src/ppr_tokenizer.l"
    )
add_custom_target(
  ${PPR_TARGET_NAME}.PreBuild DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/parse.ppr.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/lex.ppr.cpp
)
##
## TARGET
##
message("Target name: ${PPR_TARGET_NAME}")

if (PPR_BUILD_STATIC)
  add_library(${PPR_TARGET_NAME} STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/lex.ppr.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/parse.ppr.cpp
   "src/ppr_tokenizer.cpp"
   "src/ppr_macroctx.cpp" 
   "src/ppr_transform.cpp")
else(PPR_BUILD_STATIC)
  add_library(${PPR_TARGET_NAME} SHARED 
   ${CMAKE_CURRENT_BINARY_DIR}/lex.ppr.cpp
   ${CMAKE_CURRENT_BINARY_DIR}/parse.ppr.cpp
   "src/ppr_tokenizer.cpp"
   "src/ppr_macroctx.cpp" 
   "src/ppr_transform.cpp"
  )
  target_compile_definitions(${PPR_TARGET_NAME} PUBLIC PPR_DYN_LIB_)
endif(PPR_BUILD_STATIC)

target_compile_definitions(${PPR_TARGET_NAME} PRIVATE PPR_EXPORT_)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if (PPR_USE_LIBCPP_CLANG)
    target_compile_options(${PPR_TARGET_NAME} PUBLIC "-stdlib=libc++")
    target_link_options(${PPR_TARGET_NAME} PUBLIC "-stdlib=libc++")
  endif(PPR_USE_LIBCPP_CLANG)
endif()


add_dependencies(${PPR_TARGET_NAME} ${PPR_TARGET_NAME}.PreBuild)
add_library(${PROJECT_NAME}::${PPR_TARGET_NAME} ALIAS ${PPR_TARGET_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(
    ${PPR_TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(
    ${PPR_TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_compile_features(${PPR_TARGET_NAME} INTERFACE cxx_std_20)

##
## TESTS
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND PPR_BUILD_TESTS)
    enable_testing()
    #add_subdirectory(unit_tests)
endif()

# Utility
add_executable(
  tokenizer_util
  ${PROJECT_SOURCE_DIR}/utility/tokenizer_util.cpp
)
target_link_libraries(tokenizer_util PRIVATE ${PPR_TARGET_NAME})

add_executable(
  transform_util
  ${PROJECT_SOURCE_DIR}/utility/transform_util.cpp
)
target_link_libraries(transform_util PRIVATE ${PPR_TARGET_NAME})

# INSTALL

install(TARGETS ${PROJECT_NAME}
      EXPORT ${PROJECT_NAME}_Targets
       ARCHIVE DESTINATION lib
       LIBRARY DESTINATION lib
       RUNTIME DESTINATION lib
       )

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

if(DEFINED CMAKE_INSTALL_DATAROOTDIR)
    configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION
    ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)


    install(EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)
endif()

install(DIRECTORY include DESTINATION .)
