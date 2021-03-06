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

#include "FilterScorer.hpp"
#include "../decoder/Enforcer.hpp"
using namespace cat;


//// FilterScorer

int FilterScorer::partitionHigh(int left, int right, int pivotIndex) {
	CAT_DEBUG_ENFORCE(left >= 0 && right >= 0 && left <= right);

	int pivotValue = _list[pivotIndex].score;

	// Move pivot to end
	swap(pivotIndex, right);

	int storeIndex = left;

	for (int ii = left; ii < right; ++ii) {
		if (_list[ii].score > pivotValue) {
			swap(storeIndex, ii);

			++storeIndex;
		}
	}

	// Move pivot to its final place
	swap(right, storeIndex);

	return storeIndex;
}

void FilterScorer::quickSortHigh(int left, int right) {
	if (left < right) {
		int pivotIndex = left + (right - left) / 2;
		int pivotNewIndex = partitionHigh(left, right, pivotIndex);
		quickSortHigh(left, pivotNewIndex - 1);
		quickSortHigh(pivotNewIndex + 1, right);
	}
}

int FilterScorer::partitionLow(int left, int right, int pivotIndex) {
	CAT_DEBUG_ENFORCE(left >= 0 && right >= 0 && left <= right);

	int pivotValue = _list[pivotIndex].score;

	// Move pivot to end
	swap(pivotIndex, right);

	int storeIndex = left;

	for (int ii = left; ii < right; ++ii) {
		if (_list[ii].score < pivotValue) {
			swap(storeIndex, ii);

			++storeIndex;
		}
	}

	// Move pivot to its final place
	swap(right, storeIndex);

	return storeIndex;
}

void FilterScorer::quickSortLow(int left, int right) {
	if (left < right) {
		int pivotIndex = left + (right - left) / 2;
		int pivotNewIndex = partitionLow(left, right, pivotIndex);
		quickSortLow(left, pivotNewIndex - 1);
		quickSortLow(pivotNewIndex + 1, right);
	}
}

void FilterScorer::init(int count) {
	CAT_DEBUG_ENFORCE(count > 0);

	_list.resize(count);
}

void FilterScorer::reset() {
	for (int ii = 0, iiend = _list.size(); ii < iiend; ++ii) {
		_list[ii].score = 0;
		_list[ii].index = ii;
	}
}

FilterScorer::Score *FilterScorer::getLowest() {
	Score *lowest = _list.get();
	int lowestScore = lowest->score;

	for (int ii = 1, iiend = _list.size(); ii < iiend; ++ii) {
		int score = _list[ii].score;

		if (lowestScore > score) {
			lowestScore = score;
			lowest = _list.get() + ii;
		}
	}

	return lowest;
}

FilterScorer::Score *FilterScorer::getHigh(int k, bool sorted) {
	CAT_DEBUG_ENFORCE(k >= 1);

	if (k >= _list.size()) {
		k = _list.size();
	}

	const int listSize = k;
	int left = 0;
	int right = _list.size() - 1;
	int pivotIndex = 0;

	for (;;) {
		int pivotNewIndex = partitionHigh(left, right, pivotIndex);

		int pivotDist = pivotNewIndex - left + 1;
		if (pivotDist == k) {
			// Sort the list we are returning
			if (sorted) {
				quickSortHigh(0, listSize - 1);
			}

			return _list.get();
		} else if (k < pivotDist) {
			right = pivotNewIndex - 1;
		} else {
			k -= pivotDist;
			left = pivotNewIndex + 1;
		}
	}

	CAT_DEBUG_EXCEPTION();
	return 0;
}

FilterScorer::Score *FilterScorer::getLow(int k, bool sorted) {
	CAT_DEBUG_ENFORCE(k >= 1);

	if (k >= _list.size()) {
		k = _list.size();
	}

	const int listSize = k;
	int left = 0;
	int right = _list.size() - 1;
	int pivotIndex = 0;

	for (;;) {
		int pivotNewIndex = partitionLow(left, right, pivotIndex);

		int pivotDist = pivotNewIndex - left + 1;
		if (pivotDist == k) {
			// Sort the list we are returning
			if (sorted) {
				quickSortLow(0, listSize - 1);
			}

			return _list.get();
		} else if (k < pivotDist) {
			right = pivotNewIndex - 1;
		} else {
			k -= pivotDist;
			left = pivotNewIndex + 1;
		}
	}

	CAT_DEBUG_EXCEPTION();
	return 0;
}

