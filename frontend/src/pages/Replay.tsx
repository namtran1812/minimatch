import {
  useState,
} from "react";

import {
  useMarketData,
} from "../context/MarketDataContext";

import StatusBadge from "../components/StatusBadge";
import BboHistoryChart from "../components/charts/BboHistoryChart";

import ReplayProgressChart, {
  type ReplayProgressPoint,
} from "../components/charts/ReplayProgressChart";

export default function Replay() {
  const {
    mode,
    setMode,
    snapshot,
    bboHistory,
    status,
    sendCommand,
  } = useMarketData();

  const replay = snapshot?.replay;

  const [pendingSeek, setPendingSeek] =
    useState<number | null>(null);

  function play() {
    sendCommand({
      command: "play",
    });
  }

  function pause() {
    sendCommand({
      command: "pause",
    });
  }

  function restart() {
    sendCommand({
      command: "restart",
    });
  }

  function changeSpeed(
    speed: number
  ) {
    sendCommand({
      command: "speed",
      speed,
    });
  }

  function seek(
    percent: number
  ) {
    if (
      !replay ||
      replay.totalRecords <= 0
    ) {
      return;
    }

    const recordIndex =
      Math.round(
        (
          percent / 100
        ) *
          replay.totalRecords
      );

    sendCommand({
      command: "seek",
      recordIndex,
    });
  }

  const progress =
    replay &&
    replay.totalRecords > 0
      ? (
          replay.recordIndex /
          replay.totalRecords
        ) * 100
      : 0;

  const displayedProgress =
    pendingSeek ??
    progress;


  const progressHistory:
    ReplayProgressPoint[] =
    replay &&
    replay.totalRecords > 0
      ? [
          {
            sample: 0,
            recordIndex: 0,
            progress: 0,
          },
          {
            sample: 1,
            recordIndex:
              replay.recordIndex,
            progress,
          },
        ]
      : [];

  const timestampElapsed =
    replay &&
    replay.currentTimestampNs >=
      replay.firstTimestampNs
      ? replay.currentTimestampNs -
        replay.firstTimestampNs
      : 0;

  const speedOptions = [
    0.25,
    0.5,
    1,
    2,
    5,
  ];

  const connected =
    status === "CONNECTED";

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">
          DETERMINISTIC REPLAY
        </span>

        <h1>Replay</h1>
      </div>

      <div className="replay-info">
        <div className="replay-info__section">
          <span className="eyebrow">
            WHAT THIS PAGE DOES
          </span>

          <p>
            Replay reconstructs recorded MiniMatch
            market state deterministically from the
            event stream, allowing historical exchange
            behavior to be inspected record by record.
          </p>
        </div>

        <div className="replay-info__section">
          <span className="eyebrow">
            HOW TO USE
          </span>

          <p>
            Enter replay mode, control playback speed,
            pause or restart processing, and drag the
            timeline to seek to a specific point in the
            recorded event stream.
          </p>
        </div>

        <div className="replay-info__section">
          <span className="eyebrow">
            DETERMINISM
          </span>

          <p>
            The replay engine exposes its generation,
            state checksum, rejected-record count, and
            current event position so reconstructed
            state can be verified across runs.
          </p>
        </div>
      </div>

      <div className="replay-status-strip">
        <div>
          <span>SOCKET</span>

          <StatusBadge
            value={status}
          />
        </div>

        <div>
          <span>MODE</span>

          <StatusBadge
            value={mode.toUpperCase()}
          />
        </div>

        <div>
          <span>ENGINE</span>

          <StatusBadge
            value={
              replay?.status ??
              (
                mode === "replay"
                  ? "WAITING"
                  : "OFFLINE"
              )
            }
          />
        </div>

        <div>
          <span>COMPLETE</span>

          <StatusBadge
            value={
              replay?.complete
                ? "YES"
                : "NO"
            }
          />
        </div>
      </div>

      {mode !== "replay" && (
        <div className="panel replay-offline-panel">
          <div>
            <span className="eyebrow">
              REPLAY ENGINE
            </span>

            <h2>Replay Offline</h2>

            <p>
              Switch the market data source to
              REPLAY to connect to the deterministic
              replay engine.
            </p>
          </div>

          <button
            onClick={() =>
              setMode("replay")
            }
          >
            ENTER REPLAY MODE
          </button>
        </div>
      )}

      {mode === "replay" && (
        <>
          <div className="metrics replay-metrics">
            <div className="metric-card">
              <span className="eyebrow">
                EVENT SEQUENCE
              </span>

              <strong>
                {replay?.recordIndex ??
                  "—"}
              </strong>

              <span className="metric-detail">
                OF{" "}
                {replay?.totalRecords ??
                  0}
              </span>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                PROGRESS
              </span>

              <strong>
                {progress.toFixed(1)}%
              </strong>

              <span className="metric-detail">
                EVENT STREAM
              </span>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                SPEED
              </span>

              <strong>
                {replay?.speed ?? 1}x
              </strong>

              <span className="metric-detail">
                PLAYBACK RATE
              </span>
            </div>

            <div className="metric-card">
              <span className="eyebrow">
                REJECTED
              </span>

              <strong>
                {replay
                  ?.rejectedRecords ??
                  "—"}
              </strong>

              <span className="metric-detail">
                RECORDS
              </span>
            </div>
          </div>

          <div className="panel replay-control-panel">
            <div className="panel-title">
              <div>
                <span className="eyebrow">
                  PLAYBACK
                </span>

                <h2>Replay Control</h2>
              </div>

              <StatusBadge
                value={
                  replay?.status ??
                  status
                }
              />
            </div>

            <div className="replay-transport">
              <button
                onClick={restart}
                disabled={!connected}
              >
                RESTART
              </button>

              <button
                onClick={play}
                disabled={!connected}
              >
                PLAY
              </button>

              <button
                onClick={pause}
                disabled={!connected}
              >
                PAUSE
              </button>

              <div className="replay-speed-group">
                {speedOptions.map(
                  (speed) => (
                    <button
                      key={speed}
                      type="button"
                      disabled={!connected}
                      className={
                        (
                          replay?.speed ??
                          1
                        ) === speed
                          ? "replay-speed--active"
                          : ""
                      }
                      onClick={() =>
                        changeSpeed(
                          speed
                        )
                      }
                    >
                      {speed}x
                    </button>
                  )
                )}
              </div>
            </div>

            <div className="replay-timeline">
              <div className="replay-timeline__labels">
                <span>
                  RECORD{" "}
                  {replay?.recordIndex ??
                    0}
                </span>

                <strong>
                  {displayedProgress.toFixed(
                    1
                  )}
                  %
                </strong>

                <span>
                  {replay?.totalRecords ??
                    0} TOTAL
                </span>
              </div>

              <input
                type="range"
                min="0"
                max="100"
                step="0.1"
                value={displayedProgress}
                disabled={
                  !connected ||
                  !replay
                }
                onChange={(event) =>
                  setPendingSeek(
                    Number(
                      event.target.value
                    )
                  )
                }
                onPointerUp={() => {
                  if (
                    pendingSeek !== null
                  ) {
                    seek(pendingSeek);

                    setPendingSeek(
                      null
                    );
                  }
                }}
              />

              <div className="replay-progress-track">
                <div
                  className="replay-progress-fill"
                  style={{
                    width:
                      `${Math.min(
                        100,
                        Math.max(
                          0,
                          displayedProgress
                        )
                      )}%`,
                  }}
                />
              </div>
            </div>
          </div>

          <div className="replay-visualization-heading">
            <div>
              <span className="eyebrow">
                REPLAY TELEMETRY
              </span>

              <h2>
                Historical Reconstruction
              </h2>
            </div>

            <span>
              {replay?.recordIndex ?? 0}
              {" / "}
              {replay?.totalRecords ?? 0}
              {" RECORDS"}
            </span>
          </div>

          <div className="replay-chart-grid">
            <BboHistoryChart
              data={bboHistory}
            />

            <ReplayProgressChart
              data={progressHistory}
            />
          </div>

          <div className="replay-state-grid">
            <div className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">
                    DETERMINISM
                  </span>

                  <h2>
                    State Verification
                  </h2>
                </div>
              </div>

              <div className="replay-detail-row">
                <span>STATE CHECKSUM</span>

                <strong>
                  {replay?.checksum ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>GENERATION</span>

                <strong>
                  {replay?.generation ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>
                  REJECTED RECORDS
                </span>

                <strong>
                  {replay
                    ?.rejectedRecords ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>COMPLETE</span>

                <strong>
                  {replay?.complete
                    ? "YES"
                    : "NO"}
                </strong>
              </div>
            </div>

            <div className="panel">
              <div className="panel-title">
                <div>
                  <span className="eyebrow">
                    REPLAY POSITION
                  </span>

                  <h2>
                    Event Timeline
                  </h2>
                </div>
              </div>

              <div className="replay-detail-row">
                <span>SYMBOL</span>

                <strong>
                  {snapshot?.symbol ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>
                  FIRST TIMESTAMP
                </span>

                <strong>
                  {replay
                    ?.firstTimestampNs ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>
                  CURRENT TIMESTAMP
                </span>

                <strong>
                  {replay
                    ?.currentTimestampNs ??
                    "—"}
                </strong>
              </div>

              <div className="replay-detail-row">
                <span>
                  ELAPSED
                </span>

                <strong>
                  {replay
                    ? `${(
                        timestampElapsed /
                        1_000_000_000
                      ).toFixed(3)}s`
                    : "—"}
                </strong>
              </div>
            </div>
          </div>
        </>
      )}
    </section>
  );
}
