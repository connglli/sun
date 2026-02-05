# suni executable
add_executable(suni
    suni.cpp
)
target_link_libraries(suni PRIVATE sunigv suninterp sunir sunutil)

# Install targets
install(TARGETS suni DESTINATION bin)
