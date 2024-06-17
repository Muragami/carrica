/*
	host.c

	wren running under lua 5.1+
	implementation of Table class

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "cls_table.h"
#include "cls_array.h"
#include "vm.h"

#define WERR(x) { wrenError(vm, x); return; }

vmWrenReference* tvmLuaNewTable(carricaVM *cvm) {
	lua_State *L = cvm->L;
	vmWrenReference* ret = lua_newuserdata(L, VM_REF_SIZE);
	ret->type = VM_WREN_SHARE_TABLE;
	ret->refCount = 1;
	lua_pushlightuserdata(L, ret);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	return ret;
}

// ********************************************************************************
// functions

void tvmGet(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	vmWrenReference *ref = reref->pref;
	const char *str;
	int len;
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, ref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			str = wrenGetSlotBytes(cvm->vm, 1, &len);
			lua_pushlstring(cvm->L, str, len);
			lua_gettable(cvm->L, -2);
			wrenSetSlotFromLua(cvm, 0, -1);
			break;
		case WREN_TYPE_NUM:
			lua_pushnumber(cvm->L, wrenGetSlotDouble(vm, 1));
			lua_gettable(cvm->L, -2);
			wrenSetSlotFromLua(cvm, 0, -1);
			break;
		default:
			wrenError(vm, "bad key passed to Table.get() numbers and strings only");
			break;
	}
	lua_pop(cvm->L, 1);
}

void tvmSet(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	vmWrenReference *ref = reref->pref;
	const char *str;
	int len;
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, ref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			str = wrenGetSlotBytes(cvm->vm, 1, &len);
			lua_pushlstring(cvm->L, str, len);
			luaPushFromWrenSlot(cvm, 2);
			lua_settable(cvm->L, -3);
			break;
		case WREN_TYPE_NUM:
			lua_pushnumber(cvm->L, wrenGetSlotDouble(vm, 1));
			luaPushFromWrenSlot(cvm, 2);
			lua_settable(cvm->L, -3);
			break;
		default:
			wrenError(vm, "bad key passed to Table.set() numbers and strings only");
			break;
	}
	lua_pop(cvm->L, 1);
}

void tvmClear(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_newtable(cvm->L);
	lua_settable(cvm->L, LUA_REGISTRYINDEX);
}

void tvmContainsKey(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	vmWrenReference *ref = reref->pref;
	const char *str;
	int len;
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, ref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_STRING:
			str = wrenGetSlotBytes(cvm->vm, 1, &len);
			lua_pushlstring(cvm->L, str, len);
			lua_gettable(cvm->L, -2);
			wrenSetSlotBool(vm, 0, (lua_type(cvm->L, -1) != LUA_TNIL));
			break;
		case WREN_TYPE_NUM:
			lua_pushnumber(cvm->L, wrenGetSlotDouble(vm, 1));
			lua_gettable(cvm->L, -2);
			wrenSetSlotBool(vm, 0, (lua_type(cvm->L, -1) != LUA_TNIL));
			break;
		default:
			wrenError(vm, "bad key passed to Table.containsKey(), numbers and strings only");
			break;
	}
	lua_pop(cvm->L, 1);
}

void tvmCount(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	double cnt = 0;
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
    lua_pushnil(cvm->L);
    while (lua_next(cvm->L, -2) != 0) {
    	cnt++;
       	lua_pop(cvm->L, 1);
    }
    lua_pop(cvm->L, 2);
	wrenSetSlotDouble(vm, 0, cnt);
}

void tvmKeys(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	lua_pushnil(cvm->L);
    while (lua_next(cvm->L, -2) != 0) {
    	wrenSetSlotFromLua(cvm, 1, -2);
    	wrenInsertInList(vm, 0, -1, 1);
       	lua_pop(cvm->L, 1);
    }
    lua_pop(cvm->L, 2);
}

void tvmValues(WrenVM* vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	lua_pushnil(cvm->L);
    while (lua_next(cvm->L, -2) != 0) {
    	wrenSetSlotFromLua(cvm, 1, -1);
    	wrenInsertInList(vm, 0, -1, 1);
       	lua_pop(cvm->L, 1);
    }
    lua_pop(cvm->L, 2);
}

// create and return a new Table
void tvmAllocate(WrenVM* vm) {
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 0, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_TABLE;
	ref->pref = tvmLuaNewTable(wrenGetUserData(vm));
	ref->vm = wrenGetUserData(vm);
}

// remove a table
void tvmFinalize(void *obj) {
	vmWrenReReference* ref = obj;
	ref->pref->refCount--;
	// last to hold the object? clean it up
	if (ref->pref->refCount == 0) {
		lua_pushlightuserdata(ref->vm->L, ref->pref);
		lua_pushnil(ref->vm->L);
		lua_settable(ref->vm->L, LUA_REGISTRYINDEX);
		free(ref->pref);
	}
}

void tvmHold(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	reref->pref->refCount++;
}

void tvmRelease(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	if (reref->pref->refCount > 0) reref->pref->refCount--;
}

void tvmInsertAll(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	vmWrenReReference *tref = NULL;
	int end = 0;
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	switch (wrenGetSlotType(vm, 1)) {
		case WREN_TYPE_LIST:
			// a passed in list must be [ key, value, key, value, ... ]
			end = wrenGetListCount(vm, 1);
			if (end % 2) {
				wrenError(vm, "bad List passed to Table.insertAll(), unmatched key-value pair");
			} else {
				wrenEnsureSlots(vm, 4);
				for (int i = 0; i < end; i = i + 2) {
					wrenGetListElement(vm, 1, i, 2);
					wrenGetListElement(vm, 1, i + 1, 3);
					luaPushFromWrenSlot(cvm, 2);
					luaPushFromWrenSlot(cvm, 3);
					lua_settable(cvm->L, -3);
				}
			}
			break;
		case WREN_TYPE_FOREIGN:
			tref = wrenGetSlotForeign(vm, 1);
			if (tref->type == VM_WREN_SHARE_TABLE) {
				lua_pushlightuserdata(cvm->L, tref->pref);
				lua_gettable(cvm->L, LUA_REGISTRYINDEX);
				lua_pushnil(cvm->L);
				while (lua_next(cvm->L, -2) != 0) {
					lua_settable(cvm->L, -4);
					lua_pop(cvm->L, 1);
				}
				lua_pop(cvm->L, 2);
				return;
			} else if (tref->type == VM_WREN_SHARE_ARRAY) {
				// a passed array must be [ key, value, key, value, ... ]
				lua_pushlightuserdata(cvm->L, tref->pref);
				lua_gettable(cvm->L, LUA_REGISTRYINDEX);
				end = lua_objlen(cvm->L, -1);
				for (int i = 1; i <= end; i = i + 2) {
					lua_rawgeti(cvm->L, -1, i);
					lua_rawgeti(cvm->L, -1, i + 1);
					lua_settable(cvm->L, -4);
				}
				lua_pop(cvm->L, 2);
				return;
			} else 
				wrenError(vm, "bad foreign class passed to Table.insertAll()");
			break;
		default:
			wrenError(vm, "bad type passed to Table.insertAll()");
	}
	lua_pop(cvm->L, 1);
}

void tvmArray(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	if (cvm->handle.Array == NULL) cvm->handle.Array = lcvmGetClassHandle(cvm, "carrica", "Array");
	wrenEnsureSlots(vm, 3);
	wrenSetSlotHandle(vm, 1, cvm->handle.Array);
	vmWrenReReference *aref = wrenSetSlotNewForeign(vm, 0, 1, VM_REREF_SIZE);
	aref->type = VM_WREN_SHARE_ARRAY;
	aref->pref = avmLuaNewArray(cvm);
	lua_pushlightuserdata(cvm->L, aref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_pushnil(cvm->L);
	int i = 1;
    while (lua_next(cvm->L, -3) != 0) {
    	lua_pushvalue(cvm->L, -2);
    	lua_rawseti(cvm->L, -4, i++);
    	lua_rawseti(cvm->L, -3, i++);
    }
    lua_pop(cvm->L, 2);
}

void tvmList(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	lua_pushnil(cvm->L);
    while (lua_next(cvm->L, -2) != 0) {
    	wrenSetSlotFromLua(cvm, 1, -2);
    	wrenInsertInList(vm, 0, -1, 1);
    	wrenSetSlotFromLua(cvm, 1, -1);
    	wrenInsertInList(vm, 0, -1, 1);
       	lua_pop(cvm->L, 1);
    }
    lua_pop(cvm->L, 2);
}

void tvmIterate(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL) {
		lua_pushnil(cvm->L);
		if (lua_next(cvm->L, -2) != 0) {
			wrenSetSlotBool(vm, 0, true);
			// store this key for the next call
			lua_pushlightuserdata(cvm->L, reref);
			lua_pushvalue(cvm->L, -3);
			lua_settable(cvm->L, LUA_REGISTRYINDEX);
		} else {
			wrenSetSlotBool(vm, 0, false);
			lua_pop(cvm->L, 2);
			return;
		}
	} else {
		// pull the last key out of the registry
		lua_pushlightuserdata(cvm->L, reref);
		lua_gettable(cvm->L, LUA_REGISTRYINDEX);
		if (lua_next(cvm->L, -2) != 0) {
			wrenSetSlotBool(vm, 0, true);
			// store this key for the next call
			lua_pushlightuserdata(cvm->L, reref);
			lua_pushvalue(cvm->L, -3);
			lua_settable(cvm->L, LUA_REGISTRYINDEX);
		} else {
			wrenSetSlotBool(vm, 0, false);
			lua_pop(cvm->L, 2);
			return;
		}
	}
	lua_pop(cvm->L, 3);
}

void tvmIteratorValue(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	// get the stored key
	lua_pushlightuserdata(cvm->L, reref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_pushvalue(cvm->L, -1);
	// pull the value from our table
	lua_gettable(cvm->L, -3);
	// key is at -2, and value is at -1 with the table at -3
	wrenEnsureSlots(vm, 2);
	if (cvm->handle.TableEntry == NULL) cvm->handle.TableEntry = lcvmGetClassHandle(cvm, "carrica", "TableEntry");
	wrenSetSlotHandle(vm, 1, cvm->handle.TableEntry);
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 1, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_TABLE_ENTRY;
	ref->pref = NULL;
	ref->vm = cvm;
	lua_pushlightuserdata(cvm->L, &ref->pref);
	lua_pushvalue(cvm->L, -2);
	lua_settable(cvm->L, LUA_REGISTRYINDEX);
	lua_pop(cvm->L, 1);
	lua_pushlightuserdata(cvm->L, &ref->type);
	lua_pushvalue(cvm->L, -2);
	lua_settable(cvm->L, LUA_REGISTRYINDEX);
	lua_pop(cvm->L, 2);
}

// create and return a new Table
void tevmAllocate(WrenVM* vm) {
	vmWrenReReference* ref = wrenSetSlotNewForeign(vm, 0, 0, VM_REREF_SIZE);
	ref->type = VM_WREN_SHARE_TABLE_ENTRY;
	ref->pref = NULL;
	ref->vm = wrenGetUserData(vm);
}

// remove a table
void tevmFinalize(void *obj) {
	vmWrenReReference* ref = obj;
	// remove the stored lua objects
	lua_State *L = ref->vm->L;
	lua_pushlightuserdata(L, &ref->type);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pushlightuserdata(L, &ref->pref);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
}

void tevmKey(WrenVM *vm) {
	vmWrenReReference *ref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_State *L = cvm->L;
	lua_pushlightuserdata(L, &ref->type);
	lua_gettable(L, LUA_REGISTRYINDEX);
	wrenSetSlotFromLua(cvm, 0, -1);
	lua_pop(L, 1);
}

void tevmValue(WrenVM *vm) {
	vmWrenReReference *ref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_State *L = cvm->L;
	lua_pushlightuserdata(L, &ref->pref);
	lua_gettable(L, LUA_REGISTRYINDEX);
	wrenSetSlotFromLua(cvm, 0, -1);
	lua_pop(L, 1);
}

// ********************************************************************************
// wrap it all up for Wren

const vmForeignMethodDef _t_func[] = {
	{ false, "[_]", tvmGet },
	{ false, "[_]=(_)", tvmSet },
	{ false, "clear()", tvmClear },
	{ false, "containsKey(_)", tvmContainsKey },
	{ false, "count", tvmCount },
	{ false, "keys", tvmKeys },
	{ false, "values", tvmValues },
	{ false, "hold()", tvmHold },
	{ false, "release()", tvmRelease },	
	{ false, "finsertAll(_)", tvmInsertAll },
	{ false, "array", tvmArray },
	{ false, "list", tvmList },
	{ false, "iterate(_)", tvmIterate },
	{ false, "iteratorValue(_)", tvmIteratorValue },	
	{ false, NULL, NULL }
};

const vmForeignMethodDef _te_func[] = {
	{ false, "key", tevmKey },
	{ false, "value", tevmValue },
	{ false, NULL, NULL }
};

// class methods in this module
const vmForeignMethodTable _t_mtab[] = { 
	{ "Table", _t_func }, 
	{ "TableEntry", _te_func }, 
	{ NULL, NULL } };

// foreign classes in this module
const vmForeignClassDef _t_cdef[] = { 
	{ "Table", { tvmAllocate, tvmFinalize } },
	{ "TableEntry", { tevmAllocate, tevmFinalize } },
	{ NULL, { NULL, NULL } } };
const vmForeignClassTable _t_ctab[] = { { _t_cdef } };

const vmForeignModule vmiTable = { _t_mtab, _t_ctab };
