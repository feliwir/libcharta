/*
   Source File : Type2CharStringWriter.cpp


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
#include "text/type1/Type2CharStringWriter.h"
#include "io/IByteWriter.h"

using namespace charta;

Type2CharStringWriter::Type2CharStringWriter(charta::IByteWriter *inTargetStream)
{
    mTargetStream = inTargetStream;
}

void Type2CharStringWriter::Assign(charta::IByteWriter *inTargetStream)
{
    mTargetStream = inTargetStream;
}

EStatusCode Type2CharStringWriter::WriteHintMask(unsigned long inMask, unsigned long inMaskSize)
{
    unsigned long maskByteSize = inMaskSize / 8 + (inMaskSize % 8 != 0 ? 1 : 0);

    EStatusCode status = WriteOperator(19);
    if (status != charta::eSuccess)
        return status;

    return WriteMaskBytes(inMask, maskByteSize);
}

EStatusCode Type2CharStringWriter::WriteMaskBytes(unsigned long inMask, unsigned long inMaskByteSize)
{
    EStatusCode status;

    if (inMaskByteSize > 1)
    {
        status = WriteMaskBytes(inMask >> 1, inMaskByteSize - 1);
        if (status != charta::eSuccess)
            return status;
    }
    return WriteByte((uint8_t)(inMask & 0xff));
}

EStatusCode Type2CharStringWriter::WriteByte(uint8_t inValue)
{
    return (mTargetStream->Write(&inValue, 1) == 1 ? charta::eSuccess : charta::eFailure);
}

EStatusCode Type2CharStringWriter::WriteIntegerOperand(long inOperand)
{
    long value = inOperand;

    if (-107 <= value && value <= 107)
    {
        return WriteByte((uint8_t)(value + 139));
    }
    if (108 <= value && value <= 1131)
    {
        uint8_t byte0, byte1;

        value -= 108;
        byte0 = ((value >> 8) & 0xff) + 247;
        byte1 = value & 0xff;

        if (WriteByte(byte0) != charta::eSuccess)
            return charta::eFailure;

        if (WriteByte(byte1) != charta::eSuccess)
            return charta::eFailure;
        return charta::eSuccess;
    }
    if (-1131 <= value && value <= -108)
    {
        uint8_t byte0, byte1;

        value = -(value + 108);

        byte0 = ((value >> 8) & 0xff) + 251;
        byte1 = value & 0xff;

        if (WriteByte(byte0) != charta::eSuccess)
            return charta::eFailure;

        if (WriteByte(byte1) != charta::eSuccess)
            return charta::eFailure;
        return charta::eSuccess;
    }
    if (-32768 <= value && value <= 32767)
    {
        uint8_t byte1, byte2;

        byte1 = (value >> 8) & 0xff;
        byte2 = value & 0xff;

        if (WriteByte(28) != charta::eSuccess)
            return charta::eFailure;

        if (WriteByte(byte1) != charta::eSuccess)
            return charta::eFailure;

        if (WriteByte(byte2) != charta::eSuccess)
            return charta::eFailure;
        return charta::eSuccess;
    }
    return charta::eFailure;
}

EStatusCode Type2CharStringWriter::WriteOperator(uint16_t inOperatorCode)
{
    if ((inOperatorCode & 0xff00) == 0x0c00)
    {
        if (WriteByte(0x0c) != charta::eSuccess)
            return charta::eFailure;
    }
    return WriteByte(uint8_t(inOperatorCode & 0x00ff));
}
