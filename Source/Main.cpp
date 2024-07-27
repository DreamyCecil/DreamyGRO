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

#include "Main.h"
#include "CommandLine.h"
#include "DictionaryReader.h"

#include <ZipLib/ZipFile.h>
#include <ZipLib/ZipArchiveEntry.h>

// Implement Dreamy Utilities
#include <DreamyUtilities/DreamyUtilities.cpp>

CString _strRoot = "";
CString _strMod = "";
Strings_t _aScanFiles;
Strings_t _aNoCompression;
CHashArray _aStdDepends;

CListedFiles _aFilesToPack;
bool _bCountFiles = false;
size_t _ctFiles = 0;

CString _strGRO = "";

u32 _iFlags = 0;
bool _bPauseAtTheEnd = false;

// Check if the file is already in standard dependencies
bool InDepends(const CString &strFilename, size_t *piHash) {
  // Get filename hash
  std::hash<CString> hash;
  const size_t iHash = hash(strFilename);

  if (piHash != nullptr) {
    *piHash = iHash;
  }

  CHashArray::const_iterator it = std::find(_aStdDepends.begin(), _aStdDepends.end(), iHash);

  // Already in there
  return (it != _aStdDepends.end());
};

// Check if the file is already added
bool InFiles(CString strFilename) {
  strFilename.ToLower();

  CListedFiles::const_iterator it;

  // Case insensitive search
  for (it = _aFilesToPack.begin(); it != _aFilesToPack.end(); ++it) {
    CString strInFiles = it->strFile.AsLower();

    if (strInFiles == strFilename) {
      return true;
    }
  }

  return false;
};

// Add new file to the list and return true if it wasn't there before
bool AddFile(const CString &strFilename) {
  if (InFiles(strFilename)) {
    return false;
  }

  if (_bCountFiles) {
    // Count one dependency
    ++_ctFiles;
    std::cout << _ctFiles << ". " << strFilename << '\n';

    _aFilesToPack.push_back(ListedFile_t(strFilename, _ctFiles));

  } else {
    _aFilesToPack.push_back(ListedFile_t(strFilename, 0));
  }

  return true;
};

// Replace MP directories with normal ones
void ReplaceRevDirs(CString &strFilename) {
  CString strCheck = strFilename.AsLower();
  s32 ctChars = 0;

  // Remove 'MP' from some directories
  if (strCheck.StartsWith("modelsmp") || strCheck.StartsWith("soundsmp")) {
    ctChars = 6;
  } else if (strCheck.StartsWith("musicmp")) {
    ctChars = 5;
  } else if (strCheck.StartsWith("datamp")) {
    ctChars = 4;
  } else if (strCheck.StartsWith("texturesmp")) {
    ctChars = 8;
  } else if (strCheck.StartsWith("animationsmp")) {
    ctChars = 10;
  }

  if (ctChars == 0) return;
  strFilename.erase(ctChars, 2);
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

// Check if some listed dependency exists
// Return values: 0 - doesn't exist; 1 - exists under root; 2 - exists under mod
static s32 CheckFile(CString strFile) {
  // Dependency exists in the mod directory
  if (_strMod != "" && FileExists(_strRoot + _strMod + strFile)) return 2;

  // Dependency exists in the root directory
  if (FileExists(_strRoot + strFile)) return 1;

  // Try again for SSR directories
  if (IsRev()) {
    ReplaceRevDirs(strFile);
    if (FileExists(_strRoot + strFile)) return 1;
  }

  return 0;
};

// Display a list of files that cannot be used
static bool DisplayFailedFiles(const CListedFiles &aFailed, const CString &strError) {
  if (aFailed.empty()) {
    return false;
  }

  std::cout << strError << '\n';

  for (size_t i = 0; i < aFailed.size(); ++i) {
    const ListedFile_t &file = aFailed[i];
    std::cout << file.iNumber << ". " << file.strFile << '\n';
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
  std::cout << "DreamyGRO - (c) Dreamy Cecil, 2022-2024\n";

  if (ctArgs < 2) {
    std::cout << "Please specify a path to any file or use command line arguments. Use --help for more info.\n";
    return 1;
  }

  // Get program arguments
  Strings_t aArgs;
  std::cout << "Command line: ";

  for (s32 iArg = 1; iArg < ctArgs; ++iArg) {
    aArgs.push_back(astrArgs[iArg]);
    std::cout << astrArgs[iArg] << ' ';
  }
  std::cout << "\n";

  try {
    // Parse command line arguments
    bool bParsed = ParseArguments(aArgs);

    // Parse one file if couldn't parse the arguments
    if (!bParsed) {
      // Force the pause to be able to see the output
      _bPauseAtTheEnd = true;

      CString strFile = aArgs[0];

      // Turn relative path into absolute if running a script with a file nearby
      if (strFile.IsRelative()) strFile = GetCurrentPath() + strFile;
      strFile.Normalize();

      // Make sure the file can be opened
      CFileDevice d(strFile.c_str());

      if (!d.Open(IReadWriteDevice::OM_READONLY)) {
        throw CMessageException("Cannot open the file!");
      }

      const CString strCheckExt = strFile.GetFileExt().AsLower();

      if (strCheckExt == ".wld") {
        // Verify world file
        CDataStream strm(&d);
        VerifyWorldFile(strm);

        CString strRelative = FromFullFilePath(strFile, "Levels");
        AddFile(strRelative); // The world itself should be packed too

      } else if (strCheckExt == ".dll") {
        FromFullFilePath(strFile, "Bin");

      } else {
        FromFullFilePath(strFile, "");
      }
    }

    std::cout << "Standard dependencies: " << _aStdDepends.size() << '\n';

    // Start counting dependencies
    _bCountFiles = true;
    _ctFiles = 0;

    const size_t ctScanFiles = _aScanFiles.size();

    // No files to scan
    if (ctScanFiles == 0) {
      throw CMessageException("No files to scan for dependencies");
    }

    for (size_t iScanFile = 0; iScanFile < ctScanFiles; ++iScanFile) {
      CString strFile = _aScanFiles[iScanFile];

      std::cout << "\nExtra dependencies for '" << strFile << "':\n";

      const CString strCheckExt = strFile.GetFileExt().AsLower();

      if (strCheckExt == ".wld") {
        ScanWorld(strFile);
      } else {
        ScanAnyFile(strFile, strCheckExt == ".dll");
      }
    }

  } catch (bool bExit) {
    // Terminate application execution
    if (bExit) return 0;

  } catch (CMessageException &ex) {
    std::cout << "Error: " << ex.what() << '\n';

    Pause();
    return 1;
  }

  // Files that couldn't be packed
  CListedFiles aFailed;

  const size_t ctFiles = _aFilesToPack.size();

  // No dependencies to pack
  if (ctFiles == 0) {
    std::cout << "\nAll files are already in standard dependencies! Nothing else needs to be packed :)\n";

  // Pack all the dependencies
  } else if (!OnlyDep()) {
    std::cout << "\nPacking files...\n";

    try {
      // Create a new GRO file
      std::remove(_strGRO.c_str());
      ZipArchive::Ptr pGro = ZipFile::Open(_strGRO);

      // Go through file dependencies
      for (size_t iFile = 0; iFile < ctFiles; ++iFile) {
        const ListedFile_t &listed = _aFilesToPack[iFile];
        CString strFile = listed.strFile;

        const s32 iCheck = CheckFile(strFile);

        // Skip if no file
        if (iCheck == 0) {
          aFailed.push_back(listed);
          continue;
        }

        // Add file under the relative directory
        ZipArchiveEntry::Ptr pEntry = pGro->CreateEntry(strFile);

        // Skip if can't create an entry
        if (pEntry == nullptr) {
          aFailed.push_back(listed);
          continue;
        }

        // Write real file
        std::ifstream file;

        if (iCheck == 2) {
          file.open(_strRoot + _strMod + strFile, std::ios::binary);
        } else {
          file.open(_strRoot + strFile, std::ios::binary);
        }

        // Determine compression method
        ICompressionMethod::Ptr pMethod;

        // Find extension in the list
        const CString strExt = strFile.GetFileExt().AsLower();
        Strings_t::const_iterator itStore = std::find(_aNoCompression.begin(), _aNoCompression.end(), strExt);

        // Store files of this type
        if (itStore != _aNoCompression.end()) {
          pMethod = StoreMethod::Create();
        } else {
          pMethod = DeflateMethod::Create();
        }

        pEntry->SetCompressionStream(file, pMethod, ZipArchiveEntry::CompressionMode::Immediate);
      }

      ZipFile::SaveAndClose(pGro, _strGRO);

    } catch (std::runtime_error &err) {
      std::cout << "Error: " << err.what() << " (" << strerror(errno) << ")\n";

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
    for (size_t iFile = 0; iFile < _aFilesToPack.size(); ++iFile) {
      const ListedFile_t &listed = _aFilesToPack[iFile];
      const s32 iCheck = CheckFile(listed.strFile);

      // Skip if no file
      if (iCheck == 0) {
        aFailed.push_back(listed);
        continue;
      }
    }

    // Display files that don't exist
    if (!DisplayFailedFiles(aFailed, "\nFiles that aren't on disk:")) {
      // No failed files
      std::cout << "\nAll files exist!\n";
    }
  }

  Pause();
  return 0;
};
