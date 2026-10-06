// node --experimental-websocket shoot.mjs <out.png> [command ...]: opens the hosted viewer in a Chrome window on the GPU with ?cmd= commands, screenshots.
import { spawn } from "node:child_process";
import { writeFileSync, mkdtempSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

const [out, ...commands] = process.argv.slice(2);
const port = 9333;
const chrome = spawn("google-chrome", [
  `--remote-debugging-port=${port}`, `--user-data-dir=${mkdtempSync(join(tmpdir(), "shot-"))}`,
  "--window-size=1600,1000", "--enable-unsafe-webgpu", "--ozone-platform=wayland", "--use-angle=vulkan", "--enable-features=Vulkan",
  "--ignore-gpu-blocklist", "--no-first-run", "--new-window", "about:blank",
], { stdio: "ignore", env: { ...process.env, VK_DRIVER_FILES: "/usr/share/vulkan/icd.d/radeon_icd.json", __EGL_VENDOR_LIBRARY_FILENAMES: "/usr/share/glvnd/egl_vendor.d/50_mesa.json" } });
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
await send("Page.navigate", { url: "https://petrasvestartas.github.io/session/" + (commands.length ? "?cmd=" + encodeURIComponent(commands.join(";")) : "") });
await sleep(15000);

await sleep(2000);
const shot = await send("Page.captureScreenshot", { format: "png" });
writeFileSync(out, Buffer.from(shot.result.data, "base64"));
console.log(logs.slice(0, 8).join("\n"));
ws.close();
chrome.kill();
