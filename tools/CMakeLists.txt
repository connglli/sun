# suni executable
add_executable(suni
    suni.cpp
)
target_link_libraries(suni PRIVATE sunigv suninterp sunir sunutil)

# sunigv executable (tool name must differ from library name)
add_executable(sunigv_tool
    sunigv.cpp
)
target_link_libraries(sunigv_tool PRIVATE sunigv sunir sunutil)
set_target_properties(sunigv_tool PROPERTIES OUTPUT_NAME sunigv)

# Install targets
install(TARGETS suni sunigv_tool DESTINATION bin)
