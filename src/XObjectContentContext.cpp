/*
   Source File : XObjectContentContext.cpp


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
#include "XObjectContentContext.h"
#include "DocumentContext.h"
#include "IFormEndWritingTask.h"
#include "PDFFormXObject.h"

using namespace charta;

XObjectContentContext::XObjectContentContext(charta::DocumentContext *inDocumentContext, PDFFormXObject *inFormXObject)
    : AbstractContentContext(inDocumentContext)
{
    mPDFFormXObjectOfContext = inFormXObject;
    SetPDFStreamForWrite(inFormXObject->GetContentStream());
}

XObjectContentContext::~XObjectContentContext() = default;

ResourcesDictionary *XObjectContentContext::GetResourcesDictionary()
{
    return &(mPDFFormXObjectOfContext->GetResourcesDictionary());
}

class FormImageWritingTask : public IFormEndWritingTask
{
  public:
    FormImageWritingTask(const std::string &inImagePath, unsigned long inImageIndex, ObjectIDType inObjectID,
                         const PDFParsingOptions &inPDFParsingOptions)
    {
        mImagePath = inImagePath;
        mImageIndex = inImageIndex;
        mObjectID = inObjectID;
        mPDFParsingOptions = inPDFParsingOptions;
    }

    ~FormImageWritingTask() override = default;

    charta::EStatusCode Write(PDFFormXObject * /*inFormXObject*/, ObjectsContext * /*inObjectsContext*/,
                              charta::DocumentContext *inDocumentContext) override
    {
        return inDocumentContext->WriteFormForImage(mImagePath, mImageIndex, mObjectID, mPDFParsingOptions);
    }

  private:
    std::string mImagePath;
    unsigned long mImageIndex;
    ObjectIDType mObjectID;
    PDFParsingOptions mPDFParsingOptions;
};

void XObjectContentContext::ScheduleImageWrite(const std::string &inImagePath, unsigned long inImageIndex,
                                               ObjectIDType inObjectID, const PDFParsingOptions &inParsingOptions)
{
    mDocumentContext->RegisterFormEndWritingTask(
        mPDFFormXObjectOfContext, new FormImageWritingTask(inImagePath, inImageIndex, inObjectID, inParsingOptions));
}