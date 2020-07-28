local Timeline = record
	component : TapeComponent
end

function Timeline:Push()

end

function Timeline:Pop()

end

function Timeline:Seek(index : number)

end

function Timeline.New() : Timeline
	local object = {
		tapeComponent = TapeComponentModule.New()
	}

	return setmetatable(object, TapeComponent as METATABLE) as Timeline
end

return Timeline