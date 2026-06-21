-- DS5Dongle Telemetry — D1 schema
-- Apply with: wrangler d1 execute ds5-telemetry --file=schema.sql

-- Historical append log — every successful collect call inserts one row.
-- Useful for time-series graphs and debugging without altering the users table.
CREATE TABLE IF NOT EXISTS stats (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    installation_id TEXT    NOT NULL DEFAULT 'Unknown',
    platform        TEXT    NOT NULL DEFAULT 'Unknown',  -- process.platform value
    os              TEXT    NOT NULL DEFAULT 'Unknown',  -- human label (e.g. "Ubuntu 24.04")
    version         TEXT    NOT NULL DEFAULT 'Unknown',  -- app version
    firmware        TEXT    NOT NULL DEFAULT 'Unknown',  -- dongle firmware string
    ts              INTEGER NOT NULL                     -- Unix timestamp in ms
);

CREATE INDEX IF NOT EXISTS idx_stats_installation_id ON stats (installation_id);
CREATE INDEX IF NOT EXISTS idx_stats_platform        ON stats (platform);
CREATE INDEX IF NOT EXISTS idx_stats_os              ON stats (os);
CREATE INDEX IF NOT EXISTS idx_stats_version         ON stats (version);
CREATE INDEX IF NOT EXISTS idx_stats_firmware        ON stats (firmware);

-- One row per anonymous installation_id.
-- Upserted on every collect: always reflects the user's current state.
CREATE TABLE IF NOT EXISTS users (
    installation_id TEXT    PRIMARY KEY,
    platform        TEXT    NOT NULL DEFAULT 'Unknown',
    os              TEXT    NOT NULL DEFAULT 'Unknown',
    version         TEXT    NOT NULL DEFAULT 'Unknown',
    firmware        TEXT    NOT NULL DEFAULT 'Unknown',
    first_seen      INTEGER NOT NULL,  -- Unix timestamp ms
    last_seen       INTEGER NOT NULL   -- Unix timestamp ms
);

CREATE INDEX IF NOT EXISTS idx_users_platform ON users (platform);
CREATE INDEX IF NOT EXISTS idx_users_os       ON users (os);
CREATE INDEX IF NOT EXISTS idx_users_version  ON users (version);
CREATE INDEX IF NOT EXISTS idx_users_firmware ON users (firmware);
