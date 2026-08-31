cmake_minimum_required(VERSION 3.23)

project(ak-series LANGUAGES CXX VERSION 0.0.1)

option(SETUP_TEST_IFNAME "Set-up the test VCAN interface automatically" OFF)
option(PYTHON_BINDINGS "Build python bindings" OFF)
option(BUILD_TESTING "Builds test folder, only use for testing" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake QUIET)
if (ament_cmake_FOUND)
  message(STATUS "Ament cmake detected.")
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "SocketCAN is Linux-only; this project cannot be built on ${CMAKE_SYSTEM_NAME}.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SocketCAN REQUIRED IMPORTED_TARGET libsocketcan)

add_library(ak_series_lib STATIC)
add_library(ak-series::ak_series_lib ALIAS ak_series_lib)

target_sources(ak_series_lib
  PRIVATE
  src/can/MIT_frame.cpp
  src/can/Servo_frame.cpp
  src/can/comms.cpp
  src/AKSeries.cpp
)

target_compile_features(ak_series_lib PUBLIC cxx_std_17)

target_include_directories(ak_series_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/
  DESTINATION include/
)
install(TARGETS ak_series_lib
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)


if (BUILD_TESTING)
  enable_testing()
	find_package(GTest REQUIRED)
  include(GoogleTest)
	add_executable(run_tests
	  # Add testing files here once populated
    tests/run_tests.cpp
    tests/can/ServoFrameTests.cpp
    tests/can/MitFrameTests.cpp
    tests/can/CommsTest.cpp
    tests/AKSeriesTests.cpp
	)
	set(AK_SERIES_TESTS ak_series_lib GTest::gtest_main)
	target_link_libraries(run_tests PUBLIC ${AK_SERIES_TESTS})

	if (SETUP_TEST_IFNAME)
	  set(VCAN_IFNAME "vcan_test")

	  configure_file(
	    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CTestCustom.cmake.in
	    ${CMAKE_BINARY_DIR}/CTestCustom.cmake
	    @ONLY
	  )
	endif()

  gtest_discover_tests(run_tests
    TEST_SUFFIX .noargs
    TEST_LIST noArgsTests
    PROPERTIES
      TIMEOUT 10
      ENVIRONMENT "SETUP_TEST_IFNAME=${VCAN_IFNAME}"
  )

  target_compile_features(run_tests PUBLIC
    cxx_std_17
  )

endif()


if (ament_cmake_FOUND)
  ament_export_targets(${PROJECT_NAME}Targets
    HAS_LIBRARY_TARGET
  )
  ament_package()
else()
  include(GNUInstallDirs)
  set(INSTALL_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  install(EXPORT ${PROJECT_NAME}Targets
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION ${INSTALL_CMAKE_CONFIG_DIR}
  )
  include(CMakePackageConfigHelpers)
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${INSTALL_CMAKE_CONFIG_DIR}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
  )
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${INSTALL_CMAKE_CONFIG_DIR}
  )
endif()


target_link_libraries(ak_series_lib PUBLIC PkgConfig::SocketCAN)
