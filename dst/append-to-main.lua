-- Add these to the end of DST's main.lua

--------------------------
-- LuaPanda integration --
--------------------------

local lp = require "LuaPanda"
-- Make strict mode happy
lua_extension = nil
luapanda_chook = nil
jit = nil
lp.start()
