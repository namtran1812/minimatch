#include "minimatch/market_recorder.hpp"

namespace minimatch {

MarketRecorder::MarketRecorder(
    const std::string& path
)
: out_(path, std::ios::binary)
{
}

void MarketRecorder::write_snapshot(
    const MarketDataSnapshot&
)
{
}

void MarketRecorder::write_update(
    const std::vector<MarketDataUpdate>&
)
{
}

void MarketRecorder::flush()
{
    out_.flush();
}

}
