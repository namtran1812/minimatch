import { useState } from "react";
import {
  pauseReplay,
  playReplay,
  restartReplay,
  seekReplay,
} from "../api/replay";

export default function Replay() {
  const [progress, setProgress] = useState(0);
  const [speed, setSpeed] = useState(1);
  const [status, setStatus] = useState("READY");

  async function play() {
    await playReplay();
    setStatus("PLAYING");
  }

  async function pause() {
    await pauseReplay();
    setStatus("PAUSED");
  }

  async function restart() {
    await restartReplay();
    setProgress(0);
    setStatus("READY");
  }

  async function seek(value: number) {
    setProgress(value);
    await seekReplay(value);
  }

  return (
    <section className="page">
      <div className="page-heading">
        <span className="eyebrow">DETERMINISTIC REPLAY</span>
        <h1>Replay</h1>
      </div>

      <div className="panel">
        <div className="panel-title">
          <h2>Replay Control</h2>
        </div>

        <div className="replay">
          <button onClick={restart}>RESTART</button>
          <button onClick={play}>PLAY</button>
          <button onClick={pause}>PAUSE</button>

          <input
            type="range"
            min="0"
            max="100"
            value={progress}
            onChange={(e) => seek(Number(e.target.value))}
          />

          <span>{progress}%</span>
        </div>

        <label>
          SPEED
          <select
            value={speed}
            onChange={(e) => setSpeed(Number(e.target.value))}
          >
            <option value={0.25}>0.25x</option>
            <option value={0.5}>0.5x</option>
            <option value={1}>1x</option>
            <option value={2}>2x</option>
            <option value={5}>5x</option>
          </select>
        </label>

        <div className="feedback">
          STATUS: {status} · SPEED: {speed}x
        </div>
      </div>

      <div className="metrics">
        <div className="metric-card">
          <span className="eyebrow">EVENT SEQUENCE</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">STATE HASH</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">ACTIVE ORDERS</span>
          <strong>—</strong>
        </div>

        <div className="metric-card">
          <span className="eyebrow">SYMBOLS</span>
          <strong>—</strong>
        </div>
      </div>
    </section>
  );
}
