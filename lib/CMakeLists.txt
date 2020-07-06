# Add target for the library
add_library(dht11 src/dht11lib.c)
target_include_directories(dht11 PUBLIC inc)

# Add the unit test
add_unit_test(
    TARGET_NAME
        test_dht11
    SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test_dht11.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dht11_arduino_constants.c
)
target_link_libraries(test_dht11 PUBLIC dht11)
