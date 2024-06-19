include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME util_mmd)
pr_add_library(${PROJ_NAME} STATIC)

pr_add_dependency(${PROJ_NAME} vfilesystem TARGET PUBLIC)

pr_set_include_path(utfcpp "${CMAKE_CURRENT_LIST_DIR}/third_party_libs/utfcpp/source")
pr_add_external_dependency(${PROJ_NAME} utfcpp HEADER_ONLY)

pr_add_headers(${PROJ_NAME} "include/")
pr_add_sources(${PROJ_NAME} "src/")

pr_finalize(${PROJ_NAME})
