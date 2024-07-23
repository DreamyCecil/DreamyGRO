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

#ifndef _DREAMYGRO_INCL_MAIN_H
#define _DREAMYGRO_INCL_MAIN_H

#include <DreamyUtilities/Types/Arrays.hpp>
#include <DreamyUtilities/Files/Filenames.hpp>
#include <DreamyUtilities/IO/DataStream.hpp>
#include <DreamyUtilities/IO/FileDevice.hpp>

using namespace dreamy;

#include <iostream>
#include <fstream>
#include <vector>

typedef std::vector<size_t> CHashArray;

struct ListedFile_t {
  Str_t strFile;
  size_t iNumber;

  ListedFile_t(const Str_t &strSet, size_t iSet) : strFile(strSet), iNumber(iSet) {};
};

typedef std::vector<ListedFile_t> CListedFiles;

// Preparation
extern CPath _strRoot;            // Game folder directory
extern Strings_t _aScanFiles;     // List of files to scan for dependencies
extern Strings_t _aNoCompression; // List of files for packing without compression
extern CHashArray _aStdDepends;   // List of standard dependencies

extern CListedFiles _aFilesToPack; // Final list of files to pack
extern bool _bCountFiles; // Start counting extra dependencies using the counter below
extern size_t _ctFiles; // Dependency counter

extern CPath _strGRO; // Output GRO archive

enum EPackerFlags {
  SCAN_SSR = (1 << 0), // Parsing a world from Serious Sam Revolution
  SCAN_INI = (1 << 1), // Include INI configs with their MDL files
  SCAN_OGG = (1 << 2), // Pack OGG files if haven't found MP3 files
  SCAN_DEP = (1 << 3), // Only show list of dependencies without packing
  SCAN_GRO = (1 << 4), // Automatically detect GRO files from certain games
};

extern u32 _iFlags; // Packer behavior flags
extern bool _bPauseAtTheEnd; // Pause program execution before closing it

inline bool IsRev(void)     { return (_iFlags & SCAN_SSR) != 0; };
inline bool PackINI(void)   { return (_iFlags & SCAN_INI) != 0; };
inline bool PackOGG(void)   { return (_iFlags & SCAN_OGG) != 0; };
inline bool OnlyDep(void)   { return (_iFlags & SCAN_DEP) != 0; };
inline bool DetectGRO(void) { return (_iFlags & SCAN_GRO) != 0; };

// Check if the file is already in standard dependencies
bool InDepends(const Str_t &strFilename, size_t *piHash = nullptr);

// Check if the file is already added
bool InFiles(Str_t strFilename);

// Add new file to the list and return true if it wasn't there before
bool AddFile(const Str_t &strFilename);

// Replace MP directories with normal ones
void ReplaceRevDirs(Str_t &strFilename);

// Replace spaces with underscores
void ReplaceSpaces(Str_t &strFilename);

// Check if it's a valid world file
void VerifyWorldFile(CDataStream &strmWorld);

#endif
