#
# A passworded server must refuse players who send no password
#

add_executable(test_join_password
    ${SOURCE_DIR}/fgame/tests/test_join_password.cpp
)

add_test(NAME test_join_password COMMAND test_join_password)
set_tests_properties(test_join_password PROPERTIES TIMEOUT 15)
