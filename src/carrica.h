/*
	carrica.h

	wren running under lua/luajit 5.1+

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "lauxlib.h"
#include "wren.h"

// development names: [Tenma] Michiru Uruka Iori Ashella Nasa Pippa
// release names: Lumi Dokuro Shiina Panko Isami Muyu Sakana
#define CARRICA_VERSION		"0.1.0 Tenma"

#define LUA_NAME_WRENVM		"1-CARRCIA-WRENVM"
#define LUA_NAME_STABLE		"2-CARRCIA-STABLE"
#define LUA_NAME_SARRAY		"3-CARRCIA-SARRAY"
#define LUA_NAME_S_UOBJ		"3-CARRCIA-S_UOBJ"

// the function that starts it all
int luaopen_carrica(lua_State* L);
