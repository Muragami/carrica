import "carrica" for Host, Table

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

import "trait" for Const, Var, TypedVar

Const["VERSION"] = Host.version

io.write("\ncarrica version is: " + Const["VERSION"])

Const["VERSION"] = "?"

io.write("\ncarrica version still is: " + Const["VERSION"])

