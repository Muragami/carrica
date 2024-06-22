
import "carrica" for Host, Array

// a simple way to wrap around host into something nicer for usage
class IO {
	construct new() {
		_wref = Host.ref("write")
	}

	write(str) {
		Host.call(_wref, str)
	}
}

var io = IO.new()
io.write("\nHello world from Wren under carrica!\n")

var numList = [ 1, 5, 2, 7, 3, 9, 8, 6, 4, 10 ]

var numArray = Array.fromList(numList)

var i = 0
var end = numArray.count

io.write("\ncarrica array contents:!\n")
while (i < end) {
	io.write("\t#" + i.toString + " = " + numArray[i].toString)
	i = i + 1
}

numArray.sort()

i = 0
end = numArray.count
io.write("\ncarrica array, sorted:!\n")
while (i < end) {
	io.write("\t#" + i.toString + " = " + numArray[i].toString)
	i = i + 1
}

end = numArray.count / 2
i = 0
while (i < end) {
	numArray.swap(i, i + 5)
	i = i + 1
}

i = 0
end = numArray.count
io.write("\ncarrica array, half-swapped:!\n")
while (i < end) {
	io.write("\t#" + i.toString + " = " + numArray[i].toString)
	i = i + 1
}

numArray.clear()
io.write("\ncleared carrica array has " + numArray.count.toString + " keys.\n")

numArray.addAll(numList)

io.write("\n.addAll() carrica array has " + numArray.count.toString + " keys.\n")

i = 0
end = numArray.count
io.write("\ncarrica array contents:!\n")
while (i < end) {
	io.write("\t#" + i.toString + " = " + numArray[i].toString)
	i = i + 1
}