-- LavenderClient.lua | Luau | StarterPlayer.StarterPlayerScripts (LocalScript)
-- Talks ONLY to 127.0.0.1:7777 (the lavender.gg desktop app). Requires:
--   Game Settings > Security > Allow HTTP Requests = ON
-- This only reaches the bridge while testing in Studio (Play/Local Server) or a
-- server actually running on this machine — HttpService can't route out of a
-- live Roblox-hosted server back to your PC.

local HttpService      = game:GetService("HttpService")
local Players           = game:GetService("Players")
local RunService        = game:GetService("RunService")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

local LocalPlayer = Players.LocalPlayer
local BASE = "http://127.0.0.1:7777"

local AdminRemote = ReplicatedStorage:WaitForChild("LavenderAdmin")

local POLL_INTERVAL = 0.5
local activeHighlights = {}   -- [Player] = Highlight instance
local activeLabels     = {}   -- [Player] = BillboardGui instance

-- lightweight fps tracker, decoupled from the poll loop so postState never
-- blocks on a render frame
local lastFrameTime = os.clock()
local currentFps = 60
RunService.RenderStepped:Connect(function()
    local now = os.clock()
    local delta = now - lastFrameTime
    lastFrameTime = now
    if delta > 0 then currentFps = 1 / delta end
end)

local function safeJsonDecode(s)
    local ok, v = pcall(HttpService.JSONDecode, HttpService, s)
    if ok then return v end
    return nil
end

local function postState()
    local char = LocalPlayer.Character
    local humanoid = char and char:FindFirstChildOfClass("Humanoid")

    local state = {
        player = {
            name = LocalPlayer.Name,
            health = humanoid and humanoid.Health or 0,
            maxHealth = humanoid and humanoid.MaxHealth or 100,
        },
        session = {
            placeId = tostring(game.PlaceId),
            jobId = game.JobId,
            fps = math.floor(currentFps),
        },
        notifications = {}, -- extend: push your own test-session notices here
    }

    local ok, body = pcall(HttpService.JSONEncode, HttpService, state)
    if not ok then return end
    pcall(function()
        HttpService:PostAsync(BASE .. "/api/state", body, Enum.HttpContentType.ApplicationJson)
    end)
end

-- ─── visuals: applied with real Roblox instances, not an external overlay draw ──
local function clearVisualsFor(plr)
    if activeHighlights[plr] then activeHighlights[plr]:Destroy(); activeHighlights[plr] = nil end
    if activeLabels[plr] then activeLabels[plr]:Destroy(); activeLabels[plr] = nil end
end

local function applyVisuals(cfg)
    for _, plr in ipairs(Players:GetPlayers()) do
        if plr == LocalPlayer then continue end
        local char = plr.Character
        if not char then clearVisualsFor(plr); continue end

        if cfg.objectHighlight then
            local hl = activeHighlights[plr]
            if not hl then
                hl = Instance.new("Highlight")
                hl.Parent = char
                activeHighlights[plr] = hl
            end
            hl.FillTransparency = 1 - (cfg.opacity or 0.85)
            hl.OutlineTransparency = 0
            local c = cfg.highlightColor or {189, 140, 255}
            hl.OutlineColor = Color3.fromRGB(c[1], c[2], c[3])
        elseif activeHighlights[plr] then
            activeHighlights[plr]:Destroy(); activeHighlights[plr] = nil
        end

        if cfg.playerLabels then
            local label = activeLabels[plr]
            local head = char:FindFirstChild("Head")
            if head then
                if not label then
                    label = Instance.new("BillboardGui")
                    label.Name = "LavenderLabel"
                    label.Size = UDim2.new(0, 160, 0, 36)
                    label.StudsOffset = Vector3.new(0, 2.2, 0)
                    label.AlwaysOnTop = true
                    label.Parent = head

                    local text = Instance.new("TextLabel")
                    text.Name = "Text"
                    text.Size = UDim2.new(1, 0, 1, 0)
                    text.BackgroundTransparency = 1
                    text.TextColor3 = Color3.new(1, 1, 1)
                    text.TextStrokeTransparency = 0.4
                    text.Font = Enum.Font.GothamBold
                    text.TextSize = 14
                    text.Parent = label

                    activeLabels[plr] = label
                end
                local text = label:FindFirstChild("Text")
                if text then
                    if cfg.showDistance then
                        local dist = (head.Position - (LocalPlayer.Character and
                            LocalPlayer.Character:FindFirstChild("HumanoidRootPart") and
                            LocalPlayer.Character.HumanoidRootPart.Position or head.Position)).Magnitude
                        text.Text = string.format("%s [%dm]", plr.Name, math.floor(dist))
                    else
                        text.Text = plr.Name
                    end
                end
            end
        elseif activeLabels[plr] then
            activeLabels[plr]:Destroy(); activeLabels[plr] = nil
        end
    end
end

local function pollVisuals()
    local ok, res = pcall(HttpService.GetAsync, HttpService, BASE .. "/api/visuals")
    if not ok then return end
    local cfg = safeJsonDecode(res)
    if not cfg then return end

    local cam = workspace.CurrentCamera
    if cam and cfg.fov then cam.FieldOfView = cfg.fov end

    applyVisuals(cfg)
end

-- ─── commands: forwarded to the server for validation, never executed locally ──
local function pollCommands()
    local ok, res = pcall(HttpService.GetAsync, HttpService, BASE .. "/api/commands")
    if not ok then return end
    local list = safeJsonDecode(res)
    if not list then return end

    for _, cmd in ipairs(list) do
        AdminRemote:FireServer(cmd)
    end
end

Players.PlayerRemoving:Connect(clearVisualsFor)

task.spawn(function()
    while task.wait(POLL_INTERVAL) do
        postState()
        pollVisuals()
        pollCommands()
    end
end)
