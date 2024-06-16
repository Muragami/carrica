/*
	vm.h

	wren running under lua 5.1+
	this is the main internal code for handling a Wren VM
	in the carrica 'style'

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#ifndef CARRICA_VM_HEADER

#define CARRICA_VM_HEADER

#include "carrica.h"
#include "uthash.h"
#include <stdbool.h>

// ********************************************************************************
// configuration option
// threads are WIP status, disabled for now - TODO NYI
//#define CARRICA_USE_THREADS		// you can disable this if you don't want threads
#define CARRICA_SAFETY			// use safety checks for sanity (at cost of very little speed)
#define VM_DEBUG				// this will emit a lot of status messages, to help debug issues

// ********************************************************************************
// configuration option
#ifdef CARRICA_USE_THREADS		// we use native threading on windows, pthread everywhere else
	#include "xthread.h"
#endif

// ********************************************************************************
// a stored method external to Wren
typedef struct _vmWrenMethod {
	char name[256];
	WrenHandle* hClass;
	WrenHandle* hMethod;
	UT_hash_handle hh;
} vmWrenMethod;

// ********************************************************************************
// general structure of this VM system

typedef struct _carricaModDef {
	const char* name;
	const char* source;
} carricaModDef;

typedef struct _carricaModule {
	carricaModDef def;
	WrenBindForeignMethodFn bindForeignMethod;
	WrenBindForeignClassFn bindForeignClass;
} carricaModule;

typedef struct _carricaModTable {
	carricaModule *mod;
	int max;
	int count;
} carricaModTable;

typedef struct _carricaTypeHandles {
	WrenHandle* Table;
	WrenHandle* Array;
} carricaTypeHandles;

typedef struct _carricaLuaRefs {
	void *loadModule;
} carricaLuaRefs;

typedef struct _carricaVM {
	WrenConfiguration config;
	carricaTypeHandles handle;
	carricaLuaRefs refs;
	lua_State* L;
	WrenVM* vm;
	char buffer[256];
	carricaModTable modtable;
	int id;
	char* name;
	vmWrenMethod *methodHash;
#ifdef CARRICA_USE_THREADS
	pthread_mutex_t lock;
#endif
} carricaVM;

// shared internal function for classes
WrenHandle* lcvmGetClassHandle(carricaVM *cvm, const char* module, const char* name);

// ********************************************************************************
// a simple struct system for binary/source modules in Wren
// we also use this internally to handle the internal module(s)

typedef struct _vmForeignMethodDef {
	bool isStatic;
	const char* signature;
	WrenForeignMethodFn method;
} vmForeignMethodDef;

typedef struct _vmForeignClassDef {
	const char* className;
	WrenForeignClassMethods methods;
} vmForeignClassDef;

typedef struct _vmForeignMethodTable {
	const char* className;
	const vmForeignMethodDef* entry;
} vmForeignMethodTable;

typedef struct _vmForeignClassTable {
	const vmForeignClassDef* entry;
} vmForeignClassTable;

typedef struct _vmForeignModule {
	const vmForeignMethodTable* tmethod;
	const vmForeignClassTable* tclass;
} vmForeignModule;

typedef int (*vmDebugEmit)(const char *s, ...);

// ********************************************************************************
// a shared object from wren to lua

#define VM_WREN_SHARE_ARRAY		0xF0F00001
#define VM_WREN_SHARE_TABLE		0xF0F00002
#define VM_WREN_SHARE_LSOBJ		0xF0F00003

typedef struct _vmWrenReference {
	int type;
	int refCount;
	WrenHandle* handle;
} vmWrenReference;

typedef struct _vmWrenReReference {
	int type;
	carricaVM *vm;
	vmWrenReference *pref;
} vmWrenReReference;

// ********************************************************************************
// some internal cofiguration

// allocate 16 entries for modules when initialized
#define VM_MODULE_MINIMUM		16
// size of the VM struct
#define VM_BYTE_SIZE			sizeof(carricaVM)
// size of the VM module struct
#define VM_MOD_BYTE_SIZE		sizeof(carricaModule)
// size of the VM module def struct
#define VM_MOD_DEF_SIZE			sizeof(carricaModDef)
// size of the VM module table struct
#define VM_MOD_TAB_SIZE			sizeof(carricaModTable)
// size of the VM module table struct
#define VM_REF_SIZE				sizeof(vmWrenReference)
// size of the VM module table struct
#define VM_REREF_SIZE			sizeof(vmWrenReReference)
// size of the wrenMethod table struct
#define VM_WMETHOD_SIZE			sizeof(vmWrenReReference)


// ********************************************************************************
// some utility functions, mostly internal

void wrenError(WrenVM* vm, const char* err);
void wrenSetSlotFromLua(carricaVM *cvm, int slot, int idx);
void luaPushFromWrenSlot(carricaVM *cvm, int slot);
bool wrenSlotIsLuaSafe(carricaVM *cvm, int slot);

// ********************************************************************************
// VM functions

// if we are debugging, give us a bit of control of how that works
void vmSetEmitFunction(vmDebugEmit e);
// are we in debugging mode?
bool vmHasDebug();
// call before we use the system
void vmBegin();
// call to cleanup at the end
void vmEnd();
// create a new VM
void vmNew(lua_State* L, carricaVM *vm, const char *name);
// release a VM
void vmRelease(carricaVM* vm);
// is this a valid VM instance?
bool vmIsValid(carricaVM* vm);
// does the VM hold a certain top level variable?
bool vmHasVariable(carricaVM* vm, const char* module, const char* name);
// does the VM have a certain module loaded?
bool vmHasModule(carricaVM* vm, const char* name);
// install modules into the VM
// FYI: we own nothing here, so passed in pointers have to exist when the VM exists
void vmInstallSourceDef(carricaVM* cvm, carricaModDef* def);
void vmInstallSource(carricaVM* cvm, const char* name, const char* source);
void vmInstallBinaryMod(carricaVM* cvm, carricaModule* mod);
// install modules into shared VM system (added to all present and future VMs)
void vmInstallSharedSourceDef(carricaModDef* def);
void vmInstallSharedSource(const char* name, const char* source);
void vmInstallSharedBinaryMod(carricaModule* mod);
// make some code happen
void vmInterpret(carricaVM* vm, const char* code, const char* module);
// get a method call handle
vmWrenMethod *vmGetMethod(carricaVM* vm, const char *module, const char* className, const char* sig);
// free a method call handle
void vmFreeMethod(carricaVM* cvm, vmWrenMethod *p);
// call a wren method from lua (with arguments on the stack)
void vmCallMethodFromLua(carricaVM *cvm, vmWrenMethod *p, int top);

#endif