#!/usr/bin/env node
// AFJ의 deny-by-default TTY 경계를 fl-git tui 실행에만 명시적으로 연다.
// Git 명령 본체는 계속 FreeLang AFJ로 실행된다.
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..");
const runner = "/home/kim/kim/platform/freelang-afj/bootstrap.js";
const tui = path.join(root, "src", "tui.fl");

globalThis.__flTerminalCapabilities = { tty: true, pty: false };
process.argv = [process.argv[0], runner, "run", tui, ...process.argv.slice(2)];
await import(runner);
