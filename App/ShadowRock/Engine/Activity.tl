local Activity = {}
local Core = require("Engine/Core")
--[[
local Playback = require("Engine/Data/Playback")

local State = { Time = 0.0 }
Activity.State = State
Activity.Playback = Playback -- Not nullable
Activity.SnapshotPath = ""

local function InitInstance(state)
	local curInstance = Core.Instance()
	return Core.Invoke(function ()
		Core.Post(curInstance)	
		return Core.Run(state)
	end)
end

function Activity:Seek(time : number)
	-- Reinit instance
	self.State.Time = time
	self.Playback.Instance = InitInstance(self.State)
	self.Playback:Seek(time)
end

function Activity:Pause()
	return self.Playback:Pause()
end

function Activity:Continue()
	return self.Playback:Continue()
end

function Activity:SaveSnapshot()
	local file = SnowyStream.NewFile(self.SnapshotPath, true)
	if file then
		return SnowyStream.WriteFile(file, Core.Encode(self.State))
	else
		return false
	end
end

function Activity:LoadSnapshot()
	local file = SnowyStream.NewFile(self.SnapshotPath, false)
	local filesize = SnowyStream.GetFileSize(file)
	local filedata = SnowyStream.ReadFile(file, filesize)

	local state : State = Core.Decode(filedata)
	if state then
		self.State = state
	end

	local playback = self.Playback	
	playback:ClearQueuedEvents()
	playback.Instance = InitInstance(self.State)

	Core.Post(playback.Instance)
end

function Activity.New(package : string, routine : string, archivePath : string, snapshotPath : string, interval : integer, bufferCount : integer, filename : string)
	local state = Core.State(package, routine)
	local instance = InitInstance(state)
	local playback = Playback.New(instance, interval, bufferCount, filename)
	local object = {
		Playback = playback,
		State = state,
		ArchivePath = filename,
		SnapshotPath = snapshotPath
	}

	return object
end
]]

return Activity