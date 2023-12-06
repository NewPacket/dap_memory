add_library(dap_memory INTERFACE)
set(dap_memory_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(dap_memory PRIVATE 
                            memory_manager.h
                            memory_resource.h
                            memory_resource_manager.h
)

target_include_directories (dap_memory INTERFACE ${dap_memory_ROOT_DIR})