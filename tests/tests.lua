--[[
	simple tests for carrica
]]

function readFile(file)
    local f = assert(io.open(file, "r"))
    local content = f:read("*all")
    f:close()
    return content
end

function runTest(file)
    print('\n~~~ TEST: ' .. file .. '\n\n')
    -- create a new vm
    local vm = carrica.newVM(file)
    -- get a file string to execute
    local code = readFile(file)
    -- execute it
    vm:interpret(code)
    print('\n~~~\n')
    vm:release()
end

function customEmit(str)
    io.write("EMIT:: " .. str)
end

-- load the dynamic library
carrica = require 'carrica'
print("Carrica module version: " .. carrica.version())
print("\tdebug: " .. tostring(carrica.hasDebug()))

runTest('simple.wren')
print('\n---\n')

runTest('table.wren')
print('\n---\n')

runTest('array.wren')
print('\n---\n')

carrica.setDebugEmit(customEmit)
runTest('simple.wren')
print('\n---\n')

-- add a shared module
carrica.installModule('trait', readFile('trait.wren'))

runTest('final.wren')
print('\n---\n')

print('tests.lua complete\n')