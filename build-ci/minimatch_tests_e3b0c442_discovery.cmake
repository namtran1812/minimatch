include("/opt/homebrew/share/cmake/Modules/GoogleTestAddTests.cmake")
gtest_discover_tests_impl(
  TEST_EXECUTABLE [==[/Users/namtran/minimatch/build-ci/minimatch_tests]==]
  TEST_EXECUTOR [==[]==]
  TEST_WORKING_DIR [==[/Users/namtran/minimatch/build-ci]==]
  TEST_EXTRA_ARGS [==[]==]
  TEST_PROPERTIES [==[]==]
  TEST_PREFIX [==[]==]
  TEST_SUFFIX [==[]==]
  TEST_FILTER [==[]==]
  NO_PRETTY_TYPES [==[FALSE]==]
  NO_PRETTY_VALUES [==[FALSE]==]
  TEST_LIST [==[minimatch_tests_TESTS]==]
  CTEST_FILE [==[/Users/namtran/minimatch/build-ci/minimatch_tests_e3b0c442_tests.cmake]==]
  TEST_DISCOVERY_TIMEOUT [==[5]==]
  TEST_DISCOVERY_EXTRA_ARGS [==[]==]
  TEST_XML_OUTPUT_DIR [==[]==]
  TEST_JSON_OUTPUT_DIR [==[/Users/namtran/minimatch/build-ci]==]
)
