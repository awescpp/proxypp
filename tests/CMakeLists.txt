find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework multiprecision)

function(proxypp_add_boost_tests target)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES LIBRARIES DEFINITIONS LABELS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if (NOT ARG_SOURCES)
        set(ARG_SOURCES "${target}.cpp")
    endif ()

    add_executable(${target} ${ARG_SOURCES})
    target_compile_definitions(${target} PRIVATE BOOST_TEST_DYN_LINK ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE Boost::unit_test_framework ${ARG_LIBRARIES})
    add_test(NAME ${target} COMMAND ${target})

    if (ARG_LABELS)
        set_tests_properties(${target} PROPERTIES LABELS "${ARG_LABELS}")
    endif ()
endfunction()

proxypp_add_boost_tests(test_demo)
proxypp_add_boost_tests(test_boost_url LIBRARIES Boost::url)
proxypp_add_boost_tests(test_http_message_utils LIBRARIES Boost::multiprecision proxypp::http)
proxypp_add_boost_tests(test_http_body_info LIBRARIES proxypp::http)
proxypp_add_boost_tests(test_qjs_api LIBRARIES proxypp::script)
proxypp_add_boost_tests(test_qjs_runtime LIBRARIES proxypp::script)
proxypp_add_boost_tests(test_qjs_context LIBRARIES proxypp::script)
proxypp_add_boost_tests(test_qjs_value
        SOURCES test_qjs_evaluator.cpp qjs_fixture.cpp
        LIBRARIES proxypp::script
)
proxypp_add_boost_tests(test_qjs_evaluator
        SOURCES test_qjs_value.cpp qjs_fixture.cpp
        LIBRARIES proxypp::script
)
