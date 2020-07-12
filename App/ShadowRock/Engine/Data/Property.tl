local Property = record
	name : string
	value : number
	_update : function (Property)
	_inputs : { Property }
	_outputs : { Property }
end

function Property:UpdaterConst() end

function Property:UpdaterAdd()
	local sum = 0
	for _, v in ipairs(self._inputs) do
		sum = sum + v.value
	end

	self.value = sum
end

function Property:UpdaterMultiply()
	local result = 1
	for _, v in ipairs(self._inputs) do
		result = result * v.value
	end

	self.value = result
end

local min = math.min
function Property:UpdateMin()
	assert(#self._inputs ~= 0)
	local result = self._inputs[1].value
	for _, v in ipairs(self._inputs) do
		result = min(result, v.value)
	end

	self.value = result
end

function Property:UpdateClampMin()
	assert(#self._inputs ~= 0)
	local result = self.value
	for _, v in ipairs(self._inputs) do
		result = min(result, v.value)
	end

	self.value = result
end

local max = math.max
function Property:UpdateMax()
	assert(#self._inputs ~= 0)
	local result = self._inputs[1].value
	for _, v in ipairs(self._inputs) do
		result = max(result, v.value)
	end

	self.value = result
end

function Property:UpdateClampMax()
	assert(#self._inputs ~= 0)
	local result = self.value
	for _, v in ipairs(self._inputs) do
		result = max(result, v.value)
	end

	self.value = result
end

function Property:Update()
	assert(next(self._inputs) == nil)
	local lockMap : { Property : number } = {}
	local updateQueue : { Property } = { self }

	while next(updateQueue) ~= nil do
		local property = table.remove(updateQueue)
		property:_update()

		for _, sub in ipairs(property._outputs) do
			local lockCount = (lockMap[sub] or #sub._inputs) - 1
			if lockCount == 0 then
				lockMap[sub] = nil
				table.insert(updateQueue, sub)
			else
				lockMap[sub] = lockCount
			end
		end
	end

	assert(next(lockMap) == nil) -- all properties must be updated
end

function Property:AddInput(property : Property)
	table.insert(self._inputs, property)
	table.insert(property._outputs, self)
end

function Property:AddOutput(property : Property)
	property:AddInput(self)
end

function Property.New(name : string, value : number, update : function (Property)) : Property
	return setmetatable({
		name = name,
		value = value,
		_update = update,
		_inputs = {},
		_outputs = {},
	}, Property as METATABLE) as Property
end