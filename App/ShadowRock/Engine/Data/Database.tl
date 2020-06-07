local Database = record
	Handle : Hive
end

rawset(Database as { string : any }, "__index", Database)

function Database:Execute(sql : string, data) : Honey
	return self.Handle:Execute(sql, data)
end

function Database:Lines(sql : string, cacheCount : integer, data)
	if cacheCount == nil then
		cacheCount = -1
	end

	local honey = self:Execute(sql, data)
	if not honey then return function () return end end

	local cache : { integer : any } = {}

	return function (t, k : integer)
		if k == nil or k == cacheCount then
			-- reinit
			local s : any = honey:Step(cacheCount)
			cache = s
			k = nil
		end

		-- tail call to next
		return next(cache, k)
	end
end

function Database:Commit()

end

function Database:Query(sql : string, data)
	print("Query Result: ")
	print("--------------------------------------")
	local rowCount = 0
	for k, line in self:Lines(sql, 5, data) do
		local item = ""
		for i, v in ipairs(line) do
			if i == 1 then
				item = tostring(v)
			else
				item = item .. "\t|" .. tostring(v)
			end
		end

		print(item)
		rowCount = rowCount + 1
	end
	print("Lines: " .. tostring(rowCount))
	print("--------------------------------------")
end

function Database.New(path : string)
	local handle : Hive = Remembery.NewDatabase(path, "", "", true)
	local meta = {}
	meta.Handle = handle
	return setmetatable(meta, { __index = Database })
end

return Database