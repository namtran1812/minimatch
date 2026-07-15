#pragma once

#include "minimatch/market_data.hpp"

#include <fstream>
#include <string>

namespace minimatch {

class MarketRecorder {
public:
    explicit MarketRecorder(
        const std::string& path
    );

    void write_snapshot(
        const MarketDataSnapshot&
    );

    void write_update(
        const std::vector<MarketDataUpdate>&
    );

    void flush();

private:
    std::ofstream out_;
};

}
