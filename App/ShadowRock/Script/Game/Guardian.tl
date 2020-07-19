local Guardian = record end
local MakeComponent = require("Engine/MakeComponent")

function Guardian.New() : Guardian
	-- TODO:
	return setmetatable({}, Guardian as METATABLE)
end

return Guardian