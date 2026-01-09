/***
 * Self-contained: U++ source code included internally (BSD License)
 * U++ official website: https://www.ultimatepp.org/
 * 
 * Copyright (c) 2025 Du Jie (@ceepuka). All rights reserved.
 * This software is currently freeware, planned to convert to
 * the BSD 3-Clause open source license on 2026-06-30.
**/

#ifndef _uppFilemodel_fjiffyldg_h_
#define _uppFilemodel_fjiffyldg_h_

#ifdef FJIFFYLDG_SHARED
  #if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(BUILDING_FJIFFYLDG)
        #define FJIFFYLDG_API __declspec(dllexport)
    #else
        #define FJIFFYLDG_API __declspec(dllimport)
    #endif
  #elif defined(__GNUC__) || defined(__clang__)
        #define FJIFFYLDG_API __attribute__((visibility("default")))
  #else
        #define FJIFFYLDG_API
        #pragma warning "Unknown platform, export may not work!"
  #endif
#else
        #define FJIFFYLDG_API
#endif

/* Important Notes:
 * - File line indices and content positions are zero-based.
 * - Never free data pointers obtained from interface functions directly.
 * - C users must pair 'fjiffyldg_create()' with 'fjiffyldg_clear()' for resource management.
 * - Theoretical file size limit: < 8EB
 */

#ifdef __cplusplus

  #include <cstddef>
  #include <memory>
  // C++ interface...
  namespace Fjiffyldg {class Fjiffyldg;}
  
#else
  #include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// A handle for a fjiffyldg file processing method.
typedef struct fjiffyldg_t * fjiffyldg_ptr;

// Disable for dynamic memory functions in C++ .
#if !defined(__cplusplus)

// Create and return an instance of fjiffyldg_ptr.
FJIFFYLDG_API	fjiffyldg_ptr fjiffyldg_create(void);

// Clean up an instance.
// @param 'fm' in the fjiffyldg_ptr handle to clear.
FJIFFYLDG_API	void fjiffyldg_clear(fjiffyldg_ptr fm);
#endif

/* File Loading Error Codes:
 *  0: Success (no error)
 * -1: File does not exist or was never loaded
 *  1: File content or attributes inaccessible
 *  2: File stream error
 *  3: Memory-mapped file error
 */

// Loads file content while building its line index structure.
// @return 0 if scanning started successfully and first block loaded.
// Non-zero error code on failure (refer to error code definitions).
// Note: Function returns immediately, scanning runs concurrently in background.
FJIFFYLDG_API	int LoadAndScanFile(fjiffyldg_ptr fm, const char *name);

// Loads raw file content (no line structure processing).
// @return 0 if successful, non-zero error code on failure. (refer to error code definitions)
FJIFFYLDG_API	int LoadFileOnly(fjiffyldg_ptr fm, const char *name);

// Gets the file loading status.
// @return 0 if loading system is operational, non-zero error code on failure. (refer to error code definitions)
FJIFFYLDG_API	int GetFileIsLoaded(fjiffyldg_ptr fm);

// Rescans the file's line structure.
// @param 'offset' Number of bytes to offset the scan start position (typically 0)
/* @param 'utf' Text encoding scan mode:
 *            0: Default mode (ASCII control character compatible)
 *            1: UTF-16LE (Little Endian)
 *            2: UTF-16BE (Big Endian)
 *            3: UTF-32LE (Little Endian)
 *            4: UTF-32BE (Big Endian)
 *           -1: Automatic detection (determined by BOM at file beginning)
 *           Other: Use default mode
 */
FJIFFYLDG_API	void RestartScanFile(fjiffyldg_ptr fm, const char *name, long long offset, int utf);

// Blocks until the background file scanning task finishes.
FJIFFYLDG_API	void WaitFileScanTaskFinished(fjiffyldg_ptr fm);

// Gets the total number of lines in the file.
/* @return >0 : Total line count (scanning completed)
 *          0 : Scanning not yet started
 *         <0 : Scanning in progress, line count unknown
 */
FJIFFYLDG_API	long long GetFileLineCount(fjiffyldg_ptr fm);

// Gets the byte offset position of the specified line in the file.
// @param 'index' line index in the file
// @return Byte offset (>=0), or negative if line position unknown.
FJIFFYLDG_API	long long GetFileLinePos(fjiffyldg_ptr fm, long long index);

// Gets the byte length of the specified line (excluding line break characters).
// @return Non-negative value indicates success.
FJIFFYLDG_API	long long GetFileLineLength(fjiffyldg_ptr fm, long long index);

// Finds the line index containing the specified byte position.
// @param 'pos' byte offset in file
// @return Line index (>=0), or negative if not found.
FJIFFYLDG_API	long long GetFileLineIndex(fjiffyldg_ptr fm, long long pos);

// Reads file data from the specified byte position.
// @param 'pos' Byte offset from file start where reading begins
// @param 'len' Number of bytes to read (0 = use default size).
//			Receives the actual number of bytes read on return
// @return Pointer to data buffer on success, NULL on failure.
FJIFFYLDG_API	const char* ReadFileData(fjiffyldg_ptr fm, long long pos, unsigned int *len);

// Reads file data by lines with truncation for overly long lines (max 4KB per line).
/* Reads up to 'len' bytes starting from line 'index', respecting line boundaries
 * and truncating lines >4KB. The function may return less data than the ideal
 * line-aligned range [bpos, epos) when hitting internal chunk limits.
 *
 * @param 'index' [in/out] Line index to start reading from.
 *                           Returns the line index at 'epos' position.
 * @param 'bpos'  [out]    Byte offset of the starting position (line 'index').
 * @param 'epos'  [out]    Ideal byte offset for complete line-aligned read.
 *                           Indicates where reading SHOULD end to maintain
 *                           line integrity.
 * @param 'len'   [in/out] Expected byte count (0 = default size).
 *                      Returns actual bytes read (may be less than epos-bpos).
 *
 * @return Pointer to data buffer on success, NULL on failure.
 *
 * @note Check if actual_len == (epos - bpos) to verify complete line-aligned read.
 *       If less, continue reading from current position to reach 'epos'.
 */
FJIFFYLDG_API	const char* ReadFileDataLLineCut(fjiffyldg_ptr fm, long long *index, long long *bpos, long long *epos, unsigned int *len);

// Reads data starting from a specified byte position within a known line.
/* Reads at most 'len' bytes beginning at 'pos', constrained only by file boundaries.
 * The 'index' parameter must indicate the line containing 'pos'.
 *
 * @param 'index' Line index containing 'pos' (hint for optimized lookup)
 * @param 'pos'   Starting byte offset in file (must be ≥ 'line[index].start')
 * @param 'len'   [in/out] Maximum bytes to read (0 = default),
 *                      returns actual bytes read
 *
 * @return Pointer to data buffer on success, NULL on failure.
 */
FJIFFYLDG_API	const char* ReadFileDataEndOfLine(fjiffyldg_ptr fm, long long index, long long pos, unsigned int *len);

// Attempts to memory-map the entire file (independent of Load functions).
// Internal automatically prevents duplicate mappings of same file.
// @param 'bufferSize' [out] Returns the size of the mapped buffer in bytes
// @return Pointer to mapped data, NULL on failure.
FJIFFYLDG_API	const char* GetFileMappedHuge(fjiffyldg_ptr fm, const char *fileName, long long *bufferSize);

// Clean up the huge memory-map buffer.
// Note: fjiffyldg_clear() will also clean all mapping resources.
FJIFFYLDG_API	void ClearHugeBuffer(fjiffyldg_ptr fm);

/**
* The following interface functions do not require a fjiffyldg_ptr handle.
*/

// Gets the file size in bytes.
// @return File size in bytes (>=0), negative value on failure.
FJIFFYLDG_API	long long GetFileSizeByteCount(const char *name);

// Checks if the text contains only ASCII characters.
// @return 0 if fully ASCII, otherwise the length of remaining non-ASCII text.
FJIFFYLDG_API	unsigned int CheckTextASCII(const char *text, unsigned int len);

// Validates complete UTF-8 text without truncation.
// Text must be complete (no truncated multi-byte characters).
// @return 0 if valid UTF-8, otherwise length of remaining invalid text.
FJIFFYLDG_API	unsigned int CheckWholeTextUtf8(const char *text, unsigned int len);

// Checks randomly-sampled text segment for UTF-8 encoding.
// @note Prefer text length >= 10 bytes for reliable detection.
// @return 0 if UTF-8, otherwise length of invalid segment.
FJIFFYLDG_API	unsigned int CheckExtractTextUtf8(const char *text, unsigned int len);

// Counts UTF-8 characters (not bytes) in text.
// @param 'text' [in/out] Pointer to text start address,
//                 updated to position where counting stopped.
// @return Count of valid UTF-8 characters (stops at invalid character).
FJIFFYLDG_API	unsigned int GetUtf8TextCharCount(const char * *text, unsigned int len);

// Copies a file.
// @return 0 on success, 1 if file does not meet expectations, negative for more severe errors.
FJIFFYLDG_API	int ToCloneFile(const char *oldFileName, const char *newFileName);

// Saves specified data content to a file.
// @return 0 on success, as stated above.
FJIFFYLDG_API	int ToSaveFile(const char *fileName, const char *buffer, long long len);

// Appends specified data to a file.
// @return 0 on success, as stated above.
FJIFFYLDG_API	int ToAppendFile(const char *fileName, const char *buffer, long long len);

// Concatenates two files, appending second file's content to the first.
// @return 0 on success, as stated above.
FJIFFYLDG_API	int ToConcatenateFile(const char *catFileName, const char *appendFileName);

#ifdef __cplusplus
}
#endif

//  PIMPL pattern C++ API Handle
#ifdef __cplusplus
namespace Fjiffyldg {
	class FJIFFYLDG_API Fjiffyldg {
	public:
		Fjiffyldg();
		// Gets the file model operation handle
		fjiffyldg_ptr GetFjiffyldgHandle();
		Fjiffyldg(Fjiffyldg &&) = default;
		Fjiffyldg& operator=(Fjiffyldg &&) = default;
		
		Fjiffyldg(const Fjiffyldg &) = delete;
		Fjiffyldg& operator=(const Fjiffyldg &) = delete;
		~Fjiffyldg();

	private:
		struct Impl;
		std::unique_ptr<Impl> pimpl;
	};
}
#endif


#endif
