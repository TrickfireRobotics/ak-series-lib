cmake_minimum_required(VERSION 3.23)

project(ak_series LANGUAGES CXX)

option(SETUP_TEST_IFNAME "Set-up the test VCAN interface automatically" OFF)
option(PYTHON_BINDINGS "Build python bindings" OFF)
option(BUILD_TESTING "Builds test folder, only use for testing" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "SocketCAN is Linux-only; this project cannot be built on ${CMAKE_SYSTEM_NAME}.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SocketCAN REQUIRED IMPORTED_TARGET libsocketcan)

add_library(ak_series_lib STATIC)
add_library(ak_series::ak_series_lib ALIAS ak_series_lib)

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
  #Lets us define what happens and how tests are run
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

	  # Generate CTestCustom.cmake at the top of the build tree so that
	  # ctest actually picks up the CTEST_CUSTOM_PRE_TEST/POST_TEST commands
	  # that bring the virtual CAN interface up/down around the test run.
	  configure_file(
	    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CTestCustom.cmake.in
	    ${CMAKE_BINARY_DIR}/CTestCustom.cmake
	    @ONLY
	  )
	endif()

  # ENVIRONMENT is what actually reaches each test process; the pre-test
  # shell's own exports do not, so the interface name is injected here.
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


target_link_libraries(ak_series_lib PUBLIC PkgConfig::SocketCAN)


