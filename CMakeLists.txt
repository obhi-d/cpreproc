cmake_minimum_required(VERSION 3.1)

project(cpreproc VERSION 0.0.1 LANGUAGES CXX)


option(CPREPROC_BUILD_TESTS      "Build the unit tests when BUILD_TESTING is enabled." ON)
option(CPREPROC_BUILD_STATIC     "Build as static lib."                                ON)
option(CPREPROC_USE_LIBCPP_CLANG "Use libc++ for clang"                                ON)
set   (CPREPROC_BISON            "bison"                                               CACHE STRING "Bison execuatable")
set   (CPREPROC_FLEX              "flex"                                                CACHE STRING "Flex executable"  )

##
## CONFIGURATION
##
set(CPREPROC_TARGET_NAME               ${PROJECT_NAME})
set(CPREPROC_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_CXX_STANDARD 20)

add_custom_command( 
	OUTPUT 
		"${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp" 
	 COMMAND ${CPREPROC_BISON} -o ${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp --no-lines ${PROJECT_SOURCE_DIR}/src/cpreproc_parser.yy
	 DEPENDS 
		"${PROJECT_SOURCE_DIR}/src/cpreproc_parser.yy"
 )
add_custom_command( 
	OUTPUT 
		"${CMAKE_CURRENT_BINARY_DIR}/lex.cpreproc.cpp" 
	 COMMAND ${CPREPROC_FLEX} -o ${CMAKE_CURRENT_BINARY_DIR}/lex.cpreproc.cpp -L "${PROJECT_SOURCE_DIR}/src/cpreproc_lexer.l"
	 DEPENDS 
		"${PROJECT_SOURCE_DIR}/src/cpreproc_lexer.l"
		"${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp"
    )
add_custom_target(
  ${CPREPROC_TARGET_NAME}.PreBuild DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/lex.cpreproc.cpp
)
##
## TARGET
##
message("Target name: ${CPREPROC_TARGET_NAME}")

if (CPREPROC_BUILD_STATIC)
  add_library(${CPREPROC_TARGET_NAME} STATIC
    
    ${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lex.cpreproc.cpp
  )
else(CPREPROC_BUILD_STATIC)
  add_library(${CPREPROC_TARGET_NAME} SHARED 
    
    ${CMAKE_CURRENT_BINARY_DIR}/parse.cpreproc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lex.cpreproc.cpp
  )
  target_compile_definitions(${CPREPROC_TARGET_NAME} PUBLIC CPPROC_DYN_LIB_)
endif(CPREPROC_BUILD_STATIC)

target_compile_definitions(${CPREPROC_TARGET_NAME} PRIVATE CPPROC_EXPORT_)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if (CPREPROC_USE_LIBCPP_CLANG)
    target_compile_options(${CPREPROC_TARGET_NAME} PUBLIC "-stdlib=libc++")
    target_link_options(${CPREPROC_TARGET_NAME} PUBLIC "-stdlib=libc++")
  endif(CPREPROC_USE_LIBCPP_CLANG)
endif()


add_dependencies(${CPREPROC_TARGET_NAME} ${CPREPROC_TARGET_NAME}.PreBuild)
add_library(${PROJECT_NAME}::${CPREPROC_TARGET_NAME} ALIAS ${CPREPROC_TARGET_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(
    ${CPREPROC_TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(
    ${CPREPROC_TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_compile_features(${CPREPROC_TARGET_NAME} INTERFACE cxx_std_20)

##
## TESTS
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND CPREPROC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(unit_tests)
endif()

# Utility
# add_subdirectory(utility)
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
