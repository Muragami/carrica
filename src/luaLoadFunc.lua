return function(name)
	local f = io.open(name .. '.wren', 'r')
    if f == nil then return nil end
    local content = f:read('*all')
    f:close()
    return content
end