/*
   Source File : Type1ToType2Converter.h


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

#include <map>
#include <set>
#include <string>
#include <vector>

namespace charta
{
class IByteWriter;
}
class Type1Input;

struct ConversionNode
{
    uint16_t mMarkerType;
    LongList mOperands;
};

typedef std::list<ConversionNode> ConversionNodeList;

struct Stem
{
    Stem()
    {
        mOrigin = 0;
        mExtent = 0;
    }
    Stem(const Stem &inStem)
    {
        mOrigin = inStem.mOrigin;
        mExtent = inStem.mExtent;
    }
    Stem(long inOrigin, long inExtent)
    {
        mOrigin = inOrigin;
        mExtent = inExtent;
    }

    long mOrigin;
    long mExtent;
};

struct StemLess
{
    bool operator()(const Stem &inLeft, const Stem &inRight) const
    {
        if (inLeft.mOrigin == inRight.mOrigin)
            return inLeft.mExtent < inRight.mExtent;
        else
            return inLeft.mOrigin < inRight.mOrigin;
    }
};

typedef std::vector<const Stem *> StemVector;
typedef std::set<Stem, StemLess> StemSet;
typedef std::set<size_t> SizeTSet;
typedef std::map<Stem, size_t, StemLess> StemToSizeTMap;

class Type1ToType2Converter : IType1InterpreterImplementation
{
  public:
    Type1ToType2Converter(void);
    ~Type1ToType2Converter(void);

    charta::EStatusCode WriteConvertedFontProgram(const std::string &inGlyphName, Type1Input *inType1Input,
                                                  charta::IByteWriter *inByteWriter);

    // IType1InterpreterImplementation
    virtual charta::EStatusCode Type1Hstem(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Vstem(const LongList &inOperandList);
    virtual charta::EStatusCode Type1VMoveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1RLineto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1HLineto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1VLineto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1RRCurveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1ClosePath(const LongList &inOperandList);
    virtual Type1CharString *GetSubr(long inSubrIndex);
    virtual charta::EStatusCode Type1Return(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Hsbw(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Endchar(const LongList &inOperandList);
    virtual charta::EStatusCode Type1RMoveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1HMoveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1VHCurveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1HVCurveto(const LongList &inOperandList);
    virtual charta::EStatusCode Type1DotSection(const LongList &inOperandList);
    virtual charta::EStatusCode Type1VStem3(const LongList &inOperandList);
    virtual charta::EStatusCode Type1HStem3(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Seac(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Sbw(const LongList &inOperandList);
    virtual charta::EStatusCode Type1Div(const LongList &inOperandList);
    virtual bool IsOtherSubrSupported(long inOtherSubrsIndex);
    virtual charta::EStatusCode CallOtherSubr(const LongList &inOperandList, LongList &outPostScriptOperandStack);
    virtual charta::EStatusCode Type1Pop(const LongList &inOperandList, const LongList &inPostScriptOperandStack);
    virtual charta::EStatusCode Type1SetCurrentPoint(const LongList &inOperandList);
    virtual charta::EStatusCode Type1InterpretNumber(long inOperand);
    virtual unsigned long GetLenIV();

  private:
    Type1Input *mHelper;
    ConversionNodeList mConversionProgram;
    bool mHintReplacementEncountered;
    bool mHintAdditionEncountered;
    bool mFirstPathConstructionEncountered;
    bool mIsFirst2Coordinates;
    long mSideBearing[2];
    long mWidth[2];

    // hints handling
    StemToSizeTMap mHStems;
    StemToSizeTMap mVStems;
    SizeTSet mCurrentHints;

    // flex handling
    bool mInFlexCollectionMode;
    LongList mFlexParameters;

    charta::EStatusCode RecordOperatorWithParameters(uint16_t inMarkerType, const LongList &inOperandList);
    void RecordOperatorMarker(uint16_t inMarkerType);
    charta::EStatusCode AddHStem(long inOrigin, long inExtent);
    charta::EStatusCode AddVStem(long inOrigin, long inExtent);
    void ConvertStems();
    ConversionNodeList::iterator CollectHintIndexesFromHere(ConversionNodeList::iterator inFirstStemHint);
    ConversionNodeList::iterator InsertOperatorMarker(uint16_t inMarkerType,
                                                      ConversionNodeList::iterator inInsertPosition);
    void SetupStemHintsInNode(const StemVector &inStems, long inOffsetCoordinate, ConversionNode &refNode);
    bool IsStemHint(uint16_t inMarkerType);
    long GenerateHintMaskFromCollectedHints();
    void ConvertPathConsturction();
    ConversionNodeList::iterator MergeSameOperators(ConversionNodeList::iterator inStartingNode);
    ConversionNodeList::iterator MergeSameOperators(ConversionNodeList::iterator inStartingNode, uint16_t inOpCode);
    ConversionNodeList::iterator MergeAltenratingOperators(ConversionNodeList::iterator inStartingNode,
                                                           uint16_t inAlternatingOpcode);
    void AddInitialWidthParameter();
    charta::EStatusCode WriteProgramToStream(charta::IByteWriter *inByteWriter);
};
