# carrica
Carrica is [Wren](https://wren.io/) hosted by [LuaJIT](https://luajit.org/), a bridge between languages. The name is
a derivitive of the Portuguese word for Wren: carriça (say KAH-HE-SA). It primarily targets the model of lua calling
Wren, and not vice-versa. The whole system is written in C and compiled into a binary module you can load from LuaJIT.
While there are many planned features, this first limited version for testing has only support for three classes in
Wren: Host, Table, and Array. A roadmap is provided below of planned features and releases.

# getting started
When you download the repo you have the needed C source and a simple Makefile to build. This Makefile can detect 
Windows, Linux, and Mac OS platforms and adjusts the build as needed. It is primarily developed under MSYS2 (ucrt64)
on Windows, but I will be testing and making builds on at least Debian 12 and Mac OS.

# the basics
Once you have a compiled module, you simply need to load it into the lua state with the normal use of require. The
loaded modules exposes the following functions:
```lua
     carrica.newVM()
     carrica.newVM(name)
```
This creates a new Wren VM with a given name (or an automatically generated name equal to it's id number in the
global internal table of all VMs, such that the first created VM is named "0").
```lua
     carrica.version()
```
This returns the version string for the built module, in the format "X.X.X <codename>" currently returning
"0.1.0 Tenma" for the initial testing version.
