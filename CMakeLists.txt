cmake_minimum_required(VERSION 3.0)
project(spmemvfs)

add_library(spmemvfs spmemvfs/spmemvfs.h spmemvfs/spmemvfs.c)

install(FILES spmemvfs/spmemvfs.h DESTINATION include)
install(FILES ${PROJECT_BINARY_DIR}/$<CONFIG>/spmemvfs.lib DESTINATION lib/$<CONFIG>)
install(FILES ${PROJECT_BINARY_DIR}/$<CONFIG>/spmemvfs.pdb DESTINATION lib/$<CONFIG> CONFIGURATIONS Debug)
