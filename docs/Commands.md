# Commands

Doll's commands are divided into several subsystems:

- [**Core**](Commands-Core.md) hosts the most fundamental systems to Doll, such
  as memory management, logging, and configuration.
- [**Front**](Commands-Front.md) is the frontend to Doll. It enables easy
  initialization and usage of Doll's lower level systems, and gives a simplified
  input and setup model.
- [**Gfx**](Commands-Gfx.md) is the graphics system. It holds the lower level
  rendering APIs and some higher level commands for interfacing with the system,
  like sprites, layers, and text.
- [**IO**](Commands-IO.md) is responsible for accessing and managing data. Doll
  has a comprehensive and extensible VFS (virtual file system) model for
  accessing and managing files no matter what the source is. It also sports an
  asynchronous IO system so you don't have to stall for each and every file
  operation to complete.
- [**Math**](Commands-Math.md) has high-level math commands ranging from basic
  scalar operations to vectors and matrices.
- [**OS**](Commands-OS.md) holds low-level operating system abstractions which
  allows Doll to ignore which OS it's running on when it doesn't matter, or
  implement specialized operations when it does. Includes monitor enumeration,
  and window and event management.
- [**Snd**](Commands-Snd.md) manages audio. Supports playback of wave files and
  and mixing of various tracks, with controls for each track.
- [**Util**](Commands-Util.md) has various utility functions for things that
  don't quite fit anywhere else.

Additionally, Doll implements the [Axlib](https://github.com/axia-sw/axlib)
APIs.

- [**Strings**](Commands-Axlib-String.md)
- [**Arrays and lists**](Commands-Axlib-ArraysAndLists.md)
- [**Time**](Commands-Axlib-Time.md)
