/*
   Source File : IndirectObjectsReferenceRegistry.h


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
/*
    IndirectObjectsReferenceRegistry does two jobs:
    1. It maintains the reference list for all indirect objects (initially just their total count), and allowing to get
   a new object number
    2. It maintains file writing information, such as whether the object was written and if so at what position
*/

#include "EStatusCode.h"
#include "ObjectsBasicTypes.h"
#include <stdint.h>
#include <stdio.h>
#include <utility>
#include <vector>

class ObjectsContext;
class PDFParser;

struct ObjectWriteInformation
{
    // although there's no "update" method in this project (being a single flat write), the free marker is used
    // for the first element in the registry.
    enum EObjectReferenceType
    {
        Free,
        Used
    };

    // is object already written to file (when in incremental changes, it includes previous document versions as well)
    bool mObjectWritten;
    // has anything changed (true for initial file writing. when in incremental changes, dependent on whether something
    // was done to this object)
    bool mIsDirty;
    // value is undefined if mObjectWritten is false or if free
    long long mWritePosition;
    // free or used object
    EObjectReferenceType mObjectReferenceType;
    // object generation number
    unsigned long mGenerationNumber;
};

typedef std::pair<bool, ObjectWriteInformation> GetObjectWriteInformationResult;
typedef std::vector<ObjectWriteInformation> ObjectWriteInformationVector;

class IndirectObjectsReferenceRegistry
{
  public:
    IndirectObjectsReferenceRegistry(void);
    ~IndirectObjectsReferenceRegistry(void);

    ObjectIDType AllocateNewObjectID();

    charta::EStatusCode MarkObjectAsWritten(ObjectIDType inObjectID, long long inWritePosition);
    GetObjectWriteInformationResult GetObjectWriteInformation(ObjectIDType inObjectID) const;

    ObjectIDType GetObjectsCount() const;
    // should be used with safe object IDs. use GetObjectsCount to verify the maximum ID
    const ObjectWriteInformation &GetNthObjectReference(ObjectIDType inObjectID) const;

    // modified PDF methods
    charta::EStatusCode DeleteObject(ObjectIDType inObjectID);
    charta::EStatusCode MarkObjectAsUpdated(ObjectIDType inObjectID, long long inNewWritePosition);

    charta::EStatusCode WriteState(ObjectsContext *inStateWriter, ObjectIDType inObjectID);
    charta::EStatusCode ReadState(PDFParser *inStateReader, ObjectIDType inObjectID);

    void Reset();

    void SetupXrefFromModifiedFile(PDFParser *inModifiedFileParser);

  private:
    ObjectWriteInformationVector mObjectsWritesRegistry;

    void SetupInitialFreeObject();
    void AppendExistingItem(ObjectWriteInformation::EObjectReferenceType inObjectReferenceType,
                            unsigned long inGenerationNumber, long long inWritePosition);
};
