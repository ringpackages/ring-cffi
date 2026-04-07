if isWindows()
	loadlib("ring_cffi.dll")
but isLinux() or isFreeBSD()
	loadlib("libring_cffi.so")
but isMacOSX()
	loadlib("libring_cffi.dylib")
else
	raise("Unsupported OS! You need to build the library for your OS.")
ok

load "src/cffi.ring"
