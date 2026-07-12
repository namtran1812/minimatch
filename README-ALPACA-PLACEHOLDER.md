# Alpaca provider placeholder

This release intentionally presents Alpaca Stocks as a planned venue integration rather than an offline or broken feed.

## Current provider status

- Binance.US Spot: enabled live adapter
- Recorded/demo database: enabled
- Alpaca Stocks: coming soon placeholder

Selecting Alpaca in the Live Market workspace displays the planned coverage, data model, feed target, and integration roadmap. It does not poll the backend or populate misleading zero-value charts.

The existing `live_feed/live_market.py` and `scripts/run_live_alpaca.sh` files remain in the repository for future authenticated development, but the public UI does not claim that the integration is production-ready.
