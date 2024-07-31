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

static void DisplayHelp(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // List all commands if nothing specified
  if (itNext == itEnd) {
    CmdHelp(-1);

  } else {
    s32 iCommand = -1;
    const CString &strArg = *itNext;

    for (s32 i = 0; i < _ctCmdArgs; ++i) {
      const CmdArg_t &arg = _aCmdArgs[i];

      if (strArg.Compare(arg.strShort) || strArg.Compare(arg.strFull)) {
        iCommand = i;
        break;
      }
    }

    if (iCommand != -1) {
      CmdHelp(iCommand);
    } else {
      CMessageException::Throw("Unknown command line argument '%s'", strArg.c_str());
    }
  }

  // Terminate the execution
  throw true;
};

static void ParseRoot(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // Root has been already set
  if (_strRoot != "") {
    throw CMessageException("'-r' command cannot be used more than once!");
  }

  // No root path
  if (itNext == itEnd) {
    throw CMessageException("Expected a path to a game folder after '-r'!");
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

static void ParseMod(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // Mod has been already set
  if (_strMod != "") {
    throw CMessageException("'-m' command cannot be used more than once!");
  }

  // No mod name
  if (itNext == itEnd) {
    throw CMessageException("Expected a mod name after '-m'!");
  }

  // Set mod path
  _strMod = "Mods/" + *itNext + "/";
  ++it;
};

static void ParseOutput(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No output path
  if (itNext == itEnd) {
    throw CMessageException("Expected a path to an output file after '-o'!");
  }

  _strGRO = *itNext;
  ++it;
};

static void ParseInclude(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No path
  if (itNext == itEnd) {
    throw CMessageException("Expected a path to a file after '-i'!");
  }

  // Add relative path to the file
  CString strFile = *itNext;
  strFile.Replace('\\', '/'); // Fix slashes

  _aScanFiles.push_back(strFile);

  // The world itself should be packed too
  const CString strExt = strFile.GetFileExt().AsLower();

  if (strExt == ".wld") {
    AddFile(strFile);
  }

  ++it;
};

static void ParseStoreFile(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No filename filter
  if (itNext == itEnd) {
    throw CMessageException("Expected a file type after '-s'!");
  }

  // Get filename filter
  CString strExt = (*itNext).AsLower();
  ++it;

  // Add a period to the extension
  if (strExt[0] != '.') {
    strExt = "." + strExt;
  }

  // Add lowercase extension
  _aNoCompression.push_back(strExt);
};

// Dependency files and GROs
static Strings_t _astrDependencies;

static void ParseDependency(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No file path
  if (itNext == itEnd) {
    throw CMessageException("Expected a path to a file after '-d'!");
  }

  // Add dependency file
  _astrDependencies.push_back(*itNext);
  ++it;
};

static void ParseFlag(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Get next argument immediately
  Strings_t::const_iterator itNext = it;

  // No flags
  if (itNext == itEnd) {
    throw CMessageException("Expected a flag after '-f'!");
  }

  // Add specific flag
  const CString strFlag = (*itNext).AsLower();
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

static void ParsePause(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd) {
  // Pause at the end of execution
  _bPauseAtTheEnd = true;
};

const CmdArg_t _aCmdArgs[] {
  { "help", "h", "Display available command line arguments",
    "  -h",
    &DisplayHelp },

  { "root", "r", "Set root directory of a game on Serious Engine 1",
    "  -r \"/usr/games/SeriousSam/\"\n"
    "  -r \"C:\\SeriousSam\\\"",
    &ParseRoot },

  { "mod", "m", "Set name of a mod folder where the files are being included from",
    "  -m MyMod",
    &ParseMod },

  { "output", "o", "Set output GRO filename. If full path isn't specified, defaults to the root directory + mod folder",
    "  -o MyMap.gro",
    &ParseOutput },

  { "include", "i", "Include a file that will be scanned for extra dependencies",
    "  -i Levels/MyLevel.wld\n"
    "  -i Data/Messages/MyLevel.txt\n"
    "  -i Textures/MyEffectTexture.tex\n"
    "  -i Bin/MyEntities.dll",
    &ParseInclude },

  { "store", "s", "Specify file types to store in the archive without any compression",
    "  -s wld\n"
    "  -s .ogg",
    &ParseStoreFile },

  { "depend", "d", "Mark specific resources or entire GRO archives as \"standard\" dependencies that will be skipped during scanning",
    "  -d MyResources.gro\n"
    "  -d Textures/MyTexture.tex",
    &ParseDependency },

  { "flag", "f", "Set certain behavior flags",
    "  -f dep - display a list of dependencies of included files without packing anything into a GRO\n"
    "  -f gro - automatically detect GRO files from certain games instead of manually adding them\n"
    "  -f ini - include INI files alongside their respective MDL files\n"
    "  -f mod - erase mod directory from paths to dependencies (e.g. packs \"Mods\\MyMod\\Texture1.tex\" as \"Texture1.tex\")\n"
    "  -f ogg - check for the existence of OGG files if MP3 files cannot be found\n"
    "  -f ssr - mark files as being from Serious Sam Revolution (detects automatically from WLD files)",
    &ParseFlag },

  { "pause", "p", "Pause program execution at the very end in order to see the final output",
    "  -p",
    &ParsePause },
};

const size_t _ctCmdArgs = (sizeof(_aCmdArgs) / sizeof(CmdArg_t));

// Display help about command line arguments
void CmdHelp(s32 iCommand) {
  // List all of the commands
  if (iCommand == -1) {
    for (size_t i = 0; i < _ctCmdArgs; ++i) {
      const CmdArg_t &arg = _aCmdArgs[i];

      std::cout << "\n--" << arg.strFull << " / -" << arg.strShort << "\n  " << arg.strDescription << '\n';
    }

    std::cout << "\nType --help <command> to see usage examples for a specific command.\n";
    return;
  }

  // Display usage examples for some command
  const CmdArg_t &arg = _aCmdArgs[iCommand];

  std::cout << "\n--" << arg.strFull << " / -" << arg.strShort << " : " << arg.strDescription
    << "\n\nExample:\n" << arg.strExample << '\n';
};

// Parse command line arguments
bool ParseArguments(Strings_t &aArgs) {
  _astrDependencies.clear();

  Strings_t::const_iterator it = aArgs.begin();
  const size_t ctArgs = aArgs.size();

  while (it != aArgs.end()) {
    // Get the string and advance the argument
    const CString &str = *(it++);

    // Starts with one dash
    const bool bShort = (str[0] == '-' && str[1] != '-');
    bool bProcessArgument = false;

    for (size_t iArg = 0; iArg < _ctCmdArgs; ++iArg) {
      const CmdArg_t &arg = _aCmdArgs[iArg];

      if (bShort) {
        bProcessArgument = str.StartsWith(CString("-") + arg.strShort);
      } else {
        bProcessArgument = str.StartsWith(CString("--") + arg.strFull);
      }

      // Parse the argument
      if (bProcessArgument) {
        arg.pFunc(it, aArgs.end());
        break;
      }
    }

    // Couldn't process the argument
    if (!bProcessArgument) {
      // Can't parse multiple arguments
      if (ctArgs > 1) {
        CMessageException::Throw("Unknown command line argument '%s'", str.c_str());

      // One invalid argument must be a path to a file
      } else {
        return false;
      }
    }
  }

  // No files to scan
  if (_aScanFiles.size() == 0) {
    throw CMessageException("No files have been specified for scanning!");
  }

  // No root path set
  if (_strRoot == "") {
    throw CMessageException("Game folder path has not been set!");
  }

  if (!OnlyDep()) {
    // Make path to a GRO from the first file to be scanned
    if (_strGRO == "") {
      const CString strFile = _aScanFiles[0];
      _strGRO = _strRoot + _strMod + "DreamyGRO_" + strFile.GetFileName() + ".gro";

    } else {
      // Relative to the mod directory
      if (_strMod != "") {
        _strGRO = _strMod + _strGRO;
      }

      // Relative to the root directory
      if (_strGRO.IsRelative()) {
        _strGRO = _strRoot + _strGRO;
      }
    }

    _strGRO.Normalize();
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
  for (size_t iDepend = 0; iDepend < _astrDependencies.size(); ++iDepend) {
    const CString &strDepend = _astrDependencies[iDepend];

    // Copy path for various checks
    const CString strCheck = strDepend.AsLower();

    // Scan entire GRO archive
    if (strCheck.GetFileExt() == ".gro") {
      IgnoreGRO(strDepend);
      continue;
    }

    // Skip if it doesn't exist under either directory
    const CString strUnderMod = _strRoot + _strMod + strDepend;
    const CString strUnderRoot = _strRoot + strDepend;

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

  return true;
};

// Detect root game directory from a full path to the file
static size_t DetermineRootDir(const CString &strFile, const CString &strDefaultFolderInRoot, EGameType &eGame) {
  CString strCurrentDir = strFile;
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

  CString strGameType;

  switch (eGame) {
    case GAME_TFE: strGameType = "(TFE)"; break;
    case GAME_TSE: strGameType = "(TSE)"; break;
    case GAME_REV: strGameType = "(SSR)"; break;
    default: strGameType = "(unknown)";
  }

  std::cout << "Assumed game directory " << strGameType << ": " << _strRoot << '\n';

  // If path to the file is under a mod directory right after the root
  const CString strModDir = strFile.AsLower().substr(iDir);

  if (strModDir.StartsWith("mods/")) {
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
static void ManualSetup(const CString &strFile) {
  // Only show dependencies
  if (ConsoleYN("Display dependencies instead of packing?", false)) {
    _iFlags |= SCAN_DEP;
    return;
  }

  // Set output GRO
  CString strCustomGRO = ConsoleInput("Enter output GRO file (blank for automatic): ");

  if (strCustomGRO == "") {
    _strGRO = _strRoot + _strMod + "DreamyGRO_" + strFile.GetFileName() + ".gro";

  } else {
    _strGRO = _strRoot + _strMod + strCustomGRO;

    // Append extension
    CString strCheck = _strGRO.GetFileExt().AsLower();

    if (strCheck != ".gro") {
      _strGRO += ".gro";
    }
  }

  _strGRO.Normalize();

  // Store music files
  if (ConsoleYN("Pack uncompressed music files?", true)) {
    _aNoCompression.push_back(".ogg");
    _aNoCompression.push_back(".mp3");
  }

  // Store the world file
  CString strExt = strFile.GetFileExt().AsLower();

  if (strExt == ".wld" && ConsoleYN("Pack uncompressed world file?", false)) {
    _aNoCompression.push_back(".wld");
  }
};

// Build parameters from the full path and return file path relative to the root directory
CString FromFullFilePath(const CString &strFile, const CString &strDefaultFolderInRoot) {
  // Determine game and root directory
  EGameType eGame;
  size_t iDir = DetermineRootDir(strFile, strDefaultFolderInRoot, eGame);

  // Setup for individual files
  ManualSetup(strFile);

  // Get relative path to the root or the mod
  CString strRelative = strFile.substr(iDir);
  _aScanFiles.push_back(strRelative);

  IgnoreGame(eGame, true);
  return strRelative;
};

// Detect default GRO packages in some directory to determine the game
EGameType DetectGame(const CString &strDir) {
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
void IgnoreGRO(const CString &strGRO) {
  CString strFullPath = _strRoot + _strMod + strGRO;

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
    CString strCheck = file->GetFullName();
    strCheck.ToLower();

    size_t iHash;

    // Add to existing dependencies if it's not there
    if (!InDepends(strCheck, &iHash)) {
      _aStdDepends.push_back(iHash);
    }
  }
};
