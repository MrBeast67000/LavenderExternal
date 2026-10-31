# lavender.gg

External ImGui/DX11 overlay + Roblox Studio companion scripts, for your own test place.

## Build (Visual Studio 2022)

```
git clone <this folder as a repo, or just use it as-is>
cd lavender_gg
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

First configure will take a minute — it's fetching imgui (docking branch), cpp-httplib,
and nlohmann/json via `FetchContent`. Output exe: `build\Release\lavender_gg.exe`.

Needs internet access for the first CMake configure. No other external deps —
DirectX 11 and Win32 are part of the Windows SDK.

## Roblox side setup

1. In Studio: **Game Settings → Security → Allow HTTP Requests** → ON.
2. Put `LavenderClient.lua` in `StarterPlayer > StarterPlayerScripts` as a **LocalScript**.
3. Put `LavenderAdminServer.lua` in `ServerScriptService` as a **Script**.
4. Open `LavenderAdminServer.lua`, replace the placeholder `000000000` in
   `ADMIN_WHITELIST` with your own UserId.
5. (Optional, for Teleport) add a Folder named `LavenderWaypoints` under
   `workspace`, with Parts inside named for each waypoint (e.g. `Spawn`, `Stage2`).
6. Run `lavender_gg.exe`, then **Play** the place in Studio (F5). The Home tab
   should flip to "Connected" within ~1 second.

## What's genuinely client-side vs. what needs the Roblox loop running

| Feature | How it actually works |
|---|---|
| The overlay window itself (drag, resize, shimmer, INSERT/END, all tabs rendering) | 100% native C++/DX11 — works with or without Roblox open. |
| Home tab player/session/notification data | **Requires** the Roblox LocalScript running and posting to the bridge. Nothing is read from Roblox's process — if the script isn't running, the Home tab just stays "Disconnected" and shows nothing fabricated. |
| Visuals (labels, highlight, tracers via Beam/Highlight) | Rendered **inside Roblox** by the LocalScript using real `Highlight`/`BillboardGui` instances, driven by toggles pulled from the C++ app. Not an external overlay drawn on top of the game window. |
| Camera FOV | Set client-side in Roblox (`Camera.FieldOfView`) from the same config poll. |
| Camera preview | **Real screen capture** of the Roblox window via Windows DXGI Desktop Duplication (same API OBS/Discord use) — not a Roblox API, doesn't need the LocalScript running, works as long as the Roblox window is open and visible. Assumes Roblox is on your primary monitor (see the `FILL` comment in `capture.cpp` for secondary-monitor setups). |
| Heal / Respawn / Teleport / Movement | Overlay only **queues a command**. The Roblox LocalScript polls and forwards it to a server `Script`, which checks your UserId against a whitelist before doing anything. The overlay never claims these succeeded — the Home tab's connected/notifications state is the only feedback channel, and nothing here fakes a result. |

## Important limitation — Studio only

`HttpService` requests from a Roblox client can only reach `127.0.0.1` when that
client is running **locally** — i.e. Studio Play-testing, or a local server you're
hosting on the same machine. A live place running on Roblox's own cloud servers
cannot route `HttpService` calls back to your desktop; the Home/Visuals/Admin
features will simply show "Disconnected" there. This matches the brief (a private
test place) but is worth knowing if you later try this against a published server.

## What this deliberately does NOT do

No process memory reads of `RobloxPlayerBeta.exe`, no DLL injection, no modification
of Roblox's binary, no interaction with any experience other than one you own and have
instrumented yourself. Every "admin" action is server-validated against an explicit
UserId list — the overlay has no inherent authority, it's a remote control that the
place you built decides whether to listen to.
