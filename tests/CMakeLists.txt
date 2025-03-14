include(ProcessorCount)
find_package(Git REQUIRED)
enable_testing()

# Tests samples
# =============
set(SAMPLES_GIT_URL "https://github.com/lief-project/samples.git" CACHE STRING "URL to tests samples")
set(SAMPLES_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/lief-samples")
set(SAMPLES_TAG master)

ExternalProject_Add(lief_samples
  PREFIX            ${SAMPLES_PREFIX}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  GIT_REPOSITORY    ${SAMPLES_GIT_URL}
  GIT_SHALLOW       1
  GIT_TAG           ${SAMPLES_TAG}
  UPDATE_COMMAND    ${GIT_EXECUTABLE} pull
)

ExternalProject_Get_Property(lief_samples source_dir)
set(LIEF_SAMPLES_DIRECTORY "${source_dir}" CACHE INTERNAL "Path to LIEF samples")
message(STATUS "Samples directory: ${LIEF_SAMPLES_DIRECTORY}")

# Tests options
# =============

if (NOT LIEF_ELF OR NOT LIEF_PE OR NOT LIEF_MACHO)
  message(FATAL_ERROR "Tests require all LIEF's modules activated" )
endif()

set(LIEF_EXAMPLES_DIRECTORY  "${PROJECT_SOURCE_DIR}/examples")
set(LIEF_EXAMPLES_BINARY_DIR "${PROJECT_BINARY_DIR}/examples")
#set(LIEF_SAMPLES_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/samples")

if (MSVC)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/utils.py.in"
    "${PROJECT_BINARY_DIR}/api/python/utils.py"
    @ONLY)
else()
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/utils.py.in"
    "${PROJECT_BINARY_DIR}/api/python/utils.py"
    @ONLY)
endif()

add_definitions(-DPATH_TO_CONFIG="${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DPATH_TO_SAMPLES="${LIEF_SAMPLES_DIRECTORY}")


# Catch
# =====
set(CATCH_VERSION 2.2.3)
set(CATCH_SHA256 SHA256=46606cb0eae047a4a5f70f0794244d26c51f72e695b52f4f11c54ea1d1421360)
set(CATCH_URL "${THIRD_PARTY_DIRECTORY}/Catch2-${CATCH_VERSION}.zip" CACHE STRING "URL to the Catch repo")

ExternalProject_Add(catch
    PREFIX         ${CATCH_PREFIX}
    URL            ${CATCH_URL}
    URL_HASH       ${CATCH_SHA256}
    TIMEOUT 10
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)
ExternalProject_Get_Property(catch source_dir)
set(CATCH_INCLUDE_DIR "${source_dir}" CACHE INTERNAL "Path to include folder for Catch")



# Code covergage
# ==============
if(LIEF_COVERAGE)
  find_program(GCOV_PATH gcov)
  find_program(LCOV_PATH lcov)
  find_program(GENHTML_PATH genhtml)
  find_program(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/tests)

  set(output_name "lief_coverage")
  set(coverage_info "${CMAKE_BINARY_DIR}/${output_name}.info")
  set(coverage_cleaned "${coverage_info}.cleaned")

  add_custom_target(lief_coverage

    # Cleanup lcov
    ${LCOV_PATH} --zerocounters --directory .

    # Run tests
    COMMAND make check-lief
    #COMMAND ctest -R test_iterators

    # Capturing lcov counters and generating report
    COMMAND ${LCOV_PATH} --directory . --capture --output-file ${coverage_info}
    COMMAND ${LCOV_PATH} --remove ${coverage_info} 'utf8/*' 'filesystem/*' 'libjson/*' 'tests/*' '/usr/*' 'pybind11/*' '*mbedtls*' rang_cpp_color/* easyloggingpp/* -output-file ${coverage_cleaned}
    COMMAND ${GENHTML_PATH} -o ${output_name} ${coverage_cleaned}
    #COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_cleaned}

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

endif()


# Fuzzing
# =======
if(UNIX AND NOT APPLE)
  set(MELKOR_VERSION ac2495b) # From the fork: https://github.com/romainthomas/elf_fuzzer
  set(MELKOR_SHA256 SHA256=8cccc4ca5e05e305215cc74761413746b660b76f5869a563f52cec1f23d79f2e)
  set(MELKOR_URL "${THIRD_PARTY_DIRECTORY}/Melkor_ELF_Fuzzer-${MELKOR_VERSION}.zip" CACHE STRING "URL to the Melkor package")

  set(MELKOR_PREFIX      "${CMAKE_CURRENT_BINARY_DIR}/Melkor")
  set(MELKOR_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/Melkor")
  ExternalProject_Add(MELKOR
   PREFIX            ${MELKOR_PREFIX}
   URL               ${MELKOR_URL}
   URL_HASH          ${MELKOR_SHA256}
   INSTALL_DIR       ${MELKOR_INSTALL_DIR}
   UPDATE_COMMAND    ""
   CONFIGURE_COMMAND ""
   INSTALL_COMMAND   ""
   BUILD_COMMAND     make clean && make
   BUILD_IN_SOURCE   ON)

  ExternalProject_get_property(MELKOR SOURCE_DIR)
  set(MELKOR_BINARY "${SOURCE_DIR}/melkor")
  message(STATUS "${MELKOR_BINARY}")
endif()

# Tests
# =====
add_executable(test_iterators "${CMAKE_CURRENT_SOURCE_DIR}/test_iterators.cpp")

if (MSVC)
  target_compile_options(test_iterators PUBLIC /FIiso646.h)
  set_property(TARGET test_iterators PROPERTY LINK_FLAGS /NODEFAULTLIB:MSVCRT)
endif()

set_target_properties(
  test_iterators
  PROPERTIES CXX_STANDARD           11
             CXX_STANDARD_REQUIRED  ON)

target_include_directories(test_iterators PUBLIC
  $<TARGET_PROPERTY:LIB_LIEF,INCLUDE_DIRECTORIES>
  ${CATCH_INCLUDE_DIR})

if (LIEF_COVERAGE)
  target_compile_options(test_iterators PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
  target_link_libraries(test_iterators gcov)
endif()

add_dependencies(test_iterators catch LIB_LIEF)

target_link_libraries(test_iterators LIB_LIEF)

add_test(test_iterators ${CMAKE_CURRENT_BINARY_DIR}/test_iterators)

# Python
# ======
if(WIN32)
  configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/run_python_test.bat.in"
  "${CMAKE_CURRENT_BINARY_DIR}/run_python_test.bat"
  @ONLY)
endif()

macro(ADD_PYTHON_TEST name command)
if(UNIX)
  add_test(${name}
      "bash"
      "-c"
      "PYTHONPATH=\"${PROJECT_BINARY_DIR}/api/python:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}\" ${command} ${ARGN}")
endif()

if(WIN32)
   add_test(${name}
      "${PROJECT_BINARY_DIR}/tests/run_python_test.bat"
    "${command}"
    "${ARGN}")
endif()
endmacro()

if (LIEF_PYTHON_API)
  set(PYTHON_TESTS_ENABLED OFF)
  get_target_property(PYTHON_VERSION pyLIEF PYTHON_VERSION)
  find_package(PythonInterp ${PYTHON_VERSION})

  message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")

  if (PYTHONINTERP_FOUND)
    set(PYTHON_TESTS_ENABLED ON)
  endif()

  if (PYTHON_TESTS_ENABLED)

    # nm
    # --
    ADD_PYTHON_TEST(EXAMPLE_PYTHON_nm_elf
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/nm.py \
      ${LIEF_SAMPLES_DIRECTORY}/MachO/MachO64_x86-64_binary_ls.bin")

    ADD_PYTHON_TEST(EXAMPLE_PYTHON_nm_pe
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/nm.py \
      ${LIEF_SAMPLES_DIRECTORY}/PE/PE32_x86_binary_winhello-mingw.exe")

    ADD_PYTHON_TEST(EXAMPLE_PYTHON_nm_macho
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/nm.py \
      ${LIEF_SAMPLES_DIRECTORY}/ELF/ELF32_ARM_binary_ls.bin")

    # Abstract
    # --------
    ADD_PYTHON_TEST(EXAMPLE_PYTHON_abstract_reader_pe
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/abstract_reader.py \
      ${LIEF_SAMPLES_DIRECTORY}/PE/PE32_x86_binary_cmd-upx.exe")

    ADD_PYTHON_TEST(EXAMPLE_PYTHON_abstract_reader_elf
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/abstract_reader.py \
      ${LIEF_SAMPLES_DIRECTORY}/ELF/ELF32_ARM_binary_ls.bin")

    ADD_PYTHON_TEST(EXAMPLE_PYTHON_abstract_reader_macho
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/abstract_reader.py \
      ${LIEF_SAMPLES_DIRECTORY}/MachO/MachO64_x86-64_binary_id.bin")

    ADD_PYTHON_TEST(EXAMPLE_PYTHON_abstract_json
      ${PYTHON_EXECUTABLE}
      "${LIEF_EXAMPLES_DIRECTORY}/python/abstract_json.py \
      ${LIEF_SAMPLES_DIRECTORY}/ELF/ELF64_x86-64_binary_ls.bin")

    ADD_PYTHON_TEST(UNITTEST_PYTHON_abstract
      ${PYTHON_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/abstract/abstract_tests.py")
  endif()


endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/elf")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vdex")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/art")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dex")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/oat")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/pe")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/macho")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/api")

# /!\ Bad Hack /!\
add_dependencies(LIB_LIEF lief_samples)
ProcessorCount(N)
if(N EQUAL 0)
  set(N 1)
endif()


if (CMAKE_CONFIGURATION_TYPES)
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure
        --build-config "$<CONFIGURATION>")
else()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
        --force-new-ctest-process --output-on-failure)
endif()
if (CMAKE_CONFIGURATION_TYPES)
  add_custom_target(check-lief
    COMMAND ${CMAKE_CTEST_COMMAND} --parallel 1 --output-on-failure --build-config "$<CONFIGURATION>"
    DEPENDS lief_samples)
else()
  add_custom_target(check-lief
    COMMAND ${CMAKE_CTEST_COMMAND} --parallel 1 --output-on-failure
    DEPENDS lief_samples)
endif()
