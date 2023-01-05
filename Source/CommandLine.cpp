/* Copyright (c) 2022 Dreamy Cecil
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

// Parse command line arguments
void ParseArguments(Strings_t &aArgs) {
  // Dependency files and GROs
  Strings_t astrDependencies;

  Strings_t::const_iterator it = aArgs.begin();

  while (it != aArgs.end()) {
    // Get the string and advance the argument
    const Str_t &str = *(it++);

    // Get next argument immediately
    Strings_t::const_iterator itNext = it;
    
    if (str == ARG_ROOT) {
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

      // Add another slash at the end
      if (_strRoot.rfind('\\') != _strRoot.length() - 1) {
        _strRoot += '/';
      }

    } else if (str == ARG_OUTPUT) {
      // No output path
      if (itNext == aArgs.end()) {
        CMessageException::Throw("Expected a path to an output file after '%s'!", ARG_OUTPUT);
      }

      // Add world
      _strGRO = *itNext;
      ++it;

    } else if (str == ARG_WORLD) {
      // No world path
      if (itNext == aArgs.end()) {
        CMessageException::Throw("Expected a path to a world file after '%s'!", ARG_WORLD);
      }

      // Add world
      Str_t strWorld = *itNext;
      _aWorlds.push_back(strWorld);
      ++it;

      // Add to the files
      AddFile(strWorld);
      
    } else if (str == ARG_STORE) {
      // No filename filter
      if (itNext == aArgs.end()) {
        CMessageException::Throw("Expected a filename filter '%s'!", ARG_STORE);
      }

      // Get filename filter
      Str_t strExt = *itNext;
      ++it;

      // Add a period to the extension
      if (strExt.find('.') != 0) {
        strExt = "." + strExt;
      }

      // Add lowercase extension
      ToLower(strExt);
      _aStore.push_back(strExt);
      
    } else if (str == ARG_DEPEND) {
      // No file path
      if (itNext == aArgs.end()) {
        CMessageException::Throw("Expected a path to a file after '%s'!", ARG_DEPEND);
      }
      
      // Add dependency file
      astrDependencies.push_back(*itNext);
      ++it;
      
    } else if (str == ARG_FLAGS) {
      // No flags
      if (itNext == aArgs.end()) {
        CMessageException::Throw("Expected flags after '%s'!", ARG_FLAGS);
      }
      
      // Add specific flag
      Str_t strFlag = *itNext;
      ToLower(strFlag);
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

  // No output path set
  if (_strGRO == "") {
    CMessageException::Throw("Output GRO file has not been set! Please use '%s <GRO file>'!", ARG_OUTPUT);
  }

  // Relative path to the output GRO
  #if !_DREAMY_UNIX
    bool bRelative = (_strGRO.find(':') == Str_t::npos); // No disc with the colon (e.g. "C:")
  #else
    bool bRelative = (_strGRO.find('/') != 0); // Doesn't start with a slash
  #endif

  if (bRelative) {
    _strGRO = _strRoot + _strGRO;
  }

  // Add GRO files from games automatically
  if (DetectGRO()) {
    try {
      AutoIgnoreGames(false);
    } catch (CMessageException &ex) {
      std::cout << ex.what() << '\n';
    }
  }

  // Go through dependencies
  for (size_t iDepend = 0; iDepend < astrDependencies.size(); ++iDepend) {
    const Str_t &strDepend = astrDependencies[iDepend];

    // Copy path for various checks
    CPath strCheck = strDepend;
    ToLower(strCheck);

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
      _aDepend.push_back(iHash);
    }
  }
};

// Build parameters from just a path to the world file
void FromWorldPath(const CPath &strWorld) {
  // Verify world file
  {
    CFileDevice d((_strRoot + strWorld).c_str());
    d.Open(IReadWriteDevice::OM_READONLY);
  
    CDataStream strm(&d);
    VerifyWorldFile(strm);
  }

  // Determine root directory from the path to WLD
  size_t iDir = strWorld.GoUpUntilDir("Levels");

  if (iDir == Str_t::npos) {
    CMessageException::Throw("You may only open WLD files that reside within 'Levels' folder of a game directory!\n");
  }

  // Set root path and output GRO
  _strRoot = strWorld.substr(0, iDir);

  // Only show dependencies
  if (ConsoleYN("Show world dependencies instead of packing?", false)) {
    _iFlags |= SCAN_DEP;
  }
  
  if (!OnlyDep()) {
    Str_t strCustomGRO = ConsoleInput("Enter output GRO file (blank for automatic): ");

    if (strCustomGRO == "") {
      _strGRO = _strRoot + "DreamyGRO_" + strWorld.GetFileName() + ".gro";

    } else {
      _strGRO = _strRoot + strCustomGRO;

      // Append extension
      Str_t strCheck = _strGRO.GetFileExt();
      ToLower(strCheck);

      if (strCheck != ".gro") {
        _strGRO += ".gro";
      }
    }

    // Store music files
    if (ConsoleYN("Pack uncompressed music files?", true)) {
      _aStore.push_back(".ogg");
      _aStore.push_back(".mp3");
    }

    // Store the world file
    if (ConsoleYN("Pack uncompressed world file?", false)) {
      _aStore.push_back(".wld");
    }
  }

  // Add relative path to the world
  Str_t strRelative = strWorld.substr(iDir);

  _aWorlds.push_back(strRelative);
  _aFiles.push_back(strRelative);

  AutoIgnoreGames(true);
};

// Automatically detect GRO files from specific games
void AutoIgnoreGames(bool bFromWorld) {
  // Ignore SE1.10 resources
  if (FileExists(_strRoot + "SE1_10.gro")) {
    std::cout << "\nDetected GRO files from Serious Engine 1.10...\n";

    IgnoreGRO("SE1_10.gro");

  // Ignore TSE resources
  } else if (FileExists((_strRoot + "SE1_00.gro").c_str())) {
    std::cout << "\nDetected GRO files from The Second Encounter...\n";

    IgnoreGRO("SE1_00.gro");
    IgnoreGRO("SE1_00_Extra.gro");
    IgnoreGRO("SE1_00_ExtraTools.gro");
    IgnoreGRO("SE1_00_Music.gro");
    IgnoreGRO("1_04_patch.gro");
    IgnoreGRO("1_07_tools.gro");

  // Ignore SSR resources
  } else if (FileExists(_strRoot + "All_01.gro")
          && FileExists(_strRoot + "All_02.gro")) {
    std::cout << "\nDetected GRO files from Revolution...\n";
    if (bFromWorld) _iFlags |= SCAN_SSR;

    IgnoreGRO("All_01.gro");
    IgnoreGRO("All_02.gro");

  // Ignore TFE resources
  } else if (FileExists(_strRoot + "1_00c.gro")) {
    std::cout << "\nDetected GRO files from The First Encounter...\n";
    if (bFromWorld) _iFlags |= SCAN_OGG;

    IgnoreGRO("1_00_ExtraTools.gro");
    IgnoreGRO("1_00_music.gro");
    IgnoreGRO("1_00c.gro");
    IgnoreGRO("1_00c_scripts.gro");
    IgnoreGRO("1_04_patch.gro");

  // Unknown game
  } else {
    CMessageException::Throw("Couldn't automatically determine the game directory! (no 'SE1_00.gro', '1_00c.gro', 'All_01.gro' or 'SE1_10.gro')\n");
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
    Str_t strCheck = file->GetFullName();
    ToLower(strCheck);

    size_t iHash;

    // Add to existing dependencies if it's not there
    if (!InDepends(strCheck, &iHash)) {
      _aDepend.push_back(iHash);
    }
  }
};
