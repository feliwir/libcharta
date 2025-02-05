/*
   Source File : InputBufferedStream.h


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
#include "IByteReader.h"

#include <memory>
namespace charta
{
class PDFArray;
}
class PDFParser;

class ArrayOfInputStreamsStream : public charta::IByteReader
{
  public:
    ArrayOfInputStreamsStream(std::shared_ptr<charta::PDFArray> inArrayOfStreams, PDFParser *inParser);
    virtual ~ArrayOfInputStreamsStream(void);

    // IByteReader implementation
    virtual size_t Read(uint8_t *inBuffer, size_t inBufferSize);
    virtual bool NotEnded();

  private:
    charta::IByteReader *GetActiveStream();

    charta::IByteReader *mCurrentStream;
    PDFParser *mParser;
    std::shared_ptr<charta::PDFArray> mArray;
    unsigned long mCurrentIndex;
};
