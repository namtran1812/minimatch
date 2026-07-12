# Massive Flat Files integration

MiniMatch supports Massive as a historical-data provider. Massive Flat Files are compressed CSV datasets exposed through an S3-compatible endpoint. They are intended for bulk historical research, not live matching.

## Security first

Never commit provider credentials. If a credential was pasted into a chat, issue tracker, terminal recording, or screenshot, rotate it before use.

## Configure

```bash
cp .env.example .env
```

Edit `.env` and add newly rotated credentials:

```dotenv
MASSIVE_ACCESS_KEY_ID=...
MASSIVE_SECRET_ACCESS_KEY=...
MASSIVE_S3_ENDPOINT=https://files.massive.com
MASSIVE_S3_BUCKET=flatfiles
AWS_DEFAULT_REGION=us-east-1
```

Load credentials into the current shell:

```bash
source scripts/setup_massive.sh
```

Install the AWS CLI and test access:

```bash
brew install awscli
./scripts/check_massive.sh
```

## Browse files

```bash
./scripts/list_massive_files.sh us_stocks_sip/minute_aggs_v1 2026
```

## Download

```bash
./scripts/download_massive_file.sh \
  us_stocks_sip/minute_aggs_v1/2026/07/2026-07-10.csv.gz
```

## Import one ticker

```bash
python3 scripts/import_massive_minute_aggs.py \
  data/massive/raw/2026-07-10.csv.gz \
  --ticker AAPL \
  --json data/historical/massive_aapl.json
```

## Query

```bash
python3 scripts/query_historical_db.py --ticker AAPL --limit 20
```

The import creates `data/historical/market_history.db` with a `historical_bars` table.

## Included offline demo

The repository includes a generated AAPL session with 390 minute bars:

```bash
python3 scripts/query_historical_db.py --ticker AAPL --limit 10
```

Regenerate it with:

```bash
python3 scripts/generate_demo_massive.py
python3 scripts/import_massive_minute_aggs.py \
  data/massive/demo/2026-07-10.csv.gz \
  --ticker AAPL \
  --json data/historical/massive_aapl.json
```

## One-command automation

After creating `.env` with rotated Massive credentials, run:

```bash
./scripts/auto_massive.sh AAPL
```

This verifies credentials, finds the latest minute-aggregate file for the selected year, downloads it, imports the requested ticker into SQLite, writes frontend JSON, validates the rows, and records a log.

To import and immediately launch the dashboard:

```bash
./scripts/run_massive_dashboard.sh AAPL
```

Optional arguments:

```bash
./scripts/run_massive_dashboard.sh NVDA 2026 8080
```
