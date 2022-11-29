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

#include "Main.h"
#include "CommandLine.h"
#include "DictionaryReader.h"

#include <ZipLib/ZipFile.h>
#include <ZipLib/ZipArchiveEntry.h>

Str_t _strRoot = "";
Strings_t _aWorlds;
Strings_t _aStore;
CHashArray _aDepend;

Strings_t _aFiles;
CPath _strGRO = "";

u32 _iFlags = 0;
bool _bPauseAtTheEnd = false;

// Check if the file is already in standard dependencies
bool InDepends(const Str_t &strFilename, size_t *piHash) {
  // Get filename hash
  std::hash<Str_t> hash;
  const size_t iHash = hash(strFilename);
  
  if (piHash != nullptr) {
    *piHash = iHash;
  }

  CHashArray::const_iterator it = std::find(_aDepend.begin(), _aDepend.end(), iHash);

  // Already in there
  return (it != _aDepend.end());
};

// Check if the file is already added
bool InFiles(Str_t strFilename) {
  ToLower(strFilename);

  Strings_t::const_iterator it;
  
  // Case insensitive search
  for (it = _aFiles.begin(); it != _aFiles.end(); ++it) {
    Str_t strInFiles = *it;
    ToLower(strInFiles);

    if (strInFiles == strFilename) {
      return true;
    }
  }

  return false;
};

// Add new file to the list and return true if it wasn't there before
bool AddFile(const Str_t &strFilename) {
  if (InFiles(strFilename)) {
    return false;
  }

  _aFiles.push_back(strFilename);
  return true;
};

// Replace MP directories with normal ones
void ReplaceRevDirs(Str_t &strFilename) {
  Str_t strCheck = strFilename;
  ToLower(strCheck);

  s32 ctChars = 0;

  // Remove 'MP' from some directories
  if (strCheck.find("modelsmp") == 0
   || strCheck.find("soundsmp") == 0) {
    ctChars = 6;

  } else if (strCheck.find("musicmp") == 0) {
    ctChars = 5;

  } else if (strCheck.find("datamp") == 0) {
    ctChars = 4;

  } else if (strCheck.find("texturesmp") == 0) {
    ctChars = 8;

  } else if (strCheck.find("animationsmp") == 0) {
    ctChars = 10;
  }

  if (ctChars == 0) return;
  strFilename.erase(ctChars, 2);
};

// Replace spaces with underscores
void ReplaceSpaces(Str_t &strFilename) {
  Replace(strFilename, ' ', '_');
};

// Check if it's a valid world file
void VerifyWorldFile(CDataStream &strmWorld) {
  // Parse build version and world chunks to verify that it's a world file
  if (strmWorld.Read(4) != "BUIV") {
    throw CMessageException("Expected a world file from Serious Sam Classics!");
  }

  s32 iBuildVersion;
  strmWorld >> iBuildVersion;

  if (strmWorld.Read(4) != "WRLD") {
    throw CMessageException("Expected a world file from Serious Sam Classics!");
  }
};

// Display a list of files that cannot be used
static bool DisplayFailedFiles(const Strings_t &astr, const Str_t &strError) {
  if (astr.empty()) {
    return false;
  }

  std::cout << strError << '\n';

  for (size_t iFailed = 0; iFailed < astr.size(); ++iFailed) {
    std::cout << "- " << astr[iFailed] << '\n';
  }

  std::cout << '\n';
  return true;
};

// Pause command line execution
static void Pause(void) {
  if (_bPauseAtTheEnd) {
    #if !_DREAMY_UNIX
      system("pause");
    #endif
  }
};

// Entry point
int main(int ctArgs, char *astrArgs[]) {
  std::cout << "DreamyGRO - (c) Dreamy Cecil, 2022\n\n";

  if (ctArgs < 2) {
    std::cout << "Please specify path to a world file or use command line arguments:\n";

    for (s32 iCmd = 0; iCmd < 7; iCmd++) {
      std::cout << "  " << _astrArgDesc[iCmd] << '\n';
    }

    return 1;
  }
  
  // Get program arguments
  Strings_t aArgs;

  for (s32 iArg = 1; iArg < ctArgs; ++iArg) {
    aArgs.push_back(astrArgs[iArg]);
    std::cout << astrArgs[iArg] << ' ';
  }
  std::cout << "\n";

  try {
    // Parse one world
    if (aArgs.size() == 1) {
      FromWorldPath(aArgs[0]);

    // Parse command line arguments
    } else {
      ParseArguments(aArgs);
    }

    std::cout << "\nStandard dependencies: " << _aDepend.size() << "\n\n";

    for (size_t iWorld = 0; iWorld < _aWorlds.size(); ++iWorld) {
      Str_t strWorld = _aWorlds[iWorld];
      Replace(strWorld, '\\', '/');

      std::cout << "Extra dependencies for '" << strWorld << "':\n";

      ScanWorld(strWorld);
    }

  } catch (CMessageException &ex) {
    std::cout << "Error: " << ex.what() << '\n';

    Pause();
    return 1;
  }

  // Files that couldn't be packed
  Strings_t aFailed;

  if (!OnlyDep()) {
    std::cout << "\nPacking files...\n";

    try {
      // Create a new GRO file
      std::remove(_strGRO.c_str());
      ZipArchive::Ptr pGro = ZipFile::Open(_strGRO);

      // Create streams for each file
      size_t ctFiles = _aFiles.size();
      CFileInputs aFileStreams(ctFiles);

      // Go through file dependencies
      for (size_t iFile = 0; iFile < ctFiles; ++iFile) {
        CPath strFile = _aFiles[iFile];
        std::ifstream &file = aFileStreams[iFile];

        // Skip if no file
        if (!FileExists(_strRoot + strFile)) {
          if (!IsRev()) {
            aFailed.push_back(strFile);
            continue;
          }

          // Try again for SSR directories
          ReplaceRevDirs(strFile);

          // Still nothing
          if (!FileExists(_strRoot + strFile)) {
            aFailed.push_back(strFile);
            continue;
          }
        }

        // Add file under the relative directory
        ZipArchiveEntry::Ptr pEntry = pGro->CreateEntry(strFile);

        // Skip if can't create an entry
        if (pEntry == nullptr) {
          aFailed.push_back(strFile);
          continue;
        }

        // Write real file
        file.open(_strRoot + strFile, std::ios::binary);

        // Determine compression method
        ICompressionMethod::Ptr pMethod;

        // Find extension in the list
        Str_t strExt = strFile.GetFileExt();
        ToLower(strExt);

        Strings_t::const_iterator itStore = std::find(_aStore.begin(), _aStore.end(), strExt);

        // Store files of this type
        if (itStore != _aStore.end()) {
          pMethod = StoreMethod::Create();
        } else {
          pMethod = DeflateMethod::Create();
        }

        pEntry->SetCompressionStream(file, pMethod);
      }

      ZipFile::SaveAndClose(pGro, _strGRO);

    } catch (std::runtime_error &err) {
      std::cout << "Error: " << err.what() << '\n';

      Pause();
      return 1;
    }
    
    // Display files that couldn't be packed
    DisplayFailedFiles(aFailed, "\nCouldn't pack these files:");
    std::cout << '"' << _strGRO << "\" is ready!\n";

  // Only show dependencies
  } else {
    std::cout << "\nChecking for physical existence of files...\n";

    // Go through file dependencies
    for (size_t iFile = 0; iFile < _aFiles.size(); ++iFile) {
      Str_t strFile = _aFiles[iFile];

      // Skip if no file
      if (!FileExists(_strRoot + strFile)) {
        if (!IsRev()) {
          aFailed.push_back(strFile);
          continue;
        }

        // Try again for SSR directories
        ReplaceRevDirs(strFile);

        // Still nothing
        if (!FileExists(_strRoot + strFile)) {
          aFailed.push_back(strFile);
          continue;
        }
      }
    }

    // Display files that don't exist
    if (!DisplayFailedFiles(aFailed, "\nFiles that aren't on disk:")) {
      // No failed files
      std::cout << "\nAll files exist!\n";
    }

    // Forced pause to be able to see the file list
    _bPauseAtTheEnd = true;
  }

  Pause();
  return 0;
};
