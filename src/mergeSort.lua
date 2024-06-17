--[[
	A merge sort routine in lua/luajit
]]

local function mergeHalves(array, first, last)
	local left = first
	local leftTail = math.floor((first + last) / 2)
	local right = leftTail + 1
	local temp = {unpack(array)}

	-- Compare left and right halves of array, sort into temp --
	for i = first, last do
		if (right > last or ((array[left] <= array[right]) and left <= leftTail)) then
			temp[i] = array[left]
			left = left + 1
		else 
			temp[i] = array[right]
			right = right + 1
		end
	end

	-- Copy sorted section back to array --
	for i = first, last do
		array[i] = temp[i]
	end
end

local function mergeSort(array, first, last)
	local first = 1
	local last = #array
	-- Size == 1 --
	if first < last then 
		local middle = math.floor((first + last) / 2)
		mergeSort(array, first, middle)
		mergeSort(array, (middle+1), last)
		mergeHalves(array, first, last)
	end
end

return mergeSort