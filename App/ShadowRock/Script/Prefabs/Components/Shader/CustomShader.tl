local CustomShader = record end

local Input = record
	Type : string
	Stages : { string }
	Value : any
	Binding : string
end

local ShaderDescription = record
	Template : string -- [Runtime]/ShaderResource/CustomMaterialPass
	Name : string
	Priority : number
	Shaders : { string : string }
	Inputs : { string : Input }
	States : { string : string }
end

local function FromValue(value : any) : (string, string)
	local t : string = type(value)
	if t == "string" then
		return "Texture", value as string
	elseif t == "boolean" then
		return "bool", tostring(value)
	elseif t == "table" then
		local count = #(value as { number })
		local v = table.concat(value as { string }, ",")
		if count == 9 then return "MatrixFloat3x3", v
		elseif count == 16 then return "MatrixFloat4x4", v
		else return "Float" .. count, v  end -- vector
	else
		return "float", tostring(value)
	end
end

function CustomShader.New(description : ShaderDescription) : ShaderComponent
	local shaderResource : ShaderResource = SnowyStream.NewResource(description.Template, "ShaderResource", false) as ShaderResource
	assert(shaderResource)

	local shaderComponent : ShaderComponent = ShaderComponentModule.New(shaderResource, description.Name)
	for key, value in pairs(description.Inputs) do
		for _, stage in ipairs(value.Stages) do
			local t, v = FromValue(value.Value)
			shaderComponent:SetInput(stage, t, key as string, v, {})
		end
	end

	for stage, text in pairs(description.Shaders) do
		shaderComponent:SetCode(stage, text, {})
	end

	shaderComponent:SetCallback(function (info : string, code : string)
		if info and #info ~= 0 then
			print("[CustomShader.New] Compiling shader: " .. description.Name .. " failed!")
			print("[CustomShader.New] Error: " .. info)
			print("[CustomShader.New] Shader generated: ")
			print(code)
		else
			print("[CustomShader.New] Compiling shader: " .. description.Name)
		end
	end)

	shaderComponent:SetComplete()

	return shaderComponent
end