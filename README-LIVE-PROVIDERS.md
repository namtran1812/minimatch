# Free Live Market Data Providers

MiniMatch separates working no-key public market feeds from providers that require credentials or paid entitlements.

## Recommended working providers

### Coinbase Advanced Trade

Primary provider for the portfolio demo. It exposes public real-time Level 2 market data and trades without API authentication.

```bash
./scripts/setup_live_feed.sh
./scripts/run_live_coinbase.sh btcusd
```

### Kraken Spot

Secondary public venue with Level 2 order-book and trade channels.

```bash
./scripts/run_live_kraken.sh btcusd
```

### Binance.US Spot

Public market data remains supported. Some USD pairs can update less frequently than Coinbase or Kraken.

```bash
./scripts/run_live_binance.sh btcusd
```

### Recorded demo

Works offline and is useful for deterministic screenshots and recruiter demos.

## Coming soon placeholders

- Alpaca IEX equities: requires an account and API credentials.
- Massive / Polygon Flat Files: requires dataset entitlement; current Flat Files keys returned HTTP 403.
- Massive / Polygon live stocks: requires a suitable API key and live-data plan.

The UI displays these providers as roadmap items instead of showing broken empty charts.

## Test public connections

```bash
./scripts/test_live_providers.sh
```

## Run a provider and dashboard together

```bash
./scripts/run_live_dashboard.sh coinbase btcusd 8080
```
