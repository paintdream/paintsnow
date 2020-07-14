function MakeConsole(filter, ...)
	local args = table.pack(...)
	local prefixes = { "", "return "}
	ListenConsole(function (command)
		if filter then
			command = filter(command)
		end

		if not command then return end

		local v, message
		for i, prefix in ipairs(prefixes) do
			v, message = load(prefix .. command, "Console", "t", _ENV)
			if v then
				print(v(table.unpack(args)))
			end
		end
		
		print("Invalid command: " .. command .. "\n" .. tostring(message))
	end)
end