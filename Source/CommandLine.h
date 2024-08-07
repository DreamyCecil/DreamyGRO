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

typedef void (*FProcessCmdArg)(Strings_t::const_iterator &it, const Strings_t::const_iterator &itEnd);

struct CmdArg_t {
  const c8 *strFull; // Full command name, e.g. "root"
  const c8 *strShort; // Short command name, e.g. "r"

  const c8 *strDescription; // Brief description of the command
  const c8 *strExample; // Usage example

  FProcessCmdArg pFunc; // Function for processing the command
};

// Available commands
extern const CmdArg_t _aCmdArgs[];
extern const size_t _ctCmdArgs;

// Display help about command line arguments
void CmdHelp(s32 iCommand);

// Games that can be detected automatically
enum EGameType {
  GAME_NONE, // Unknown game
  GAME_TFE,
  GAME_TSE,
  GAME_REV,
};

// Parse command line arguments
bool ParseArguments(Strings_t &aArgs);

// Build parameters from the full path and return file path relative to the root directory
CString FromFullFilePath(const CString &strFile, const CString &strDefaultFolderInRoot);

// Detect default GRO packages in some directory to determine the game
EGameType DetectGame(const CString &strDir);

// Automatically ignore GRO files from a specific game
void IgnoreGame(EGameType eGame, bool bSetFlagsFromGame);

// Ignore dependencies from a GRO file
void IgnoreGRO(const CString &strGRO);

#endif
