local Test = record end

function Test.Main()
    local component = ComputeComponentModule.New()
    local func = component:Load("print(...) return 2345")
    component:Call(func, "1234")
    component:CallAsync(function (value)
        print("RET: " .. tostring(value))
    end, func, "0123")
end

return Test