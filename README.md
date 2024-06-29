# carrica
Carrica is [Wren](https://wren.io/) hosted by [LuaJIT](https://luajit.org/), a bridge between languages. The name is
a derivitive of the Portuguese word for Wren: carriça (say KAH-HE-SA). It primarily targets the model of lua calling
Wren, and not vice-versa. The whole system is written in C and compiled into a binary module you can load from LuaJIT.
While there are many planned features, this first limited version for testing has only support for three classes in
Wren: Host, Table, and Array. A roadmap is provided below of planned features and releases.

# getting started
When you download the repo you have the needed C source and a simple Makefile to build. This Makefile can detect 
Windows, Linux, and Mac OS platforms and adjusts the build as needed. It is primarily developed under MSYS2 (ucrt64)
on Windows, but I will be testing and making builds on at least Debian 12 and Mac OS.

# lua - the module itself
Once you have a compiled module, you simply need to load it into the lua state with the normal use of require. The
loaded modules exposes the following functions:
```lua
     carrica.newVM()
     carrica.newVM(name)
```
Creates a new Wren VM with a given name (or an automatically generated name equal to it's id number in the
global internal table of all VMs, such that the first created VM is named "0").
```lua
     carrica.version()
```
Returns the version string for the built module, in the format "X.X.X [codename]" currently returning
"0.1.0 Tenma" for the initial testing version.
```lua
     carrica.hasDebug()
```
Returns a boolean which reports if the module was compiled with VM_DEBUG. If so, this allows a lot of internal
messages to be logged which can assist debugging any issues.
```lua
     carrica.setDebugEmit(func)
```
Sets a function to be called each time the debug version of the module emits a message. If not set the module
calls C printf() internally for all output.
```lua
     carrica.installModule(name, codeString)
```
Installs a shared Wren module for all VMs with the given name and Wren code.

# lua - the VM
A VM returned from carrica.newVM() has quite a few functions which allow you to interact with the contained
Wren VM inside.
```lua
     vm:release()
     vm:isValid()
     vm:renew()
```
These functions all have to do with the core functionality of the VM contained. If you call .release() the
internal VM is released and the container is set to an invalid state - it is an error to call most functions
on such a vm: However you can call .isValid() to determine if a VM is released or not, and you can call
.renew() on an invalid state to create a new internal one.
```lua
     vm:setLoadFunction(func)
     vm:setLoadFunction(modeString)
```
You can use this function to install a function (or mode of operation) that is called when 'import "XModule"'
is called from Wren and XModule is not found internally. A function passed in here accepts the string name
of the module and either returns a Wren code string or nil if the module does not exist. In addition, you
can select either of two operating modes: "lua.filesystem" and "love.filesystem" which will install functions
internally to use io.open() or love.filesystem.read() respectively to resolve missing modules.
```lua
     vm:handler(funcTable)
     vm:handler(funcName, func)
```
Call this to install handler functions that can be called from Wren using the Host static class. If you pass
a table, each string key with a matching function is added to the internal mapping. If you pass a name
string and a function, that is added to the internal mapping. Be default, two handlers are installed into
the VM: 'write' which calls lua print(), and 'error' which calls lua error(). You can override these with
your own functions at will.
```lua
     vm:interpret(codeString)
     vm:interpret(codeString, moduleName)
```
Interprets the Wren code passed in codeString, either as module "main" by default, or your own module name
passed in parameter 2.
```lua
     func = vm:getMethod(moduleName, className, methodSig)
     vm:freeMethod(moduleName, className, methodSig)
```
Locate the 'className.methodSig' method in module 'moduleName' - methodSig is a full Wren signature. This
returns a function that calls that method when it is called in lua. Calling .freeMethod releases the internal
mapping, and calling func() after that will fail in a spectacular manner.
```lua
     vm:hasVariable(moduleName, varName)
     vm:hasModule(moduleName)
```
Returns true is the VM contains a given module, or a given top-level variable in a module, false if it does
not.
```lua
     myArray = vm:newArray()
     myTable = vm:newTable()
```
Creates and returns a Array or Table that can be sent to and from the internal Wren vm: Due to performance
concerns, carrica will not marshal table types between Wren and lua. Instead these create shared table types:
Array being a Wren List and Table being a Wren Map 'alike', but the data is purely on the lua side.
```lua
     array.ref()
     array.hold()
     array.release()
```
Both Array and Table types have the following functions in lua. .ref() returns the underlying table for the
shared object. .hold() increases the ref count inside to make sure it is not garbage collected until wanted,
and .release() decreases the ref count inside so it can be garbage collected.

# Wren - the carrica module
When you provide Wren code to the carrica VM, it has access to the following module name "carrica":
```wren
foreign class Array {
	construct new() { }
	construct copy(other) { addAll(other) }
	foreign static filled(size, element)
	foreign static fromList(list)

	foreign add(item)
	foreign addAll(other)
	foreign clear()
	foreign count
	foreign indexOf(value)
	foreign insert(index, item)
	foreign remove(value)
	foreign removeAt(index)
	foreign sort()
	foreign swap(a, b)
	foreign +(other)
	foreign *(count)
	foreign list
	foreign hold()
	foreign release()
	foreign iterate(iter)
	foreign iteratorValue(iter)

	foreign [idx]
	foreign [idx]=(value)
}

foreign class TableEntry {
	construct new() { }

	foreign key
	foreign value
}

// this is a lua/host side table
foreign class Table {
	construct new() { }

	construct copy(other) { insertAll(other) }

	static fromMap(map) {
		var ret = Table.new()
		for (entry in map) {
			ret[entry.key] = entry.value
		}
		return ret
	}

	foreign clear()
	foreign containsKey(key)
	foreign count
	foreign keys
	foreign values
	foreign array
	foreign list
	foreign hold()
	foreign release()
	foreign finsertAll(other)
	foreign iterate(iter)
	foreign iteratorValue(iter)

	insertAll(other) {
		if (other is Map) {
			for (entry in other) {
				this[entry.key] = entry.value
			}
		} else finsertAll(other)
	}

	foreign [key]
	foreign [key]=(value)
}

// this allows you to make calls on the host via 'handlers' installed
class Host {
	// get a reference to a handler from it's name
	foreign static ref(name)
	// make a call via a handler reference
	foreign static call(ref)
	foreign static call(ref, a)
	foreign static call(ref, a, b)
	foreign static call(ref, a, b, c)
	foreign static call(ref, a, b, c, d)
	foreign static call(ref, a, b, c, d, e)
	foreign static call(ref, a, b, c, d, e, f)
	foreign static call(ref, a, b, c, d, e, f, g)
	foreign static call(ref, a, b, c, d, e, f, g, h)
}
```
This means you can do something like the following from Wren (this is the simple.wren file from under /tests):
```wren
import "carrica" for Host

var printRef = Host.ref("write")

Host.call(printRef, "\nHello world from carrica!")
```
The Host class is a static foreign object that allows you to call back to lua from Wren, though because of
differences in VM design it is a bit ugly. First you get a reference to a handler (number which is -1 if
invalid) from Host.ref("funcName"). Then you can make any calls to that handler with Host.call(ref, ...),
and the call system accepts 0 to 8 variables to pass.

## Tables
A Table is a lua table that exists in the lua VM, but with a foreign class that allows you to access it
from Wren. The provided Wren interface mirrors most of Wren Map functionality. You must pass Tables to
the Host, and not Maps. The system will not marshal the object for you due to performance issues.

## Arrays
A Array is a lua table that exists in the lua VM, and only contains numeric keys (1 based on the lua side,
0 based on the Wren side). The provided Wren interface mirrors most of Wren List functionality. You must pass
Arrays to the Host, and not Lists. The system will not marshal the object for you due to performance issues.

# roadmap of features
Once this testing version is tested and debugged, I'll move on to adding features in coming releases. This is
the roadmap as it currently stands, subject to change:
* 0.1.0 Tenma - testing and debug of the base system and Host, Array, Table Wren classes.
* 0.2.0 Michiru - exposed system for lua to "get" Wren classes/class objects.
* 0.3.0 Uruka - fully functional threading and thread safety.
* 0.4.0 Iora - Buffer Wren class for a shared binary memory buffer.
* 0.5.0 Ashella - fully implemented support for loading Wren binary modules.
* 0.6.0 Nasa - ???
* 0.7.0 Pippa - (final development version) ???
