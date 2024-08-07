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
#include "DictionaryReader.h"

#if !_DREAMY_UNIX
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>

  // Try to get offset of a section that's right after ".text" inside an executable file
  static size_t GetSecondSectionOffset(const c8 *hModule) {
    PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((size_t)pDOSHeader + pDOSHeader->e_lfanew);
    PIMAGE_FILE_HEADER pFileHeader = &pNTHeader->FileHeader;
    PIMAGE_SECTION_HEADER aSections = IMAGE_FIRST_SECTION(pNTHeader);

    // Not enough sections
    if (pFileHeader->NumberOfSections < 2) return NULL_POS;

    PIMAGE_SECTION_HEADER pAfterText = aSections + 1;
    return pAfterText->VirtualAddress;
  };
#endif

// Fix filename if it's improper
static void FixFilename(CString &strFilename) {
  // Forward slashes are only in SSR
  if (strFilename.find('/') != NULL_POS) {
    _iFlags |= SCAN_SSR;
  }

  // Make consistent slashes
  strFilename.Replace('\\', '/');

  CString strFixed = "";
  CString::const_iterator it;

  for (it = strFilename.begin(); it != strFilename.end(); ++it) {
    // Get next char
    CString::const_iterator itNext = it + 1;

    // Double slashes are only in SSR
    if (*it == '/' && *itNext == '/') {
      _iFlags |= SCAN_SSR;
      continue;
    }

    // Add character
    strFixed += *it;
  }

  // Prefix slashes are only in SSR
  if (strFixed[0] == '/') {
    strFixed = strFixed.erase(0, 1);
    _iFlags |= SCAN_SSR;
  }

  strFilename = strFixed;
};

// Add extra files with MDL
static void AddExtrasWithMDL(const CString &strFilename) {
  // Pack INI configs for models
  if (PackINI()) {
    AddFile(strFilename.RemoveExt() + ".ini");
  }
};

// Add extra files with TEX
static void AddExtrasWithTEX(const CString &strRelativeTextureFile) {
  CString strFilename = _strRoot + _strMod + strRelativeTextureFile;

  if (_strMod != "" && !FileExists(_strRoot + _strMod + strRelativeTextureFile)) {
    strFilename = _strRoot + strRelativeTextureFile;
  }

  // Pack base textures with FX textures
  CFileDevice dTex(strFilename.c_str());
  if (!dTex.Open(IReadWriteDevice::OM_READONLY)) return;

  // Skip texture version and data with 6 values (including two chunks)
  CDataStream strmTex(&dTex);
  strmTex.Seek(36);

  // Make sure that FX texture is written
  if (strmTex.Peek(4) != "FXDT") return;

  // Skip to the end of the texture
  strmTex.Seek(strmTex.Device()->Size() - 1);

  s32 ctChars = 0;

  // Go backwards until the null character
  // 56 is the fixed amount of bytes unrelated to the base texture path
  while (strmTex.Pos() > 56)
  {
    c8 ch;
    strmTex.Peek(&ch, 1);

    // Encountered a null character
    if (ch == '\0') {
      // Skip it
      strmTex.Skip(1);
      break;
    }

    // Move backward
    strmTex.Seek(strmTex.Pos() - 1);
    ++ctChars;
  }

  // No characters
  if (ctChars == 0) return;

  // Extract a string from the current position to the end of file
  CByteArray baBaseTex = strmTex.Read(ctChars);

  // Read base texture file
  CString strBaseTex = baBaseTex.ConstData();
  FixFilename(strBaseTex);

  // Check if the file already exists in the list of dependencies
  CString strCheckTex = strBaseTex.AsLower();

  // Proceed only if it's not there
  if (InDepends(strCheckTex)) return;

  bool bAdd = true;

  if (IsRev()) {
    // Try with different directories
    ReplaceRevDirs(strCheckTex);
    bAdd = !InDepends(strCheckTex);

    // Try replacing spaces
    if (bAdd) {
      strCheckTex.Replace(' ', '_');
      bAdd = !InDepends(strCheckTex);
    }
  }

  // Add base texture file
  if (bAdd) AddFile(strBaseTex);
};

// Add a specific file only if it exists and it's not in any lists of dependencies
static void TryToAddFile(CString strFilename) {
  // Check if the file already exists in the list of dependencies
  CString strCheckFile = strFilename.AsLower();

  // Remove mod directory
  if (EraseMod() && _strMod != "") {
    const CString strModCheck = _strMod.AsLower();

    if (strCheckFile.StartsWith(strModCheck)) {
      const size_t ctModCheck = strModCheck.length();

      strFilename.erase(0, ctModCheck);
      strCheckFile.erase(0, ctModCheck);
    }
  }

  // Skip if already in there
  if (InDepends(strCheckFile)) return;

  // Try with OGG if no MP3
  if (PackOGG() && strCheckFile.GetFileExt() == ".mp3") {
    strCheckFile = strCheckFile.RemoveExt() + ".ogg";

    // OGG is present instead of MP3
    if (InDepends(strCheckFile)) return;
  }

  if (IsRev()) {
    // Try with different directories
    ReplaceRevDirs(strCheckFile);
    if (InDepends(strCheckFile)) return;

    // Try replacing spaces
    strCheckFile.Replace(' ', '_');
    if (InDepends(strCheckFile)) return;
  }

  // Add filename to the list if it's not there yet
  if (!AddFile(strFilename)) return;

  const CString strExt = strCheckFile.GetFileExt();

  if (strExt == ".mdl") {
    AddExtrasWithMDL(strFilename);

  } else if (strExt == ".tex") {
    AddExtrasWithTEX(strCheckFile);
  }
};

// Scan the dictionary of a world and extract filenames that aren't in existing dependencies
static void ScanWorldDictionary(CDataStream &strm) {
  // Dictionary beginning
  strm.Expect(CByteArray("DICT", 4));

  s32 ctFiles;
  strm >> ctFiles;

  // For each filename
  for (s32 iFile = 0; iFile < ctFiles; ++iFile) {
    // Read filename
    strm.Expect(CByteArray("DFNM", 4));

    CString strFilename;
    strm >> strFilename;

    // Skip empty filenames
    if (strFilename == "") continue;

    FixFilename(strFilename);
    TryToAddFile(strFilename);
  }

  // Dictionary end
  strm.Expect(CByteArray("DEND", 4));
};

// Scan the world dictionary for dependencies
void ScanWorld(const CString &strWorld) {
  CFileDevice d((_strRoot + _strMod + strWorld).c_str());

  if (!d.Open(IReadWriteDevice::OM_READONLY)) {
    throw CMessageException("Cannot open the file!");
  }

  // Verify world file first
  CDataStream strm(&d);
  VerifyWorldFile(strm);

  // Parse world info before parsing the dictionary
  {
    CString strDummy;

    // Skip world info chunk
    strm.Expect(CByteArray("WLIF", 4));

    // Skip translation chunk, if any
    if (strm.Peek(4) == "DTRS") {
      strm.Skip(4);
    }

    // Skip SSR leaderboards chunk
    if (strm.Peek(4) == "LDRB") {
      strm.Skip(4);
      strm >> strDummy;

      _iFlags |= SCAN_SSR;
    }

    // Skip another SSR chunk and its data (4 + 12)
    if (strm.Peek(4) == "Plv0") {
      strm.Skip(16);

      _iFlags |= SCAN_SSR;
    }

    // Skip world name and spawn flags
    strm >> strDummy;
    strm.Skip(4);

    // Skip SSR special gamemode chunk
    if (strm.Peek(4) == "SpGM") {
      strm.Skip(4);

      _iFlags |= SCAN_SSR;
    }

    // Skip world description
    strm >> strDummy;
  }

  size_t ctLastFiles = _ctFiles;

  {
    const CString strNoExt = strWorld.RemoveExt();

    // Add thumbnail texture
    CString strExtra = strNoExt + "Tbn.tex";

    // Try regular thumbnail
    if (!FileExists(_strRoot + _strMod + strExtra)) {
      strExtra = strNoExt + ".tbn";
    }

    if (FileExists(_strRoot + _strMod + strExtra)) {
      AddFile(strExtra);
    }

    // Add VIS file
    strExtra = strNoExt + ".vis";

    if (FileExists(_strRoot + _strMod + strExtra)) {
      AddFile(strExtra);
    }
  }

  while (!strm.AtEnd()) {
    // Found dictionary position
    if (strm.Peek(4) == "DPOS") {
      strm.Skip(4);
      break;
    }

    // Move forward
    strm.Skip(1);
  }

  // Go to the dictionary
  s32 iPos;
  strm >> iPos;
  strm.Seek(iPos);

  // Brush textures dictionary
  ScanWorldDictionary(strm);

  // Expect the next dictionary position
  strm.Expect(CByteArray("DPOS", 4));

  // Go to the next dictionary
  strm >> iPos;
  strm.Seek(iPos);

  // Entity resources dictionary
  ScanWorldDictionary(strm);

  d.Close();

  // No dependencies have been added
  if (_ctFiles == ctLastFiles) {
    std::cout << "No dependencies\n";
  }
};

// Scan any file for dependencies
void ScanAnyFile(const CString &strFile, bool bLibrary) {
  CFileDevice d((_strRoot + _strMod + strFile).c_str());

  if (!d.Open(IReadWriteDevice::OM_READONLY)) {
    throw CMessageException("Cannot open the file!");
  }

  CDataStream strm(&d);
  size_t ctLastFiles = _ctFiles;

#if !_DREAMY_UNIX
  // Scan a library file to skip through bytes that don't have any strings
  if (bLibrary) {
    CByteArray aDLL = strm.Read(d.Size());
    d.Reset();
    strm.ResetStatus();

    const size_t iPosAfterText = GetSecondSectionOffset(aDLL.ConstData());

    if (iPosAfterText != NULL_POS) {
      strm.Seek(iPosAfterText);
    }
  }
#endif

  // Possible chunks
  const u32 iDFNM = *reinterpret_cast<const u32 *>("DFNM");
  const u32 iEFNM = *reinterpret_cast<const u32 *>("EFNM");
  const u32 iTFNM = *reinterpret_cast<const u32 *>("TFNM");

  c8 aReadChunk[4];

  while (!strm.AtEnd()) {
    // Try reading a chunk
    if (strm.Peek(aReadChunk, 4) != 4) break;

    const u32 iChunk = *reinterpret_cast<const u32 *>(aReadChunk);
    CString strFilename = "";

    // Filename inside a data file
    if (iChunk == iDFNM) {
      strm.Skip(4);

      u32 iSize = 0;
      strm.Peek(&iSize, 4);

      // Make sure that's it's not too long
      if (iSize < 254) {
        strm >> strFilename;
      }

    // Filename inside an executable file
    } else if (iChunk == iEFNM) {
      strm.Skip(4);

      while (!strm.AtEnd()) {
        c8 ch;
        size_t iRead = strm.Peek(&ch, 1);

        // Until either end
        if (iRead != 1 || ch == '\0') break;

        // Add the character
        strFilename += ch;
        strm.Skip(1);
      }

    // Filename inside a text file
    } else if (iChunk == iTFNM) {
      strm.Skip(5); // Skip extra space

      while (!strm.AtEnd()) {
        c8 ch;
        size_t iRead = strm.Peek(&ch, 1);

        // Until either end
        if (iRead != 1 || ch == '\n' || ch == '\r'  || ch == '\0' || strFilename.length() >= 254) break;

        // Add the character
        strFilename += ch;
        strm.Skip(1);
      }
    }

    // Add read filename
    if (strFilename != "") {
      FixFilename(strFilename);
      TryToAddFile(strFilename);

    // Otherwise move forward
    } else {
      strm.Skip(1);
    }
  }

  d.Close();

  // No dependencies have been added
  if (_ctFiles == ctLastFiles) {
    std::cout << "No dependencies\n";
  }
};
