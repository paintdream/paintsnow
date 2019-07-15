local Synchronizer = {}
local Core = require("Engine/Core")

local weakMetaObjects = {
	__mode = "kv"
}

local stack : { any : any } = {}
Synchronizer.PendingWriteStack = stack
local objects : { any : any } = {}
Synchronizer.Objects = objects
local listeners : { any : any } = {}
Synchronizer.Listeners = listeners

function Synchronizer.New()
	local object = {
		Objects = setmetatable({}, weakMetaObjects),
		PendingWriteStack = {}
	}

	return setmetatable(object, Synchronizer)
end

function Synchronizer:AcceptWrite(guid, changeSet)
	local object = self.Objects[guid]
	if object then
		-- Bypass first hooker
		local metatable = getmetatable(object)
		local holder = metatable.__Holder
		if holder then
			local collection = changeSet.Collection
			if changeSet.MergeWrite then
				for key, value in pairs(collection) do
					holder[key] = value.Value
				end
			else
				for index, value in ipairs(changeSet) do
					holder[value.Key] = value.Value
				end
			end
		end
	end
end

function Synchronizer:Pop()
	local ret = self.PendingWriteStack
	self.PendingWriteStack = {}
	return ret
end

function Synchronizer:Push(remotePendingWriteStack)
	for guid, change in pairs(remotePendingWriteStack) do
		self:AcceptWrite(guid, change)
	end
end

function Synchronizer:InvokeWrite(guid, key, value)
	local pendingWrite = {
		Key = key,
		Value = value,
	}

	local mergeWrite = self.Objects[guid].MergeWrite
	self.PendingWriteStack[guid] = self.PendingWriteStack[guid] or { Collection = {}, MergeWrite = mergeWrite }
	local collection = self.PendingWriteStack[guid].Collection

	if mergeWrite then
		collection[key] = { Value = value }
	else
		table.insert(collection, { Key = key, Value = value })
	end
end


function Synchronizer:HookObject(guid, object, mergeWrite)
	-- Installs hook on this object
	-- We do not care read operations
	Core.Hook(object, nil, function (object, key, oldValue, newValue)
		for handler in pairs(self.Listeners) do
			if handler then
				handler(object, key, oldValue, newValue)
			end
		end

		return newValue
	end)

	mergeWrite = mergeWrite or false
	self.Objects[guid] = { Object = object, MergeWrite = mergeWrite }
end

function Synchronizer:UnHookObject(object)
	Core.UnHook(object)
end

return Synchronizer