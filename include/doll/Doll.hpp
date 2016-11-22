#pragma once

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Objbase.h>
#undef min
#undef max

#include <stddef.h>
#include <stdlib.h>

#include "Core/Defs.hpp"
#include "Core/Version.hpp"

#include "Core/Config.hpp"
#include "Core/Logger.hpp"
#include "Core/Memory.hpp"
#include "Core/MemoryTags.hpp"

#include "Front/Frontend.hpp"
#include "Front/Input.hpp"
#include "Front/Setup.hpp"

#include "Gfx/Action.hpp"
#include "Gfx/API.hpp"
#include "Gfx/Layer.hpp"
#include "Gfx/LayerEffect.hpp"
#include "Gfx/OSText.hpp"
#include "Gfx/PrimitiveBuffer.hpp"
#include "Gfx/RenderCommands.hpp"
#include "Gfx/Sprite.hpp"
#include "Gfx/Texture.hpp"
#include "Gfx/Vertex.hpp"

#include "IO/AsyncIO.hpp"
#include "IO/File.hpp"
#include "IO/SysFS.hpp"
#include "IO/VFS.hpp"

#include "Math/Math.hpp"

#include "OS/App.hpp"
#include "OS/Key.hpp"
#include "OS/Monitor.hpp"
#include "OS/OpenGL.hpp"
#include "OS/Window.hpp"

#include "Script/Compiler.hpp"
#include "Script/CompilerMemory.hpp"
#include "Script/Diagnostics.hpp"
#include "Script/Ident.hpp"
#include "Script/LanguageVersion.hpp"
#include "Script/Lexer.hpp"
#include "Script/ProgramData.hpp"
#include "Script/Scripting.hpp"
#include "Script/Source.hpp"
#include "Script/SourceLoc.hpp"
#include "Script/Token.hpp"
#include "Script/Type.hpp"

#include "Snd/ChannelUtil.hpp"
#include "Snd/SoundCore.hpp"
#include "Snd/SoundMgr.hpp"
#include "Snd/WaveFile.hpp"
#include "Snd/WaveFmt.hpp"

#include "Util/ByteSwap.hpp"
#include "Util/CountDivs.hpp"
#include "Util/Counter.hpp"
#include "Util/FuncMgr.hpp"
#include "Util/Hash.hpp"
#include "Util/Messages.hpp"
#include "Util/Metrics.hpp"
#include "Util/ValueStack.hpp"

#include "UX/DialogueWidget.hpp"
#include "UX/Widget.hpp"
