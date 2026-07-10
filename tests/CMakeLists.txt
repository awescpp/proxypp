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

proxypp_add_boost_tests(test_json LIBRARIES proxypp::common)

# http
proxypp_add_boost_tests(test_demo)
proxypp_add_boost_tests(test_boost_url LIBRARIES Boost::url)
proxypp_add_boost_tests(test_http_message_utils LIBRARIES Boost::multiprecision proxypp::http)
proxypp_add_boost_tests(test_http_body_info LIBRARIES proxypp::http)
proxypp_add_boost_tests(test_http_beast_request_adapter LIBRARIES proxypp::http)
proxypp_add_boost_tests(test_http_beast_response_adapter LIBRARIES proxypp::http)


# qjs
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
proxypp_add_boost_tests(test_qjs_object_builder
        SOURCES test_qjs_object_builder.cpp qjs_fixture.cpp
        LIBRARIES proxypp::script
)

# rule
proxypp_add_boost_tests(test_rule_op LIBRARIES proxypp::rule)
proxypp_add_boost_tests(test_rule_match LIBRARIES proxypp::rule)
proxypp_add_boost_tests(test_rule_match_context LIBRARIES proxypp::rule proxypp::script)
proxypp_add_boost_tests(test_rule_http_model LIBRARIES proxypp::rule)
proxypp_add_boost_tests(test_rule_config LIBRARIES proxypp::rule)
proxypp_add_boost_tests(test_rule_engine LIBRARIES proxypp::rule proxypp::script)
proxypp_add_boost_tests(test_rule_schema_validator LIBRARIES proxypp::rule)
proxypp_add_boost_tests(test_rule_schema_validation LIBRARIES proxypp::rule)
target_compile_definitions(test_rule_schema_validation PRIVATE PROXYPP_RULE_SCHEMA_V1_PATH="${CMAKE_SOURCE_DIR}/schemas/proxypp_rules_schema_v1.json")
proxypp_add_boost_tests(test_rule_http_action_executor LIBRARIES proxypp::rule proxypp::http)
proxypp_add_boost_tests(test_rule_http_context_injector LIBRARIES proxypp::rule proxypp::http proxypp::script)
proxypp_add_boost_tests(test_rule_http_apply LIBRARIES proxypp::rule proxypp::http proxypp::script)