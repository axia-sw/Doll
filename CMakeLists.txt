project(Doll LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.4)

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
		lib/OS/macOS/Cocoa.mm
	)
	set(Doll_OS_SPECIFIC_HEADERS
		lib/OS/macOS/Cocoa.h
	)
endif()

add_library(Doll
	ext/glew/src/glew.c
	lib/DollAxlib.cpp
	lib/Core/Config.cpp
	lib/Core/Defs.cpp
	lib/Core/Engine.cpp
	lib/Core/Logger.cpp
	lib/Core/Memory.cpp
	lib/Front/Frontend.cpp
	lib/Front/Input.cpp
	lib/Front/Setup.cpp
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
	lib/IO/AsyncIO.cpp
	lib/IO/File.cpp
	lib/IO/SysFS.cpp
	lib/IO/VFS.cpp
	lib/IO/VFS-SysFS.cpp
	lib/Math/Matrix.cpp
	lib/Math/Vector.cpp
	lib/OS/App.cpp
	lib/OS/Monitor.cpp
	lib/OS/OpenGL.cpp
	lib/OS/Window.cpp
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
	lib/Snd/API-XA2.cpp
	lib/Snd/ChannelUtil.cpp
	lib/Snd/SoundCore.cpp
	lib/Snd/SoundMgr.cpp
	lib/Snd/WaveFile.cpp
	lib/Snd/WaveFmt.cpp
	lib/Util/Counter.cpp
	lib/Util/FuncMgr.cpp
	lib/Util/Hash.cpp
	lib/Util/Messages.cpp
	lib/Util/Metrics.cpp
	lib/UX/DialogueWidget.cpp
	lib/UX/Widget.cpp
	${Doll_OS_SPECIFIC_SOURCES}
	${CMAKE_BINARY_DIR}/include/doll/private/gitinfo.gen.h
	include/doll/AxlibConfig.h
	include/doll/Doll.hpp
	include/doll/Core/Config.hpp
	include/doll/Core/Defs.hpp
	include/doll/Core/Engine.hpp
	include/doll/Core/EngineDirs.def.hpp
	include/doll/Core/Logger.hpp
	include/doll/Core/Memory.hpp
	include/doll/Core/MemoryTags.hpp
	include/doll/Core/Version.hpp
	include/doll/Front/Frontend.hpp
	include/doll/Front/Input.hpp
	include/doll/Front/Setup.hpp
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
	include/doll/IO/AsyncIO.hpp
	include/doll/IO/File.hpp
	include/doll/IO/SysFS.hpp
	include/doll/IO/VFS.hpp
	include/doll/IO/VFS-SysFS.hpp
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
	include/doll/OS/App.hpp
	include/doll/OS/Key.hpp
	include/doll/OS/Monitor.hpp
	include/doll/OS/OpenGL.hpp
	include/doll/OS/Window.hpp
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
	include/doll/Snd/API-XA2.hpp
	include/doll/Snd/ChannelUtil.hpp
	include/doll/Snd/SoundCore.hpp
	include/doll/Snd/SoundMgr.hpp
	include/doll/Snd/WaveFile.hpp
	include/doll/Snd/WaveFmt.hpp
	include/doll/Types/IntVector2.hpp
	include/doll/Types/Rect.hpp
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
	include/doll/UX/DialogueWidget.hpp
	include/doll/UX/Widget.hpp
	${Doll_OS_SPECIFIC_HEADERS}
)

set_source_files_properties(ext/glew/src/glew.c PROPERTIES LANGUAGE C)

set_target_properties(Doll PROPERTIES
    CXX_STANDARD          14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        ON
)

target_include_directories(
	Doll
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../axlib/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext/glew/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext/STB
	PRIVATE ${CMAKE_BINARY_DIR}/include
)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_include_directories(
		Doll
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext/XA2
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
		PUBLIC glfw
		PUBLIC ${OPENGL_gl_LIBRARY}
		PUBLIC "-framework Foundation"
		PUBLIC "-framework AppKit"
		PUBLIC "-framework Cocoa"
	)
else()
	target_link_libraries(
		Doll
		PUBLIC GL
		PUBLIC pthread
		PUBLIC glfw
	)
endif()

target_compile_definitions(
	Doll
	PRIVATE GLEW_STATIC=1
	PRIVATE DOLL__BUILD=1
)


find_package(PNG REQUIRED)

target_include_directories(
	Doll
	PRIVATE ${PNG_INCLUDE_DIRS}
)
target_link_libraries(
	Doll
	PRIVATE ${PNG_LIBRARY}
)
target_compile_definitions(
	Doll
	PRIVATE ${PNG_DEFINITIONS}
)
