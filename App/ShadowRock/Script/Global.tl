local Global = {}
Global.__index = Global

local instance = {
	GameLogger = require("Engine/Data/Logger").New()
}

function Global.Instance()
	return instance
end

return Global