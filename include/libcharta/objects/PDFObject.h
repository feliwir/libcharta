/*
   Source File : PDFObject.h


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

#include <map>
#include <string>

namespace charta
{
class IDeletable;
using StringToIDeletable = std::map<std::string, charta::IDeletable *>;

class PDFObject
{
  public:
    enum EPDFObjectType
    {
        ePDFObjectBoolean,
        ePDFObjectLiteralString,
        ePDFObjectHexString,
        ePDFObjectNull,
        ePDFObjectName,
        ePDFObjectInteger,
        ePDFObjectReal,
        ePDFObjectArray,
        ePDFObjectDictionary,
        ePDFObjectIndirectObjectReference,
        ePDFObjectStream,
        ePDFObjectSymbol // symbol is parallel to unkown. mostly be a keyword, or simply a mistake in the file
    };

    PDFObject(EPDFObjectType inType);
    PDFObject(int inType);
    virtual ~PDFObject(void);

    EPDFObjectType GetType();

    static const char *scPDFObjectTypeLabel(int index);

    /*
        metadata will automatically be deleted when object is released
    */
    void SetMetadata(const std::string &inKey,
                     charta::IDeletable *inValue); // will automatically delete old data in the same key
    charta::IDeletable *GetMetadata(const std::string &inKey);
    // Detach will only remove the pointer from metadata map, Delete will also delete the inValue pointer
    charta::IDeletable *DetachMetadata(const std::string &inKey);
    void DeleteMetadata(const std::string &inKey);

  private:
    EPDFObjectType mType;
    StringToIDeletable mMetadata;
};
} // namespace charta