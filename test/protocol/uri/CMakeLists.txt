set(TESTS
        uri_parse_test
        uri_parse_scheme_test
        uri_parse_path_test
        uri_test
        uri_encoding_test
        uri_normalization_test
        uri_comparison_test
        uri_reference_test
        uri_resolve_test
        uri_builder_test
        uri_stream_test
        optional_test
)

foreach (test ${TESTS})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} PUBLIC ${LINK_LIBS_GLOBAL} PUBLIC GTest::gtest_main)
  gtest_discover_tests(${test})
endforeach (test)
