return function(name)
	return love.filesystem.read(name .. '.wren')
end