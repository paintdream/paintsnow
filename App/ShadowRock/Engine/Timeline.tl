local Core = require("Engine/Core")

local Timeline = record
	component : TapeComponent
	isReplay : boolean
	pushHead : { string : any }
	popHead : { string : any }
end

function Timeline:Push(seq : number, event : { string : any })
	-- blocks all pushes on replaying
	if self.isReplay then
		return
	end

	-- record event
	self.component:Write(seq, Core.Encode(event))

	-- chain to timeline
	event.__next = self.pushHead
	event.__seq = seq

	self.pushHead = event
	self.popHead = self.popHead or event
end

function Timeline:Pop() : (number, { string : any })
	if self.isReplay then
		-- read from component
		local info = self.component:Read()
		return info[1] as number, (Core.Decode(info[2] as string)) as { string : any } -- seq, event
	else
		local event = self.popHead
		if event then
			self.popHead = self.popHead.__next as { string : any }
			local seq = event.__seq as number
			event.__seq = nil
			event.__next = nil

			return seq, event
		end
	end
end

function Timeline:Seek(index : number)

end

function Timeline.New() : Timeline
	local object = {
		component = TapeComponentModule.New(),
		isReplay = false,
		pushHead = nil,
		popHead = nil,
	}

	return setmetatable(object, TapeComponent as METATABLE) as Timeline
end

return Timeline