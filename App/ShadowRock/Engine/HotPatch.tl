local HotPatch = {}
local Core = require("Engine/Core")

function HotPatch:HotUpdate(orig)
	-- Nothing to be done here.
end

function HotPatch:Require(moduleName : string, newModuleData)
	local orig : any = require(moduleName)
	package.loaded[moduleName] = nil
	newModuleData = newModuleData or require(moduleName)

	assert(newModuleData)
	-- make translation
	local hotUpdate = newModuleData.HotUpdate
	if hotUpdate then
		newModuleData:HotUpdate(orig)
	end

	-- then patch table
	Core.Patch(orig, newModuleData)
	-- reset package slot
	package.loaded[moduleName] = orig
	return orig
end

return HotPatch