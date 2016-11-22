#ifndef DOLL_ENGINE__FIXED_DIR
# define DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_)
# define DOLL_ENGINE__UNDEF__FIXED_DIR
#endif

#ifndef DOLL_ENGINE__DIR
# define DOLL_ENGINE__DIR(RW_,LongName_,ShortName_,DefVal_) DOLL_ENGINE__FIXED_DIR(RW_,LongName_,ShortName_)
# define DOLL_ENGINE__UNDEF__DIR
#endif

// NOTE: In the following directory listings, "<AppName>" refers to a
//       potentially configurable application name. This may actually be a
//       company name followed by an app name. e.g., "Key/Rewrite" such that a
//       separate shared directory exists.

// Directory the process launched in
DOLL_ENGINE__FIXED_DIR(R, Launch      , Launch    ) // core_getLaunchDir()
// Directory of the executable
DOLL_ENGINE__FIXED_DIR(R, App         , App       ) // core_getAppDir()
// Directory of the main configuration file (or $App: if invalid)
DOLL_ENGINE__FIXED_DIR(R, MainConfig  , Cfg       ) // core_getCfgDir()
// Local application data folder partitioned for this app ("%LocalAppData%/<AppName>")
DOLL_ENGINE__FIXED_DIR(W, LocalAppData, AppData   ) // core_getAppDataDir()
// Roaming application data folder partitioned for this app ("%AppData%/../Roaming/<AppName>")
DOLL_ENGINE__FIXED_DIR(W, CloudAppData, Roaming   ) // core_getRoamingDir()
// User's "My Documents" directory (e.g., "%UserProfile%/Documents")
DOLL_ENGINE__FIXED_DIR(R, Documents   , Docs      ) // core_getDocsDir()
// Writable location for documents specific to the app ("$Docs:<AppName>")
DOLL_ENGINE__FIXED_DIR(W, AppDocuments, AppDocs   ) // core_getAppDocsDir()

// Root folder of the game (e.g., "$App:GameData")
DOLL_ENGINE__DIR(R,Root            , Root   , "$App:"          ) // core_getRootDir()

// Folder that saves are written to
DOLL_ENGINE__DIR(W,Saves           , Sav    , "$AppDocs:sav"   ) // core_getSavDir()
// Folder that logs are written to
DOLL_ENGINE__DIR(W,Logs            , Log    , "$AppData:log"   ) // core_getLogDir()

// Abstract / miscellaneous data folder
DOLL_ENGINE__DIR(R,Data            , Dat    , "$Root:dat"      ) // core_getDatDir()
// Fonts and font data folder
DOLL_ENGINE__DIR(R,Fonts           , Fon    , "$Dat:"          ) // core_getFonDir()
// Character descriptors folder
DOLL_ENGINE__DIR(R,Characters      , Chr    , "$Dat:chr"       ) // core_getChrDir()
// Scripts/scenes folder
DOLL_ENGINE__DIR(R,Scripts         , Scr    , "$Dat:scn"       ) // core_getScrDir()

// Top-level graphics folder
DOLL_ENGINE__DIR(R,Graphics        , Gfx    , "$Root:gfx"      ) // core_getGfxDir()
// Prerendered movies
DOLL_ENGINE__DIR(R,Movies          , Mov    , "$Gfx:av"        ) // core_getMovDir()
// Background images
DOLL_ENGINE__DIR(R,Backgrounds     , BG     , "$Gfx:bg"        ) // core_getBGDir()
// Gallery-enabled background images
DOLL_ENGINE__DIR(R,ComputerGraphics, CG     , "$Gfx:cg"        ) // core_getCGDir()

// Top-level sounds folder
DOLL_ENGINE__DIR(R,Sounds          , Snd    , "$Root:snd"      ) // core_getSndDir()
// Background music
DOLL_ENGINE__DIR(R,BackgroundMusic , BGM    , "$Snd:bgm"       ) // core_getBGMDir()
// Voice tracks
DOLL_ENGINE__DIR(R,Koe             , Koe    , "$Snd:koe"       ) // core_getKoeDir()
// Sound effects
DOLL_ENGINE__DIR(R,SoundEffects    , SE     , "$Snd:sfx"       ) // core_getSEDir()

#ifdef DOLL_ENGINE__UNDEF__FIXED_DIR
# undef DOLL_ENGINE__UNDEF__FIXED_DIR
# undef DOLL_ENGINE__FIXED_DIR
#endif

#ifdef DOLL_ENGINE__UNDEF__DIR
# undef DOLL_ENGINE__UNDEF__DIR
# undef DOLL_ENGINE__DIR
#endif
