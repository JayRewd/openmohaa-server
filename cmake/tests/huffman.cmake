#
# Huffman code table must match the tree walk bit for bit
#

add_executable(test_huffman_table
    ${SOURCE_DIR}/qcommon/tests/test_huffman_table.cpp
    ${SOURCE_DIR}/qcommon/huffman.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_link_libraries(test_huffman_table INTERFACE testing)
add_test(NAME test_huffman_table COMMAND test_huffman_table)
set_tests_properties(test_huffman_table PROPERTIES TIMEOUT 30)
