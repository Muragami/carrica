/*
	We have some linkage here to allow the wren code to interface to the host (lua in this case)
*/


// this is a lua/host side array
foreign class Array {
	construct new() { }
	construct copy(other) { addAll(other) }
	foreign static filled(size, element)
	foreign static fromList(list)

	foreign [idx]
	foreign [idx]=(value)
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

	foreign [key]
	foreign [key]=(value)
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

	static version { "0.1.0 Tenma" }
}

/*

// NYI: this is a lua/host side buffer memory block
foreign class Buffer {
	construct new(sz) { size(sz) }
	foreign [idx]
	foreign [idx]=(value)
	foreign clear()
	foreign count
	foreign mode
	foreign setMode(mode)
	foreign size(count)
	foreign copy(other)
	foreign slice(start, end)
	foreign hold()
	foreign release()
}

*/