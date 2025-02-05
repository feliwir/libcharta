/*
   Source File : InputFlateDecodeStream.cpp


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
#include "io/InputFlateDecodeStream.h"

#include "Trace.h"
#include <zlib.h>

charta::InputFlateDecodeStream::InputFlateDecodeStream()
{
    mZLibState = new z_stream;
    mSourceStream = nullptr;
    mCurrentlyEncoding = false;
    mEndOfCompressionEoncountered = false;
}

charta::InputFlateDecodeStream::~InputFlateDecodeStream()
{
    if (mCurrentlyEncoding)
        FinalizeEncoding();
    if (mSourceStream != nullptr)
        delete mSourceStream;
    delete mZLibState;
}

void charta::InputFlateDecodeStream::FinalizeEncoding()
{
    // no need for flushing here, there's no notion of Z_FINISH. so just end the library work
    inflateEnd(mZLibState);
    mCurrentlyEncoding = false;
}

charta::InputFlateDecodeStream::InputFlateDecodeStream(IByteReader *inSourceReader)
{
    mZLibState = new z_stream;
    mSourceStream = nullptr;
    mCurrentlyEncoding = false;

    Assign(inSourceReader);
}

void charta::InputFlateDecodeStream::Assign(IByteReader *inSourceReader)
{
    mSourceStream = inSourceReader;
    if (mSourceStream != nullptr)
        StartEncoding();
}

void charta::InputFlateDecodeStream::StartEncoding()
{
    mZLibState->zalloc = Z_NULL;
    mZLibState->zfree = Z_NULL;
    mZLibState->opaque = Z_NULL;
    mZLibState->avail_in = 0;
    mZLibState->next_in = Z_NULL;
    mEndOfCompressionEoncountered = false;

    int inflateStatus = inflateInit(mZLibState);
    if (inflateStatus != Z_OK)
        TRACE_LOG1(
            "charta::InputFlateDecodeStream::StartEncoding, Unexpected failure in initializating flate library. status "
            "code = %d",
            inflateStatus);
    else
        mCurrentlyEncoding = true;
}

static bool isError(int inflateResult)
{
    switch (inflateResult)
    {
    case Z_STREAM_ERROR:
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        return true;
    default:
        return false;
    }
}

size_t charta::InputFlateDecodeStream::Read(uint8_t *inBuffer, size_t inBufferSize)
{
    if (mCurrentlyEncoding)
        return DecodeBufferAndRead(inBuffer, inBufferSize);
    if (mSourceStream != nullptr)
        return mSourceStream->Read(inBuffer, inBufferSize);
    return 0;
}

size_t charta::InputFlateDecodeStream::DecodeBufferAndRead(const uint8_t *inBuffer, size_t inSize)
{
    if (0 == inSize)
        return 0; // inflate kinda touchy about getting 0 lengths

    int inflateResult = Z_OK;

    do
    {
        mZLibState->avail_out = (uInt)inSize;
        mZLibState->next_out = (Bytef *)inBuffer;

        // first, flush whatever is already available
        while (mZLibState->avail_in != 0 && mZLibState->avail_out != 0)
        {
            inflateResult = inflate(mZLibState, Z_NO_FLUSH);
            if (isError(inflateResult))
            {
                TRACE_LOG1(
                    "charta::InputFlateDecodeStream::DecodeBufferAndRead, failed to read zlib information. returned "
                    "error code = %d",
                    inflateResult);
                inflateEnd(mZLibState);
                break;
            }
        }
        if (Z_OK != inflateResult && Z_STREAM_END != inflateResult)
            break;

        // if not finished read buffer, repeatedly read from input stream, and inflate till done
        while ((0 < mZLibState->avail_out) && mSourceStream->NotEnded())
        {
            if (mSourceStream->Read(&mBuffer, 1) != 1)
            {
                if (mSourceStream->NotEnded())
                {
                    TRACE_LOG("charta::InputFlateDecodeStream::DecodeBufferAndRead, failed to read from source stream");
                    inflateResult = Z_STREAM_ERROR;
                }
                else
                {
                    // input stream finished, but couldn't tell in advance
                    inflateResult = Z_STREAM_END;
                }
                inflateEnd(mZLibState);
                mCurrentlyEncoding = false;
                break;
            }

            mZLibState->avail_in = 1;
            mZLibState->next_in = (Bytef *)&mBuffer;

            while (mZLibState->avail_in != 0 && mZLibState->avail_out != 0 && inflateResult != Z_STREAM_END)
            {
                inflateResult = inflate(mZLibState, Z_NO_FLUSH);
                if (isError(inflateResult))
                {
                    TRACE_LOG1("charta::InputFlateDecodeStream::DecodeBufferAndRead, failed to read zlib information. "
                               "returned "
                               "error code = %d",
                               inflateResult);
                    inflateEnd(mZLibState);
                    break;
                }
            }
            if (Z_OK != inflateResult && Z_STREAM_END != inflateResult)
                break;
        }
    } while (false);

    // should be that at the last buffer we'll get here a nice Z_STREAM_END
    mEndOfCompressionEoncountered = (Z_STREAM_END == inflateResult) || isError(inflateResult);
    if (Z_OK == inflateResult || Z_STREAM_END == inflateResult)
        return inSize - mZLibState->avail_out;
    return 0;
}

bool charta::InputFlateDecodeStream::NotEnded()
{
    if (mSourceStream != nullptr)
        return (mSourceStream->NotEnded() || mZLibState->avail_in != 0) && !mEndOfCompressionEoncountered;
    return mZLibState->avail_in != 0 && mEndOfCompressionEoncountered;
}