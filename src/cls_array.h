/*
	host.h

	wren running under lua 5.1+
	implementation of Array class

	muragami, muragami@wishray.com, Jason A. Petrasko 2024
	MIT license: https://opensource.org/license/mit/
*/

#include "vm.h"
vmWrenReference* avmLuaNewArray(carricaVM *cvm);
extern const vmForeignModule vmiArray;

