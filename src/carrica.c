/*
	carrica.h

	wren running under lua/luajit 5.1+

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "vm.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static lua_State *mstate;
static int mEmitRef = -1;
static int mSortRef = -1;
static char emitBuffer[256];

// this clobbers slot 0 in wren, FWIW
// since Wren isn't reentrant, not an issue at the moment
WrenHandle* lcvmGetClassHandle(carricaVM *cvm, const char* module, const char* name) {
#ifdef CARRICA_SAFETY
	if (!wrenHasModule(cvm->vm, module) || !wrenHasVariable(cvm->vm, module, name)) 
		// something is horribly wrong...
		luaL_error(cvm->L, "carrica -> could not find '%s' class in module '%s' in Wren vm",
								name, module);
#endif
	wrenGetVariable(cvm->vm, module, name, 0);
	return wrenGetSlotHandle(cvm->vm, 0);
}

int lctGC(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_STABLE);
	ref->refCount--;
	if (ref->refCount == 0) {
		lua_pushlightuserdata(L, ref);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);
		free(ref);
	}
	return 0;
}

int lctRef(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_STABLE);
	lua_pushlightuserdata(L, ref);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}

int lctHold(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_STABLE);
	ref->refCount++;
	return 0;
}

int lctRelease(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_STABLE);
	if (ref->refCount > 0) ref->refCount--;
	return 0;
}

luaL_Reg lctfunc[] = {
	{ "ref", lctRef },
	{ "hold", lctHold },
	{ "release", lctRelease },
	{ NULL, NULL }
};

int lcaGC(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_STABLE);
	ref->refCount--;
	if (ref->refCount == 0) {
		lua_pushlightuserdata(L, ref);
		lua_pushnil(L);
		lua_settable(L, LUA_REGISTRYINDEX);
		free(ref);
	}
	return 0;
}

int lcaRef(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_SARRAY);
	lua_pushlightuserdata(L, ref);
	lua_gettable(L, LUA_REGISTRYINDEX);
	return 1;
}

int lcaHold(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_SARRAY);
	ref->refCount++;
	return 0;
}

int lcaRelease(lua_State* L) {
	vmWrenReference *ref = luaL_checkudata(L, 1, LUA_NAME_SARRAY);
	if (ref->refCount > 0) ref->refCount--;
	return 0;
}


luaL_Reg lcafunc[] = {
	{ "ref", lcaRef },
	{ "hold", lcaHold },
	{ "release", lcaRelease },
	{ NULL, NULL }
};

int lcvmRelease(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".release()");
	vmRelease(cvm);
	return 0;
}

int lcvmIsValid(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	lua_pushboolean(L, vmIsValid(cvm));
	return 1;
}

int lcvmRenew(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an VALID VM instance", ".renew()");
	const char *name = lua_tostring(L, 2);
	vmNew(cvm->L, cvm, name);
	return 0;
}

const char *luaLoadFunc = 
#include "luaLoadFunc.lua.txt"
;

const char *loveLoadFunc = 
#include "loveLoadFunc.lua.txt"
;

int lcvmSetLoadFunction(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an VALID VM instance", ".setLoadFunction()");
	if (lua_isnil(L, 2)) {
		if (cvm->refs.loadModule) {
			lua_pushlightuserdata(L, cvm->refs.loadModule);
			lua_pushnil(L);
			lua_settable(L, LUA_REGISTRYINDEX);
			cvm->refs.loadModule = NULL;
		}
	} else if (lua_isfunction(L, 2)) {
		cvm->refs.loadModule = &cvm->refs.loadModule;
		lua_pushlightuserdata(L, cvm->refs.loadModule);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	} else if (lua_isstring(L, 2)) {
		const char* txt = lua_tostring(L, 2);
		cvm->refs.loadModule = &cvm->refs.loadModule;
		lua_pushlightuserdata(L, cvm->refs.loadModule);
		if (!strcmp(txt, "lua.filesystem")) {
			luaL_dostring (L, luaLoadFunc);
		} else if (!strcmp(txt, "love.filesystem")) {
			luaL_dostring (L, loveLoadFunc);
		} else {
			luaL_error(L, "carrica -> %s called with bad parameter: '%s'", ".setLoadFunction()", txt);
		}
		lua_settable(L, LUA_REGISTRYINDEX);
	} else {
		luaL_error(L, "carrica -> %s called with bad parameter", ".setLoadFunction()");
	}
	return 0;
}

int lcvmHandler(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".handler()");
	if (lua_type(L, 2) == LUA_TTABLE) {
		// called as .handler(tableOfHandlers)
		lua_pushlightuserdata(L, cvm);
		lua_gettable(L, LUA_REGISTRYINDEX);
		int t = lua_gettop(L);
		// iterate over the string keys in the table
		lua_pushnil(L);  /* first key */
     	while (lua_next(L, 2) != 0) {
       		/* uses 'key' (at index -2) and 'value' (at index -1) */
       		if (lua_type(L, -2) == LUA_TSTRING) {
       			// we only work with strings
       			// store the function in the registry table
       			lua_pushvalue(L, -2);
       			lua_pushvalue(L, -2);
       			// key and value duplicated on the stack, set now
       			lua_settable(L, t);
       		}
       		// pop the value and use the key to find the next key and repeat
       		lua_pop(L, 1);
     	}
     	lua_pop(L, 2); // pop the last key and the table
	} else if (lua_type(L, 2) == LUA_TSTRING) {
		const char *name = luaL_checkstring(L, 2);
		// called as .handler("error", errorFunction)
		lua_pushlightuserdata(L, cvm);
		lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushstring(L, name);
		lua_pushvalue(L, 3);
		lua_settable(L, -3);
		lua_pop(L, 1);
	}
	luaL_error(L, "carrica -> badly formatted call to %s", ".handler()");
	return 0;
}

int lcvmInterpret(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".interpret()");
	const char *code = luaL_checkstring(L, 2);
	const char *module = "main";
	if (lua_type(L, 3) == LUA_TSTRING) module = lua_tostring(L, 3);
	vmInterpret(cvm, code, module);
	return 0;
}

int callMethod(lua_State* L) {
	carricaVM *cvm = lua_touserdata(L, lua_upvalueindex(1));
	vmWrenMethod *p = lua_touserdata(L, lua_upvalueindex(2));
	vmCallMethodFromLua(cvm, p, lua_gettop(L));
	if (wrenSlotIsLuaSafe(cvm, 0) ) {
		luaPushFromWrenSlot(cvm, 0);
		return 1;
	}
	return 0;
}

int lcvmGetMethod(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".call()");
	const char *moduleName = luaL_checkstring(L, 2);
	const char *className = luaL_checkstring(L, 3);
	const char *methodSig = luaL_checkstring(L, 4);
	vmWrenMethod *p = vmGetMethod(cvm, moduleName, className, methodSig);
	lua_pushlightuserdata(L, cvm);
	lua_pushlightuserdata(L, p);
	lua_pushcclosure(L, callMethod, 2);
	return 1;
}

int lcvmFreeMethod(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".call()");
	const char *moduleName = luaL_checkstring(L, 2);
	const char *className = luaL_checkstring(L, 3);
	const char *methodSig = luaL_checkstring(L, 4);
	static char buffer[256];
	snprintf(buffer, 256, "%s:%s.%s", moduleName, className, methodSig);
	vmWrenMethod *ret = NULL;
	HASH_FIND_STR(cvm->methodHash, buffer, ret);
	if (ret) {
		HASH_DEL(cvm->methodHash, ret);
		if (ret->hClass) wrenReleaseHandle(cvm->vm, ret->hClass);
		if (ret->hMethod) wrenReleaseHandle(cvm->vm, ret->hMethod);
		free(ret);
	}
	return 0;
}

int lcvmHasVariable(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".hasVariable()");
	const char *module = luaL_checkstring(L, 2);
	const char *name = luaL_checkstring(L, 3);
	lua_pushboolean(L, vmHasVariable(cvm, module, name));
	return 1;
}

int lcvmHasModule(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".hasModule()");
	const char *name = luaL_checkstring(L, 2);
	lua_pushboolean(L, vmHasModule(cvm, name));
	return 1;
}

int lcvmGC(lua_State* L) {
	carricaVM *vm = lua_touserdata(L, 1);
	vmRelease(vm);
	return 0;
}

int lcvmNewArray(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	vmWrenReference *ref = lua_newuserdata(L, VM_REF_SIZE);
	ref->type = VM_WREN_SHARE_ARRAY;
	lua_pushlightuserdata(L, ref);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	// table is tucked away in the registry for later, now attach the class metatable
	luaL_getmetatable (L, LUA_NAME_STABLE);
	lua_setmetatable(L, -2);
	// now we need to make a wren side reference for this object
	wrenEnsureSlots(cvm->vm, 2);
	if (cvm->handle.Array == NULL) cvm->handle.Array = lcvmGetClassHandle(cvm, "carrica", "Array");
	wrenSetSlotHandle(cvm->vm, 1, cvm->handle.Array);
	vmWrenReReference *reref = wrenSetSlotNewForeign(cvm->vm, 0, 1, VM_REREF_SIZE);
	reref->type = VM_WREN_SHARE_ARRAY;
	reref->pref = ref;
	reref->vm = cvm;
	ref->refCount = 1;
	ref->handle = wrenGetSlotHandle(cvm->vm, 0);
	// all good, return the userdata
	return 1;
}

int lcvmNewTable(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	vmWrenReference *ref = lua_newuserdata(L, VM_REF_SIZE);
	ref->type = VM_WREN_SHARE_TABLE;
	lua_pushlightuserdata(L, ref);
	lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	// table is tucked away in the registry for later, now attach the class metatable
	luaL_getmetatable (L, LUA_NAME_STABLE);
	lua_setmetatable(L, -2);
	// now we need to make a wren side reference for this object
	wrenEnsureSlots(cvm->vm, 2);
	if (cvm->handle.Table == NULL) cvm->handle.Table = lcvmGetClassHandle(cvm, "carrica", "Table");
	wrenSetSlotHandle(cvm->vm, 1, cvm->handle.Table);
	vmWrenReReference *reref = wrenSetSlotNewForeign(cvm->vm, 0, 1, VM_REREF_SIZE);
	reref->type = VM_WREN_SHARE_TABLE;
	reref->pref = ref;
	reref->vm = cvm;
	ref->refCount = 1;
	ref->handle = wrenGetSlotHandle(cvm->vm, 0);
	// all good, return the userdata
	return 1;
}

// NYI
int lcvmGetClass(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".getClassObj()");
	luaL_error(L, "carrica -> vm.getClassObj() not yet implemented");
	return 0;
}

/// NYI
int lcvmFreeClass(lua_State* L) {
	carricaVM *cvm = luaL_checkudata (L, 1, LUA_NAME_WRENVM);
	if (!vmIsValid(cvm)) luaL_error(L, "carrica -> %s called on an invalid VM instance", ".freeClassObj()");
	luaL_error(L, "carrica -> vm.freeClassObj() not yet implemented");
	return 0;
}

luaL_Reg lcvmfunc[] = {
	// catch and release basics
	{ "release", lcvmRelease },					// release this VM, making it empty
	{ "isValid", lcvmIsValid },					// is this a valid VM or released? (valid call to released VMs)
	{ "renew", lcvmRenew },						// re-initialize this VM, only if released earlier
	// now for low level cantrips and such
	{ "setLoadFunction", lcvmSetLoadFunction },	// set a function that gets called when a module needs loaded
	{ "handler", lcvmHandler },					// set handlers for this VM
	{ "interpret", lcvmInterpret },				// interpret code in the VM
	{ "getMethod", lcvmGetMethod },				// get a method as a lua function
	{ "freeMethod", lcvmFreeMethod },			// get a method as a lua function
	{ "hasVariable", lcvmHasVariable },			// VM has a variable (top level)
	{ "hasModule", lcvmHasModule },				// VM has a given module
	{ "newArray", lcvmNewArray },				// create a new shared array
	{ "newTable", lcvmNewTable },				// create a new shared table
	// higher level magicks
	{ "getClassObj", lcvmGetClass },			// get a 'proper' class object from the VM
	{ "freeClassObj", lcvmFreeClass },			// free a 'proper' class object from the VM
    { NULL, NULL }
};

int lcNewVM(lua_State* L) {
	carricaVM *vm = lua_newuserdata (L, VM_BYTE_SIZE);
	const char *name = lua_tostring(L, 1);
	vmNew(L, vm, name);
	// add default handlers
	lua_pushlightuserdata(L, vm);
	lua_gettable(L, LUA_REGISTRYINDEX);
		lua_pushstring(L, "write");
		lua_getglobal(L, "print");
	lua_settable(L, -3);
		lua_pushstring(L, "error");
		lua_getglobal(L, "error");
	lua_settable(L, -3);
	lua_pop(L, 1);	// remove our registry table, so we can return the new VM
	luaL_getmetatable (L, LUA_NAME_WRENVM);
	lua_setmetatable(L, -2);
	return 1;
}

int lcVersion(lua_State* L) { lua_pushstring(L, CARRICA_VERSION); return 1; }
int lcHasDebug(lua_State* L) { lua_pushboolean(L, vmHasDebug()); return 1; }

int lcEmitDebug(const char *s, ...) {
	int ret;
	va_list args;
	va_start(args, s);
	ret = vsnprintf(emitBuffer, 256, s, args);
	va_end(args);
	if (mEmitRef != -1) {
		lua_pushlightuserdata(mstate, &mEmitRef);
		lua_gettable(mstate, LUA_REGISTRYINDEX);
		lua_rawgeti(mstate, -1, mEmitRef);
		lua_pushstring(mstate, emitBuffer);
		lua_call(mstate, 1, 0);
		lua_pop(mstate, 1);
	}
	return ret;
}

int lcSetDebugEmit(lua_State *L) {
	lua_pushlightuserdata(L, &mEmitRef);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (mEmitRef != -1) {
		// we already have a ref, so nil it out
		luaL_unref (L, -1, mEmitRef);
	}
	if (lua_type(L, 1) != LUA_TFUNCTION) {
		vmSetEmitFunction(printf);
	} else {
		lua_pushvalue(L, -2);
		mEmitRef = luaL_ref(L, -2);
		vmSetEmitFunction(lcEmitDebug);
	}
	lua_pop(L, 1);
	return 0;
}

int lcInstallModule(lua_State *L) {
	const char *name = luaL_checkstring(L, 1);
	const char *source = luaL_checkstring(L, 2);
	vmInstallSharedSource(name, source);
	return 0;
}

luaL_Reg lfunc[] = {
	{ "newVM", lcNewVM },						// create a new VM
	{ "version", lcVersion },					// version of carrica
	{ "hasDebug", lcHasDebug },					// compiled with debug?
	{ "setDebugEmit", lcSetDebugEmit },			// set a function to accept debug emits
	{ "installModule", lcInstallModule },		// install a shared source module for all VMs
    { NULL, NULL }
};

// register a new metatable in the lua state
void lua_newmeta(lua_State *L, const char *name, luaL_Reg* func, lua_CFunction gc) {
	luaL_newmetatable(L, name);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_register(L, NULL, func);
	if (gc != NULL) {
		lua_pushcfunction(L, gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_pop(L, 1);
}

// load carrica into the lua state
int luaopen_carrica(lua_State* L) {
	// store the state if we need it later
	mstate = L;
	// open up the VM subsystem
	vmBegin();
	// metatables!
	lua_newmeta(L, LUA_NAME_WRENVM, lcvmfunc, lcvmGC);
	lua_newmeta(L, LUA_NAME_STABLE, lctfunc, lctGC);
	lua_newmeta(L, LUA_NAME_SARRAY, lcafunc, lcaGC);
	//lua_newmeta(L, LUA_NAME_S_UOBJ, lcufunc, lcuGC);
	// a table for some internal data
		lua_pushlightuserdata(L, &mEmitRef);
		lua_newtable(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	// store table.sort() as our sort function
	lua_getglobal(L, "table");
	lua_pushlightuserdata(L, &mSortRef);
	lua_getfield(L, -2, "sort");
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1); // remove global 'table'
	// register our functions
	luaL_register(L, "carrica", lfunc);
	return 1;
}
