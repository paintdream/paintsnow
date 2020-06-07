local RepositoryCompressor = {}
RepositoryCompressor.__index = RepositoryCompressor
local Database = require("Engine/Data/Database")

function RepositoryCompressor.Compress(database : Database)
	if database then
		print("\tCompressing resources ...")

		local texID = "1"
		for _, line in database:Lines("select ID from `Names` where `Name` == 'TextureResource'", 1) do
			texID = tostring(table.unpack(line))
		end

		local runningCount = 0
		print("TextureResource Unique ID " .. texID)

		for _, line in database:Lines("select Path from `Resources` where `TypeID` == " .. texID .. ";", 64) do
			local path = table.unpack(line)
			local textureResource : ResourceBase = SnowyStream.NewResource(path, "TextureResource", false)
			if textureResource then
				local resource = textureResource
				runningCount = runningCount + 1
				SnowyStream.CompressResourceAsync(textureResource, "BPTC", function ()
					print("Compressed texture generated: " .. path)
					SnowyStream.PersistResource(resource, "")

					runningCount = runningCount - 1
					if runningCount == 0 then
						print("Compress finished.")
					end
				end)
			end
		end
	end
end


return RepositoryCompressor