local Logger = {}
Logger.WARNING = "Warning ->"
Logger.ERROR = "Error  ->"
Logger.INFO = "Info   ->"

function Logger:Log(level : string, message : string, ...)
	-- now use print to write log
	print(level .. string.format(message, ...))
end

function Logger.New()
	return setmetatable({}, Logger)
end

return Logger