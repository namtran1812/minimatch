#include "minimatch/live_router_quotes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace minimatch {
namespace {

std::string load_file(
    const std::string& path
) {
    std::ifstream input(path);

    if (!input) {
        return {};
    }

    std::ostringstream out;
    out << input.rdbuf();

    return out.str();
}

std::optional<double> json_number_field(
    const std::string& body,
    const std::string& field
) {
    const std::string marker =
        "\"" + field + "\":";

    const std::size_t begin =
        body.find(marker);

    if (begin == std::string::npos) {
        return std::nullopt;
    }

    std::size_t position =
        begin + marker.size();

    while (
        position < body.size() &&
        std::isspace(
            static_cast<unsigned char>(
                body[position]
            )
        )
    ) {
        ++position;
    }

    std::size_t end = position;

    while (
        end < body.size() &&
        (
            std::isdigit(
                static_cast<unsigned char>(
                    body[end]
                )
            ) ||
            body[end] == '.' ||
            body[end] == '-' ||
            body[end] == '+' ||
            body[end] == 'e' ||
            body[end] == 'E'
        )
    ) {
        ++end;
    }

    if (end == position) {
        return std::nullopt;
    }

    try {
        return std::stod(
            body.substr(
                position,
                end - position
            )
        );
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<VenueLevel> json_book_levels(
    const std::string& body,
    const std::string& side,
    std::size_t maximum_levels = 20
) {
    std::vector<VenueLevel> levels;

    const std::string marker =
        "\"" + side + "\":[";

    const std::size_t array_begin =
        body.find(marker);

    if (array_begin == std::string::npos) {
        return levels;
    }

    std::size_t position =
        array_begin + marker.size();

    while (levels.size() < maximum_levels) {
        const std::size_t object_begin =
            body.find('{', position);

        const std::size_t array_end =
            body.find(']', position);

        if (
            object_begin == std::string::npos ||
            array_end == std::string::npos ||
            object_begin > array_end
        ) {
            break;
        }

        const std::size_t object_end =
            body.find('}', object_begin);

        if (
            object_end == std::string::npos ||
            object_end > array_end
        ) {
            break;
        }

        const std::string object =
            body.substr(
                object_begin,
                object_end -
                    object_begin +
                    1
            );

        const auto price =
            json_number_field(
                object,
                "price"
            );

        const auto quantity =
            json_number_field(
                object,
                "quantity"
            );

        if (
            price &&
            quantity &&
            *price > 0.0 &&
            *quantity > 0.0
        ) {
            levels.push_back(
                VenueLevel{
                    *price,
                    *quantity
                }
            );
        }

        position =
            object_end + 1;
    }

    return levels;
}

double environment_double(
    const char* name,
    double fallback
) {
    const char* raw =
        std::getenv(name);

    if (
        raw == nullptr ||
        *raw == '\0'
    ) {
        return fallback;
    }

    try {
        const double value =
            std::stod(raw);

        if (!std::isfinite(value)) {
            return fallback;
        }

        return value;
    } catch (...) {
        return fallback;
    }
}

double venue_fee_bps(
    const std::string& venue
) {
    if (venue == "coinbase") {
        return environment_double(
            "ROUTER_COINBASE_TAKER_FEE_BPS",
            60.0
        );
    }

    if (venue == "kraken") {
        return environment_double(
            "ROUTER_KRAKEN_TAKER_FEE_BPS",
            40.0
        );
    }

    if (venue == "binance") {
        return environment_double(
            "ROUTER_BINANCE_TAKER_FEE_BPS",
            10.0
        );
    }

    return 0.0;
}

double venue_latency_ms(
    const std::string& venue
) {
    if (venue == "coinbase") {
        return environment_double(
            "ROUTER_COINBASE_LATENCY_MS",
            0.0
        );
    }

    if (venue == "kraken") {
        return environment_double(
            "ROUTER_KRAKEN_LATENCY_MS",
            0.0
        );
    }

    if (venue == "binance") {
        return environment_double(
            "ROUTER_BINANCE_LATENCY_MS",
            0.0
        );
    }

    return 0.0;
}

}

std::vector<VenueQuote>
read_live_router_quotes(
    const std::string& symbol
) {
    const std::array<std::string, 3>
        venues{
            "coinbase",
            "kraken",
            "binance"
        };

    std::vector<VenueQuote> quotes;

    quotes.reserve(
        venues.size()
    );

    const double latency_cost =
        std::max(
            0.0,
            environment_double(
                "ROUTER_LATENCY_COST_BPS_PER_MS",
                0.0
            )
        );

    for (const auto& venue : venues) {
        const std::string path =
            "data/live/" +
            venue +
            "_" +
            symbol +
            ".json";

        const std::string body =
            load_file(path);

        const auto bids =
            json_book_levels(
                body,
                "bids",
                20
            );

        const auto asks =
            json_book_levels(
                body,
                "asks",
                20
            );

        const bool healthy =
            !bids.empty() &&
            !asks.empty();

        quotes.push_back(
            VenueQuote{
                venue,
                bids,
                asks,
                venue_fee_bps(
                    venue
                ),
                venue_latency_ms(
                    venue
                ),
                latency_cost,
                healthy
            }
        );
    }

    return quotes;
}

}
