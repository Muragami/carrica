/*
	cls_array.c

	wren running under lua 5.1+
	implementation of Array class

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "cls_array.h"
#include "vm.h"

#define WERR(x) { wrenError(vm, x); return; }

vmWrenReference* avmLuaNewArray(carricaVM *cvm) {
	lua_State *L = cvm->L;
	vmWrenReference* ret = lua_newuserdata(L, VM_REF_SIZE);
	ret->type = VM_WREN_SHARE_ARRAY;
	ret->refCount = 1;
	lua_pushlightuserdata(L, ret);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	return ret;
}

void avmGet(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NUM) {
		lua_pushinteger(cvm->L, (int)wrenGetSlotDouble(vm, 1) + 1);
		lua_gettable(cvm->L, -2);
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
		return;
	} else 
		wrenError(vm, "bad index passed to Array.get() numbers only");
	lua_pop(cvm->L, 1);
}

void avmSet(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NUM) {
		lua_pushinteger(cvm->L, (int)wrenGetSlotDouble(vm, 1) + 1);
		luaPushFromWrenSlot(cvm, 2);
		lua_settable(cvm->L, -3);
	} else
		wrenError(vm, "bad index passed to Array.set() numbers only");
	lua_pop(cvm->L, 1);	
}

void avmClear(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_newtable(cvm->L);
	lua_settable(cvm->L, LUA_REGISTRYINDEX);	
}

void avmCount(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	wrenSetSlotDouble(vm, 0, lua_objlen(cvm->L, -1));
	lua_pop(cvm->L, 1);	
}

void avmFilled(WrenVM *vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (cvm->handle.Array == NULL) cvm->handle.Array = lcvmGetClassHandle(cvm, "carrica", "Array");
	wrenSetSlotHandle(vm, 1, cvm->handle.Array);
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 1, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_ARRAY;
	ref->pref = avmLuaNewArray(wrenGetUserData(vm));
	int cnt = (int)wrenGetSlotDouble(vm, 1);
	lua_pushlightuserdata(cvm->L, ref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	luaPushFromWrenSlot(cvm, 2);
	for (int i = 1; i <= cnt; i++) {
		lua_pushvalue(cvm->L, -1);
		lua_rawseti(cvm->L, -2, i);
	}
	lua_pop(cvm->L, 2);
}

void avmFromList(WrenVM *vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (cvm->handle.Array == NULL) cvm->handle.Array = lcvmGetClassHandle(cvm, "carrica", "Array");
	wrenSetSlotHandle(vm, 1, cvm->handle.Array);
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 1, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_ARRAY;
	ref->pref = avmLuaNewArray(wrenGetUserData(vm));
	int pos = 0;
	wrenEnsureSlots(vm, 2);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_LIST) {
		int cnt = wrenGetListCount(vm, 1);
		int c = 0;
		while (c < cnt) {
			wrenGetListElement(vm, 1, c++, 1);
			luaPushFromWrenSlot(cvm, 1);
			lua_rawseti(cvm->L, -2, ++pos);
		}
	} else
		wrenError(vm, "bad value passed to Array.fromList() list only");
	lua_pop(cvm->L, 2);
}

void avmAdd(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	luaPushFromWrenSlot(cvm, 1);
	lua_rawseti(cvm->L, -1, lua_objlen(cvm->L, -1));
	lua_pop(cvm->L, 1);	
}


void avmAddAll(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int pos = lua_objlen(cvm->L, -1);
	wrenEnsureSlots(vm, 2);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_LIST) {
		int cnt = wrenGetListCount(vm, 1);
		int c = 0;
		while (c < cnt) {
			wrenGetListElement(vm, 1, c++, 1);
			luaPushFromWrenSlot(cvm, 1);
			lua_rawseti(cvm->L, -2, ++pos);
		}
	} else
		wrenError(vm, "bad value passed to Array.addAll() list only");
	lua_pop(cvm->L, 1);
}
void avmPlus(WrenVM *vm) { avmAddAll(vm); }

void avmIndexOf(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int end = lua_objlen(cvm->L, -1);
	int pos = 0;
	luaPushFromWrenSlot(cvm, 1);
	while (pos < end) {
		lua_rawgeti(cvm->L, -2, pos + 1);
		if (lua_equal(cvm->L, -1, -2)) {
			lua_pop(cvm->L, 2);
			wrenSetSlotDouble(vm, 0, pos);
		} else {
			lua_pop(cvm->L, 1);
			pos++;
		}
	}
	lua_pop(cvm->L, 2);
	wrenSetSlotDouble(vm, 0, -1);
}

void avmInsert(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int end = lua_objlen(cvm->L, -1);
	int pos = (int)wrenGetSlotDouble(vm, 1);
	if (pos < 0) {
		pos = end + pos;
		if (pos < 0) {
			lua_pop(cvm->L, 1);
			wrenError(vm, "index out of bounds on Array.insert()");
			return;
		}
	}
	if (pos >= end) {
		lua_pop(cvm->L, 1);
		wrenError(vm, "index out of bounds on Array.insert()");
		return;
	}
	if (pos + 1 == end) {
		// adding to the end, easey peasey
		luaPushFromWrenSlot(cvm, 2);
		lua_rawseti(cvm->L, -2, pos + 1);
	} else {
		// ok we need to move stuff down first
		int mp = pos + 1;
		while (mp < end) {
			lua_rawgeti(cvm->L, -1, mp++);
			lua_rawseti(cvm->L, -1, mp);
		}
		luaPushFromWrenSlot(cvm, 2);
		lua_rawseti(cvm->L, -2, pos + 1);
	}
	lua_pop(cvm->L, 1);
}

void avmRemove(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int end = lua_objlen(cvm->L, -1);
	int pos = 0;
	luaPushFromWrenSlot(cvm, 1);
	while (pos < end) {
		lua_rawgeti(cvm->L, -2, pos + 1);
		if (lua_equal(cvm->L, -1, -2)) {
			lua_pop(cvm->L, 2);
			// found the index, as now remove it
			// just move everything up one
			pos++;
			while (pos < end) {
				lua_rawgeti(cvm->L, -2, pos + 1);
				lua_rawseti(cvm->L, -2, pos);
			}
			// and remove the last element
			lua_pushnil(cvm->L);
			lua_rawseti(cvm->L, -2, pos);
		} else {
			lua_pop(cvm->L, 1);
			pos++;
		}
	}
	lua_pop(cvm->L, 2);
}

void avmRemoveAt(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int end = lua_objlen(cvm->L, -1);
	int pos = (int)wrenGetSlotDouble(vm, 1);
	if (pos < 0) {
		pos = end + pos;
		if (pos < 0) {
			lua_pop(cvm->L, 1);
			wrenError(vm, "index out of bounds on Array.removeAt()");
			return;
		}
	}
	if (pos >= end) {
		lua_pop(cvm->L, 1);
		wrenError(vm, "index out of bounds on Array.removeAt()");
		return;
	}
	if (pos + 1 == end) {
		// removing the end, so really easy
		lua_pushnil(cvm->L);
		lua_rawseti(cvm->L, -2, pos + 1);
	} else {
		// found the index, as now remove it
		// just move everything up one
		pos++;
		while (pos < end) {
			lua_rawgeti(cvm->L, -2, pos + 1);
			lua_rawseti(cvm->L, -2, pos);
		}
		// and remove the last element
		lua_pushnil(cvm->L);
		lua_rawseti(cvm->L, -2, pos);	
	}
	lua_pop(cvm->L, 1);
}

void avmSwap(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	int end = lua_objlen(cvm->L, -1);
	int a = (int)wrenGetSlotDouble(vm, 1);
	int b = (int)wrenGetSlotDouble(vm, 1);
	if (a < 0) {
		a = end + a;
		if (a < 0) {
			lua_pop(cvm->L, 1);
			wrenError(vm, "index a out of bounds on Array.swap()");
			return;
		}
	}
	if (b < 0) {
		b = end + b;
		if (b < 0) {
			lua_pop(cvm->L, 1);
			wrenError(vm, "index b out of bounds on Array.swap()");
			return;
		}
	}
	if (a >= end) {
		lua_pop(cvm->L, 1);
		wrenError(vm, "index a out of bounds on Array.swap()");
		return;
	}
	if (b >= end) {
		lua_pop(cvm->L, 1);
		wrenError(vm, "index b out of bounds on Array.swap()");
		return;
	}
	lua_rawgeti(cvm->L, -1, a + 1);
	lua_rawgeti(cvm->L, -2, b + 1);
	lua_rawseti(cvm->L, -3, a + 1);
	lua_rawseti(cvm->L, -2, b + 1);
	lua_pop(cvm->L, 1);
}

void avmHold(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	reref->pref->refCount++;
}

void avmRelease(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	if (reref->pref->refCount > 0) reref->pref->refCount--;
}

void avmSort(WrenVM *vm) {
	// NYI
	wrenError(vm, "Array.sort() not yet implemented");
}

void avmTimes(WrenVM *vm) {
	// NYI
	wrenError(vm, "Array.*(_) not yet implemented");
}

// create and return a new Table
void avmAllocate(WrenVM* vm) {
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 0, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_ARRAY;
	ref->pref = avmLuaNewArray(wrenGetUserData(vm));
	ref->vm = wrenGetUserData(vm);
}

// remove a table
void avmFinalize(void *obj) {
	vmWrenReReference* ref = obj;
	ref->pref->refCount--;
	if (ref->pref->refCount == 0) {
		lua_pushlightuserdata(ref->vm->L, ref->pref);
		lua_pushnil(ref->vm->L);
		lua_settable(ref->vm->L, LUA_REGISTRYINDEX);
		free(ref->pref);
	}
}

void avmIterate(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	int i = 0;
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) {
		wrenSetSlotDouble(vm, 0, 0);
		return;
	} else {
		i = (int)wrenGetSlotDouble(vm, 1);
		carricaVM *cvm = wrenGetUserData(vm);
		lua_pushlightuserdata(cvm->L, reref->pref);
		lua_gettable(cvm->L, LUA_REGISTRYINDEX);
		if (i >= lua_objlen(cvm->L, -1)) {
			wrenSetSlotBool(vm, 0, false);
			lua_pop(cvm->L, 1);
		} else {
			wrenSetSlotDouble(vm, 0, (double)++i);
		}
		lua_pop(cvm->L, 1);
	}
}

void avmIteratorValue(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, (int)wrenGetSlotDouble(vm, 1) + 1);
	wrenSetSlotFromLua(cvm, 0, -1);
	lua_pop(cvm->L, 2);
}

// ********************************************************************************
// wrap it all up for Wren

const vmForeignMethodDef _a_func[] = {
	{ false, "[_]", avmGet },
	{ false, "[_]=(_)", avmSet },
	{ true, "filled(_,_)", avmFilled },
	{ true, "fromList(_)", avmFromList },
	{ false, "add(_)", avmAdd },
	{ false, "addAll(_)", avmAddAll },
	{ false, "clear()", avmClear },
	{ false, "count", avmCount },
	{ false, "indexOf(_)", avmIndexOf },
	{ false, "insert(_,_)", avmInsert },
	{ false, "remove(_)", avmRemove },
	{ false, "removeAt(_)", avmRemoveAt },
	{ false, "sort()", avmSort },
	{ false, "swap(_,_)", avmSwap },
	{ false, "+(_)", avmPlus },
	{ false, "*(_)", avmTimes },
	{ false, "hold()", avmHold },
	{ false, "release()", avmRelease },
	{ false, "iterate(_)", avmIterate },
	{ false, "iteratorValue(_)", avmIteratorValue },
	{ false, NULL, NULL }
};

// class methods in this module
const vmForeignMethodTable _a_mtab[] = { 
	{ "Array", _a_func }, 
	{ NULL, NULL } };

// foreign classes in this module
const vmForeignClassDef _a_cdef[] = { 
	{ "Array", { avmAllocate, avmFinalize } },
	{ NULL, { NULL, NULL } } };
const vmForeignClassTable _a_ctab[] = { { _a_cdef } };

const vmForeignModule vmiArray = { _a_mtab, _a_ctab };
