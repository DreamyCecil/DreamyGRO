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
#define ARG_OUTPUT "-o" // Output GRO file; can be an absolute path or relative to the root directory
#define ARG_WORLD  "-w" // Scans dictionary of this WLD file and adds all dependencies in the list
#define ARG_STORE  "-s" // Don't compress files of a certain type when packing them
#define ARG_DEPEND "-d" // Don't include files in GRO specified as dependencies or files within other GRO files
#define ARG_FLAGS  "-f" // Set certain behavior flags (see EPackerFlags definition)
#define ARG_PAUSE  "-p" // Pause program execution before closing the console application

// Argument descriptions
extern const char *_astrArgDesc[];

// Parse command line arguments
void ParseArguments(Strings_t &aArgs);

// Build parameters from just a path to the world file
void FromWorldPath(const CPath &strWorld);

// Automatically detect GRO files from specific games
void AutoIgnoreGames(bool bFromWorld);

// Ignore dependencies from a GRO file
void IgnoreGRO(const Str_t &strGRO);

#endif
