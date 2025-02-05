/*
   Source File : CharStringType2Interpreter.cpp


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
#include "text/cff/CharStringType2Interpreter.h"
#include "Trace.h"
#include <math.h>
#include <stdlib.h>

using namespace charta;

CharStringType2Interpreter::CharStringType2Interpreter()
{
    mImplementationHelper = nullptr;
}

CharStringType2Interpreter::~CharStringType2Interpreter() = default;

EStatusCode CharStringType2Interpreter::Intepret(const CharString &inCharStringToIntepret,
                                                 IType2InterpreterImplementation *inImplementationHelper)
{
    uint8_t *charString = nullptr;
    EStatusCode status;

    do
    {
        mImplementationHelper = inImplementationHelper;
        mGotEndChar = false;
        mStemsCount = 0;
        mCheckedWidth = false;
        if (inImplementationHelper == nullptr)
        {
            TRACE_LOG(
                "CharStringType2Interpreter::Intepret, null implementation helper passed. pass a proper pointer!!");
            status = charta::eFailure;
            break;
        }

        status = mImplementationHelper->ReadCharString(inCharStringToIntepret.mStartPosition,
                                                       inCharStringToIntepret.mEndPosition, &charString);
        if (status != charta::eSuccess)
        {
            TRACE_LOG2(
                "CharStringType2Interpreter::Intepret, failed to read charstring starting in %lld and ending in %lld",
                inCharStringToIntepret.mStartPosition, inCharStringToIntepret.mEndPosition);
            break;
        }

        status =
            ProcessCharString(charString, inCharStringToIntepret.mEndPosition - inCharStringToIntepret.mStartPosition);

    } while (false);

    delete[] charString;

    return status;
}

EStatusCode CharStringType2Interpreter::ProcessCharString(uint8_t *inCharString, long long inCharStringLength)
{
    EStatusCode status = charta::eSuccess;
    uint8_t *pointer = inCharString;
    bool gotEndExecutionOperator = false;

    while (pointer - inCharString < inCharStringLength && charta::eSuccess == status && !gotEndExecutionOperator &&
           !mGotEndChar)
    {
        if (IsOperator(pointer))
        {
            pointer = InterpretOperator(pointer, gotEndExecutionOperator);
            if (pointer == nullptr)
                status = charta::eFailure;
        }
        else
        {
            pointer = InterpretNumber(pointer);
            if (pointer == nullptr)
                status = charta::eFailure;
        }
    }
    return status;
}

bool CharStringType2Interpreter::IsOperator(uint8_t *inProgramCounter)
{
    return ((*inProgramCounter) <= 27) || (29 <= (*inProgramCounter) && (*inProgramCounter) <= 31);
}

uint8_t *CharStringType2Interpreter::InterpretNumber(uint8_t *inProgramCounter)
{
    CharStringOperand operand;
    uint8_t *newPosition = inProgramCounter;

    if (28 == *newPosition)
    {
        operand.IsInteger = true;
        operand.IntegerValue = (short)(((uint16_t)(*(newPosition + 1)) << 8) + (*(newPosition + 2)));
        newPosition += 3;
    }
    else if (32 <= *newPosition && *newPosition <= 246)
    {
        operand.IsInteger = true;
        operand.IntegerValue = (short)*newPosition - 139;
        ++newPosition;
    }
    else if (247 <= *newPosition && *newPosition <= 250)
    {
        operand.IsInteger = true;
        operand.IntegerValue = (*newPosition - 247) * 256 + *(newPosition + 1) + 108;
        newPosition += 2;
    }
    else if (251 <= *newPosition && *newPosition <= 254)
    {
        operand.IsInteger = true;
        operand.IntegerValue = -(short)(*newPosition - 251) * 256 - *(newPosition + 1) - 108;
        newPosition += 2;
    }
    else if (255 == *newPosition)
    {
        operand.IsInteger = false;
        operand.RealValue = (short)(((uint16_t)(*(newPosition + 1)) << 8) + (*(newPosition + 2)));
        if (operand.RealValue > 0)
            operand.RealValue += (double)(((uint16_t)(*(newPosition + 3)) << 8) + (*(newPosition + 4))) / (1 << 16);
        else
            operand.RealValue -= (double)(((uint16_t)(*(newPosition + 3)) << 8) + (*(newPosition + 4))) / (1 << 16);
        newPosition += 5;
    }
    else
        newPosition = nullptr; // error

    if (newPosition != nullptr)
    {
        mOperandStack.push_back(operand);
        EStatusCode status = mImplementationHelper->Type2InterpretNumber(operand);
        if (status != charta::eSuccess)
            return nullptr;
    }

    return newPosition;
}

uint8_t *CharStringType2Interpreter::InterpretOperator(uint8_t *inProgramCounter, bool &outGotEndExecutionCommand)
{
    uint16_t operatorValue;
    uint8_t *newPosition = inProgramCounter;
    outGotEndExecutionCommand = false;

    if (12 == *newPosition)
    {
        operatorValue = 0x0c00 + *(newPosition + 1);
        newPosition += 2;
    }
    else
    {
        operatorValue = *newPosition;
        ++newPosition;
    }

    switch (operatorValue)
    {
    case 1: // hstem
        CheckWidth();
        newPosition = InterpretHStem(newPosition);
        break;
    case 3: // vstem
        CheckWidth();
        newPosition = InterpretVStem(newPosition);
        break;
    case 4: // vmoveto
        CheckWidth();
        newPosition = InterpretVMoveto(newPosition);
        break;
    case 5: // rlineto
        newPosition = InterpretRLineto(newPosition);
        break;
    case 6: // hlineto
        newPosition = InterpretHLineto(newPosition);
        break;
    case 7: // vlineto
        newPosition = InterpretVLineto(newPosition);
        break;
    case 8: // rrcurveto
        newPosition = InterpretRRCurveto(newPosition);
        break;
    case 10: // callsubr
        newPosition = InterpretCallSubr(newPosition);
        break;
    case 11: // return
        newPosition = InterpretReturn(newPosition);
        outGotEndExecutionCommand = true;
        break;
    case 14: // endchar
        CheckWidth();
        newPosition = InterpretEndChar(newPosition);
        break;
    case 18: // hstemhm
        CheckWidth();
        newPosition = InterpretHStemHM(newPosition);
        break;
    case 19: // hintmask
        CheckWidth();
        newPosition = InterpretHintMask(newPosition);
        break;
    case 20: // cntrmask
        CheckWidth();
        newPosition = InterpretCntrMask(newPosition);
        break;
    case 21: // rmoveto
        CheckWidth();
        newPosition = InterpretRMoveto(newPosition);
        break;
    case 22: // hmoveto
        CheckWidth();
        newPosition = InterpretHMoveto(newPosition);
        break;
    case 23: // vstemhm
        CheckWidth();
        newPosition = InterpretVStemHM(newPosition);
        break;
    case 24: // rcurveline
        newPosition = InterpretRCurveLine(newPosition);
        break;
    case 25: // rlinecurve
        newPosition = InterpretRLineCurve(newPosition);
        break;
    case 26: // vvcurveto
        newPosition = InterpretVVCurveto(newPosition);
        break;
    case 27: // hhcurveto
        newPosition = InterpretHHCurveto(newPosition);
        break;
    case 29: // callgsubr
        newPosition = InterpretCallGSubr(newPosition);
        break;
    case 30: // vhcurveto
        newPosition = InterpretVHCurveto(newPosition);
        break;
    case 31: // hvcurveto
        newPosition = InterpretHVCurveto(newPosition);
        break;

    case 0x0c00: // dotsection, depracated
        // ignore
        break;
    case 0x0c03: // and
        newPosition = InterpretAnd(newPosition);
        break;
    case 0x0c04: // or
        newPosition = InterpretOr(newPosition);
        break;
    case 0x0c05: // not
        newPosition = InterpretNot(newPosition);
        break;
    case 0x0c09: // abs
        newPosition = InterpretAbs(newPosition);
        break;
    case 0x0c0a: // add
        newPosition = InterpretAdd(newPosition);
        break;
    case 0x0c0b: // sub
        newPosition = InterpretSub(newPosition);
        break;
    case 0x0c0c: // div
        newPosition = InterpretDiv(newPosition);
        break;
    case 0x0c0e: // neg
        newPosition = InterpretNeg(newPosition);
        break;
    case 0x0c0f: // eq
        newPosition = InterpretEq(newPosition);
        break;
    case 0x0c12: // drop
        newPosition = InterpretDrop(newPosition);
        break;
    case 0x0c14: // put
        newPosition = InterpretPut(newPosition);
        break;
    case 0x0c15: // get
        newPosition = InterpretGet(newPosition);
        break;
    case 0x0c16: // ifelse
        newPosition = InterpretIfelse(newPosition);
        break;
    case 0x0c17: // random
        newPosition = InterpretRandom(newPosition);
        break;
    case 0x0c18: // mul
        newPosition = InterpretMul(newPosition);
        break;
    case 0x0c1a: // sqrt
        newPosition = InterpretSqrt(newPosition);
        break;
    case 0x0c1b: // dup
        newPosition = InterpretDup(newPosition);
        break;
    case 0x0c1c: // exch
        newPosition = InterpretExch(newPosition);
        break;
    case 0x0c1d: // index
        newPosition = InterpretIndex(newPosition);
        break;
    case 0x0c1e: // roll
        newPosition = InterpretRoll(newPosition);
        break;
    case 0x0c22: // hflex
        newPosition = InterpretHFlex(newPosition);
        break;
    case 0x0c23: // flex
        newPosition = InterpretFlex(newPosition);
        break;
    case 0x0c24: // hflex1
        newPosition = InterpretHFlex1(newPosition);
        break;
    case 0x0c25: // flex1
        newPosition = InterpretFlex1(newPosition);
        break;
    }
    return newPosition;
}

void CharStringType2Interpreter::CheckWidth()
{
    if (!mCheckedWidth)
    {
        if (mOperandStack.size() % 2 != 0) // has width
            mOperandStack.pop_front();
        mCheckedWidth = true;
    }
}

uint8_t *CharStringType2Interpreter::InterpretHStem(uint8_t *inProgramCounter)
{
    mStemsCount += (uint16_t)(mOperandStack.size() / 2);

    EStatusCode status = mImplementationHelper->Type2Hstem(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

EStatusCode CharStringType2Interpreter::ClearNFromStack(uint16_t inCount)
{
    if (mOperandStack.size() >= inCount)
    {
        for (uint16_t i = 0; i < inCount; ++i)
            mOperandStack.pop_back();
        return charta::eSuccess;
    }
    return charta::eFailure;
}

void CharStringType2Interpreter::ClearStack()
{
    mOperandStack.clear();
}

uint8_t *CharStringType2Interpreter::InterpretVStem(uint8_t *inProgramCounter)
{
    mStemsCount += (uint16_t)(mOperandStack.size() / 2);

    EStatusCode status = mImplementationHelper->Type2Vstem(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretVMoveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Vmoveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRLineto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Rlineto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHLineto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hlineto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretVLineto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Vlineto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRRCurveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2RRCurveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretCallSubr(uint8_t *inProgramCounter)
{
    CharString *aCharString = nullptr;
    aCharString = mImplementationHelper->GetLocalSubr(mOperandStack.back().IntegerValue);
    mOperandStack.pop_back();

    if (aCharString != nullptr)
    {
        uint8_t *charString = nullptr;
        EStatusCode status =
            mImplementationHelper->ReadCharString(aCharString->mStartPosition, aCharString->mEndPosition, &charString);

        do
        {
            if (status != charta::eSuccess)
            {
                TRACE_LOG2("CharStringType2Interpreter::InterpretCallSubr, failed to read charstring starting in %lld "
                           "and ending in %lld",
                           aCharString->mStartPosition, aCharString->mEndPosition);
                break;
            }

            status = ProcessCharString(charString, aCharString->mEndPosition - aCharString->mStartPosition);
        } while (false);

        delete[] charString;
        if (status != charta::eSuccess)
            return nullptr;
        return inProgramCounter;
    }

    return nullptr;
}

uint8_t *CharStringType2Interpreter::InterpretReturn(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Return(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretEndChar(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Endchar(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    mGotEndChar = true;
    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHStemHM(uint8_t *inProgramCounter)
{
    mStemsCount += (uint16_t)(mOperandStack.size() / 2);

    EStatusCode status = mImplementationHelper->Type2Hstemhm(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHintMask(uint8_t *inProgramCounter)
{
    mStemsCount += (uint16_t)(mOperandStack.size() / 2);

    EStatusCode status = mImplementationHelper->Type2Hintmask(mOperandStack, inProgramCounter);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter + (mStemsCount / 8 + (mStemsCount % 8 != 0 ? 1 : 0));
}

uint8_t *CharStringType2Interpreter::InterpretCntrMask(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Cntrmask(mOperandStack, inProgramCounter);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter + (mStemsCount / 8 + (mStemsCount % 8 != 0 ? 1 : 0));
}

uint8_t *CharStringType2Interpreter::InterpretRMoveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Rmoveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHMoveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hmoveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretVStemHM(uint8_t *inProgramCounter)
{
    mStemsCount += (uint16_t)(mOperandStack.size() / 2);

    EStatusCode status = mImplementationHelper->Type2Vstemhm(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRCurveLine(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Rcurveline(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRLineCurve(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Rlinecurve(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretVVCurveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Vvcurveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHHCurveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hhcurveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretCallGSubr(uint8_t *inProgramCounter)
{
    CharString *aCharString = nullptr;
    aCharString = mImplementationHelper->GetGlobalSubr(mOperandStack.back().IntegerValue);
    mOperandStack.pop_back();

    if (aCharString != nullptr)
    {
        uint8_t *charString = nullptr;
        EStatusCode status =
            mImplementationHelper->ReadCharString(aCharString->mStartPosition, aCharString->mEndPosition, &charString);

        do
        {
            if (status != charta::eSuccess)
            {
                TRACE_LOG2("CharStringType2Interpreter::InterpretCallSubr, failed to read charstring starting in %lld "
                           "and ending in %lld",
                           aCharString->mStartPosition, aCharString->mEndPosition);
                break;
            }

            status = ProcessCharString(charString, aCharString->mEndPosition - aCharString->mStartPosition);
        } while (false);

        delete[] charString;
        if (status != charta::eSuccess)
            return nullptr;
        return inProgramCounter;
    }

    return nullptr;
}

uint8_t *CharStringType2Interpreter::InterpretVHCurveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Vhcurveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHVCurveto(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hvcurveto(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretAnd(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2And(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;
    newOperand.IsInteger = true;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    newOperand.IntegerValue = (((valueB.IsInteger ? valueB.IntegerValue : valueB.RealValue) != 0.0) &&
                               ((valueA.IsInteger ? valueA.IntegerValue : valueA.RealValue) != 0.0))
                                  ? 1
                                  : 0;
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretOr(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Or(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;
    newOperand.IsInteger = true;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    newOperand.IntegerValue = (((valueB.IsInteger ? valueB.IntegerValue : valueB.RealValue) != 0.0) ||
                               ((valueA.IsInteger ? valueA.IntegerValue : valueA.RealValue) != 0.0))
                                  ? 1
                                  : 0;
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretNot(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Not(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;
    CharStringOperand newOperand;
    newOperand.IsInteger = true;

    value = mOperandStack.back();
    mOperandStack.pop_back();

    newOperand.IntegerValue = (value.IsInteger ? value.IntegerValue : value.RealValue) != 0.0 ? 1 : 0;
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretAbs(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Abs(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;
    CharStringOperand newOperand;

    value = mOperandStack.back();
    newOperand.IsInteger = value.IsInteger;
    mOperandStack.pop_back();

    if (value.IsInteger)
        newOperand.IntegerValue = labs(value.IntegerValue);
    else
        newOperand.RealValue = fabs(value.RealValue);
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretAdd(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Add(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    if (!valueA.IsInteger || !valueB.IsInteger)
    {
        newOperand.IsInteger = false;
        newOperand.RealValue = (valueA.IsInteger ? (double)valueA.IntegerValue : valueA.RealValue) +
                               (valueB.IsInteger ? (double)valueB.IntegerValue : valueB.RealValue);
    }
    else
    {
        newOperand.IsInteger = true;
        newOperand.IntegerValue = valueA.IntegerValue + valueB.IntegerValue;
    }
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretSub(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Sub(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    if (!valueA.IsInteger || !valueB.IsInteger)
    {
        newOperand.IsInteger = false;
        newOperand.RealValue = (valueA.IsInteger ? (double)valueA.IntegerValue : valueA.RealValue) -
                               (valueB.IsInteger ? (double)valueB.IntegerValue : valueB.RealValue);
    }
    else
    {
        newOperand.IsInteger = true;
        newOperand.IntegerValue = valueA.IntegerValue - valueB.IntegerValue;
    }
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretDiv(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Div(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    if (!valueA.IsInteger || !valueB.IsInteger)
    {
        newOperand.IsInteger = false;
        newOperand.RealValue = (valueA.IsInteger ? (double)valueA.IntegerValue : valueA.RealValue) /
                               (valueB.IsInteger ? (double)valueB.IntegerValue : valueB.RealValue);
    }
    else
    {
        newOperand.IsInteger = true;
        newOperand.IntegerValue = valueA.IntegerValue / valueB.IntegerValue;
    }
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretNeg(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Neg(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;
    CharStringOperand newOperand;

    value = mOperandStack.back();
    newOperand.IsInteger = value.IsInteger;
    mOperandStack.pop_back();

    if (value.IsInteger)
        newOperand.IntegerValue = -value.IntegerValue;
    else
        newOperand.RealValue = -value.RealValue;
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretEq(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Eq(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    newOperand.IsInteger = true;
    newOperand.IntegerValue = ((valueB.IsInteger ? valueB.IntegerValue : valueB.RealValue) ==
                               (valueA.IsInteger ? valueA.IntegerValue : valueA.RealValue))
                                  ? 1
                                  : 0;
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretDrop(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Drop(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    mOperandStack.pop_back();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretPut(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Put(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    mStorage[(valueB.IsInteger ? valueB.IntegerValue : (long)valueB.RealValue)] = valueA;

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretGet(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Get(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;

    value = mOperandStack.back();
    mOperandStack.pop_back();
    long index = (value.IsInteger ? value.IntegerValue : (long)value.RealValue);

    if ((mOperandStack.size() > (unsigned long)index) && (index >= 0))
    {
        mOperandStack.push_back(mStorage[index]);
        return inProgramCounter;
    }
    return nullptr;
}

uint8_t *CharStringType2Interpreter::InterpretIfelse(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Ifelse(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand valueC;
    CharStringOperand valueD;

    valueD = mOperandStack.back();
    mOperandStack.pop_back();
    valueC = mOperandStack.back();
    mOperandStack.pop_back();
    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    if (!valueC.IsInteger || !valueD.IsInteger)
    {
        if ((valueC.IsInteger ? (double)valueC.IntegerValue : valueC.RealValue) >
            (valueD.IsInteger ? (double)valueD.IntegerValue : valueD.RealValue))
            mOperandStack.push_back(valueB);
        else
            mOperandStack.push_back(valueA);
    }
    else
    {
        if (valueC.IntegerValue > valueD.IntegerValue)
            mOperandStack.push_back(valueB);
        else
            mOperandStack.push_back(valueA);
    }

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRandom(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Random(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand newOperand;

    newOperand.IsInteger = false;
    newOperand.RealValue = ((double)rand() + 1) / ((double)RAND_MAX + 1);

    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretMul(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Mul(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;
    CharStringOperand newOperand;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    if (!valueA.IsInteger || !valueB.IsInteger)
    {
        newOperand.IsInteger = false;
        newOperand.RealValue = (valueA.IsInteger ? (double)valueA.IntegerValue : valueA.RealValue) *
                               (valueB.IsInteger ? (double)valueB.IntegerValue : valueB.RealValue);
    }
    else
    {
        newOperand.IsInteger = true;
        newOperand.IntegerValue = valueA.IntegerValue * valueB.IntegerValue;
    }
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretSqrt(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Sqrt(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;
    CharStringOperand newOperand;

    value = mOperandStack.back();
    mOperandStack.pop_back();

    newOperand.IsInteger = false;
    newOperand.RealValue = sqrt(value.IsInteger ? value.IntegerValue : value.RealValue);
    mOperandStack.push_back(newOperand);
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretDup(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Dup(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    mOperandStack.push_back(mOperandStack.back());
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretExch(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Exch(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    mOperandStack.push_back(valueB);
    mOperandStack.push_back(valueA);

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretIndex(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Index(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand value;

    value = mOperandStack.back();
    mOperandStack.pop_back();
    long index = (value.IsInteger ? value.IntegerValue : (long)value.RealValue);
    auto it = mOperandStack.rbegin();

    while (index > 0)
        ++it;
    mOperandStack.push_back(*it);

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretRoll(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Roll(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    CharStringOperand valueA;
    CharStringOperand valueB;

    valueB = mOperandStack.back();
    mOperandStack.pop_back();
    valueA = mOperandStack.back();
    mOperandStack.pop_back();

    long shiftAmount = (valueB.IsInteger ? valueB.IntegerValue : (long)valueB.RealValue);
    long itemsCount = (valueA.IsInteger ? valueA.IntegerValue : (long)valueA.RealValue);

    CharStringOperandList groupToShift;

    for (long i = 0; i < itemsCount; ++i)
    {
        groupToShift.push_front(mOperandStack.back());
        mOperandStack.pop_back();
    }

    if (shiftAmount > 0)
    {
        for (long j = 0; j < shiftAmount; ++j)
        {
            groupToShift.push_front(groupToShift.back());
            groupToShift.pop_back();
        }
    }
    else
    {
        for (long j = 0; j < -shiftAmount; ++j)
        {
            groupToShift.push_back(groupToShift.front());
            groupToShift.pop_front();
        }
    }

    for (long i = 0; i < itemsCount; ++i)
    {
        mOperandStack.push_back(mOperandStack.front());
        mOperandStack.pop_front();
    }

    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHFlex(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hflex(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretFlex(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Flex(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretHFlex1(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Hflex1(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}

uint8_t *CharStringType2Interpreter::InterpretFlex1(uint8_t *inProgramCounter)
{
    EStatusCode status = mImplementationHelper->Type2Flex1(mOperandStack);
    if (status != charta::eSuccess)
        return nullptr;

    ClearStack();
    return inProgramCounter;
}