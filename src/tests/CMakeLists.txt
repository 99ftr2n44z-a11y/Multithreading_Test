add_executable(tests EXCLUDE_FROM_ALL tests.hpp tests.cpp)
target_link_libraries(tests PRIVATE logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

foreach(LOGGER_HEADER IN LISTS LOGGER_HEADERS)
  get_filename_component(LOGGER_NAME "${LOGGER_HEADER}" NAME_WLE)
  add_library(${LOGGER_NAME}_test OBJECT EXCLUDE_FROM_ALL ${LOGGER_HEADER} tests.hpp test.cpp)
  target_link_libraries(${LOGGER_NAME}_test PUBLIC ${THIRD_PARTY_LIBRARIES})
  target_compile_definitions(${LOGGER_NAME}_test PRIVATE
    "LOGGER_NAME=${LOGGER_NAME}" "LOGGER_HEADER=<${LOGGER_HEADER}>")
  if(WIN32)
    target_compile_definitions(${LOGGER_NAME}_test PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
  endif()
  target_link_libraries(tests PRIVATE ${LOGGER_NAME}_test)
endforeach()
