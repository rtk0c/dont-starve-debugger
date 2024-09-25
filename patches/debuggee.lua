-- Klei's main.lua has a section of this, which seems to be their internal debugging tool
-- We are just camouflage as the expected debuggee.lua and let their code drive us
-- TODO that requires somehow turning CONFIGURATION == "not production" and TheSim:ShouldInitDebugger(), which is a native function
--[[
DEBUGGER_ENABLED = TheSim:ShouldInitDebugger() and IsNotConsole() and CONFIGURATION ~= "PRODUCTION" and not TheNet:IsDedicated()
if DEBUGGER_ENABLED then
	Debuggee = require 'debuggee'
end
--]]

local LuaPanda = require "LuaPanda"

-- Make strict mode happy
lua_extension = nil
luapanda_chook = nil
jit = nil

LuaPanda.start()
