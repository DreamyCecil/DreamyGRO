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
#include "DictionaryReader.h"

// Dependency counter
static s32 _ctDepend = 0;

// Count one file
static void CountFile(const Str_t &strFilename) {
  ++_ctDepend;
  std::cout << _ctDepend << ". " << strFilename << '\n';
};

// Fix filename if it's improper
static void FixFilename(Str_t &strFilename) {
  // Forward slashes are only in SSR
  if (strFilename.find('/') != Str_t::npos) {
    _iFlags |= SCAN_SSR;
  }

  // Make consistent slashes
  Replace(strFilename, '\\', '/');

  Str_t strFixed = "";
  Str_t::iterator it;

  for (it = strFilename.begin(); it != strFilename.end(); ++it) {
    // Get next char
    Str_t::iterator itNext = it + 1;

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

// Scan the dictionary and extract filenames that aren't in existing dependencies
void ScanDictionary(CDataStream &strm) {
  s32 ctFiles;
  strm >> ctFiles;

  // For each filename
  for (s32 iFile = 0; iFile < ctFiles; ++iFile) {
    // Read filename
    strm.Expect(CByteArray("DFNM", 4));

    CPath strFilename;
    strm >> strFilename;

    // Skip empty filenames
    if (strFilename == "") {
      continue;
    }
    
    FixFilename(strFilename);

    // Check if the file already exists in the list of dependencies
    CPath strCheck = strFilename;
    ToLower(strCheck);

    // Skip if already in there
    if (InDepends(strCheck)) continue;

    // Try with OGG if no MP3
    if (PackOGG() && strCheck.GetFileExt() == ".mp3") {
      strCheck = strCheck.RemoveExt() + ".ogg";

      // OGG is present instead of MP3
      if (InDepends(strCheck)) continue;
    }
    
    if (IsRev()) {
      // Try with different directories
      ReplaceRevDirs(strCheck);
      if (InDepends(strCheck)) continue;

      // Try replacing spaces
      ReplaceSpaces(strCheck);
      if (InDepends(strCheck)) continue;
    }

    // Add filename to the list if it's not there yet
    if (AddFile(strFilename)) {
      CountFile(strFilename);

      Str_t strExt = strCheck.GetFileExt();

      // Pack INI configs for models
      if (strExt == ".mdl") {
        if (PackINI()) {
          Str_t strINI = strFilename.RemoveExt() + ".ini";

          if (AddFile(strINI)) {
            CountFile(strINI);
          }
        }

      // Pack base textures with FX textures
      } else if (strExt == ".tex") {
        CFileDevice dTex((_strRoot + strCheck).c_str());
        bool bOpen = dTex.Open(IReadWriteDevice::OM_READONLY);

        if (bOpen) {
          CDataStream strmTex(&dTex);

          // Skip texture version and data with 6 values (including two chunks)
          strmTex.Seek(36);

          // FX texture is written
          if (strmTex.Peek(4) == "FXDT") {
            // Skip to the end of the texture
            strmTex.Seek(strmTex.Device()->Size() - 1);

            s32 ctChars = 0;

            // Go backwards until the null character
            // 56 is the fixed amount of bytes unrelated to the base texture path
            while (strmTex.Pos() > 56)
            {
              c8 cByte;
              dTex.Peek(&cByte, 1);

              // Encountered a null character
              if (cByte == '\x00') {
                // Skip it
                strmTex.Skip(1);
                break;
              }

              // Move backward
              strmTex.Seek(strmTex.Pos() - 1);
              ++ctChars;
            }

            if (ctChars > 0) {
              // Extract a string from the current position to the end of file
              CByteArray baBaseTex = strmTex.Read(ctChars);

              // Read base texture file
              Str_t strBaseTex = baBaseTex.ConstData();
              FixFilename(strBaseTex);
            
              // Check if the file already exists in the list of dependencies
              Str_t strCheckTex = strBaseTex;
              ToLower(strCheckTex);

              // Proceed if it's not there
              if (!InDepends(strCheckTex)) {
                bool bAdd = true;

                if (IsRev()) {
                  // Try with different directories
                  ReplaceRevDirs(strCheckTex);
                  bAdd = !InDepends(strCheckTex);
                
                  // Try replacing spaces
                  if (bAdd) {
                    ReplaceSpaces(strCheckTex);
                    bAdd = !InDepends(strCheckTex);
                  }
                }
            
                // Add base texture file
                if (bAdd && AddFile(strBaseTex)) {
                  CountFile(strBaseTex);
                }
              }
            }
          }

          dTex.Close();
        }
      }
    }
  }

  // Dictionary end
  strm.Expect(CByteArray("DEND", 4));
};

// Scan the world dictionary for dependencies
void ScanWorld(const CPath &strWorld) {
  CFileDevice d((_strRoot + strWorld).c_str());
  d.Open(IReadWriteDevice::OM_READONLY);
  
  // Verify world file first
  CDataStream strm(&d);
  VerifyWorldFile(strm);

  // Parse world info before parsing the dictionary
  {
    Str_t strDummy;

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

  _ctDepend = 0;

  {
    const Str_t strNoExt = strWorld.RemoveExt();

    // Add thumbnail texture
    Str_t strExtra = strNoExt + "Tbn.tex";

    // Try regular thumbnail
    if (!FileExists(_strRoot + strExtra)) {
      strExtra = strNoExt + ".tbn";
    }

    if (FileExists(_strRoot + strExtra)) {
      if (AddFile(strExtra)) {
        CountFile(strExtra);
      }
    }

    // Add VIS file
    strExtra = strNoExt + ".vis";

    if (FileExists(_strRoot + strExtra)) {
      if (AddFile(strExtra)) {
        CountFile(strExtra);
      }
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
  strm.Expect(CByteArray("DICT", 4));
  ScanDictionary(strm);

  // Expect the next dictionary position
  strm.Expect(CByteArray("DPOS", 4));

  // Go to the next dictionary
  strm >> iPos;
  strm.Seek(iPos);

  // Entity resources dictionary
  strm.Expect(CByteArray("DICT", 4));
  ScanDictionary(strm);

  d.Close();

  // No dependencies have been added
  if (_ctDepend == 0) {
    std::cout << "No dependencies\n";
  }
};
