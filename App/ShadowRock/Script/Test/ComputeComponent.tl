local Test = record end

function Test.Main()
    local component = ComputeComponentModule.New()
    local func = component:Load("print(...)")
    component:Call(func)
end

return Test