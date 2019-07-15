local GUI = {}
-- derive from entity
GUI.__index = GUI

function GUI.New()
	-- local rootEntity = MythForest.NewEntity(-1)
	return setmetatable({}, { __index = GUI, __chain = true })
end

return GUI