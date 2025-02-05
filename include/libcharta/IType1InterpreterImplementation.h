/*
   Source File : IType1InterpreterImplementation.h


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
#include <stdint.h>
#include <stdio.h>

#include <list>

struct Type1CharString
{
    uint8_t *Code;
    int CodeLength;
};

typedef std::list<long> LongList;

class IType1InterpreterImplementation
{
  public:
    virtual charta::EStatusCode Type1Hstem(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Vstem(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1VMoveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1RLineto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1HLineto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1VLineto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1RRCurveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1ClosePath(const LongList &inOperandList) = 0;
    virtual Type1CharString *GetSubr(long inSubrIndex) = 0;
    virtual charta::EStatusCode Type1Return(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Hsbw(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Endchar(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1RMoveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1HMoveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1VHCurveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1HVCurveto(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1DotSection(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1VStem3(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1HStem3(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Seac(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Sbw(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1Div(const LongList &inOperandList) = 0;
    virtual bool IsOtherSubrSupported(long inOtherSubrsIndex) = 0;
    virtual charta::EStatusCode CallOtherSubr(const LongList &inOperandList, LongList &outPostScriptOperandStack) = 0;
    virtual charta::EStatusCode Type1Pop(const LongList &inOperandList, const LongList &inPostScriptOperandStack) = 0;
    virtual charta::EStatusCode Type1SetCurrentPoint(const LongList &inOperandList) = 0;
    virtual charta::EStatusCode Type1InterpretNumber(long inOperand) = 0;
    virtual unsigned long GetLenIV() = 0;
};

class Type1InterpreterImplementationAdapter : public IType1InterpreterImplementation
{
  public:
    virtual charta::EStatusCode Type1Hstem(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Vstem(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1VMoveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1RLineto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1HLineto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1VLineto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1RRCurveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1ClosePath(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual Type1CharString *GetSubr(long inSubrIndex)
    {
        (void)inSubrIndex;
        return NULL;
    }
    virtual charta::EStatusCode Type1Return(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Hsbw(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Endchar(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1RMoveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1HMoveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1VHCurveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1HVCurveto(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1DotSection(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1VStem3(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1HStem3(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Seac(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Sbw(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Div(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual bool IsOtherSubrSupported(long inOtherSubrsIndex)
    {
        (void)inOtherSubrsIndex;
        return false;
    }
    virtual charta::EStatusCode CallOtherSubr(const LongList &inOperandList, LongList &outPostScriptOperandStack)
    {
        (void)inOperandList;
        (void)outPostScriptOperandStack;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1Pop(const LongList &inOperandList, const LongList &inPostScriptOperandStack)
    {
        (void)inOperandList;
        (void)inPostScriptOperandStack;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1SetCurrentPoint(const LongList &inOperandList)
    {
        (void)inOperandList;
        return charta::eSuccess;
    }
    virtual charta::EStatusCode Type1InterpretNumber(long inOperand)
    {
        (void)inOperand;
        return charta::eSuccess;
    }
    virtual unsigned long GetLenIV()
    {
        return 4;
    }
};
