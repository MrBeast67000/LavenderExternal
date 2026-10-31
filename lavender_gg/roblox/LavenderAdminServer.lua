-- LavenderAdminServer.lua | Luau | ServerScriptService (Script)
-- Validates every command against an explicit UserId whitelist before touching
-- anything. This is the ONLY place gameplay-authoritative actions actually happen —
-- the overlay and the client LocalScript only ever suggest a command, never execute one.

local Players           = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

-- FILL: your UserId(s). Get yours from https://www.roblox.com/users/profile while
-- logged in, or print(game.Players.LocalPlayer.UserId) in a LocalScript once.
local ADMIN_WHITELIST = {
    [000000000] = true, -- replace with your UserId
}

local AdminRemote = ReplicatedStorage:FindFirstChild("LavenderAdmin")
if not AdminRemote then
    AdminRemote = Instance.new("RemoteEvent")
    AdminRemote.Name = "LavenderAdmin"
    AdminRemote.Parent = ReplicatedStorage
end

local function isAllowed(plr)
    return ADMIN_WHITELIST[plr.UserId] == true
end

local function getHumanoid(plr)
    local char = plr.Character
    return char and char:FindFirstChildOfClass("Humanoid")
end

local handlers = {}

handlers.heal = function(plr)
    local hum = getHumanoid(plr)
    if hum then hum.Health = hum.MaxHealth end
end

handlers.respawn = function(plr)
    plr:LoadCharacter()
end

handlers.teleport = function(plr, cmd)
    local name = cmd.waypoint
    if type(name) ~= "string" then return end
    local waypoints = workspace:FindFirstChild("LavenderWaypoints")
    if not waypoints then return end
    local target = waypoints:FindFirstChild(name)
    if not target or not target:IsA("BasePart") then return end

    local char = plr.Character
    local hrp = char and char:FindFirstChild("HumanoidRootPart")
    if hrp then
        hrp.CFrame = target.CFrame + Vector3.new(0, 3, 0)
    end
end

handlers.setmovement = function(plr, cmd)
    local hum = getHumanoid(plr)
    if not hum then return end
    if type(cmd.walkspeed) == "number" then
        hum.WalkSpeed = math.clamp(cmd.walkspeed, 0, 500)
    end
    if type(cmd.jumppower) == "number" then
        hum.JumpPower = math.clamp(cmd.jumppower, 0, 1000)
    end
end

AdminRemote.OnServerEvent:Connect(function(plr, cmd)
    if not isAllowed(plr) then
        warn(("[lavender.gg] rejected admin command from non-whitelisted user %s (%d)")
            :format(plr.Name, plr.UserId))
        return
    end
    if type(cmd) ~= "table" or type(cmd.type) ~= "string" then return end

    local handler = handlers[cmd.type]
    if handler then
        handler(plr, cmd)
    else
        warn("[lavender.gg] unknown command type: " .. tostring(cmd.type))
    end
end)
