CREATE TABLE IF NOT EXISTS route_executions (

    execution_id INTEGER PRIMARY KEY AUTOINCREMENT,

    created_at TEXT NOT NULL,

    symbol TEXT NOT NULL,

    side TEXT NOT NULL,

    requested_quantity REAL,

    filled_quantity REAL,

    remaining_quantity REAL,

    average_price REAL,

    total_notional REAL,

    total_fees REAL,

    total_latency_ms REAL,

    complete INTEGER

);

CREATE TABLE IF NOT EXISTS child_executions (

    child_id INTEGER PRIMARY KEY AUTOINCREMENT,

    execution_id INTEGER NOT NULL,

    venue TEXT,

    level_index INTEGER,

    requested_quantity REAL,

    filled_quantity REAL,

    remaining_quantity REAL,

    price REAL,

    notional REAL,

    fee REAL,

    latency_ms REAL,

    status TEXT,

    FOREIGN KEY(execution_id)

        REFERENCES route_executions(execution_id)

);
