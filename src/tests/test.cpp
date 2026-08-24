#include LOGGER_HEADER
#include "tests.hpp"

#ifndef DIABLE_SOLUTION
#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
static const test<LOGGER_NAME> test_object{ STRINGIFY(LOGGER_NAME) };
#endif
