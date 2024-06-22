
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



