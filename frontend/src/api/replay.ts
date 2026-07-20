import { api } from "./client";

export async function playReplay() {
  return (await api.post("/replay/play")).data;
}

export async function pauseReplay() {
  return (await api.post("/replay/pause")).data;
}

export async function restartReplay() {
  return (await api.post("/replay/restart")).data;
}

export async function seekReplay(progress: number) {
  return (await api.post("/replay/seek", { progress })).data;
}

export async function getReplayState() {
  return (await api.get("/replay/state")).data;
}

export async function setReplaySpeed(speed: number) {
  return (await api.post("/replay/speed", { speed })).data;
}
