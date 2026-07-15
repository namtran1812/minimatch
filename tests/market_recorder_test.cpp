#include "minimatch/market_recorder.hpp"

#include <gtest/gtest.h>

TEST(MarketRecorder, CreatesFile)
{
    minimatch::MarketRecorder recorder(
        "/tmp/minimatch_test.bin"
    );

    recorder.flush();

    SUCCEED();
}
