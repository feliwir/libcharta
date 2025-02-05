/*
   Source File : Type1Input.h


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
#include "EStatusCode.h"
#include "IType1InterpreterImplementation.h"
#include "io/InputPFBDecodeStream.h"

#include <map>
#include <set>
#include <string>
#include <vector>

enum EType1EncodingType
{
    eType1EncodingTypeStandardEncoding,
    eType1EncodingTypeCustom
};

struct Type1Encoding
{
    EType1EncodingType EncodingType;
    std::string mCustomEncoding[256];
};

struct Type1FontDictionary
{
    std::string FontName;
    int PaintType;
    int FontType;
    double FontMatrix[6];
    double FontBBox[4];
    int UniqueID;
    // Metrics ignored
    double StrokeWidth;
    bool FSTypeValid;
    uint16_t fsType;
};

struct Type1FontInfoDictionary
{
    std::string version;
    std::string Notice;
    std::string Copyright;
    std::string FullName;
    std::string FamilyName;
    std::string Weight;
    double ItalicAngle;
    bool isFixedPitch;
    double UnderlinePosition;
    double UnderlineThickness;
    bool FSTypeValid;
    uint16_t fsType;
};

struct Type1PrivateDictionary
{
    int UniqueID;
    std::vector<int> BlueValues;
    std::vector<int> OtherBlues;
    std::vector<int> FamilyBlues;
    std::vector<int> FamilyOtherBlues;
    double BlueScale;
    int BlueShift;
    int BlueFuzz;
    double StdHW;
    double StdVW;
    std::vector<double> StemSnapH;
    std::vector<double> StemSnapV;
    bool ForceBold;
    int LanguageGroup;
    int lenIV;
    bool RndStemUp;
};

typedef std::set<uint8_t> ByteSet;
typedef std::set<uint16_t> UShortSet;

struct CharString1Dependencies
{
    ByteSet mCharCodes;    // from seac operator
    UShortSet mOtherSubrs; // from callothersubr
    UShortSet mSubrs;      // from callsubr
};

typedef std::vector<Type1CharString> Type1CharStringVector;
typedef std::map<std::string, Type1CharString> StringToType1CharStringMap;
typedef std::map<std::string, uint8_t> StringToByteMap;

namespace charta
{
class IByteReaderWithPosition;
};

class Type1Input : public Type1InterpreterImplementationAdapter
{
  public:
    Type1Input(void);
    ~Type1Input(void);

    charta::EStatusCode ReadType1File(charta::IByteReaderWithPosition *inType1File);
    charta::EStatusCode CalculateDependenciesForCharIndex(uint8_t inCharStringIndex,
                                                          CharString1Dependencies &ioDependenciesInfo);
    charta::EStatusCode CalculateDependenciesForCharIndex(const std::string &inCharStringName,
                                                          CharString1Dependencies &ioDependenciesInfo);
    void Reset();
    Type1CharString *GetGlyphCharString(const std::string &inCharStringName);
    Type1CharString *GetGlyphCharString(uint8_t inCharStringIndex);
    std::string GetGlyphCharStringName(uint8_t inCharStringIndex);
    bool IsValidGlyphIndex(uint8_t inCharStringIndex);
    uint8_t GetEncoding(const std::string &inCharStringName);
    bool IsCustomEncoding() const;

    // some structs for you all laddies and lasses
    Type1FontDictionary mFontDictionary;
    Type1FontInfoDictionary mFontInfoDictionary;
    Type1PrivateDictionary mPrivateDictionary;

    // IType1InterpreterImplementation overrides
    virtual Type1CharString *GetSubr(long inSubrIndex);
    virtual charta::EStatusCode Type1Seac(const LongList &inOperandList);
    virtual bool IsOtherSubrSupported(long inOtherSubrsIndex);
    virtual unsigned long GetLenIV();

  private:
    Type1Encoding mEncoding;
    StringToByteMap mReverseEncoding;
    long mSubrsCount;
    Type1CharString *mSubrs;
    StringToType1CharStringMap mCharStrings;

    charta::InputPFBDecodeStream mPFBDecoder;

    CharString1Dependencies *mCurrentDependencies;

    void FreeTables();
    bool IsComment(const std::string &inToken);
    charta::EStatusCode ReadFontDictionary();
    charta::EStatusCode ReadFontInfoDictionary();
    std::string FromPSName(const std::string &inPostScriptName);
    charta::EStatusCode ParseEncoding();
    charta::EStatusCode ReadPrivateDictionary();
    charta::EStatusCode ParseIntVector(std::vector<int> &inVector);
    charta::EStatusCode ParseDoubleVector(std::vector<double> &inVector);
    charta::EStatusCode ParseSubrs();
    charta::EStatusCode ParseCharstrings();
    charta::EStatusCode ParseDoubleArray(double *inArray, int inArraySize);
    std::string FromPSString(const std::string &inPSString);
    void CalculateReverseEncoding();
};
