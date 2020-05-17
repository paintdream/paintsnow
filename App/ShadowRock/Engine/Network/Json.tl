local Json = {}

function Json.Parse(str : string) : any
	return SnowyStream.ParseJson(str)
end

local function Stringify(data) : string
	local str = "{"
	for k, v in pairs(data) do
		if type(k) == "number" or type(k) == "string" then
			local item =  "\"" .. tostring(k) .. "\" : " 
			if type(v) == "number" or type(v) == "string" then
				item = item .. "\"" .. tostring(v) .. "\""
			elseif type(v) == "table" then
				local res : string = Stringify(v)	
				item = item .. res
			end	
			str = str .. item
		end
	end

	str = str .. "}"

	return str
end

function Json:Stringify(data) : string
	return Stringify(data)
end

return Json