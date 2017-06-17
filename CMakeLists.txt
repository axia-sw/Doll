project(Doll LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.4)

set(EXTDIR "${CMAKE_CURRENT_SOURCE_DIR}/ext")

set(GLEWDIR "${EXTDIR}/glew")
set(PNGDIR "${EXTDIR}/libpng")
set(ZLIBDIR "${EXTDIR}/zlib")
set(AXLIBDIR "${CMAKE_CURRENT_SOURCE_DIR}/../axlib")

set(DollBuildVariantsList_ "DEVELOPMENT;DEBUG;PROFILE;RELEASE")

set(DollDefaultVariant_ "DEBUG")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(DollDefaultVariant_ "DEVELOPMENT")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(DollDefaultVariant_ "DEBUG")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(DollDefaultVariant_ "RELEASE")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
	set(DollDefaultVariant_ "RELEASE")
endif()

#
# Doll's "build variants" are configurations/schemes used to control how Doll's
# code is generated and which internal options are used.
#
# There are four variants:
#
# DEVELOPMENT:
#  - Enables various debug options and systems which should normally be disabled
#    Generally meant for active development of Doll or debugging Doll itself.
#
# DEBUG:
#  - Enables debug systems meant for users of Doll. This is usually what you
#    want if you're developing something with Doll.
#
# PROFILE:
#  - Same as the "RELEASE" option but enables profiling information collection.
#    Useful for profile-guided-optimizations (PGO).
#
# RELEASE:
#  - Generates an optimized library with debug information stripped. Use this
#    when you're ready to ship. Can optionally use the PGO data dumped by the
#    PROFILE stage for extra optimizations specific to your usage patterns.
#

set(DOLL_BUILD_VARIANT "${DollDefaultVariant_}" CACHE STRING "Build variant used by Doll (${DollBuildVariantsList_})")
set_property(CACHE DOLL_BUILD_VARIANT PROPERTY STRINGS ${DollBuildVariantsList_})

if(CMAKE_BUILD_TYPE STREQUAL "")
	set(BuildTy_ "Release")
	if(DOLL_BUILD_VARIANT STREQUAL "DEVELOPMENT")
		set(BuildTy_ "Debug")
	elseif(DOLL_BUILD_VARIANT STREQUAL "DEBUG")
		set(BuildTy_ "RelWithDebInfo")
	elseif(DOLL_BUILD_VARIANT STREQUAL "PROFILE")
		set(BuildTy_ "Release")
	elseif(DOLL_BUILD_VARIANT STREQUAL "RELEASE")
		set(BuildTy_ "Release")
	endif()
	message(WARNING "Build type (CMAKE_BUILD_TYPE) not specified; defaulting to ${BuildTy_}")
	set(CMAKE_BUILD_TYPE "${BuildTy_}")
endif()

set(DollLinksToStaticDefault_ OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(DollLinksToStaticDefault_ ON)
endif()

set(DOLL_LINKS_TO_STATIC_LIBS ${DollLinksToStaticDefault_} CACHE BOOL "Whether Doll links to external libraries statically")
if(DOLL_LINKS_TO_STATIC_LIBS)
	if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	endif()
	set(BUILD_SHARED_LIBS OFF)
endif()

find_package(Git)

set(GIT_IS_INSIDE_REPO 0)
set(GIT_DIR)
if(GIT_FOUND)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" rev-parse --git-dir
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE GIT_REVPARSE_GITDIR_RESULT
		OUTPUT_VARIABLE GIT_DIR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(${GIT_REVPARSE_GITDIR_RESULT} EQUAL 0)
		set(GIT_IS_INSIDE_REPO 1)
	endif()
endif()

set(GIT_BRANCH "")
set(GIT_COMMIT "")
set(GIT_REVNUM 0)
set(GIT_TSTAMP "")

if(${GIT_IS_INSIDE_REPO} EQUAL 1)

	execute_process(
		COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE GIT_HAS_NO_COMMITS
		OUTPUT_VARIABLE GIT_BRANCH
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(${GIT_HAS_NO_COMMITS} EQUAL 0)

		execute_process(
			COMMAND "${GIT_EXECUTABLE}" log -1 --format=%h
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_COMMIT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" log -1 --format=%h
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_COMMIT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" rev-list --count ${GIT_COMMIT}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_REVNUM
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" log -1 "--date=format:%Y-%m-%d %H:%M:%S" --format=%cd
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			OUTPUT_VARIABLE GIT_TSTAMP
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

	else()

		set(GIT_BRANCH "")

	endif()

endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/gitinfo.h.in
    ${CMAKE_BINARY_DIR}/include/doll/private/gitinfo.gen.h
)

set(Doll_OS_SPECIFIC_SOURCES "")
set(Doll_OS_SPECIFIC_HEADERS "")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(Doll_OS_SPECIFIC_SOURCES
		lib/Gfx/macOS/OSText_Cocoa.mm
		lib/OS/macOS/Cocoa.mm
	)
	set(Doll_OS_SPECIFIC_HEADERS
		lib/Gfx/macOS/OSText_Cocoa.hpp
		lib/OS/macOS/Cocoa.h
	)
endif()

set(PNGHEADERS
	${PNGDIR}/png.h
	${PNGDIR}/pngconf.h
	${PNGDIR}/pnglibconf.h
)
set(PNGHEADERS_PRIVATE
	${PNGDIR}/pngpriv.h
	${PNGDIR}/pngdebug.h
	${PNGDIR}/pnginfo.h
	${PNGDIR}/pngstruct.h
)
set(PNGSOURCES
	${PNGDIR}/png.c
	${PNGDIR}/pngerror.c
	${PNGDIR}/pngget.c
	${PNGDIR}/pngmem.c
	${PNGDIR}/pngpread.c
	${PNGDIR}/pngread.c
	${PNGDIR}/pngrio.c
	${PNGDIR}/pngrtran.c
	${PNGDIR}/pngrutil.c
	${PNGDIR}/pngset.c
	${PNGDIR}/pngtrans.c
	${PNGDIR}/pngwio.c
	${PNGDIR}/pngwrite.c
	${PNGDIR}/pngwtran.c
	${PNGDIR}/pngwutil.c
)

set(PNGFILES
	${PNGHEADERS}
	${PNGHEADERS_PRIVATE}
	${PNGSOURCES}
)

set(ZLIBHEADERS
	${ZLIBDIR}/zconf.h
	${ZLIBDIR}/zlib.h
)
set(ZLIBHEADERS_PRIVATE
	${ZLIBDIR}/crc32.h
	${ZLIBDIR}/deflate.h
	${ZLIBDIR}/gzguts.h
	${ZLIBDIR}/inffast.h
	${ZLIBDIR}/inffixed.h
	${ZLIBDIR}/inflate.h
	${ZLIBDIR}/inftrees.h
	${ZLIBDIR}/trees.h
	${ZLIBDIR}/zutil.h
)
set(ZLIBSOURCES
	${ZLIBDIR}/adler32.c
	${ZLIBDIR}/compress.c
	${ZLIBDIR}/crc32.c
	${ZLIBDIR}/deflate.c
	${ZLIBDIR}/gzclose.c
	${ZLIBDIR}/gzlib.c
	${ZLIBDIR}/gzread.c
	${ZLIBDIR}/gzwrite.c
	${ZLIBDIR}/inflate.c
	${ZLIBDIR}/infback.c
	${ZLIBDIR}/inftrees.c
	${ZLIBDIR}/inffast.c
	${ZLIBDIR}/trees.c
	${ZLIBDIR}/uncompr.c
	${ZLIBDIR}/zutil.c
)

set(ZLIBFILES
	${ZLIBHEADERS}
	${ZLIBHEADERS_PRIVATE}
	${ZLIBSOURCES}
)

set(GLEWSOURCES
	${GLEWDIR}/src/glew.c
)
set(GLEWFILES
	${GLEWSOURCES}
)

set(DOLLHEADERS_PRIVATE
	${CMAKE_BINARY_DIR}/include/doll/private/gitinfo.gen.h
)

set(DOLLHEADERS_Base
	include/doll/AxlibConfig.h
	include/doll/Doll.hpp
)
set(DOLLHEADERS_Core
	include/doll/Core/Private/Variants.def.hpp
	include/doll/Core/Private/Variants.hpp
	include/doll/Core/Config.hpp
	include/doll/Core/Defs.hpp
	include/doll/Core/Engine.hpp
	include/doll/Core/EngineDirs.def.hpp
	include/doll/Core/Logger.hpp
	include/doll/Core/Memory.hpp
	include/doll/Core/MemoryTags.hpp
	include/doll/Core/Version.hpp
)
set(DOLLHEADERS_Front
	include/doll/Front/Frontend.hpp
	include/doll/Front/Input.hpp
	include/doll/Front/Setup.hpp
)
set(DOLLHEADERS_Gfx
	include/doll/Gfx/Action.hpp
	include/doll/Gfx/API.hpp
	include/doll/Gfx/API-D3D11.hpp
	include/doll/Gfx/API-GL.hpp
	include/doll/Gfx/APIs.def.hpp
	include/doll/Gfx/Layer.hpp
	include/doll/Gfx/LayerEffect.hpp
	include/doll/Gfx/OSText.hpp
	include/doll/Gfx/PrimitiveBuffer.hpp
	include/doll/Gfx/RenderCommands.hpp
	include/doll/Gfx/Sprite.hpp
	include/doll/Gfx/Texture.hpp
	include/doll/Gfx/Vertex.hpp
)
set(DOLLHEADERS_IO
	include/doll/IO/AsyncIO.hpp
	include/doll/IO/File.hpp
	include/doll/IO/SysFS.hpp
	include/doll/IO/VFS.hpp
	include/doll/IO/VFS-SysFS.hpp
)
set(DOLLHEADERS_Math
	include/doll/Math/Basic.hpp
	include/doll/Math/Bits.hpp
	include/doll/Math/Const.hpp
	include/doll/Math/HalfFloat.hpp
	include/doll/Math/Intrinsics.hpp
	include/doll/Math/Math.hpp
	include/doll/Math/Matrix.hpp
	include/doll/Math/SIMD.hpp
	include/doll/Math/Types.hpp
	include/doll/Math/Vector.hpp
)
set(DOLLHEADERS_OS
	include/doll/OS/App.hpp
	include/doll/OS/Key.hpp
	include/doll/OS/Monitor.hpp
	include/doll/OS/OpenGL.hpp
	include/doll/OS/Window.hpp
)
set(DOLLHEADERS_Script
	include/doll/Script/AST.hpp
	include/doll/Script/Compiler.hpp
	include/doll/Script/CompilerMemory.hpp
	include/doll/Script/Diagnostics.def.hpp
	include/doll/Script/Diagnostics.hpp
	include/doll/Script/DiagnosticsFmt.hpp
	include/doll/Script/Ident.hpp
	include/doll/Script/LanguageVersion.hpp
	include/doll/Script/Lexer.hpp
	include/doll/Script/Operator.hpp
	include/doll/Script/Parser.hpp
	include/doll/Script/ProgramData.hpp
	include/doll/Script/RuntimeConf.hpp
	include/doll/Script/Scripting.hpp
	include/doll/Script/Source.hpp
	include/doll/Script/SourceLoc.hpp
	include/doll/Script/Token.hpp
	include/doll/Script/Type.hpp
	include/doll/Script/Types.def.hpp
)
set(DOLLHEADERS_Snd
	include/doll/Snd/API-XA2.hpp
	include/doll/Snd/ChannelUtil.hpp
	include/doll/Snd/SoundCore.hpp
	include/doll/Snd/SoundMgr.hpp
	include/doll/Snd/WaveFile.hpp
	include/doll/Snd/WaveFmt.hpp
)
set(DOLLHEADERS_Types
	include/doll/Types/IntVector2.hpp
	include/doll/Types/Rect.hpp
)
set(DOLLHEADERS_Util
	include/doll/Util/ByteSwap.hpp
	include/doll/Util/Casting.hpp
	include/doll/Util/CountDivs.hpp
	include/doll/Util/Counter.hpp
	include/doll/Util/FuncMgr.hpp
	include/doll/Util/Hash.hpp
	include/doll/Util/Messages.hpp
	include/doll/Util/Metrics.hpp
	include/doll/Util/SafeDX.hpp
	include/doll/Util/ValueStack.hpp
)
set(DOLLHEADERS_UX
	include/doll/UX/DialogueWidget.hpp
	include/doll/UX/Widget.hpp
)

set(DOLLHEADERS
	${DOLLHEADERS_Base}
	${DOLLHEADERS_Core}
	${DOLLHEADERS_Front}
	${DOLLHEADERS_Gfx}
	${DOLLHEADERS_IO}
	${DOLLHEADERS_Math}
	${DOLLHEADERS_OS}
	${DOLLHEADERS_Script}
	${DOLLHEADERS_Snd}
	${DOLLHEADERS_Types}
	${DOLLHEADERS_Util}
	${DOLLHEADERS_UX}
)

set(DOLLSOURCES_Base
	lib/DollAxlib.cpp
)
set(DOLLSOURCES_Core
	lib/Core/Config.cpp
	lib/Core/Defs.cpp
	lib/Core/Engine.cpp
	lib/Core/Logger.cpp
	lib/Core/Memory.cpp
)
set(DOLLSOURCES_Front
	lib/Front/Frontend.cpp
	lib/Front/Input.cpp
	lib/Front/Setup.cpp
)
set(DOLLSOURCES_Gfx
	lib/Gfx/Action.cpp
	lib/Gfx/API.cpp
	lib/Gfx/API-D3D11.cpp
	lib/Gfx/API-GL.cpp
	lib/Gfx/Layer.cpp
	lib/Gfx/OSText.cpp
	lib/Gfx/PrimitiveBuffer.cpp
	lib/Gfx/RenderCommands.cpp
	lib/Gfx/Sprite.cpp
	lib/Gfx/Texture.cpp
)
set(DOLLSOURCES_IO
	lib/IO/AsyncIO.cpp
	lib/IO/File.cpp
	lib/IO/SysFS.cpp
	lib/IO/VFS.cpp
	lib/IO/VFS-SysFS.cpp
)
set(DOLLSOURCES_Math
	lib/Math/Matrix.cpp
	lib/Math/Vector.cpp
)
set(DOLLSOURCES_OS
	lib/OS/App.cpp
	lib/OS/Monitor.cpp
	lib/OS/OpenGL.cpp
	lib/OS/Window.cpp
)
set(DOLLSOURCES_Script
	lib/Script/AST.cpp
	lib/Script/Compiler.cpp
	lib/Script/CompilerMemory.cpp
	lib/Script/Diagnostics.cpp
	lib/Script/DiagnosticsFmt.cpp
	lib/Script/Ident.cpp
	lib/Script/LanguageVersion.cpp
	lib/Script/Lexer.cpp
	lib/Script/Parser.cpp
	lib/Script/ProgramData.cpp
	lib/Script/Scripting.cpp
	lib/Script/Source.cpp
	lib/Script/Token.cpp
	lib/Script/Type.cpp
)
set(DOLLSOURCES_Snd
	lib/Snd/API-XA2.cpp
	lib/Snd/ChannelUtil.cpp
	lib/Snd/SoundCore.cpp
	lib/Snd/SoundMgr.cpp
	lib/Snd/WaveFile.cpp
	lib/Snd/WaveFmt.cpp
)
set(DOLLSOURCES_Util
	lib/Util/Counter.cpp
	lib/Util/FuncMgr.cpp
	lib/Util/Hash.cpp
	lib/Util/Messages.cpp
	lib/Util/Metrics.cpp
)
set(DOLLSOURCES_UX
	lib/UX/DialogueWidget.cpp
	lib/UX/Widget.cpp
)

set(DOLLSOURCES
	${DOLLSOURCES_Base}
	${DOLLSOURCES_Core}
	${DOLLSOURCES_Front}
	${DOLLSOURCES_Gfx}
	${DOLLSOURCES_IO}
	${DOLLSOURCES_Math}
	${DOLLSOURCES_OS}
	${DOLLSOURCES_Script}
	${DOLLSOURCES_Snd}
	${DOLLSOURCES_Util}
	${DOLLSOURCES_UX}
)

set(DOLL_ALLSOURCES_HERE
	${PNGFILES}
	${ZLIBFILES}
	${GLEWFILES}

	${DOLLSOURCES}
	${Doll_OS_SPECIFIC_SOURCES}

	${DOLLHEADERS_PRIVATE}
	${DOLLHEADERS}
	${Doll_OS_SPECIFIC_HEADERS}
)

set(AXLIB_SOURCES
	${AXLIBDIR}/include/ax_array.hpp
	${AXLIBDIR}/include/ax_assert.h
	${AXLIBDIR}/include/ax_config.h
	${AXLIBDIR}/include/ax_dictionary.hpp
	${AXLIBDIR}/include/ax_fiber.h
	${AXLIBDIR}/include/ax_intdatetime.h
	${AXLIBDIR}/include/ax_list.hpp
	${AXLIBDIR}/include/ax_logger.h
	${AXLIBDIR}/include/ax_manager.hpp
	${AXLIBDIR}/include/ax_memory.h
	${AXLIBDIR}/include/ax_platform.h
	${AXLIBDIR}/include/ax_printf.h
	${AXLIBDIR}/include/ax_string.h
	${AXLIBDIR}/include/ax_thread.h
	${AXLIBDIR}/include/ax_time.h
	${AXLIBDIR}/include/ax_types.h
	${AXLIBDIR}/include/ax_typetraits.hpp
	${AXLIBDIR}/include/axlib.h
	${AXLIBDIR}/include/axlib.hpp
)

set(DOLL_ALLSOURCES
	${DOLL_ALLSOURCES_HERE}
	${AXLIB_SOURCES}
)

source_group("ext\\axlib" FILES ${AXLIB_SOURCES})
source_group("ext\\glew" FILES ${GLEWFILES})
source_group("ext\\libpng" FILES ${PNGFILES})
source_group("ext\\zlib" FILES ${ZLIBFILES})

set(HeaderComponents_
	"Core"
	"Front"
	"Gfx"
	"IO"
	"Math"
	"OS"
	"Script"
	"Snd"
	"Types"
	"Util"
	"UX"
)
set(SourceComponents_
	"Core"
	"Front"
	"Gfx"
	"IO"
	"Math"
	"OS"
	"Script"
	"Snd"
	"Util"
	"UX"
)

source_group("src\\Base" FILES ${DOLLHEADERS_Base})
source_group("src\\Base" FILES ${DOLLSOURCES_Base})

foreach(component_ IN ITEMS ${HeaderComponents_})
	source_group("src\\${component_}" FILES ${DOLLHEADERS_${component_}})
endforeach()
foreach(component_ IN ITEMS ${SourceComponents_})
	source_group("src\\${component_}" FILES ${DOLLSOURCES_${component_}})
endforeach()

source_group(TREE "${CMAKE_BINARY_DIR}/include/doll" PREFIX "src\\.gen" FILES ${DOLLHEADERS_PRIVATE})

set_source_files_properties(${GLEWSOURCES} PROPERTIES LANGUAGE C)
set_source_files_properties(${PNGSOURCES} PROPERTIES LANGUAGE C)
set_source_files_properties(${ZLIBSOURCES} PROPERTIES LANGUAGE C)

add_library(Doll
	${DOLL_ALLSOURCES}
)

set_target_properties(Doll PROPERTIES
    CXX_STANDARD          14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        ON
)

target_include_directories(
	Doll
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC ${AXLIBDIR}/include
	PRIVATE ${GLEWDIR}/include
	PRIVATE ${EXTDIR}/STB
	PRIVATE ${CMAKE_BINARY_DIR}/include
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_include_directories(
		Doll
		PRIVATE ${EXTDIR}/XA2
	)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	find_package(glfw3 3.2 REQUIRED)
	find_package(OpenGL REQUIRED)
	target_include_directories(Doll PRIVATE ${OPENGL_INCLUDE_DIR})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_link_libraries(
		Doll
		PUBLIC gdiplus
		PUBLIC opengl32
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(
		Doll
		PUBLIC pthread
		PUBLIC ${GLFW_STATIC_LIBRARIES}
		PUBLIC "-framework Foundation"
		PUBLIC "-framework AppKit"
		PUBLIC "-framework Cocoa"
	)
else()
	target_link_libraries(
		Doll
		PUBLIC pthread
		PUBLIC ${GLFW_LIBRARIES}
	)
endif()

target_compile_definitions(
	Doll
	PRIVATE GLEW_STATIC=1
	PRIVATE DOLL__BUILD=1
	PRIVATE PNG_THREAD_UNSAFE_OK=1
	PRIVATE PNG_NO_MMX_CODE=1
	PRIVATE PNG_NO_MNG_FEATURES=1
)

target_include_directories(
	Doll
	PRIVATE ${ZLIBDIR}
	PRIVATE ${PNGDIR}
)

if(DOLL_BUILD_VARIANT STREQUAL "DEVELOPMENT")
	target_compile_definitions(
		Doll
		PUBLIC _DEBUG=1
		PUBLIC AX_DEBUG_ENABLED=1
		PRIVATE DOLL_BUILD_VARIANT=DOLL_VARIANT_DEVELOPMENT
	)
elseif(DOLL_BUILD_VARIANT STREQUAL "DEBUG")
	target_compile_definitions(
		Doll
		PUBLIC _DEBUG=1
		PUBLIC AX_DEBUG_ENABLED=1
		PRIVATE DOLL_BUILD_VARIANT=DOLL_VARIANT_DEBUG
	)
elseif(DOLL_BUILD_VARIANT STREQUAL "PROFILE")
	target_compile_definitions(
		Doll
		PUBLIC NDEBUG=1
		PUBLIC AX_DEBUG_ENABLED=0
		PRIVATE DOLL_BUILD_VARIANT=DOLL_VARIANT_PROFILE
	)
elseif(DOLL_BUILD_VARIANT STREQUAL "RELEASE")
	target_compile_definitions(
		Doll
		PUBLIC NDEBUG=1
		PUBLIC AX_DEBUG_ENABLED=0
		PRIVATE DOLL_BUILD_VARIANT=DOLL_VARIANT_RELEASE
	)
endif()

set(DollBuildSuffix "-${DOLL_BUILD_VARIANT}")

set(DollExtraSuffix "")
if(NOT GIT_BRANCH STREQUAL "")
	set(DollExtraSuffix "-${GIT_BRANCH}-r${GIT_REVNUM}")
endif()

set(DollSystemSuffix "-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(DollSuffixes "${DollBuildSuffix}${DollExtraSuffix}${DollSystemSuffix}")

set_target_properties(Doll PROPERTIES
	OUTPUT_NAME "Doll${DollSuffixes}"
)

