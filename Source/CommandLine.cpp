/* Copyright (c) 2022-2024 Dreamy Cecil
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "CommandLine.h"

#include <ZipLib/ZipFile.h>
#include <ZipLib/ZipArchiveEntry.h>

// Argument descriptions
const char *_astrArgDesc[] = {
#if _DREAMY_UNIX
  ARG_ROOT   " : Root directory of some classic Serious Sam game (e.g. \"-r /usr/games/SeriousSam/\")",
#else
  ARG_ROOT   " : Root directory of some classic Serious Sam game (e.g. \"-r C:/SeriousSam/\")",
#endif
  ARG_OUTPUT " : Output GRO file. Can be an absolute path or relative to the root directory (e.g. \"-o MyMap.gro\")",
  ARG_WORLD  " : Scans dictionary of this WLD file and adds all dependencies in the list (e.g. \"-w Levels/MyLevel.wld\")",
  ARG_STORE  " : Don't compress files of a certain type when packing them (e.g. \"-s wld\" or \"-s .ogg\")",
  ARG_DEPEND " : Ignore certain resources or entire GRO archives (e.g. \"-d MyResources.gro\" or \"-d Texture.tex\")",
  ARG_FLAGS  " : Set certain behavior flags (e.g. \"-f dep\"):"
  "\n    ssr - mark world(s) as being from Serious Sam Revolution (detects automatically, this is optional)"
  "\n    ini - include INI configs alongside their respective MDL files (to share between mappers)"
  "\n    ogg - check for existence of OGG files if can't find MP3 files (mostly for The First Encounter)"
  "\n    dep - display list of dependencies of world files without packing anything into the GRO"
  "\n    gro - automatically detect GRO files from certain games instead of adding them manually via \"-d\"",
  ARG_PAUSE  " : Pause program execution before closing the console application to be able to see the output",
};

static void ParseRoot(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // Root has been already set
  if (_strRoot != "") {
    CMessageException::Throw("'%s' command cannot be used more than once!", ARG_ROOT);
  }

  // No root path
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a path to a game folder after '%s'!", ARG_ROOT);
  }

  // Set root path
  _strRoot = *itNext;
  ++it;

  // Make absolute path
  if (_strRoot.IsRelative()) _strRoot = GetCurrentPath() + _strRoot;
  _strRoot.Normalize();

  // Add another separator at the end
  if (!_strRoot.PathSeparatorAt(_strRoot.length() - 1)) {
    _strRoot += '/';
  }
};

static void ParseOutput(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No output path
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a path to an output file after '%s'!", ARG_OUTPUT);
  }

  _strGRO = *itNext;
  ++it;
};

static void ParseWorld(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No world path
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a path to a world file after '%s'!", ARG_WORLD);
  }

  // Add relative path to the world
  Str_t strWorld = *itNext;

  _aScanFiles.push_back(strWorld);
  AddFile(strWorld);

  ++it;
};

static void ParseStoreFile(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No filename filter
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a filename filter '%s'!", ARG_STORE);
  }

  // Get filename filter
  Str_t strExt = StrToLower(*itNext);
  ++it;

  // Add a period to the extension
  if (strExt[0] != '.') {
    strExt = "." + strExt;
  }

  // Add lowercase extension
  _aNoCompression.push_back(strExt);
};

static void ParseDependency(Strings_t &astrDependencies, Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No file path
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a path to a file after '%s'!", ARG_DEPEND);
  }

  // Add dependency file
  astrDependencies.push_back(*itNext);
  ++it;
};

static void ParseFlag(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No flags
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected flags after '%s'!", ARG_FLAGS);
  }

  // Add specific flag
  const Str_t strFlag = StrToLower(*itNext);
  ++it;

  if (strFlag == "ssr") {
    _iFlags |= SCAN_SSR;
  } else if (strFlag == "ini") {
    _iFlags |= SCAN_INI;
  } else if (strFlag == "ogg") {
    _iFlags |= SCAN_OGG;
  } else if (strFlag == "dep") {
    _iFlags |= SCAN_DEP;
  } else if (strFlag == "gro") {
    _iFlags |= SCAN_GRO;
  }
};

// Parse command line arguments
void ParseArguments(Strings_t &aArgs) {
  // Dependency files and GROs
  Strings_t astrDependencies;

  Strings_t::const_iterator it = aArgs.begin();

  while (it != aArgs.end()) {
    // Get the string and advance the argument
    const Str_t &str = *(it++);

    if (str == ARG_ROOT) {
      ParseRoot(it, aArgs);

    } else if (str == ARG_OUTPUT) {
      ParseOutput(it, aArgs);

    } else if (str == ARG_WORLD) {
      ParseWorld(it, aArgs);

    } else if (str == ARG_STORE) {
      ParseStoreFile(it, aArgs);

    } else if (str == ARG_DEPEND) {
      ParseDependency(astrDependencies, it, aArgs);

    } else if (str == ARG_FLAGS) {
      ParseFlag(it, aArgs);

    } else if (str == ARG_PAUSE) {
      // Pause at the end of execution
      _bPauseAtTheEnd = true;

    // Invalid arguments
    } else {
      CMessageException::Throw("Invalid command line argument: '%s'", str.c_str());
    }
  }

  // No root path set
  if (_strRoot == "") {
    CMessageException::Throw("Game folder path has not been set! Please use '%s <game folder path>'!", ARG_ROOT);
  }

  if (!OnlyDep()) {
    // No output path set
    if (_strGRO == "") {
      CMessageException::Throw("Output GRO file has not been set! Please use '%s <GRO file>'!", ARG_OUTPUT);
    }

    // Relative path to the output GRO
    if (_strGRO.IsRelative()) {
      _strGRO = _strRoot + _strGRO;
    }
  }

  // Add GRO files from games automatically
  if (DetectGRO()) {
    try {
      EGameType eGame = DetectGame(_strRoot);
      IgnoreGame(eGame, false);

    } catch (CMessageException &ex) {
      std::cout << ex.what() << '\n';
    }
  }

  // Go through dependencies
  for (size_t iDepend = 0; iDepend < astrDependencies.size(); ++iDepend) {
    const Str_t &strDepend = astrDependencies[iDepend];

    // Copy path for various checks
    const CPath strCheck = StrToLower(strDepend);

    // Scan entire GRO archive
    if (strCheck.GetFileExt() == ".gro") {
      IgnoreGRO(strDepend);
      continue;
    }

    // Skip if it doesn't exist
    if (!FileExists(_strRoot + strDepend)) {
      std::cout << '"' << strDepend << "\" does not exist!\n";
      continue;
    }

    size_t iHash;

    // Add to existing dependencies if it's not there
    if (!InDepends(strCheck, &iHash)) {
      _aStdDepends.push_back(iHash);
    }
  }
};

// Detect root game directory from a full path to the file
static size_t DetermineRootDir(const CPath &strFile, const CPath &strDefaultFolderInRoot, EGameType &eGame) {
  CPath strCurrentDir = strFile;
  size_t iFrom = NULL_POS;
  size_t iGameDir;

  eGame = GAME_NONE;

  // Go up each directory and try to detect the game from there
  do {
    iGameDir = strCurrentDir.find_last_of("/\\", iFrom);

    // No more directories
    if (iGameDir == NULL_POS) break;

    // Erase everything after the last slash
    eGame = DetectGame(strCurrentDir.erase(iGameDir + 1));

    // Exclude last slash from the next search
    iFrom = iGameDir - 1;

  // Go until the game type is determined
  } while (eGame == GAME_NONE);

  // Try to determine root directory from the default path under it as a backup
  if (eGame == GAME_NONE && strDefaultFolderInRoot != "") {
    iGameDir = strFile.GoUpUntilDir(strDefaultFolderInRoot);

  // Include a slash from detected game directory
  } else {
    ++iGameDir;
  }

  if (iGameDir == NULL_POS) {
    CMessageException::Throw("You may only open '%s' files from within '%s' folder of a game directory!",
      strFile.GetFileExt().c_str(), strDefaultFolderInRoot.c_str());
  }

  return iGameDir;
};

// Prompt the user if opening individual files
static void ManualSetup(const CPath &strFile) {
  // Only show dependencies
  if (ConsoleYN("Show world dependencies instead of packing?", false)) {
    _iFlags |= SCAN_DEP;
    return;
  }

  // Set output GRO
  Str_t strCustomGRO = ConsoleInput("Enter output GRO file (blank for automatic): ");

  if (strCustomGRO == "") {
    _strGRO = _strRoot + "DreamyGRO_" + strFile.GetFileName() + ".gro";

  } else {
    _strGRO = _strRoot + strCustomGRO;

    // Append extension
    Str_t strCheck = StrToLower(_strGRO.GetFileExt());

    if (strCheck != ".gro") {
      _strGRO += ".gro";
    }
  }

  // Store music files
  if (ConsoleYN("Pack uncompressed music files?", true)) {
    _aNoCompression.push_back(".ogg");
    _aNoCompression.push_back(".mp3");
  }

  // Store the world file
  if (ConsoleYN("Pack uncompressed world file?", false)) {
    _aNoCompression.push_back(".wld");
  }
};

// Build parameters from the full path and return file path relative to the root directory
Str_t FromFullFilePath(const CPath &strFile, const CPath &strDefaultFolderInRoot) {
  // Determine game and root directory
  EGameType eGame;
  size_t iDir = DetermineRootDir(strFile, strDefaultFolderInRoot, eGame);
  _strRoot = strFile.substr(0, iDir);

  Str_t strGameType;

  switch (eGame) {
    case GAME_TFE: strGameType = "(TFE)"; break;
    case GAME_TSE: strGameType = "(TSE)"; break;
    case GAME_REV: strGameType = "(SSR)"; break;
    default: strGameType = "(unknown)";
  }

  std::cout << "Assumed game directory " << strGameType << ": " << _strRoot << "\n\n";

  // Setup for individual files
  ManualSetup(strFile);

  // Get relative path
  Str_t strRelative = strFile.substr(iDir);
  _aScanFiles.push_back(strRelative);

  IgnoreGame(eGame, true);
  return strRelative;
};

// Detect default GRO packages in some directory to determine the game
EGameType DetectGame(const Str_t &strDir) {
  // TSE 1.05 - 1.10
  if (FileExists(strDir + "SE1_10.gro") || FileExists(strDir + "SE1_00.gro")) {
    return GAME_TSE;
  // Revolution
  } else if (FileExists(strDir + "All_01.gro") && FileExists(strDir + "All_02.gro")) {
    return GAME_REV;
  // TFE 1.00 - 1.05
  } else if (FileExists(strDir + "1_00c.gro") || FileExists(strDir + "1_00_a.gro") || FileExists(strDir + "1_00.gro")) {
    return GAME_TFE;
  }

  return GAME_NONE;
};

// Automatically ignore GRO files from a specific game
void IgnoreGame(EGameType eGame, bool bSetFlagsFromGame) {
  switch (eGame) {
    // Ignore TSE resources
    case GAME_TSE: {
      std::cout << "\nDetected GRO packages from The Second Encounter!\n";

      IgnoreGRO("SE1_00.gro");
      IgnoreGRO("SE1_10.gro");
      IgnoreGRO("SE1_00_Extra.gro");
      IgnoreGRO("SE1_00_ExtraTools.gro");
      IgnoreGRO("SE1_00_Music.gro");
      IgnoreGRO("1_04_patch.gro");
      IgnoreGRO("1_07_tools.gro");
    } break;

    // Ignore SSR resources
    case GAME_REV: {
      std::cout << "\nDetected GRO packages from Revolution!\n";
      if (bSetFlagsFromGame) _iFlags |= SCAN_SSR;

      IgnoreGRO("All_01.gro");
      IgnoreGRO("All_02.gro");
    } break;

    // Ignore TFE resources
    case GAME_TFE: {
      std::cout << "\nDetected GRO packages from The First Encounter!\n";
      if (bSetFlagsFromGame) _iFlags |= SCAN_OGG;

      IgnoreGRO("1_00.gro");
      IgnoreGRO("1_00_a.gro");
      IgnoreGRO("1_00c.gro");
      IgnoreGRO("1_00c_scripts.gro");
      IgnoreGRO("1_00_ExtraTools.gro");
      IgnoreGRO("1_00_music.gro");
      IgnoreGRO("1_04_patch.gro");
    } break;

    // Unknown game
    default:
      std::cout << "\nCouldn't detect any default GRO packages!\n";
  }
};

// Ignore dependencies from a GRO file
void IgnoreGRO(const Str_t &strGRO) {
  // Skip if it doesn't exist
  if (!FileExists(_strRoot + strGRO)) {
    std::cout << '"' << strGRO << "\" does not exist!\n";
    return;
  }

  ZipArchive::Ptr pGRO = ZipFile::Open(_strRoot + strGRO);

  for (int i = 0; i < (int)pGRO->GetEntriesCount(); ++i) {
    ZipArchiveEntry::Ptr file = pGRO->GetEntry(i);

    // Ignore directory entries
    if (file->IsDirectory()) {
      continue;
    }

    // Make hash out of the filename in lowercase
    const Str_t strCheck = StrToLower(file->GetFullName());
    size_t iHash;

    // Add to existing dependencies if it's not there
    if (!InDepends(strCheck, &iHash)) {
      _aStdDepends.push_back(iHash);
    }
  }
};
