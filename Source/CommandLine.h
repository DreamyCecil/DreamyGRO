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

#ifndef _DREAMYGRO_INCL_COMMANDLINE_H
#define _DREAMYGRO_INCL_COMMANDLINE_H

#include "Main.h"

// Command line arguments
#define ARG_ROOT   "-r" // Root directory of some Sam game; scans based on where the WLD file is if not defined
#define ARG_MOD    "-m" // Specifies the name of a mod folder where the files are being included from
#define ARG_OUTPUT "-o" // Output GRO file; can be an absolute path or relative to the root directory
#define ARG_SCAN   "-i" // Includes this file for scanning and adds all of the found dependencies in the list
#define ARG_STORE  "-s" // Don't compress files of a certain type when packing them
#define ARG_DEPEND "-d" // Don't include files in the final GRO specified as dependencies or any files within other GRO files
#define ARG_FLAGS  "-f" // Set certain behavior flags (see EPackerFlags definition)
#define ARG_PAUSE  "-p" // Pause program execution before closing the console application

// Games that can be detected automatically
enum EGameType {
  GAME_NONE, // Unknown game
  GAME_TFE,
  GAME_TSE,
  GAME_REV,
};

// Argument descriptions
extern const char *_astrArgDesc[];

// Parse command line arguments
void ParseArguments(Strings_t &aArgs);

// Build parameters from the full path and return file path relative to the root directory
Str_t FromFullFilePath(const CPath &strFile, const CPath &strDefaultFolderInRoot);

// Detect default GRO packages in some directory to determine the game
EGameType DetectGame(const Str_t &strDir);

// Automatically ignore GRO files from a specific game
void IgnoreGame(EGameType eGame, bool bSetFlagsFromGame);

// Ignore dependencies from a GRO file
void IgnoreGRO(const Str_t &strGRO);

#endif
