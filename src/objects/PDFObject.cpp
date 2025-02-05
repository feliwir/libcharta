/*
   Source File : PDFObject.cpp


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
#include "objects/PDFObject.h"
#include "objects/IDeletable.h"

const char *charta::PDFObject::scPDFObjectTypeLabel(int index)
{
    static const char *labels[] = {"Boolean", "LiteralString", "HexString", "Null",       "Name",
                                   "Integer", "Real",          "Array",     "Dictionary", "IndirectObjectReference",
                                   "Stream",  "Symbol"};
    return labels[index];
};

charta::PDFObject::PDFObject(EPDFObjectType inType)
{
    mType = inType;
}

charta::PDFObject::PDFObject(int inType)
{
    mType = (EPDFObjectType)inType;
}

charta::PDFObject::~PDFObject()
{
    auto it = mMetadata.begin();
    for (; it != mMetadata.end(); ++it)
    {
        it->second->DeleteMe();
    }
    mMetadata.clear();
}

charta::PDFObject::EPDFObjectType charta::PDFObject::GetType()
{
    return mType;
}

void charta::PDFObject::SetMetadata(const std::string &inKey, charta::IDeletable *inValue)
{
    // delete old metadata
    DeleteMetadata(inKey);

    mMetadata.insert(StringToIDeletable::value_type(inKey, inValue));
}

charta::IDeletable *charta::PDFObject::GetMetadata(const std::string &inKey)
{
    auto it = mMetadata.find(inKey);

    if (it == mMetadata.end())
        return nullptr;
    return it->second;
}

charta::IDeletable *charta::PDFObject::DetachMetadata(const std::string &inKey)
{
    auto it = mMetadata.find(inKey);

    if (it == mMetadata.end())
        return nullptr;

    charta::IDeletable *result = it->second;
    mMetadata.erase(it);
    return result;
}

void charta::PDFObject::DeleteMetadata(const std::string &inKey)
{
    charta::IDeletable *result = DetachMetadata(inKey);
    if (result != nullptr)
        result->DeleteMe();
}
