/*
	Copyright (c) 2013 Game Closure.  All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of GCIF nor the names of its contributors may be used
	  to endorse or promote products derived from this software without
	  specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include "ImageWriter.hpp"
#include "../decoder/EndianNeutral.hpp"
#include "../decoder/MappedFile.hpp"
#include "GCIFWriter.h"
using namespace cat;


//// WriteVector

void WriteVector::clear() {
	int words = HEAD_SIZE;
	u32 *ptr = _head;

	// For each rope,
	while (ptr) {
		u32 *nextPtr = *reinterpret_cast<u32**>( ptr + words );

		delete []ptr;

		ptr = nextPtr;
		words <<= 1;
	}

	_head = 0;
	_work = 0;
}

void WriteVector::grow() {
	const int newAllocated = _allocated << 1;

	// If initializing,
	u32 *newWork = new u32[newAllocated + PTR_WORDS];

	// Point current "next" pointer to new workspace
	*reinterpret_cast<u32**>( _work + _allocated ) = newWork;

	// Set "next" pointer to null
	*reinterpret_cast<u32**>( newWork + newAllocated ) = 0;

	// Update class state
	_work = newWork;
	_allocated = newAllocated;
	_used = 0;
}

void WriteVector::init() {
	clear();

	u32 *newWork = new u32[HEAD_SIZE + PTR_WORDS];
	_head = _work = newWork;

	_used = 0;
	_allocated = HEAD_SIZE;
	_size = 0;

	// Set "next" pointer to null
	*reinterpret_cast<u32**>( newWork + HEAD_SIZE ) = 0;
}

void WriteVector::write(u32 *target) {
	u32 *ptr = _head;

	// If any data to write at all,
	if (ptr) {
		int words = HEAD_SIZE;
		u32 *nextPtr = *reinterpret_cast<u32**>( ptr + words );

		// For each full rope,
		while (nextPtr) {
			memcpy(target, ptr, words * WORD_BYTES);
			target += words;

			ptr = nextPtr;
			words <<= 1;

			nextPtr = *reinterpret_cast<u32**>( ptr + words );
		}

		// Write final partial rope
		memcpy(target, ptr, _used * WORD_BYTES);
	}
}


//// ImageWriter

int ImageWriter::init(int xsize, int ysize) {
	// Validate
	if (xsize < 0 || ysize < 0 ||
		xsize > MAX_X || ysize > MAX_Y) {
		return GCIF_WE_BAD_DIMS;
	}

	// Initialize
	_header.xsize = static_cast<u16>( xsize );
	_header.ysize = static_cast<u16>( ysize );

	_work = 0;
	_bits = 0;

	_words.init();

	// Write header
	writeWord(HEAD_MAGIC);
	writeBits(xsize, MAX_X_BITS);
	writeBits(ysize, MAX_Y_BITS);

	return GCIF_WE_OK;
}

void ImageWriter::writeBits(u32 code, int len) {
	CAT_DEBUG_ENFORCE(len >= 1 && len <= 32);
	CAT_DEBUG_ENFORCE(len == 32 || (code >> len) == 0);

	int bits = _bits;

	_work |= (u64)code << (64 - len - bits);

	bits += len;

	if CAT_UNLIKELY(bits >= 32) {
		_words.push((u32)(_work >> 32));

		_work <<= 32;
		bits -= 32;
	}

	_bits = bits;
}

u32 ImageWriter::finalize() {
	// Finalize the bit data

	CAT_DEBUG_ENFORCE(_bits <= 32);

	if (_bits > 0) {
		// Write the final word
		_words.push((u32)(_work >> 32));
	}

	return _words.getWordCount();
}

int ImageWriter::write(const char *path) {
	MappedFile file;

	// Calculate file size

	const int wordCount = _words.getWordCount();
	const int totalBytes = wordCount * sizeof(u32);

	// Map the file

	if (!file.OpenWrite(path, totalBytes)) {
		return GCIF_WE_FILE;
	}

	MappedView fileView;

	if (!fileView.Open(&file)) {
		return GCIF_WE_FILE;
	}

	u8 *fileData = fileView.MapView();

	if (!fileData) {
		return GCIF_WE_FILE;
	}

	u32 *fileWords = reinterpret_cast<u32*>( fileData );

	// Copy file data

	_words.write(fileWords);

	return GCIF_WE_OK;
}

