/*
	host.c

	wren running under lua 5.1+
	implementation of Table class

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "cls_table.h"
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
	// free our pointer to the table
	free(ref);
}

void tvmHold(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	reref->pref->refCount++;
}

void tvmRelease(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	if (reref->pref->refCount > 0) reref->pref->refCount--;
}

void tvmIterate(WrenVM *vm) {
	vmWrenReReference *reref = wrenGetSlotForeign(vm, 0);
	carricaVM *cvm = wrenGetUserData(vm);
	lua_pushlightuserdata(cvm->L, reref->pref);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	if (wrenGetSlotType(vm, 1) == WREN_TYPE_NULL) {
		lua_pushnil(cvm->L);
		if (lua_next(cvm->L, -2)) {
			wrenSetSlotFromLua(cvm, 0, -2);
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
		if (lua_next(cvm->L, -2)) {
			wrenSetSlotFromLua(cvm, 0, -2);
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
	lua_gettable(cvm->L, -2);
	// key is at -2, and value is at -1 with the table at -3
	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewMap(vm, 0);
	wrenSetSlotString(vm, 1, "key");
	wrenSetSlotFromLua(cvm, 2, -2);
	wrenSetMapValue(vm, 0, 1, 2);
	wrenSetSlotString(vm, 1, "value");
	wrenSetSlotFromLua(cvm, 2, -1);
	wrenSetMapValue(vm, 0, 1, 2);
	lua_pop(cvm->L, 3);
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
	{ false, "iterate(_)", tvmIterate },
	{ false, "iteratorValue(_)", tvmIteratorValue },	
	{ false, NULL, NULL }
};

// class methods in this module
const vmForeignMethodTable _t_mtab[] = { 
	{ "Table", _t_func }, 
	{ NULL, NULL } };

// foreign classes in this module
const vmForeignClassDef _t_cdef[] = { 
	{ "Table", { tvmAllocate, tvmFinalize } },
	{ NULL, { NULL, NULL } } };
const vmForeignClassTable _t_ctab[] = { { _t_cdef } };

const vmForeignModule vmiTable = { _t_mtab, _t_ctab };
