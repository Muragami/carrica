/*
	host.c

	wren running under lua 5.1+
	implementation of Host class

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "cls_host.h"

#define WERR(x) { wrenError(vm, x); return; }

// ********************************************************************************
// functions

void hvmName(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.name called on an invalid VM instance")
	if (cvm->wrenName) {
		wrenSetSlotString(vm, 0, cvm->wrenName);
	} else {
		wrenSetSlotString(vm, 0, CARRICA_NAME);
	}
}

void hvmConst(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.const() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_STRING) 
		WERR("Host.const() called with bad parameter, string expected")
	const char *name = wrenGetSlotString(vm, 1);
	lua_pushlightuserdata(cvm->L, cvm->name);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_getfield(cvm->L, -1, name);
	if (lua_type(cvm->L, -1) != LUA_TNIL) {
		// get a ref to this for later
		lua_pushstring(cvm->L, name);
		int r = luaL_ref(cvm->L, -3);
		wrenSetSlotDouble(vm, 0, (double)r);
		lua_pop(cvm->L, 1);
	} else {
		wrenSetSlotDouble(vm, 0, -1);
		lua_pop(cvm->L, 2);
	}
}

void hvmRef(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.ref() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_STRING) 
		WERR("Host.ref() called with bad parameter, string expected")
	const char *name = wrenGetSlotString(vm, 1);
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_getfield(cvm->L, -1, name);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		// get a ref to this for later
		int r = luaL_ref(cvm->L, -2);
		wrenSetSlotDouble(vm, 0, (double)r);
		lua_pop(cvm->L, 1);
	} else {
		wrenSetSlotDouble(vm, 0, -1);
		lua_pop(cvm->L, 2);
	}
}

void hvmCall0(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		lua_call(cvm->L, 0, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall1(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		lua_call(cvm->L, 1, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall2(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		lua_call(cvm->L, 2, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall3(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		lua_call(cvm->L, 3, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall4(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		luaPushFromWrenSlot(cvm, 5);
		lua_call(cvm->L, 4, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall5(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		luaPushFromWrenSlot(cvm, 5);
		luaPushFromWrenSlot(cvm, 6);
		lua_call(cvm->L, 5, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall6(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		luaPushFromWrenSlot(cvm, 5);
		luaPushFromWrenSlot(cvm, 6);
		luaPushFromWrenSlot(cvm, 7);
		lua_call(cvm->L, 6, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall7(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		luaPushFromWrenSlot(cvm, 5);
		luaPushFromWrenSlot(cvm, 6);
		luaPushFromWrenSlot(cvm, 7);
		luaPushFromWrenSlot(cvm, 8);
		lua_call(cvm->L, 7, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

void hvmCall8(WrenVM* vm) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) 
		WERR("Host.call() called on an invalid VM instance")
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_NUM) 
		WERR("Host.call() called with bad parameter, number reference expected as first param")
	int r = (int)wrenGetSlotDouble(vm, 1);
	if (r == -1)
		WERR("Host.call() called with bad parameter, reference passed is invalid")
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	lua_rawgeti(cvm->L, -1, r);
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		luaPushFromWrenSlot(cvm, 2);
		luaPushFromWrenSlot(cvm, 3);
		luaPushFromWrenSlot(cvm, 4);
		luaPushFromWrenSlot(cvm, 5);
		luaPushFromWrenSlot(cvm, 6);
		luaPushFromWrenSlot(cvm, 7);
		luaPushFromWrenSlot(cvm, 8);
		luaPushFromWrenSlot(cvm, 9);
		lua_call(cvm->L, 8, 1);
		// marshal the return into a wren form
		wrenSetSlotFromLua(cvm, 0, -1);
		lua_pop(cvm->L, 2);
	} else {
		lua_pop(cvm->L, 2);
		WERR("Host.call() failed, no handler could be found")
	}
}

// ********************************************************************************
// wrap it all up for Wren

const vmForeignMethodDef _h_func[] = {
	{ true, "name", hvmName },
	{ true, "const(_)", hvmConst },
	{ true, "ref(_)", hvmRef },
	{ true, "call(_)", hvmCall0 },
	{ true, "call(_,_)", hvmCall1 },
	{ true, "call(_,_,_)", hvmCall2 },
	{ true, "call(_,_,_,_)", hvmCall3 },
	{ true, "call(_,_,_,_,_)", hvmCall4 },
	{ true, "call(_,_,_,_,_,_)", hvmCall5 },
	{ true, "call(_,_,_,_,_,_,_)", hvmCall6 },
	{ true, "call(_,_,_,_,_,_,_,_)", hvmCall7 },
	{ true, "call(_,_,_,_,_,_,_,_,_)", hvmCall8 },
	{ false, NULL, NULL }
};

// class methods in this module
const vmForeignMethodTable _h_mtab[] = { 
	{ "Host", _h_func }, 
	{ NULL, NULL } };

// foreign classes in this module
const vmForeignClassDef _h_cdef[] = { 
	{ NULL, { NULL, NULL } } };
const vmForeignClassTable _h_ctab[] = { { _h_cdef } };

const vmForeignModule vmiHost = { _h_mtab, _h_ctab };
