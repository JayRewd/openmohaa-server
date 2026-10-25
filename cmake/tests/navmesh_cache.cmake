#
# Navmesh disk-cache path/header unit tests (no gi/FS)
#

add_executable(test_navmesh_cache
    ${SOURCE_DIR}/fgame/tests/test_navmesh_cache.cpp
    ${SOURCE_DIR}/fgame/navigation_recast_cache.cpp
    ${SOURCE_DIR}/qcommon/q_shared.c
    ${SOURCE_DIR}/qcommon/common_light.c
)

target_include_directories(test_navmesh_cache PRIVATE
    ${SOURCE_DIR}
    ${SOURCE_DIR}/fgame
    ${SOURCE_DIR}/qcommon
)

target_compile_definitions(test_navmesh_cache PRIVATE NAVMESH_CACHE_UNIT_TEST)

add_test(NAME test_navmesh_cache COMMAND test_navmesh_cache)
set_tests_properties(test_navmesh_cache PROPERTIES TIMEOUT 15)
