
/* Cloneable is an abstract class which enables child classes to automatically be
   recognized as 'cloneable' by overriding the 'clone' method.
*/
class Cloneable {
    clone() { this } /* to be overridden by child class */
}

/* CloneableSeq is an abstract class which enables child classes to automatically be
   recognized as both Sequences and 'cloneable' by overriding the 'clone' method.
*/
class CloneableSeq is Sequence {
    clone() { this } /* to be overridden by child class */
}

/*
    Comparable is an abstract class which enables child classes to automatically
    inherit the comparison operators by just overriding the 'compare' method.
    Comparable itself inherits from Cloneable though if one does not wish to override
    the 'clone' method, it will just return the current object by default.
*/
class Comparable is Cloneable {
    compare(other) {
        // This should be overridden in child classes to return -1, 0 or +1
        // depending on whether this < other, this == other or this > other.
    }

    < (other) { compare(other) <  0 }
    > (other) { compare(other) >  0 }
    <=(other) { compare(other) <= 0 }
    >=(other) { compare(other) >= 0 }
    ==(other) { compare(other) == 0 }
    !=(other) { compare(other) != 0 }
}

/*  ByRef wraps a value to enable it to be passed by reference to a method or function. */
class ByRef {
    // Constructs a new ByRef object.
    construct new(value) {
        _value = value
    }

    // Properties to get and set the value of the current instance.
    value     { _value }
    value=(v) { _value = v }

    // Returns the string representation of the current instance's value.
    toString { _value.toString }
}

/*
    ByKey wraps a reference type object to enable it to be used as a Map key. It does this
    by using a counter to create a unique integral key for each such object and maintaining
    an internal map which enables the object to be quickly retrieved from its key.
*/
class ByKey {
    // Retrieves a ByKey object from its key. Returns null if the key is not present.
    static [key] { __map ? __map[key] : null }

    // Retrieves a ByKey object's key from the object it wraps.
    // If the same object has been wrapped more than once, returns the first key found which may
    // not be the lowest. Returns 0 if the object is unwrapped.
    static findKey(obj) {
        for (me in __map) {
            if (me.value.obj == obj) return me.key
        }
        return 0
    }

    // Returns the number of objects currently in the internal map.
    static mapCount { __tally ? __map.count : 0 }

    // Returns the total number of keys created to date.
    static keyCount { __tally ? __tally : 0 }

    // Constructs a new ByKey object.
    construct new(obj) {
        _obj = obj
        if (!__tally) {
            __map = {}
            __tally = 1
        } else {
            __tally = __tally + 1
        }
        _key = __tally
        __map[_key] = this
    }

    // Properties
    obj { _obj }  // returns the current instance
    key { _key }  // returns the current instance's key

    // Removes the current instance from the internal map and sets it to null.
    unkey() {
        __map.remove(_key)
        _obj = null
    }

    // Returns the string representation of the current instance.
    toString { _obj.toString }
}

/*
    Const represents a group of individually named read-only values which are
    stored internally in a map. Any attempt to change such a value is ignored
    though they can be removed from the map.
*/
class Const {
    // Returns the value of 'name' if it is present in the internal map
    // or null otherwise.
    static [name] { (__cmap && __cmap.containsKey(name)) ? __cmap[name] : null }

    // Associates 'value' with 'name' in the internal map.
    // If 'name' is already in the map, the change is ignored.
    static [name]=(value) {
        if (!__cmap) __cmap = {}
        if (!__cmap.containsKey(name)) {
            __cmap[name] = value
        }
    }

    // Removes 'name' and its associated value from the internal map and returns
    // that value. If 'name' was not present in the map, returns null.
    static remove(name) { __cmap.remove(name) }

    // Returns a list of the entries (name/value pairs) in the internal map.
    static entries { __cmap.toList }
}

/*
    Var represents a group of individually named read/write values which are
    stored internally in a map. It can be used to simulate the creation of
    variables at runtime. It can also be used to allow variable names which would
    otherwise be illegal in Wren such as those which include non-ASCII characters.
*/
class Var {
    // Returns the value of 'name' if it is present in the internal map
    // or null otherwise.
    static [name] { (__vmap && __vmap.containsKey(name)) ? __vmap[name] : null }

    // Associates 'value' with 'name' in the internal map.
    // Any existing value is replaced.
    static [name]=(value) {
        if (!__vmap) __vmap = {}
        __vmap[name] = value
    }

    // Removes 'name' and its associated value from the internal map and returns
    // that value. If 'name' was not present in the map, returns null.
    static remove(name) { __vmap.remove(name) }

    // Returns a list of the entries (name/value pairs) in the internal map.
    static entries { __vmap.toList }
}

/* TypedVar is similar to Var except that simulated variables always retain the
   same type as they had when they were originally created.
*/
class TypedVar {
    // Returns the value of 'name' if it is present in the internal map
    // or null otherwise.
    static [name] { (__vmap && __vmap.containsKey(name)) ? __vmap[name] : null }

    // Associates 'value' with 'name' in the internal map.
    // Any existing value is replaced. However, it is an error to attempt to replace
    // an existing value with a value of a different type.
    static [name]=(value) {
        if (!__vmap) __vmap = {}
        if (!__vmap.containsKey(name)) {
            __vmap[name] = value
        } else {
            var vtype = value.type
            var mtype = __vmap[name].type
            if (vtype != mtype) {
                Fiber.abort("Expecting an item of type %(mtype), got %(vtype).")
            } else {
                __vmap[name] = value
            }
        }
    }

    // Removes 'name' and its associated value from the internal map and returns
    // that value. If 'name' was not present in the map, returns null.
    static remove(name) { __vmap.remove(name) }

    // Returns a list of the entries (name/value pairs) in the internal map.
    static entries { __vmap.toList }
}