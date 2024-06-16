
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

var featureMap = {
	"simple":  "carrica is a simple linkage between lua host and wren client",
	"luajit":  "carrica targets luajit or lua 5.1",
  	"table":   "carrica Table is a lua table (keymap), accessible from Wren",
  	"array":   "carrica Array is a lua table (intmap), accessible from Wren"
}

// this creates a lua table form of the map
var featureTable = Table.fromMap(featureMap)

io.write("Features:")
for (entry in featureMap) {
	// we pull the key value from the lua shared table here, instead of the source
	io.write("\t" + entry.key + ":\t" + featureTable[entry.key])
}

io.write("\nfeatureTable contains " + featureTable.count.toString + " keys")
io.write("They are:")

for (k in featureTable.keys) {
	io.write("\t" + k)
}