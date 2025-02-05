/*
 Source File : PDFPageMergingHelper.h


 Copyright 2012 Gal Kahana PDFWriter

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

#include <memory>
#include <string>

class PDFPage;
namespace charta
{
class IByteReaderWithPosition;
class PDFDocumentCopyingContext;
}; // namespace charta
class PDFWriter;

class PDFPageMergingHelper
{
  public:
    PDFPageMergingHelper(PDFPage *inPage);
    virtual ~PDFPageMergingHelper(void);

    charta::EStatusCode MergePageContent(std::shared_ptr<charta::PDFDocumentCopyingContext> inCopyingContext,
                                         unsigned long inPageIndex);
    charta::EStatusCode MergePageContent(PDFWriter *inWriter, const std::string &inPDFFilePath,
                                         unsigned long inPageIndex);
    charta::EStatusCode MergePageContent(PDFWriter *inWriter, charta::IByteReaderWithPosition *inPDFStream,
                                         unsigned long inPageIndex);

  private:
    PDFPage *mPage;
};
