find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework)

add_executable(test_demo test_demo.cpp)
target_link_libraries(test_demo PRIVATE Boost::unit_test_framework)
target_compile_definitions(test_demo PRIVATE BOOST_TEST_DYN_LINK)
add_test(NAME test_demo COMMAND test_demo)