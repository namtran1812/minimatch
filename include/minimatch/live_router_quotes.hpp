#pragma once

#include "minimatch/router.hpp"

#include <string>
#include <vector>

namespace minimatch {

std::vector<VenueQuote> read_live_router_quotes(
    const std::string& symbol
);

}
