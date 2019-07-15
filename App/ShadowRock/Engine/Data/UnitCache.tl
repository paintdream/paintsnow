-- UnitCache.lua
-- By PaintDream (paintdream@paintdream.com)
-- 2018-10-9
--

local UnitCache = {}
UnitCache.__index = UnitCache
local cachedObjects : { any : any } = {}
UnitCache.CachedObjects = cachedObjects
UnitCache.Count = 0
UnitCache.MaxCount = 4096
UnitCache.Index = 0

function UnitCache.New(maxCount : integer)
	return setmetatable({
		CachedObjects = {},
		Count = 0,
		MaxCount = maxCount,
		Index = 0
	}, UnitCache)
end

function UnitCache:Cleanup()
	-- remove ids too early
	local threshold = math.max(self.Index / 2, self.Index - self.MaxCount / 2)
	local objects = self.CachedObjects
	local newObjects : { any : any } = {}
	for k, v in pairs(objects) do
		if v > threshold then
			newObjects[k] = v
		end
	end

	self.CachedObjects = newObjects
end

function UnitCache:Cache(object)
	local index = self.Index + 1
	self.Index = index
	if self.CachedObjects[object] == nil then
		self.Count = self.Count + 1
		if self.Count >= self.MaxCount then
			self:Cleanup()
		end
	end

	self.CachedObjects[object] = index
	return index
end

return UnitCache