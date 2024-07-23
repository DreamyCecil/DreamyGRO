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
  ARG_ROOT   " : Root directory of some classic Serious Sam game (e.g. \"" ARG_ROOT " /usr/games/SeriousSam/\")",
#else
  ARG_ROOT   " : Root directory of some classic Serious Sam game (e.g. \"" ARG_ROOT " C:/SeriousSam/\")",
#endif
  ARG_MOD    " : Specifies the name of a mod folder where the files are being included from (e.g. \"" ARG_MOD " MyMod\")",
  ARG_OUTPUT " : Output GRO file. Can be an absolute path or relative to the root directory (e.g. \"" ARG_OUTPUT " MyMap.gro\")",
  ARG_SCAN   " : Includes this file for scanning and adds its dependencies (e.g. \"" ARG_SCAN " Levels/MyLevel.wld\" or \"" ARG_SCAN " Bin/MyEntities.dll\")",
  ARG_STORE  " : Don't compress files of a certain type when packing them (e.g. \"" ARG_STORE " wld\" or \"" ARG_STORE " .ogg\")",
  ARG_DEPEND " : Ignore certain resources or entire GRO archives (e.g. \"" ARG_DEPEND " MyResources.gro\" or \"" ARG_DEPEND " Texture.tex\")",
  ARG_FLAGS  " : Set certain behavior flags (e.g. \"" ARG_FLAGS " dep\"):"
  "\n    ssr - mark file(s) as being from Serious Sam Revolution (detects automatically from WLD files)"
  "\n    ini - include INI configs alongside their respective MDL files (to share between mappers)"
  "\n    ogg - check for existence of OGG files if can't find MP3 files (mostly for The First Encounter)"
  "\n    dep - display a list of dependencies of included files without packing anything into a GRO"
  "\n    gro - automatically detect GRO files from certain games instead of adding them manually via \"" ARG_DEPEND "\""
  "\n    mod - erase mod directory from paths to dependencies (e.g. packs \"Mods\\MyMod\\Texture1.tex\" as \"Texture1.tex\")",
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

static void ParseMod(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // Mod has been already set
  if (_strMod != "") {
    CMessageException::Throw("'%s' command cannot be used more than once!", ARG_MOD);
  }

  // No mod name
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a mod name after '%s'!", ARG_MOD);
  }

  // Set mod path
  _strMod = "Mods/" + *itNext + "/";
  ++it;
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

static void ParseInclude(Strings_t::const_iterator &it, const Strings_t &aArgs) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No path
  if (itNext == aArgs.end()) {
    CMessageException::Throw("Expected a path to a file after '%s'!", ARG_SCAN);
  }

  // Add relative path to the file
  CPath strFile = *itNext;
  Replace(strFile, '\\', '/'); // Fix slashes

  _aScanFiles.push_back(strFile);

  // The world itself should be packed too
  const Str_t strExt = StrToLower(strFile.GetFileExt());

  if (strExt == ".wld") {
    AddFile(strFile);
  }

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
  } else if (strFlag == "mod") {
    _iFlags |= SCAN_MOD;
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

    if (CompareStrings(str, ARG_ROOT)) {
      ParseRoot(it, aArgs);

    } else if (CompareStrings(str, ARG_MOD)) {
      ParseMod(it, aArgs);

    } else if (CompareStrings(str, ARG_OUTPUT)) {
      ParseOutput(it, aArgs);

    } else if (CompareStrings(str, ARG_SCAN)) {
      ParseInclude(it, aArgs);

    } else if (CompareStrings(str, ARG_STORE)) {
      ParseStoreFile(it, aArgs);

    } else if (CompareStrings(str, ARG_DEPEND)) {
      ParseDependency(astrDependencies, it, aArgs);

    } else if (CompareStrings(str, ARG_FLAGS)) {
      ParseFlag(it, aArgs);

    } else if (CompareStrings(str, ARG_PAUSE)) {
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

    // Relative to the mod directory
    if (_strMod != "") {
      _strGRO = _strMod + _strGRO;
    }

    // Relative to the root directory
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

    // Skip if it doesn't exist under either directory
    const Str_t strUnderMod = _strRoot + _strMod + strDepend;
    const Str_t strUnderRoot = _strRoot + strDepend;

    if (!FileExists(strUnderMod) && !FileExists(strUnderRoot)) {
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
  size_t iDir;

  eGame = GAME_NONE;

  // Go up each directory and try to detect the game from there
  do {
    iDir = strCurrentDir.rfind('/', iFrom);

    // No more directories
    if (iDir == NULL_POS) break;

    // Erase everything after the last slash
    eGame = DetectGame(strCurrentDir.erase(iDir + 1));

    // Exclude last slash from the next search
    iFrom = iDir - 1;

  // Go until the game type is determined
  } while (eGame == GAME_NONE);

  // Include a slash from detected game directory
  if (eGame != GAME_NONE) {
    ++iDir;

  // Try to determine root directory from the default path under it as a backup
  } else if (strDefaultFolderInRoot != "") {
    iDir = strFile.GoUpUntilDir(strDefaultFolderInRoot);
  }

  if (iDir == NULL_POS) {
    CMessageException::Throw("You may only open '%s' files from within '%s' folder of a game directory!",
      strFile.GetFileExt().c_str(), strDefaultFolderInRoot.c_str());
  }

  // Get root directory
  _strRoot = strFile.substr(0, iDir);

  Str_t strGameType;

  switch (eGame) {
    case GAME_TFE: strGameType = "(TFE)"; break;
    case GAME_TSE: strGameType = "(TSE)"; break;
    case GAME_REV: strGameType = "(SSR)"; break;
    default: strGameType = "(unknown)";
  }

  std::cout << "Assumed game directory " << strGameType << ": " << _strRoot << '\n';

  // If path to the file is under a mod directory right after the root
  if (CompareStrings(strFile.substr(iDir, 5), "Mods/")) {
    size_t iModNameEnd = strFile.find('/', iDir + 5);

    // Get mod directory
    if (iModNameEnd != NULL_POS) {
      ++iModNameEnd; // Include a slash

      _strMod = strFile.substr(iDir, iModNameEnd - iDir);
      iDir = iModNameEnd; // Update position to the mod directory

      std::cout << "Assumed mod directory: " << _strMod << '\n';
    }
  }

  std::cout << '\n';
  return iDir;
};

// Prompt the user if opening individual files
static void ManualSetup(const CPath &strFile) {
  // Only show dependencies
  if (ConsoleYN("Display dependencies instead of packing?", false)) {
    _iFlags |= SCAN_DEP;
    return;
  }

  // Set output GRO
  Str_t strCustomGRO = ConsoleInput("Enter output GRO file (blank for automatic): ");

  if (strCustomGRO == "") {
    _strGRO = _strRoot + _strMod + "DreamyGRO_" + strFile.GetFileName() + ".gro";

  } else {
    _strGRO = _strRoot + _strMod + strCustomGRO;

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
  Str_t strExt = StrToLower(strFile.GetFileExt());

  if (strExt == ".wld" && ConsoleYN("Pack uncompressed world file?", false)) {
    _aNoCompression.push_back(".wld");
  }
};

// Build parameters from the full path and return file path relative to the root directory
Str_t FromFullFilePath(const CPath &strFile, const CPath &strDefaultFolderInRoot) {
  // Determine game and root directory
  EGameType eGame;
  size_t iDir = DetermineRootDir(strFile, strDefaultFolderInRoot, eGame);

  // Setup for individual files
  ManualSetup(strFile);

  // Get relative path to the root or the mod
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
  Str_t strFullPath = _strRoot + _strMod + strGRO;

  if (!FileExists(strFullPath)) {
    strFullPath = _strRoot + strGRO;
  }

  // Skip if it doesn't exist
  if (!FileExists(strFullPath)) {
    std::cout << '"' << strGRO << "\" does not exist!\n";
    return;
  }

  ZipArchive::Ptr pGRO = ZipFile::Open(strFullPath);

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
