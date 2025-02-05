/*
   Source File : InputCharStringDecodeStream.cpp


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
#include "io/InputCharStringDecodeStream.h"

using namespace charta;

InputCharStringDecodeStream::InputCharStringDecodeStream(charta::IByteReader *inReadFrom, unsigned long inLenIV)
{
    Assign(inReadFrom, inLenIV);
}

InputCharStringDecodeStream::~InputCharStringDecodeStream() = default;

void InputCharStringDecodeStream::Assign(charta::IByteReader *inReadFrom, unsigned long inLenIV)
{
    mReadFrom = inReadFrom;
    InitializeCharStringDecode(inLenIV);
}

static const int CONSTANT_1 = 52845;
static const int CONSTANT_2 = 22719;
static const int RANDOMIZER_INIT = 4330;
static const int RANDOMIZER_MODULU_VAL = 65536;

void InputCharStringDecodeStream::InitializeCharStringDecode(unsigned long inLenIV)
{
    uint8_t dummyByte;

    mRandomizer = RANDOMIZER_INIT;

    for (unsigned long i = 0; i < inLenIV; ++i)
        ReadDecodedByte(dummyByte);
}

EStatusCode InputCharStringDecodeStream::ReadDecodedByte(uint8_t &outByte)
{
    uint8_t buffer;

    if (mReadFrom->Read(&buffer, 1) != 1)
        return charta::eFailure;

    outByte = DecodeByte(buffer);
    return charta::eSuccess;
}

uint8_t InputCharStringDecodeStream::DecodeByte(uint8_t inByteToDecode)
{
    auto result = (uint8_t)(inByteToDecode ^ (mRandomizer >> 8));
    mRandomizer = (uint16_t)(((inByteToDecode + mRandomizer) * CONSTANT_1 + CONSTANT_2) % RANDOMIZER_MODULU_VAL);
    return result;
}

size_t InputCharStringDecodeStream::Read(uint8_t *inBuffer, size_t inBufferSize)
{
    size_t bufferIndex = 0;
    EStatusCode status = charta::eSuccess;

    while (NotEnded() && inBufferSize > bufferIndex && charta::eSuccess == status)
    {
        status = ReadDecodedByte(inBuffer[bufferIndex]);
        ++bufferIndex;
    }
    return bufferIndex;
}

bool InputCharStringDecodeStream::NotEnded()
{
    return mReadFrom->NotEnded();
}
