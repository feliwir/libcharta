/*
   Source File : WrittenFontRepresentation.h


   Copyright 2011 Gal Kahana PDFWriter

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


*/
#pragma once

#include "ObjectsBasicTypes.h"

#include <algorithm>
#include <map>
#include <stdint.h>
#include <vector>

typedef std::vector<unsigned long> ULongVector;

struct GlyphEncodingInfo
{
    uint16_t mEncodedCharacter;
    ULongVector mUnicodeCharacters;

    GlyphEncodingInfo()
    {
    }

    GlyphEncodingInfo(uint16_t inEncodedCharacter, unsigned long inUnicodeCharacter)
    {
        mEncodedCharacter = inEncodedCharacter;
        mUnicodeCharacters.push_back(inUnicodeCharacter);
    }

    GlyphEncodingInfo(uint16_t inEncodedCharacter, ULongVector inUnicodeCharacters)
    {
        mEncodedCharacter = inEncodedCharacter;
        mUnicodeCharacters = inUnicodeCharacters;
    }
};

typedef std::map<uint32_t, GlyphEncodingInfo> UIntToGlyphEncodingInfoMap;
typedef std::vector<uint32_t> UIntVector;

static UIntVector GetOrderedKeys(const UIntToGlyphEncodingInfoMap &inMap)
{
    UIntVector result;
    for (UIntToGlyphEncodingInfoMap::const_iterator it = inMap.begin(); it != inMap.end(); ++it)
        result.push_back(it->first);
    sort(result.begin(), result.end());
    return result;
}

struct WrittenFontRepresentation
{
    WrittenFontRepresentation()
    {
        mWrittenObjectID = 0;
    }

    UIntToGlyphEncodingInfoMap mGlyphIDToEncodedChar;
    ObjectIDType mWrittenObjectID;

    bool isEmpty()
    {
        return mGlyphIDToEncodedChar.empty();
    }

    UIntVector GetGlyphIDsAsOrderedVector()
    {
        return GetOrderedKeys(mGlyphIDToEncodedChar);
    }
};
