aPackageInfo = [
	:name = "Ring CFFI",
	:description = "Foreign Function Interface (FFI) for Ring. Call C libraries directly from Ring using libffi.",
	:folder = "ring-cffi",
	:developer = "ysdragon",
	:email = "youssefelkholey@gmail.com",
	:license = "MIT",
	:version = "1.0.0",
	:ringversion = "1.25",
	:versions = 	[
		[
			:version = "1.0.0",
			:branch = "master"
		]
	],
	:libs = 	[
		[
			:name = "",
			:version = "",
			:providerusername = ""
		]
	],
	:files = 	[
		# Root
		"lib.ring",
		"main.ring",
		"test.ring",
		"LICENSE",
		"README.md",
		"CMakeLists.txt",
		".clang-format",
		".gitignore",
		# CMake
		"cmake/BuildLibFFI.cmake",
		"cmake/fficonfig.h.in",
		# Source — Ring
		"src/cffi.ring",
		# Source — C
		"src/c_src/ring_cffi.c",
		"src/c_src/ring_cffi.h",
		# Source — Utilities
		"src/utils/install.ring",
		"src/utils/uninstall.ring",
		"src/utils/color.ring",
		# Tests
		"test/CFFI_test.ring",
		# Examples
		"examples/01_c_load.ring",
		"examples/02_c_call.ring",
		"examples/03_c_alloc.ring",
		"examples/04_c_pointers.ring",
		"examples/05_c_strings.ring",
		"examples/06_c_sym.ring",
		"examples/07_c_funcptr.ring",
		"examples/08_c_structs.ring",
		"examples/09_c_unions.ring",
		"examples/10_c_enums.ring",
		"examples/11_c_callbacks.ring",
		"examples/12_c_variadic.ring",
		"examples/13_c_cdef.ring",
		"examples/14_c_sizeof.ring",
		"examples/15_c_errors.ring",
		"examples/16_c_typeof.ring",
		"examples/17_oop_load.ring",
		"examples/18_oop_functions.ring",
		"examples/19_oop_memory.ring",
		"examples/20_oop_pointers.ring",
		"examples/21_oop_structs_unions.ring",
		"examples/22_oop_misc.ring",
		"examples/23_oop_qsort.ring"
	],
	:ringfolderfiles = 	[

	],
	:windowsfiles = 	[
		"lib/windows/amd64/ring_cffi.dll",
		"lib/windows/i386/ring_cffi.dll",
		"lib/windows/arm64/ring_cffi.dll"
	],
	:linuxfiles = 	[
		"lib/linux/amd64/libring_cffi.so",
		"lib/linux/arm64/libring_cffi.so",
		"lib/linux/musl/amd64/libring_cffi.so",
		"lib/linux/musl/arm64/libring_cffi.so"
	],
	:ubuntufiles = 	[

	],
	:fedorafiles = 	[

	],
	:macosfiles = 	[
		"lib/macos/amd64/libring_cffi.dylib",
		"lib/macos/arm64/libring_cffi.dylib"
	],
	:freebsdfiles = 	[
		"lib/freebsd/amd64/libring_cffi.so"
	],
	:windowsringfolderfiles = 	[

	],
	:linuxringfolderfiles = 	[

	],
	:ubunturingfolderfiles = 	[

	],
	:fedoraringfolderfiles = 	[

	],
	:freebsdringfolderfiles = 	[

	],
	:macosringfolderfiles = 	[

	],
	:run = "ring main.ring",
	:windowsrun = "",
	:linuxrun = "",
	:macosrun = "",
	:ubunturun = "",
	:fedorarun = "",
	:setup = "ring src/utils/install.ring",
	:windowssetup = "",
	:linuxsetup = "",
	:macossetup = "",
	:ubuntusetup = "",
	:fedorasetup = "",
	:remove = "ring src/utils/uninstall.ring",
	:windowsremove = "",
	:linuxremove = "",
	:macosremove = "",
	:ubunturemove = "",
	:fedoraremove = "",
    :remotefolder = "ring-cffi",
    :branch = "master",
    :providerusername = "ysdragon",
    :providerwebsite = "github.com"
]
