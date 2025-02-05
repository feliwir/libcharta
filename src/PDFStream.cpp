/*
   Source File : PDFStream.cpp


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
#include "PDFStream.h"
#include "IObjectsContextExtender.h"
#include "encryption/EncryptionHelper.h"
#include "io/InputStringBufferStream.h"
#include "io/OutputStreamTraits.h"

PDFStream::PDFStream(bool inCompressStream, charta::IByteWriterWithPosition *inOutputStream,
                     EncryptionHelper *inEncryptionHelper, ObjectIDType inExtentObjectID,
                     IObjectsContextExtender *inObjectsContextExtender)
{
    mExtender = inObjectsContextExtender;
    mCompressStream = inCompressStream;
    mExtendObjectID = inExtentObjectID;
    mStreamStartPosition = inOutputStream->GetCurrentPosition();
    mOutputStream = inOutputStream;
    if ((inEncryptionHelper != nullptr) && inEncryptionHelper->IsEncrypting())
    {
        mEncryptionStream = inEncryptionHelper->CreateEncryptionStream(inOutputStream);
    }
    else
    {
        mEncryptionStream = nullptr;
    }

    mStreamLength = 0;
    mStreamDictionaryContextForDirectExtentStream = nullptr;

    if (mCompressStream)
    {
        if ((mExtender != nullptr) && mExtender->OverridesStreamCompression())
        {
            mWriteStream =
                mExtender->GetCompressionWriteStream(mEncryptionStream != nullptr ? mEncryptionStream : inOutputStream);
        }
        else
        {
            mFlateEncodingStream.Assign(mEncryptionStream != nullptr ? mEncryptionStream : inOutputStream);
            mWriteStream = &mFlateEncodingStream;
        }
    }
    else
        mWriteStream = mEncryptionStream != nullptr ? mEncryptionStream : inOutputStream;
}

PDFStream::PDFStream(bool inCompressStream, charta::IByteWriterWithPosition *inOutputStream,
                     EncryptionHelper *inEncryptionHelper,
                     DictionaryContext *inStreamDictionaryContextForDirectExtentStream,
                     IObjectsContextExtender *inObjectsContextExtender)
{
    mExtender = inObjectsContextExtender;
    mCompressStream = inCompressStream;
    mExtendObjectID = 0;
    mStreamStartPosition = 0;
    mOutputStream = inOutputStream;
    mStreamLength = 0;
    mStreamDictionaryContextForDirectExtentStream = inStreamDictionaryContextForDirectExtentStream;

    mTemporaryOutputStream.Assign(&mTemporaryStream);
    if ((inEncryptionHelper != nullptr) && inEncryptionHelper->IsEncrypting())
    {
        mEncryptionStream = inEncryptionHelper->CreateEncryptionStream(&mTemporaryOutputStream);
    }
    else
    {
        mEncryptionStream = nullptr;
    }

    if (mCompressStream)
    {
        if ((mExtender != nullptr) && mExtender->OverridesStreamCompression())
        {
            mWriteStream = mExtender->GetCompressionWriteStream(mEncryptionStream != nullptr ? mEncryptionStream
                                                                                             : &mTemporaryOutputStream);
        }
        else
        {
            mFlateEncodingStream.Assign(mEncryptionStream != nullptr ? mEncryptionStream : &mTemporaryOutputStream);
            mWriteStream = &mFlateEncodingStream;
        }
    }
    else
        mWriteStream = mEncryptionStream != nullptr ? mEncryptionStream : &mTemporaryOutputStream;
}

PDFStream::~PDFStream() = default;

charta::IByteWriter *PDFStream::GetWriteStream()
{
    return mWriteStream;
}

void PDFStream::FinalizeStreamWrite()
{
    if ((mExtender != nullptr) && mExtender->OverridesStreamCompression() && mCompressStream)
        mExtender->FinalizeCompressedStreamWrite(mWriteStream);
    mWriteStream = nullptr;
    if (mCompressStream)
        mFlateEncodingStream.Assign(
            nullptr); // this both finished encoding any left buffers and releases ownership from mFlateEncodingStream

    if (mEncryptionStream != nullptr)
    {
        // safe to delete. encryption stream is not supposed to own the underlying stream in any case. make sure
        // to delete before measuring output, as flushing may occur at this point
        delete mEncryptionStream;
        mEncryptionStream = nullptr;
    }

    // different endings, depending if direct stream writing or not
    if (mExtendObjectID == 0)
    {
        mStreamLength = mTemporaryStream.GetCurrentWritePosition();
    }
    else
    {
        mStreamLength = mOutputStream->GetCurrentPosition() - mStreamStartPosition;
        mOutputStream = nullptr;
    }
}

long long PDFStream::GetLength() const
{
    return mStreamLength;
}

bool PDFStream::IsStreamCompressed() const
{
    return mCompressStream;
}

ObjectIDType PDFStream::GetExtentObjectID() const
{
    return mExtendObjectID;
}

DictionaryContext *PDFStream::GetStreamDictionaryForDirectExtentStream()
{
    return mStreamDictionaryContextForDirectExtentStream;
}

void PDFStream::FlushStreamContentForDirectExtentStream()
{
    mTemporaryStream.pubseekoff(0, std::ios_base::beg);

    // copy internal temporary stream to output
    charta::InputStringBufferStream inputStreamForWrite(&mTemporaryStream);
    charta::OutputStreamTraits streamCopier(mOutputStream);
    streamCopier.CopyToOutputStream(&inputStreamForWrite);

    mTemporaryStream.str();
    mOutputStream = nullptr;
}
