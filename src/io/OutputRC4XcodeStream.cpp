/*
Source File : InputRC4XcodeStream.cpp


Copyright 2016 Gal Kahana PDFWriter

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

#include "io/OutputRC4XcodeStream.h"

charta::OutputRC4XcodeStream::OutputRC4XcodeStream()
{
    mTargetStream = nullptr;
    mOwnsStream = false;
}

charta::OutputRC4XcodeStream::~OutputRC4XcodeStream()
{
    if (mOwnsStream)
        delete mTargetStream;
}

charta::OutputRC4XcodeStream::OutputRC4XcodeStream(IByteWriterWithPosition *inTargetStream,
                                                   const ByteList &inEncryptionKey, bool inOwnsStream)
    : mRC4(inEncryptionKey)
{
    mTargetStream = inTargetStream;
    mOwnsStream = inOwnsStream;
}

size_t charta::OutputRC4XcodeStream::Write(const uint8_t *inBuffer, size_t inSize)
{
    if (mTargetStream == nullptr)
        return 0;

    size_t mCurrentIndex = 0;
    uint8_t buffer;

    while (mCurrentIndex < inSize)
    {
        buffer = mRC4.DecodeNextByte(inBuffer[mCurrentIndex]);
        mTargetStream->Write(&buffer, 1);
        ++mCurrentIndex;
    }

    return mCurrentIndex;
}

long long charta::OutputRC4XcodeStream::GetCurrentPosition()
{
    if (mTargetStream == nullptr)
        return 0;

    return mTargetStream->GetCurrentPosition();
}
