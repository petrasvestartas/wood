// node --experimental-websocket screenshot_viewer.mjs <out.png> [command ...]: opens the hosted viewer in a Chrome window on the GPU with the commands as ?cmd=, waits for the scene, screenshots.
// SHOT_ZOOM (default 1.6) zooms the page as the browser's zoom does, so the layer panel and its text come out large in the 1600 x 1000 picture; SHOT_SCALE=2 writes it at 3200 x 2000.
// SHOT_QUERY adds viewer settings to the URL: SHOT_QUERY=thickness=0.5 draws the mesh edges thin under the Arctic outlines.
// SHOT_PB=<file.pb> shows that scene instead of the published one: served from this machine on 127.0.0.1 as a one-item scene the
// hosted viewer reads through ?data= and ?scene=, so nothing is uploaded (the way on Windows, where the R2 publish is not set up).
// CHROME names the browser; by default google-chrome on Linux, Chrome's usual install path on Windows and macOS.
import { spawn } from "node:child_process";
import { writeFileSync, readFileSync, mkdtempSync, rmSync, existsSync } from "node:fs";
import { createServer } from "node:http";
import { tmpdir } from "node:os";
import { join } from "node:path";

const [out, ...commands] = process.argv.slice(2);
const port = 9333;
// a profile of its own per run, some 60 MB Chrome writes, removed once Chrome has left it
const profile = mkdtempSync(join(tmpdir(), "shot-"));
// the browser and its GPU flags per platform: Linux the radeon Vulkan env of the user's launcher, Windows and macOS their own backends
const windows_chrome = ["C:/Program Files/Google/Chrome/Application/chrome.exe", "C:/Program Files (x86)/Google/Chrome/Application/chrome.exe", `${process.env.LOCALAPPDATA}/Google/Chrome/Application/chrome.exe`];
const browser = process.env.CHROME ?? (process.platform === "win32" ? windows_chrome.find((path) => existsSync(path)) : process.platform === "darwin" ? "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" : "google-chrome");
const linux = process.platform === "linux";
const gpu_flags = linux ? ["--ozone-platform=wayland", "--use-angle=vulkan", "--enable-features=Vulkan"] : [];
const gpu_env = linux ? { VK_DRIVER_FILES: "/usr/share/vulkan/icd.d/radeon_icd.json", __EGL_VENDOR_LIBRARY_FILENAMES: "/usr/share/glvnd/egl_vendor.d/50_mesa.json" } : {};
const chrome = spawn(browser, [
  `--remote-debugging-port=${port}`, `--user-data-dir=${profile}`,
  "--window-size=1600,1000", "--enable-unsafe-webgpu", ...gpu_flags,
  // a public page reading from 127.0.0.1 otherwise waits on Chrome's local network permission prompt
  ...(process.env.SHOT_PB ? ["--disable-features=LocalNetworkAccessChecks,BlockInsecurePrivateNetworkRequests,PrivateNetworkAccessSendPreflights"] : []),
  "--ignore-gpu-blocklist", "--no-first-run", "--new-window", "about:blank",
], { stdio: "ignore", env: { ...process.env, ...gpu_env } });

// a local scene: the .pb and a one-item scene naming it, served with the CORS and private-network headers the hosted page needs
let server;
let local_query = "";

if (process.env.SHOT_PB) {
  const pb = readFileSync(process.env.SHOT_PB);
  const scene = JSON.stringify({ name: "screenshot", items: [{ file: "pb/shot.pb", name: "scene", at: [0, 0, 0], size: pb.length }] });
  server = createServer((request, response) => {
    response.setHeader("Access-Control-Allow-Origin", "*");
    response.setHeader("Access-Control-Allow-Private-Network", "true");
    response.setHeader("Access-Control-Allow-Headers", "*");
    if (request.method === "OPTIONS") return response.end();
    if (request.url.startsWith("/scenes/shot.yaml")) return response.end(scene);
    if (request.url.startsWith("/pb/shot.pb")) return response.end(pb);
    response.statusCode = 404;
    response.end();
  });
  await new Promise((r) => server.listen(0, "127.0.0.1", r));
  local_query = `data=${encodeURIComponent(`http://127.0.0.1:${server.address().port}/`)}&scene=shot`;
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let targets;
for (let i = 0; i < 50 && !targets; i++) {
  await sleep(200);
  try { targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json(); } catch {}
}
const page = targets.find((t) => t.type === "page");
const ws = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((r) => (ws.onopen = r));
let id = 0;
const pending = new Map();
const logs = [];
ws.onmessage = (m) => {
  const msg = JSON.parse(m.data);
  if (msg.id && pending.has(msg.id)) { pending.get(msg.id)(msg); pending.delete(msg.id); }
  if (msg.method === "Runtime.consoleAPICalled") logs.push(msg.params.args.map((a) => a.value ?? a.description).join(" "));
};
const send = (method, params = {}) => new Promise((r) => { const n = ++id; pending.set(n, r); ws.send(JSON.stringify({ id: n, method, params })); });

await send("Runtime.enable");
await send("Page.enable");
// browser zoom: a smaller CSS viewport at a higher pixel ratio, the picture the same size
const zoom = Number(process.env.SHOT_ZOOM ?? 1.6);
// SHOT_SCALE (default 1) multiplies the pixel ratio alone: the same view, the picture that many times wider and higher
const scale = Number(process.env.SHOT_SCALE ?? 1);
await send("Emulation.setDeviceMetricsOverride", { width: Math.round(1600 / zoom), height: Math.round(1000 / zoom), deviceScaleFactor: zoom * scale, mobile: false });
const query = [local_query, commands.length ? "cmd=" + encodeURIComponent(commands.join(";")) : "", process.env.SHOT_QUERY ?? ""].filter(Boolean).join("&");
await send("Page.navigate", { url: "https://petrasvestartas.github.io/session/" + (query ? "?" + query : "") });
await sleep(15000);

await sleep(2000);
const shot = await send("Page.captureScreenshot", { format: "png" });
writeFileSync(out, Buffer.from(shot.result.data, "base64"));
console.log(logs.slice(0, 8).join("\n"));
ws.close();
server?.close();
// Chrome's helpers keep writing the profile after the main process has gone: retried for a while, never fatal
const forget_profile = (tries) => {
  try { rmSync(profile, { recursive: true, force: true }); } catch { if (tries > 0) setTimeout(() => forget_profile(tries - 1), 500); }
};
chrome.once("exit", () => forget_profile(40));
chrome.kill();
