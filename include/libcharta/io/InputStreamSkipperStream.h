/*
   Source File : InputStreamSkipperStream.h


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

#include "IByteReader.h"
#include "IReadPositionProvider.h"

namespace charta
{
/*
    This class is to help with decoder/encoder class, that don't really have "SetPosition" kind of capabilities,
    but sometimes you just want to skip. It does skip by recording the amount already read. then, if possible, reads to
   the skipped to position
*/

class InputStreamSkipperStream final : public IByteReader, public IReadPositionProvider
{
  public:
    InputStreamSkipperStream();

    // gets ownership, so if you wanna untie it, make sure to call Assign(NULL), before finishing
    InputStreamSkipperStream(IByteReader *inSourceStream);
    virtual ~InputStreamSkipperStream(void);

    void Assign(IByteReader *inSourceStream);

    // IByteReader implementation
    virtual size_t Read(uint8_t *inBuffer, size_t inBufferSize);
    virtual bool NotEnded();

    // IReadPositionProvider implementation
    virtual long long GetCurrentPosition();

    // here's the interesting part.
    bool CanSkipTo(long long inPositionInStream) const;

    // will skip if can, which is either if not passed position (check with CanSkipTo), or if while skipping will hit
    // EOF
    void SkipTo(long long inPositionInStream);

    // will skip by, or hit EOF
    void SkipBy(long long inAmountToSkipBy);

    // use this after resetting the input stream to an initial position, to try skipping to the position once more
    // this will reset the read count
    void Reset();

  private:
    IByteReader *mStream;
    long long mAmountRead;
};
} // namespace charta