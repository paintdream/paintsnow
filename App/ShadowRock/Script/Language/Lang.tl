local Lang = { __index = _ENV }
setmetatable(Lang, Lang)
local _ENV = Lang

return Lang