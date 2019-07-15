local CustomShader = {}

local NewResource = SnowyStream.NewResource
local SetShaderResourceCode = SnowyStream.SetShaderResourceCode
local SetShaderResourceInput = SnowyStream.SetShaderResourceInput
local SetShaderResourceComplete = SnowyStream.SetShaderResourceComplete

function CustomShader.New(config : any)
	local resource = NewResource("[Runtime]/ShaderResource/" .. config.Name, "ShaderResource", true)
	SetShaderResourceCode(resource, "Material", config.Code, {})
end

return CustomShader