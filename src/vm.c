/*
	vm.c

	wren running under lua 5.1+
	this is the main internal code for handling a Wren VM
	in the carrica 'style'

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "vm.h"
// internals
#include "cls_host.h"
#include "cls_table.h"
#include "cls_array.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>

// ********************************************************************************
// internal type defs

typedef struct _vmTable {
	carricaVM **vm;
	int max;
	int count;
#ifdef CARRICA_USE_THREADS
	pthread_mutex_t lock;
#endif		
} vmTable;

typedef struct _vmModTable {
	vmForeignModule* entry;
} vmModTable;

#define VM_CORE_MODULES		3

// ********************************************************************************
// general static stuff for the VM system

carricaModDef carrica;
carricaModTable shared;
vmModTable imod;
carricaModule selfMod;
vmTable vmt;
#ifdef CARRICA_USE_THREADS
	pthread_mutex_t sharedLock;
#endif

static WrenForeignClassMethods nullMethods = { NULL, NULL };

static const char *carricaSource =
	#include "carrica.wren.txt"
;

#ifdef VM_DEBUG
typedef int (*vmDebugEmit)(const char *s, ...);
static char ebuffer[256];
static vmDebugEmit EMIT = printf;
#endif

// ********************************************************************************
// utility functions

void wrenError(WrenVM* vm, const char* err) {
	wrenSetSlotString(vm, 0, err);
	wrenAbortFiber(vm, 0);
}

typedef struct _vmForkedPointer {
	union {
		const char *str;
		vmWrenReference *ref;
	};
} vmForkedPointer;

const char *luaGetMetaTableType(lua_State *L, int idx) {
	lua_getmetatable(L, idx);
	luaL_getmetatable(L, LUA_NAME_SARRAY);
	if (lua_equal(L, -1, -2))
		return LUA_NAME_SARRAY;
	else {
		lua_pop(L, 1);
		luaL_getmetatable(L, LUA_NAME_S_UOBJ);
		if (lua_equal(L, -1, -2))
			return LUA_NAME_S_UOBJ;
		else {
			lua_pop(L, 1);
			luaL_getmetatable(L, LUA_NAME_STABLE);
			if (lua_equal(L, -1, -2))
				return LUA_NAME_STABLE;
			else
				return NULL;
		}
	}
}

void wrenSetSlotFromLua(carricaVM *cvm, int slot, int idx) {
	size_t len = 0;
	vmForkedPointer p = { NULL };
	
	switch (lua_type(cvm->L, idx)) {
		case LUA_TNIL:
			wrenSetSlotNull(cvm->vm, slot);
			break;
  		case LUA_TNUMBER:
  			wrenSetSlotDouble(cvm->vm, slot, lua_tonumber(cvm->L, idx));
			break;
		case LUA_TBOOLEAN:
			wrenSetSlotBool(cvm->vm, slot, lua_toboolean(cvm->L, idx));
			break;
		case LUA_TSTRING:
			p.str = lua_tolstring(cvm->L, idx, &len);
			wrenSetSlotBytes(cvm->vm, slot, p.str, len);
			break;
		case LUA_TTABLE:
			// this is not allowed, you create an Array or Table in wren
			wrenError(cvm->vm, "VM -> you may not marshal tables from lua to Wren");
			break;
		case LUA_TUSERDATA:
			// see if this is a wren reference
			p.str = luaGetMetaTableType(cvm->L, idx);
			if (p.str == NULL) {
				wrenError(cvm->vm, "VM -> unknown user data from lua!?");
			} else {
				// it is, so marshal that into wren
				p.ref = lua_touserdata(cvm->L, idx);
				wrenSetSlotHandle(cvm->vm, slot, p.ref->handle);
			}
			break;
  		default:
  			wrenError(cvm->vm, "VM -> bad type passed from lua!?");
  			break;
	}
}

void luaPushFromWrenSlot(carricaVM *cvm, int slot) {
	const char* s;
	vmWrenReReference *ref;
	int l;
	switch (wrenGetSlotType(cvm->vm, slot)) {
		case WREN_TYPE_NULL:
			lua_pushnil(cvm->L);
			break;
  		case WREN_TYPE_STRING:
  			s = wrenGetSlotBytes(cvm->vm, slot, &l);
  			lua_pushlstring(cvm->L, s, l);
			break;
  		case WREN_TYPE_BOOL:
  			lua_pushboolean(cvm->L, wrenGetSlotBool(cvm->vm, slot));
			break;
  		case WREN_TYPE_NUM:
  			lua_pushnumber(cvm->L, wrenGetSlotDouble(cvm->vm, slot));
			break;
  		case WREN_TYPE_FOREIGN:
  			ref = wrenGetSlotForeign(cvm->vm, slot);
  			switch (ref->type) {
  				case VM_WREN_SHARE_ARRAY:
  				case VM_WREN_SHARE_TABLE:
  				case VM_WREN_SHARE_LSOBJ:
  					// find the ref and push it
  					lua_pushlightuserdata(cvm->L, ref->pref);
  					lua_gettable(cvm->L, LUA_REGISTRYINDEX);
  					break;
  				default:
  					luaL_error(cvm->L, "VM -> unsupported foreign class passed to luaPushFromWrenSlot()");
  					break;
  			}
			break;
  		case WREN_TYPE_LIST:
  		case WREN_TYPE_MAP:
  		default:
  			// this is an error, we don't support it!
  			luaL_error(cvm->L, "VM -> unsupported type passed to luaPushFromWrenSlot()");
  			break;
	}
}

bool wrenSlotIsLuaSafe(carricaVM* cvm, int slot) {
	vmWrenReReference *ref;
	switch (wrenGetSlotType(cvm->vm, slot)) {
		case WREN_TYPE_NULL:
  		case WREN_TYPE_STRING:
  		case WREN_TYPE_BOOL:
  		case WREN_TYPE_NUM:
  			return true;
		case WREN_TYPE_FOREIGN:
  			ref = wrenGetSlotForeign(cvm->vm, slot);
  			switch (ref->type) {
  				case VM_WREN_SHARE_ARRAY:
  				case VM_WREN_SHARE_TABLE:
  				case VM_WREN_SHARE_LSOBJ:
  					return true;
  				default:
  					return false;
  			}
  		case WREN_TYPE_LIST:
  		case WREN_TYPE_MAP:
  		default:
  			return false;
	}
}

// ********************************************************************************
// functions for the shared VM module table

void vmLock(carricaVM *cvm) {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_lock(&cvm->lock);
#endif
}

void vmUnlock(carricaVM *cvm) {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_unlock(&cvm->lock);
#endif
}

// ********************************************************************************
// functions for the shared VM module table

void vmsLock() {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_lock(&sharedLock);
#endif
}

void vmsUnlock() {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_unlock(&sharedLock);
#endif
}

// expand the VM table as needed
void vmsExpand() {
	if (shared.max == 0) {
#ifdef VM_DEBUG
		EMIT("\033[37mvms:: intializing memory\033[0m\n");
#endif		
		// initial allocation for modules
		shared.max = 8;
		shared.mod = calloc(8 * VM_MOD_BYTE_SIZE, 1);
		if (shared.mod == NULL) { return; }
	} else {
#ifdef VM_DEBUG
		EMIT("\033[37mvms:: expanding memory\033[0m\n");
#endif		
		// see if we are full, and expand as needed
		if (shared.count + 1 == shared.max) {
			// expand the module table
			shared.mod = realloc(shared.mod, VM_MOD_BYTE_SIZE * shared.max * 2);
			if (shared.mod == NULL) { return; }
			memset(&shared.mod[shared.max], 0, VM_MOD_BYTE_SIZE * shared.max);
			shared.max *= 2;
		}
	}
}

// return true if a given module name already exists in the shared module system
bool vmsModuleExists(const char *name) {
	for (int i = 0; i < shared.count; i++) {
		if (shared.mod[i].def.name != NULL) {
			if (!strcmp(name, shared.mod[i].def.name)) return true;
		}
	}
	return false;
}

// ********************************************************************************
// functions for the shared VM table

void vmtLock() {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_lock(&vmt.lock);
#endif
}

void vmtUnlock() {
#ifdef CARRICA_USE_THREADS
	pthread_mutex_unlock(&vmt.lock);
#endif
}

// expand the VM table as needed
void vmtExpand() {
	if (vmt.max == 0) {
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: initializing memory\033[0m\n");
#endif			
		// initial allocation for modules
		vmt.max = 8;
		vmt.vm = calloc(8 * sizeof(carricaVM*), 1);
		if (vmt.vm == NULL) return;
	} else {
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: expanding memory\033[0m\n");
#endif			
		// see if we are full, and expand as needed
		if (vmt.count + 1 == vmt.max) {
			// expand the module table
			vmt.vm = realloc(vmt.vm, sizeof(carricaVM*) * vmt.max * 2);
			if (vmt.vm == NULL) return;
			memset(&vmt.vm[vmt.max], 0, sizeof(carricaVM*) * vmt.max);
			vmt.max *= 2;
		}
	}
}

// add a VM to the table
void vmtAdd(carricaVM *cvm) {
	vmtLock();
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: adding VM '%s' to table\033[0m\n", cvm->name);
#endif			
	if (vmt.vm) {
		// see if we have an empty slot to fill
		for (int i = 0; i < vmt.count; i++) {
			if (vmt.vm[i] == NULL) {
				cvm->id = i;
				vmt.vm[i] = cvm;
				vmtUnlock();
				return;
			}
		}
		// nope, allocate as needed
		vmtExpand();
		// add the vm to the table
		cvm->id = vmt.count;
		vmt.vm[vmt.count++] = cvm;
	} else {
		// nope, allocate as needed
		vmtExpand();
		// add the vm to the table
		cvm->id = vmt.count;
		vmt.vm[vmt.count++] = cvm;
	}
	vmtUnlock();
}

// remove a VM from the table
void vmtRemove(carricaVM *cvm) {
	vmtLock();
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: removing VM '%s' from table\033[0m\n", cvm->name);
#endif			
	if (vmt.vm && cvm->id > -1) {
		vmt.vm[cvm->id] = NULL;
	}
	vmtUnlock();
}

// install all shared modules into a VM
void vmtInstallShared(carricaVM *cvm) {
	vmsLock();
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: installing all shared modules into VM '%s'\033[0m\n", cvm->name);
#endif	
	for (int i = 0; i < shared.count; i++) {
		if (shared.mod[i].def.name != NULL) {
			// install this module
			if (shared.mod[i].bindForeignMethod) {
				// it's a binary
				vmInstallBinaryMod(cvm, &shared.mod[i]);
			} else {
				// it's a source
				vmInstallSourceDef(cvm, &shared.mod[i].def);
			}
		}
	}
	vmsUnlock();
}

// add a new shared module to existing VMs
void vmtNewSharedModule(carricaModule *m) {
	vmtLock();
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: adding new shared module '%s'\033[0m\n", m->def.name);
#endif	
	if (vmt.vm) {
		// see if we have an empty slot to fill
		for (int i = 0; i < vmt.count; i++) {
			if (vmt.vm[i] != NULL) {
				if (m->bindForeignMethod) {
					vmInstallBinaryMod(vmt.vm[i], m);
				} else {
					vmInstallSourceDef(vmt.vm[i], &m->def);
				}
			}
		}
	}
	vmtUnlock();
}

// ********************************************************************************
// internal mappings from Wren VM config

void vmWriteFn(WrenVM* vm, const char* text) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) return;
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	// we just pulled our table of handlers from the registry, grab the write
	lua_getfield(cvm->L, -1, "write");
	// see what returned
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		// a function, so call it
		lua_pushvalue(cvm->L, -1);
		lua_pushstring(cvm->L, text);
		lua_call(cvm->L, 1, 0);
	} else if (lua_type(cvm->L, -1) == LUA_TTABLE) {
		// a table so call .write() on it
		lua_getfield(cvm->L, -1, "write");
		if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
			lua_pushvalue(cvm->L, -2);			// self (table)
			lua_pushstring(cvm->L, text);		// the text
			lua_call(cvm->L, 2, 0);
		}
	}
	lua_pop(cvm->L, 2);
}

void vmErrorFn(WrenVM* vm, WrenErrorType errorType, const char* module, 
				const int line, const char* msg) {
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) return;
	switch (errorType) {
			// A syntax or resolution error detected at compile time.
  			case WREN_ERROR_COMPILE:
  				snprintf(cvm->buffer, 256, "Wren Compile:: '%s' at line %d in module: %s", msg, line, module);
  				break;
			// The error message for a runtime error.
  			case WREN_ERROR_RUNTIME:
  				snprintf(cvm->buffer, 256, "Wren Runtime:: '%s' at line %d in module: %s", msg, line, module);
  				break;
  			// One entry of a runtime error's stack trace.
  			case WREN_ERROR_STACK_TRACE:
  				snprintf(cvm->buffer, 256, "            :: '%s' at line %d in module: %s", msg, line, module);
  				break;
	}
	lua_pushlightuserdata(cvm->L, cvm);
	lua_gettable(cvm->L, LUA_REGISTRYINDEX);
	// we just pulled our table of handlers from the registry, grab the write
	lua_getfield(cvm->L, -1, "error");
	// see what returned
	if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
		// a function, so call it
		lua_pushvalue(cvm->L, -1);
		lua_pushstring(cvm->L, cvm->buffer);
		lua_call(cvm->L, 1, 0);
	} else if (lua_type(cvm->L, -1) == LUA_TTABLE) {
		// a table so call .write() on it
		lua_getfield(cvm->L, -1, "error");
		if (lua_type(cvm->L, -1) == LUA_TFUNCTION) {
			lua_pushvalue(cvm->L, -2);			// self (table)
			lua_pushstring(cvm->L, cvm->buffer);		// the text
			lua_call(cvm->L, 2, 0);
		}
	}
	lua_pop(cvm->L, 2);
}

// TODO - consider the lock situation in the following 3 functions

WrenForeignMethodFn vmBindForeignMethodFn(WrenVM* vm, const char* module,
    					const char* className, bool isStatic, const char* signature) {
	WrenForeignMethodFn ret = NULL;
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) return NULL;
	carricaModTable *mod = &cvm->modtable;
#ifdef VM_DEBUG	
	const char *stat;
	if (isStatic) stat = "[STATIC]"; else stat = "";
	snprintf(ebuffer, 256, "\033[36m--   bind: method = %s / %s.%s %s\033[0m", module, className, signature, stat);
	EMIT("%s\n", ebuffer);
#endif
	for (int i = 0; i < mod->count; i++) {
		if (mod->mod[i].def.name && !strcmp(module, mod->mod[i].def.name)) {
			// found the module, let it handle the rest
			ret = mod->mod[i].bindForeignMethod(vm, module, className, isStatic, signature);
			if (ret) 
				return ret;
			else
				break;
		}
	}
#ifdef VM_DEBUG
	EMIT("!!   bind: foreign method not found! %s / %s.%s %s\n", module, className, signature, stat);
#endif	
	// no luck, error out here (we error out because a failure here is bad configuration)
	if (isStatic)
		luaL_error(cvm->L, "carrica -> could not find foreign static method '%s' in class '%s' of module '%s'",
						signature, module, className);
	else
		luaL_error(cvm->L, "carrica -> could not find foreign method '%s' in class '%s' of module '%s'",
						signature, module, className);
	// never reached but make sure the compiler doesn't complain...
	return NULL;
}

WrenForeignClassMethods vmBindForeignClassFn(WrenVM* vm, const char* module,
						const char* className) {
	WrenForeignClassMethods ret = nullMethods;
	carricaVM *cvm = wrenGetUserData(vm);
	if (!vmIsValid(cvm)) return nullMethods;
	carricaModTable *mod = &cvm->modtable;
#ifdef VM_DEBUG	
	snprintf(ebuffer, 256, "\033[36m--   bind: class = %s / %s\033[0m", module, className);
	EMIT("%s\n", ebuffer);
#endif	
	for (int i = 0; i < mod->count; i++) {
		if (mod->mod[i].def.name && !strcmp(module, mod->mod[i].def.name)) {
			// found the module, let it handle the rest
			ret = mod->mod[i].bindForeignClass(vm, module, className);
			if (ret.allocate) 
				return ret;
			else
				break;
		}
	}
#ifdef VM_DEBUG
	EMIT("\033[31m!!   bind: foreign class not found! %s / %s\033[0m\n", module, className);
#endif		
	// no luck, error out here (we error out because a failure here is bad configuration)
	luaL_error(cvm->L, "carrica -> could not find foreign class '%s' of module '%s'",
						module, className);
	// never reached but make sure the compiler doesn't complain...
	return nullMethods;
}

static void loadModuleComplete(WrenVM* vm, const char* module,
                               WrenLoadModuleResult result) {
  if (result.source) free((void*)result.source);
}

WrenLoadModuleResult vmLoadModule(WrenVM* vm, const char* name) {
	WrenLoadModuleResult result = { NULL, NULL, NULL };
	carricaVM *cvm = wrenGetUserData(vm);
	carricaModTable *mod = &cvm->modtable;
#ifdef VM_DEBUG		
	snprintf(ebuffer, 256, "\033[36m--   load: module = %s\033[0m", name);
	EMIT("%s\n", ebuffer);
#endif
	// search the internal table first, see if we have it
	for (int i = 0; i < mod->count; i++) {
		if (mod->mod[i].def.name && !strcmp(name, mod->mod[i].def.name)) {
			// found the module, return it
			result.source = mod->mod[i].def.source;
			return result;
		}
	}
	// if not found, call our routine we have to find it
	if ((result.source == NULL) && (cvm->refs.loadModule != NULL)) {
		lua_pushlightuserdata(cvm->L, cvm->refs.loadModule);
		lua_gettable(cvm->L, LUA_REGISTRYINDEX);
		lua_pushstring(cvm->L, name);
		lua_call(cvm->L, 1, 1);
		if (lua_type(cvm->L, -1) == LUA_TSTRING) {
			const char *str;
			char *mem;
			size_t len;
			str = lua_tolstring(cvm->L, -1, &len);
			mem = malloc(len + 1);
			if (mem == NULL) luaL_error(cvm->L, "memory allocation failure in vmLoadModule()");
			memcpy(mem, str, len);
			mem[len] = 0;
			result.source = mem;
			result.onComplete = loadModuleComplete;
			lua_pop(cvm->L, 1);	// pop the string left on the stack
			return result;
		}
	}
	luaL_error(cvm->L, "carrica -> could not find module '%s' to load", name);
	return result;
}

// ********************************************************************************
// internal wren binding for carrica foreigns

WrenForeignMethodFn vmBindForeignMethodInternal(WrenVM* vm, const char* module,
    					const char* className, bool isStatic, const char* signature) {
	vmForeignModule *mod = imod.entry;
	while (mod->tmethod != NULL) {
		const vmForeignMethodTable *pt = mod->tmethod;
		while (pt->className != NULL) {
			if (!strcmp(className, pt->className)) {
				// class match, look for the method now!
				const vmForeignMethodDef *pMethod = pt->entry;
				while (pMethod->signature != NULL) {
					if ((isStatic == pMethod->isStatic) && (!strcmp(signature, pMethod->signature)))
						return pMethod->method;
					pMethod++;
				}
			}
			pt++;	
		}
		mod++;
	}
	return NULL;
}

WrenForeignClassMethods vmBindForeignClassInternal(WrenVM* vm, const char* module,
						const char* className) {
	vmForeignModule *mod = imod.entry;
	// walk all internal modules
	while (mod->tmethod != NULL) {
		const vmForeignClassDef *pClass = mod->tclass->entry;
		// walk each class table to see if we have a match
		while (pClass->className != NULL) {
			if (!strcmp(className, pClass->className)) return pClass->methods;
			pClass++;
		}
		mod++;
	}
	return nullMethods;
}

// ********************************************************************************
// Debug info configuration (if enabled)


void vmSetEmitFunction(vmDebugEmit e) {
#ifdef VM_DEBUG
	EMIT = e;
#endif	
}

bool vmHasDebug() {
#ifdef VM_DEBUG
	return true;
#else
	return false;
#endif
}

// ********************************************************************************
// VM begin and end

void vmBegin() {
	// configure internals
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: initializing internals\033[0m\n");
#endif	
	carrica.name = "carrica";
	carrica.source = carricaSource;
	// install internal modules
	imod.entry = calloc(sizeof(vmForeignModule) * (VM_CORE_MODULES + 1), 1);
	memcpy(&imod.entry[0], &vmiHost, sizeof(vmForeignModule));
	memcpy(&imod.entry[1], &vmiTable, sizeof(vmForeignModule));
	memcpy(&imod.entry[2], &vmiArray, sizeof(vmForeignModule));
	// blank blank blank
	memset(&shared, 0, VM_MOD_TAB_SIZE);
	memset(&vmt, 0, sizeof(vmTable));
#ifdef CARRICA_USE_THREADS
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: initializing internal thread locks\033[0m\n");
#endif	
	pthread_mutex_init(&sharedLock, NULL);
	pthread_mutex_init(&vmt.lock, NULL);
#endif
	// the first shared mod is the internal one, added to all VMs
	selfMod.def = carrica;
	selfMod.bindForeignMethod = vmBindForeignMethodInternal;
	selfMod.bindForeignClass = vmBindForeignClassInternal;
	vmInstallSharedBinaryMod(&selfMod);
}

void vmEnd() {
	vmtLock();
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: deinitializing\n");
#endif	
	// find any VMs that might have not yet been released, and handle that
	for (int i = 0; i < vmt.count; i++) {
		if (vmt.vm[i] != NULL) vmRelease(vmt.vm[i]);
	}
	vmtUnlock();
#ifdef CARRICA_USE_THREADS
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: deinitializing internal threads\033[0m\n");
#endif	
	pthread_mutex_destroy(&sharedLock);
	pthread_mutex_destroy(&vmt.lock);
#endif	
}

// ********************************************************************************
// VM core routines

bool vmIsValid(carricaVM *cvm) { return cvm && (cvm->vm != NULL); }

void vmNew(lua_State *L, carricaVM *cvm, const char *name) {
	WrenConfiguration *conf = &cvm->config;
	// blank us
	memset(cvm, 0, VM_BYTE_SIZE);
	// set the name
	if (name) 
		// copy the explicit name
		cvm->name = strdup(name);
	 else {
	 	// make the name the id number hex
	 	cvm->name = calloc(16, 1);
	 	snprintf(cvm->name, 16, "%x", vmt.count);
	}
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: creating new VM '%s'\033[0m\n", cvm->name);
#endif	
	// configure the vm
	conf->writeFn = vmWriteFn;
	conf->errorFn = vmErrorFn;
	conf->bindForeignMethodFn = vmBindForeignMethodFn;
	conf->bindForeignClassFn = vmBindForeignClassFn;
	conf->loadModuleFn = vmLoadModule;
	// create the vm
	cvm->L = L;
	cvm->vm = wrenNewVM(conf);
	// we are going to need a function registry table for this VM in lua
		lua_pushlightuserdata(L, cvm); 	// key
		lua_newtable(L);				// table
	lua_settable(L, LUA_REGISTRYINDEX);
	// we are going to need a const registry table for this VM in lua
		lua_pushlightuserdata(L, cvm->name); 	// key
		lua_newtable(L);						// table
	lua_settable(L, LUA_REGISTRYINDEX);
#ifdef CARRICA_USE_THREADS
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: creating thread lock for new VM '%s'\033[0m\n", cvm->name);
#endif	
	pthread_mutex_init(&cvm->lock, NULL);
#endif		
	// store our user data
	wrenSetUserData(cvm->vm, cvm);
	// add the VM to our internal table
	vmtAdd(cvm);
#ifdef VM_DEBUG
		EMIT("\033[37mvmt:: VM '%s' is id %d in vmt table\033[0m\n", cvm->name, cvm->id);
#endif		
	// install shared modules
	vmtInstallShared(cvm);
}
 
void vmRelease(carricaVM *cvm) {
	if (cvm && (cvm->vm != NULL)) {
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: releasing VM '%s'\033[0m\n", cvm->name);
#endif		
		// remove the VM from our internal table
		vmtRemove(cvm);
		// remove our lua registry table
			lua_pushlightuserdata(cvm->L, cvm); 	// key
			lua_pushnil(cvm->L);					// nil (to remove the table)
		lua_settable(cvm->L, LUA_REGISTRYINDEX);
		// remove any hashed call handles lingering
		vmWrenMethod *m = NULL;
		vmWrenMethod *tmp = NULL;
		HASH_ITER(hh, cvm->methodHash, m, tmp) {
    		HASH_DEL(cvm->methodHash, m);
    		if (m->hMethod) wrenReleaseHandle(cvm->vm, m->hMethod);
			if (m->hClass) wrenReleaseHandle(cvm->vm, m->hClass);
    		free(m);
  		}
  		// free the name string
  		free(cvm->name);
		// free the VM
		wrenFreeVM(cvm->vm);
#ifdef CARRICA_USE_THREADS
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: releasing thread lock for VM '%s'\033[0m\n", cvm->name);
#endif		
		pthread_mutex_destroy(&cvm->lock);
#endif
		// empty the structure
		memset(cvm, 0, VM_BYTE_SIZE);
	}
}

void vmSetWrenName(carricaVM *cvm, const char *name) {
	if (!vmIsValid(cvm)) return;
	if (cvm->wrenName) free(cvm->wrenName);
	cvm->wrenName = strdup(name);
}

bool vmHasVariable(carricaVM *cvm, const char *module, const char *name) {
	if (!vmIsValid(cvm)) return false;
	return wrenHasVariable(cvm->vm, module, name);
}

bool vmHasModule(carricaVM *cvm, const char *name) {
	if (!vmIsValid(cvm)) return false;
	return wrenHasModule(cvm->vm, name);
}

void vmExpandModules(carricaVM *cvm) {
	carricaModTable *mod = &cvm->modtable;
	if (mod->max == 0) {
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: initializing module memory for  VM '%s'\033[0m\n", cvm->name);
#endif		
		// initial allocation for modules
		mod->max = VM_MODULE_MINIMUM;
		mod->mod = calloc(VM_MOD_BYTE_SIZE * mod->max, 1);
		if (mod->mod == NULL) luaL_error(cvm->L, "carrica -> memory allocation error from vmInstallSourceDef()");
	} else {
#ifdef VM_DEBUG
		EMIT("\033[37mvm:: expanding module memory for  VM '%s'\033[0m\n", cvm->name);
#endif		
		// see if we are full, and expand as needed
		if (mod->count + 1 == mod->max) {
			// expand the module table
			mod->mod = realloc(mod->mod, VM_MOD_BYTE_SIZE * mod->max * 2);
			if (mod->mod == NULL) luaL_error(cvm->L, "carrica -> memory allocation error from vmInstallSourceDef()");
			memset(&mod->mod[mod->max], 0, VM_MOD_BYTE_SIZE * mod->max);
			mod->max *= 2;
		}
	}
}

void vmInstallSourceDef(carricaVM *cvm, carricaModDef *def) {
	vmExpandModules(cvm);
	carricaModTable *mod = &cvm->modtable;
	memcpy(&mod->mod[mod->count].def, def, VM_MOD_DEF_SIZE);
	mod->count++;
}

void vmInstallSource(carricaVM *cvm, const char *name, const char* source) {
	vmExpandModules(cvm);
	carricaModTable *mod = &cvm->modtable;
	mod->mod[mod->count].def.name = name;
	mod->mod[mod->count].def.source = source;
	mod->count++;
}

void vmInstallBinaryMod(carricaVM *cvm, carricaModule *_mod) {
	vmExpandModules(cvm);
	carricaModTable *mod = &cvm->modtable;
	memcpy(&mod->mod[mod->count], _mod, VM_MOD_BYTE_SIZE);
	mod->count++;
}

void vmInstallSharedSourceDef(carricaModDef *def) {
	vmsLock();
	// can't install an already installed shared module
	if (vmsModuleExists(def->name)) { vmsUnlock(); return; }
	vmsExpand();
	carricaModTable *mod = &shared;
	memcpy(&mod->mod[mod->count].def, def, VM_MOD_DEF_SIZE);
	carricaModule *m = &mod->mod[mod->count];
	mod->count++;
	vmsUnlock();
	// now add to existing VMs
	vmtNewSharedModule(m);
}

void vmInstallSharedSource(const char *name, const char* source) {
	vmsLock();
	if (vmsModuleExists(name)) { vmsUnlock(); return; }
	vmsExpand();
	carricaModTable *mod = &shared;
	mod->mod[mod->count].def.name = name;
	mod->mod[mod->count].def.source = source;
	carricaModule *m = &mod->mod[mod->count];
	mod->count++;
	vmsUnlock();
	// now add to existing VMs
	vmtNewSharedModule(m);
}

void vmInstallSharedBinaryMod(carricaModule *_mod) {
	vmsLock();
	if (vmsModuleExists(_mod->def.name)) { vmsUnlock(); return; }
	vmsExpand();
	carricaModTable *mod = &shared;
	memcpy(&mod->mod[mod->count], _mod, VM_MOD_BYTE_SIZE);
	carricaModule *m = &mod->mod[mod->count];
	mod->count++;
	vmsUnlock();
	// now add to existing VMs
	vmtNewSharedModule(m);
}

void vmInterpret(carricaVM *cvm, const char *code, const char *module) {
	if (vmIsValid(cvm)) {
		if (module == NULL) module = "main";
#ifdef VM_DEBUG
		EMIT("\033[93mvm:: running interpret VM '%s' of module '%s'\033[0m\n", cvm->name, module);
#endif		
		wrenInterpret(cvm->vm, module, code);
	}
}

vmWrenMethod *vmGetMethod(carricaVM* cvm, const char *module, const char* className, const char* sig) {
	static char buffer[256];
	vmWrenMethod *ret = NULL;
	snprintf(buffer, 256, "%s:%s.%s", module, className, sig);
	HASH_FIND_STR(cvm->methodHash, buffer, ret);
	if (ret) return ret;
	ret = calloc(VM_WMETHOD_SIZE, 1);
	memcpy(ret->name, buffer, 255);
	if (wrenHasModule(cvm->vm, module) && wrenHasVariable(cvm->vm, module, className)) {
		wrenEnsureSlots(cvm->vm, 1);
		wrenGetVariable(cvm->vm, module, className, 0);
		ret->hClass = wrenGetSlotHandle(cvm->vm, 0);
		ret->hMethod = wrenMakeCallHandle(cvm->vm, sig);
		HASH_ADD_STR(cvm->methodHash, name, ret);
		return ret;
	} else {
		free(ret);
		return NULL;
	}
}

void vmFreeMethod(carricaVM* cvm, vmWrenMethod *p) {
	vmWrenMethod *e = NULL;
	HASH_FIND_STR(cvm->methodHash, p->name, e);
	if (e) {
		// remove it from our hash table
		HASH_DEL(cvm->methodHash, e);
		// clean up
		if (p->hMethod) wrenReleaseHandle(cvm->vm, p->hMethod);
		if (p->hClass) wrenReleaseHandle(cvm->vm, p->hClass);
		// release it's memory
		free(p);
	}
}

void vmCallMethodFromLua(carricaVM *cvm, vmWrenMethod *p, int top) {
	// make sure we have slots
	wrenEnsureSlots(cvm->vm, top + 1);
	// setup the receiver class
	wrenSetSlotHandle(cvm->vm, 0, p->hClass);
	// push the arguments
	for (int i = 1; i <= top; i++) 
		wrenSetSlotFromLua(cvm, i, i);
	// make the call
	wrenCall(cvm->vm, p->hMethod);
}