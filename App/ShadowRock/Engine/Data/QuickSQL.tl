local QuickSQL = {}

local ExpENV = {}
local function Preprocess(text)
	text = string.gsub(text, "%[.-%]", "(Node([%1]))") 
	text = string.gsub(text, "=", "//")
	text = string.gsub(text, "<", "<<")
	text = string.gsub(text, ">", ">>")
	return text
end

local function optostring(op)
	return op or "+"
end

local function ttostring(node) : string
	local text = ""
	local c = node
	if type(c) == "table" then
		if node.type == "binop" or node.type == "binlogic" then
			text = "(" .. ttostring(node.left) .. " " .. optostring(node.op) .. " " .. ttostring(node.right) .. ")"
		elseif node.type == "op" then
			text = optostring(node.op) .. "(" .. ttostring(node.left) .. ")"
		elseif node.type == "func" then
			text = node.handler(node)
		elseif node.type == "field" then
			text = "`" .. node.name .. "`"
		elseif node.type == "const" then
			text = node.value
		end
	elseif type(c) == "string" then
		text = "'" .. node .. "'"
	else
		text = node
	end

	return text
end

ExpENV.__tostring = ttostring

local CreateNode = function (t)
	t = t or {}
	local env : { any : any } = ExpENV
	setmetatable(t, env)
	return t
end

ExpENV.Node = function (node)
	assert(type(node) == "string" or type(node) == "table")
	return type(node) == "string" and CreateNode({ type = "field", name = node}) or node
end

ExpENV.__add = function (lhs, rhs)
	return CreateNode({ type = "binop", op = "+", left = lhs, right = rhs })
end

ExpENV.__sub = function (lhs, rhs)
	return CreateNode({ type = "binop", op = "-", left = lhs, right = rhs })
end

ExpENV.__mul = function (lhs, rhs)
	return CreateNode({ type = "binop", op = "*", left = lhs, right = rhs })
end

ExpENV.__div = function (lhs, rhs)
	return CreateNode({ type = "binop", op = "/", left = lhs, right = rhs })
end

ExpENV.__shl = function (lhs, rhs)
	return CreateNode({ type = "binlogic", op = "<", left = lhs, right = rhs })
end

ExpENV.__shr = function (lhs, rhs)
	return CreateNode({ type = "binlogic", op = ">", left = lhs, right = rhs })
end

-- It's equal!
ExpENV.__idiv = function (lhs, rhs)
	return CreateNode({ type = "binlogic", op = "=", left = lhs, right = rhs })
end

ExpENV.__band = function (lhs, rhs)
	return CreateNode({ type = "binlogic", op = "and" , left = rhs, right = rhs})
end

ExpENV.__bor = function (lhs, rhs)
	return CreateNode({ type = "binlogic", op = "or" , left = rhs, right = rhs})
end

ExpENV.__bnot = function (lhs, rhs)
	return CreateNode({ type = "oplogic", op = "not" , left = rhs, right = rhs})
end

ExpENV.__unm = function (lhs)
	return CreateNode({ type = "op", op = "-", left = lhs })
end

ExpENV.Sum = function (node)
	return CreateNode({ type = "op", op = "Sum", left = node })
end

ExpENV.Average = function (node)
	return CreateNode({ type = "op", op = "Average", left = node })
end

ExpENV.Count = function (node)
	return CreateNode({ type = "op", op = "Count", left = node })
end


ExpENV.Min = function (node)
	return CreateNode({ type = "op", op = "Min", left = node })
end

ExpENV.Max = function (node)
	return CreateNode({ type = "op", op = "Max", left = node })
end

ExpENV.Trim = function (node)
	return CreateNode({ type = "op", op = "Trim", left = node })
end

ExpENV.Instr = function (x, y)
	return CreateNode({ type = "func", op = "Instr", handler = function (node)
		return "(Instr(" .. ExpENV.__tostring(x) .. ", " .. ExpENV.__tostring(y) .. "))"
	end})
end

ExpENV.Pow = function (x, y)
	return CreateNode({ type = "func", op = "Pow", handler = function (node)
		return "(Pow(" .. ExpENV.__tostring(x) .. ", " .. ExpENV.__tostring(y) .. "))"
	end})
end

ExpENV.Substr = function (x, y, z)
	return CreateNode({ type = "func", op = "Substr", handler = function (node)
	  local text = "(Substr(" .. ExpENV.__tostring(x) .. ", " .. ExpENV.__tostring(y)
	  if z ~= nil then
		text = text .. ", " .. ExpENV.__tostring(z) .. ""
		end
		return text .. "))" 
	end})
end

ExpENV.IIf = function (node, condTrue, condFalse)
	return CreateNode({ type = "func", op = "IIf", cond = node, left = condTrue, right = condFalse, handler = function (node)
	  return "(Case When(" .. ExpENV.__tostring(node.cond) .. " then (" .. ExpENV.__tostring(node.left) .. ") Else (" .. ExpENV.__tostring(node.right) .. "))"
	end})
end

ExpENV.Sign = function (node)
	return CreateNode({ type = "op", op = "Sign", left = node })
end

ExpENV.Sin = function (node)
	return CreateNode({ type = "op", op = "Sin", left = node })
end

ExpENV.Cos = function (node)
	return CreateNode({ type = "op", op = "Cos", left = node })
end

ExpENV.Tan = function (node)
	return CreateNode({ type = "op", op = "Tan", left = node })
end

ExpENV.Cot = function (node)
	return CreateNode({ type = "op", op = "Cot", left = node })
end

ExpENV.Sec = function (node)
	return CreateNode({ type = "op", op = "Sec", left = node })
end

ExpENV.Csc = function (node)
	return CreateNode({ type = "op", op = "Csc", left = node })
end

ExpENV.Asin = function (node)
	return CreateNode({ type = "op", op = "Asin", left = node })
end

ExpENV.Acos = function (node)
	return CreateNode({ type = "op", op = "Acos", left = node })
end

ExpENV.Atan = function (node)
	return CreateNode({ type = "op", op = "Atan", left = node })
end


ExpENV.Len = function (node)
	return CreateNode({ type = "op", op = "Len", left = node })
end


local Invoke = function (status, ...)
	return ...
end

function GenerateSQL(text)
	text = Preprocess(text)
	-- print("Converted: " .. text)
	local chunk, msg = load("return " .. text, "Test", "t", ExpENV)	
	if chunk then
		return ExpENV.__tostring(Invoke(pcall(chunk))) or ""
	else
		-- print(msg)
		return ""
	end
end


-- Test
-- Testcase.txt
--[[
[Unit Price]
[Unit Price] * [User Count]
Sin[Unit Price] * Count[Unit Price] + [abc]
IIf([abc] > [def], [UnitPrice], [Profit])
Max[dce] + Min[abc] * 44 + IIf([aaa] = [bbb], 'YES', 'NO')
Substr([City], 1, 3) + Substr([Province], 4)
IIf(Instr([City], 'America') > 0, 'True', 'False')
]]


--[[
local file = io.open("Script/TestCase.txt", "r")
for text in file:lines() do
	print("Original: " .. text)
	print("SQL: " .. GenerateSQL(text))
	print("\n")
end
]]

return QuickSQL