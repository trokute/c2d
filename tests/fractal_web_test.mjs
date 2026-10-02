import { chromium } from "playwright";
import { spawn } from "child_process";

const server = spawn("python3", ["-m", "http.server", "8002", "-d", "web"], { stdio: "ignore" });
await new Promise((r) => setTimeout(r, 1000));

let code = 1;
try {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto("http://localhost:8002/index.html");
  await page.waitForTimeout(2500);
  const err = await page.textContent("#out");
  const shot = (await page.locator("#canvas").screenshot()).toString("base64");
  const px = await page.evaluate(async (b64) => {
    const img = new Image();
    img.src = "data:image/png;base64," + b64;
    await img.decode();
    const c = document.createElement("canvas");
    c.width = img.width;
    c.height = img.height;
    const ctx = c.getContext("2d");
    ctx.drawImage(img, 0, 0);
    return Array.from(ctx.getImageData(4, 4, 1, 1).data);
  }, shot);
  await browser.close();
  const ok = err === "" && px[0] === 30 && px[1] === 10 && px[2] === 90;
  code = ok ? 0 : 1;
  if (!ok) process.stderr.write("FAIL " + JSON.stringify({ err, px }) + "\n");
} finally {
  server.kill();
}
process.exit(code);