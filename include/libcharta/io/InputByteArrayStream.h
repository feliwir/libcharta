/*
   Source File : InputByteArrayStream.h


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

#include "IByteReaderWithPosition.h"

class InputByteArrayStream final : public charta::IByteReaderWithPosition
{
  public:
    InputByteArrayStream();
    InputByteArrayStream(uint8_t *inByteArray, long long inArrayLength);
    ~InputByteArrayStream(void);

    void Assign(uint8_t *inByteArray, long long inArrayLength);

    // IByteReaderWithPosition implementation
    virtual size_t Read(uint8_t *inBuffer, size_t inBufferSize);
    virtual bool NotEnded();
    virtual void Skip(size_t inSkipSize);
    virtual void SetPosition(long long inOffsetFromStart);
    virtual void SetPositionFromEnd(long long inOffsetFromEnd);
    virtual long long GetCurrentPosition();

  private:
    uint8_t *mByteArray;
    long long mArrayLength;
    long long mCurrentPosition;
};
