/*
Source File : EncryptionHelper.cpp


Copyright 2016 Gal Kahana PDFWriter

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

#include "encryption/EncryptionHelper.h"
#include "DictionaryContext.h"
#include "ObjectsContext.h"
#include "encryption/DecryptionHelper.h"
#include "io/InputStringStream.h"
#include "io/OutputAESEncodeStream.h"
#include "io/OutputRC4XcodeStream.h"
#include "io/OutputStreamTraits.h"
#include "io/OutputStringBufferStream.h"
#include "objects/PDFBoolean.h"
#include "objects/PDFDictionary.h"
#include "objects/PDFInteger.h"
#include "objects/PDFLiteralString.h"
#include "objects/PDFObjectCast.h"
#include "parsing/PDFParser.h"

#include <stdint.h>

using namespace charta;
using namespace std;

EncryptionHelper::EncryptionHelper()
{
    mIsDocumentEncrypted = false;
    mEncryptionPauseLevel = 0;
    mSupportsEncryption = true;
    mXcryptAuthentication = nullptr;
    mXcryptStreams = nullptr;
    mXcryptStrings = nullptr;
}

EncryptionHelper::~EncryptionHelper()
{
    Release();
}

void EncryptionHelper::Release()
{
    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
        delete it->second;
    mXcrypts.clear();
}

bool EncryptionHelper::SupportsEncryption() const
{
    return mSupportsEncryption;
}

bool EncryptionHelper::IsDocumentEncrypted() const
{
    return mIsDocumentEncrypted;
}

bool EncryptionHelper::IsEncrypting() const
{
    return IsDocumentEncrypted() && mEncryptionPauseLevel == 0;
}

void EncryptionHelper::PauseEncryption()
{
    ++mEncryptionPauseLevel;
}

void EncryptionHelper::ReleaseEncryption()
{
    --mEncryptionPauseLevel;
}

void EncryptionHelper::OnObjectStart(long long inObjectID, long long inGenerationNumber)
{
    if (!IsEncrypting())
        return;

    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
    {
        it->second->OnObjectStart(inObjectID, inGenerationNumber);
    }
}
void EncryptionHelper::OnObjectEnd()
{
    if (!IsEncrypting())
        return;

    auto it = mXcrypts.begin();
    for (; it != mXcrypts.end(); ++it)
    {
        it->second->OnObjectEnd();
    }
}

std::string EncryptionHelper::EncryptString(const std::string &inStringToEncrypt)
{
    if (!IsEncrypting() || (mXcryptStrings == nullptr))
        return inStringToEncrypt;

    OutputStringBufferStream buffer;

    charta::IByteWriterWithPosition *encryptStream =
        CreateEncryptionWriter(&buffer, mXcryptStrings->GetCurrentObjectKey(), mXcryptStrings->IsUsingAES());
    if (encryptStream != nullptr)
    {
        InputStringStream inputStream(inStringToEncrypt);
        OutputStreamTraits traits(encryptStream);
        traits.CopyToOutputStream(&inputStream);
        delete encryptStream; // free encryption stream (sometimes it will also mean flushing the output stream)

        return buffer.ToString();
    }
    return inStringToEncrypt;
}

charta::IByteWriterWithPosition *EncryptionHelper::CreateEncryptionStream(
    charta::IByteWriterWithPosition *inToWrapStream)
{
    return CreateEncryptionWriter(inToWrapStream, mXcryptStreams->GetCurrentObjectKey(), mXcryptStreams->IsUsingAES());
}

charta::IByteWriterWithPosition *EncryptionHelper::CreateEncryptionWriter(
    charta::IByteWriterWithPosition *inToWrapStream, const ByteList &inEncryptionKey, bool inIsUsingAES)
{
    if (inIsUsingAES)
    {
        return new OutputAESEncodeStream(inToWrapStream, inEncryptionKey, false);
    }

    return new OutputRC4XcodeStream(inToWrapStream, inEncryptionKey, false);
}

static const string scFilter = "Filter";
static const string scStandard = "Standard";
static const string scV = "V";
static const string scLength = "Length";
static const string scR = "R";
static const string scO = "O";
static const string scU = "U";
static const string scP = "P";
static const string scEncryptMetadata = "EncryptMetadata";

EStatusCode EncryptionHelper::WriteEncryptionDictionary(ObjectsContext *inObjectsContext)
{
    if (!IsDocumentEncrypted()) // document not encrypted, nothing to write. unexpected
        return eFailure;

    PauseEncryption();
    DictionaryContext *encryptContext = inObjectsContext->StartDictionary();

    // Filter
    encryptContext->WriteKey(scFilter);
    encryptContext->WriteNameValue(scStandard);

    // V
    encryptContext->WriteKey(scV);
    encryptContext->WriteIntegerValue(mV);

    // Length (if not 40. this would allow simple non usage control)
    if (mLength != 5)
    {
        encryptContext->WriteKey(scLength);
        encryptContext->WriteIntegerValue(mLength * 8);
    }

    // R
    encryptContext->WriteKey(scR);
    encryptContext->WriteIntegerValue(mRevision);

    // O
    encryptContext->WriteKey(scO);
    encryptContext->WriteHexStringValue(XCryptionCommon::ByteListToString(mO));

    // U
    encryptContext->WriteKey(scU);
    encryptContext->WriteHexStringValue(XCryptionCommon::ByteListToString(mU));

    // P
    encryptContext->WriteKey(scP);
    encryptContext->WriteIntegerValue(mP);

    // EncryptMetadata
    encryptContext->WriteKey(scEncryptMetadata);
    encryptContext->WriteBooleanValue(mEncryptMetaData);

    // Now. if using V4, define crypt filters
    if (mV == 4)
    {
        encryptContext->WriteKey("CF");
        DictionaryContext *cf = inObjectsContext->StartDictionary();
        cf->WriteKey("StdCF");

        DictionaryContext *stdCf = inObjectsContext->StartDictionary();
        stdCf->WriteKey("Type");
        stdCf->WriteNameValue("CryptFilter");

        stdCf->WriteKey("CFM");
        stdCf->WriteNameValue("AESV2");

        stdCf->WriteKey("AuthEvent");
        stdCf->WriteNameValue("DocOpen");

        stdCf->WriteKey("Length");
        stdCf->WriteIntegerValue(128);

        inObjectsContext->EndDictionary(stdCf);
        inObjectsContext->EndDictionary(cf);

        encryptContext->WriteKey("StmF");
        encryptContext->WriteNameValue("StdCF");

        encryptContext->WriteKey("StrF");
        encryptContext->WriteNameValue("StdCF");
    }

    ReleaseEncryption();

    return inObjectsContext->EndDictionary(encryptContext);
}

static const string scStdCF = "StdCF";

EStatusCode EncryptionHelper::Setup(bool inShouldEncrypt, double inPDFLevel, const string &inUserPassword,
                                    const string &inOwnerPassword, long long inUserProtectionOptionsFlag,
                                    bool inEncryptMetadata, const string &inFileIDPart1)
{

    if (!inShouldEncrypt)
    {
        SetupNoEncryption();
        return eSuccess;
    }

    mIsDocumentEncrypted = false;
    mSupportsEncryption = false;

    bool usingAES = inPDFLevel >= 1.6;
    auto *defaultEncryption = new XCryptionCommon();

    if (inPDFLevel >= 1.4)
    {
        mLength = 16;

        if (usingAES)
        {
            mV = 4;
            mRevision = 4;
        }
        else
        {
            mV = 2;
            mRevision = 3;
        }
    }
    else
    {
        mLength = 5;
        mV = 1;
        mRevision = (inUserProtectionOptionsFlag & 0xF00) != 0 ? 3 : 2;
        usingAES = false;
    }

    defaultEncryption->Setup(usingAES);
    mXcrypts.insert(StringToXCryptionCommonMap::value_type(scStdCF, defaultEncryption));
    mXcryptStreams = defaultEncryption;
    mXcryptStrings = defaultEncryption;
    mXcryptAuthentication = defaultEncryption;

    // compute P out of inUserProtectionOptionsFlag. inUserProtectionOptionsFlag can be a more relaxed number setting as
    // 1s only the enabled access. mP will restrict to PDF Expected bits
    auto truncP = int32_t(((inUserProtectionOptionsFlag | 0xFFFFF0C0) & 0xFFFFFFFC));
    mP = truncP;

    ByteList ownerPassword =
        XCryptionCommon::stringToByteList(!inOwnerPassword.empty() ? inOwnerPassword : inUserPassword);
    ByteList userPassword = XCryptionCommon::stringToByteList(inUserPassword);
    mEncryptMetaData = inEncryptMetadata;
    mFileIDPart1 = XCryptionCommon::stringToByteList(inFileIDPart1);

    mO = mXcryptAuthentication->algorithm3_3(mRevision, mLength, ownerPassword, userPassword);
    if (mRevision == 2)
        mU = mXcryptAuthentication->algorithm3_4(mLength, userPassword, mO, mP, mFileIDPart1, mEncryptMetaData);
    else
        mU = mXcryptAuthentication->algorithm3_5(mRevision, mLength, userPassword, mO, mP, mFileIDPart1,
                                                 mEncryptMetaData);

    defaultEncryption->SetupInitialEncryptionKey(inUserPassword, mRevision, mLength, mO, mP, mFileIDPart1,
                                                 mEncryptMetaData);

    mIsDocumentEncrypted = true;
    mSupportsEncryption = true;

    return eSuccess;
}

void EncryptionHelper::SetupNoEncryption()
{
    mIsDocumentEncrypted = false;
    mSupportsEncryption = true;
}

EStatusCode EncryptionHelper::Setup(const DecryptionHelper &inDecryptionSource)
{
    if (!inDecryptionSource.IsEncrypted() || !inDecryptionSource.CanDecryptDocument())
    {
        SetupNoEncryption();
        return eSuccess;
    }

    mIsDocumentEncrypted = false;
    mSupportsEncryption = false;

    do
    {

        mLength = inDecryptionSource.GetLength();
        mV = inDecryptionSource.GetV();
        mRevision = inDecryptionSource.GetRevision();
        mP = inDecryptionSource.GetP();
        mEncryptMetaData = inDecryptionSource.GetEncryptMetaData();
        mFileIDPart1 = inDecryptionSource.GetFileIDPart1();
        mO = inDecryptionSource.GetO();
        mU = inDecryptionSource.GetU();

        // initialize xcryptors
        mXcryptStreams = nullptr;
        // xcrypt to use for strings
        mXcryptStrings = nullptr;
        // xcrypt to use for password authentication
        mXcryptAuthentication = nullptr;
        auto it = inDecryptionSource.GetXcrypts().begin();
        auto itEnd = inDecryptionSource.GetXcrypts().end();
        for (; it != itEnd; ++it)
        {
            auto *xCryption = new XCryptionCommon();
            xCryption->Setup(it->second->IsUsingAES());
            xCryption->SetupInitialEncryptionKey(it->second->GetInitialEncryptionKey());
            mXcrypts.insert(StringToXCryptionCommonMap::value_type(it->first, xCryption));

            // see if it fits any of the global xcryptors
            if (it->second == inDecryptionSource.GetStreamXcrypt())
                mXcryptStreams = xCryption;
            if (it->second == inDecryptionSource.GetStringXcrypt())
                mXcryptStrings = xCryption;
            if (it->second == inDecryptionSource.GetAuthenticationXcrypt())
                mXcryptAuthentication = xCryption;
        }

        mIsDocumentEncrypted = true;
        mSupportsEncryption = true;
    } while (false);

    return eSuccess;
}

charta::EStatusCode EncryptionHelper::WriteState(ObjectsContext *inStateWriter, ObjectIDType inObjectID)
{
    inStateWriter->StartNewIndirectObject(inObjectID);
    DictionaryContext *encryptionObject = inStateWriter->StartDictionary();

    encryptionObject->WriteKey("Type");
    encryptionObject->WriteNameValue("EncryptionHelper");

    encryptionObject->WriteKey("mIsDocumentEncrypted");
    encryptionObject->WriteBooleanValue(mIsDocumentEncrypted);

    encryptionObject->WriteKey("mSupportsEncryption");
    encryptionObject->WriteBooleanValue(mSupportsEncryption);

    encryptionObject->WriteKey("mUsingAES");
    encryptionObject->WriteBooleanValue(mXcryptAuthentication != nullptr ? mXcryptAuthentication->IsUsingAES() : false);

    encryptionObject->WriteKey("mLength");
    encryptionObject->WriteIntegerValue(mLength);

    encryptionObject->WriteKey("mV");
    encryptionObject->WriteIntegerValue(mV);

    encryptionObject->WriteKey("mRevision");
    encryptionObject->WriteIntegerValue(mRevision);

    encryptionObject->WriteKey("mP");
    encryptionObject->WriteIntegerValue(mP);

    encryptionObject->WriteKey("mEncryptMetaData");
    encryptionObject->WriteBooleanValue(mEncryptMetaData);

    encryptionObject->WriteKey("mFileIDPart1");
    encryptionObject->WriteLiteralStringValue(XCryptionCommon::ByteListToString(mFileIDPart1));

    encryptionObject->WriteKey("mO");
    encryptionObject->WriteLiteralStringValue(XCryptionCommon::ByteListToString(mO));

    encryptionObject->WriteKey("mU");
    encryptionObject->WriteLiteralStringValue(XCryptionCommon::ByteListToString(mU));

    encryptionObject->WriteKey("InitialEncryptionKey");
    encryptionObject->WriteLiteralStringValue(
        mXcryptAuthentication != nullptr
            ? XCryptionCommon::ByteListToString(mXcryptAuthentication->GetInitialEncryptionKey())
            : "");

    inStateWriter->EndDictionary(encryptionObject);
    inStateWriter->EndIndirectObject();

    return eSuccess;
}

charta::EStatusCode EncryptionHelper::ReadState(PDFParser *inStateReader, ObjectIDType inObjectID)
{
    PDFObjectCastPtr<charta::PDFDictionary> encryptionObjectState(inStateReader->ParseNewObject(inObjectID));

    PDFObjectCastPtr<charta::PDFBoolean> isDocumentEncrypted =
        encryptionObjectState->QueryDirectObject("mIsDocumentEncrypted");
    mIsDocumentEncrypted = isDocumentEncrypted->GetValue();

    PDFObjectCastPtr<charta::PDFBoolean> supportsEncryption =
        encryptionObjectState->QueryDirectObject("mSupportsEncryption");
    mSupportsEncryption = supportsEncryption->GetValue();

    PDFObjectCastPtr<charta::PDFBoolean> usingAESObject = encryptionObjectState->QueryDirectObject("mUsingAES");
    bool usingAES = usingAESObject->GetValue();

    PDFObjectCastPtr<PDFInteger> length = encryptionObjectState->QueryDirectObject("mLength");
    mLength = (uint32_t)length->GetValue();

    PDFObjectCastPtr<PDFInteger> v = encryptionObjectState->QueryDirectObject("mV");
    mV = (uint32_t)v->GetValue();

    PDFObjectCastPtr<PDFInteger> revision = encryptionObjectState->QueryDirectObject("mRevision");
    mRevision = (uint32_t)revision->GetValue();

    PDFObjectCastPtr<PDFInteger> p = encryptionObjectState->QueryDirectObject("mP");
    mP = p->GetValue();

    PDFObjectCastPtr<charta::PDFBoolean> encryptMetaData = encryptionObjectState->QueryDirectObject("mEncryptMetaData");
    mEncryptMetaData = encryptMetaData->GetValue();

    PDFObjectCastPtr<charta::PDFLiteralString> fileIDPart1 = encryptionObjectState->QueryDirectObject("mFileIDPart1");
    mFileIDPart1 = XCryptionCommon::stringToByteList(fileIDPart1->GetValue());

    PDFObjectCastPtr<charta::PDFLiteralString> o = encryptionObjectState->QueryDirectObject("mO");
    mO = XCryptionCommon::stringToByteList(o->GetValue());

    PDFObjectCastPtr<charta::PDFLiteralString> u = encryptionObjectState->QueryDirectObject("mU");
    mU = XCryptionCommon::stringToByteList(u->GetValue());

    PDFObjectCastPtr<charta::PDFLiteralString> InitialEncryptionKey =
        encryptionObjectState->QueryDirectObject("InitialEncryptionKey");
    auto *defaultEncryption = new XCryptionCommon();

    // setup encryption
    defaultEncryption->Setup(usingAES);
    mXcrypts.insert(StringToXCryptionCommonMap::value_type(scStdCF, defaultEncryption));
    mXcryptStreams = defaultEncryption;
    mXcryptStrings = defaultEncryption;
    mXcryptAuthentication = defaultEncryption;
    mXcryptAuthentication->SetupInitialEncryptionKey(
        XCryptionCommon::stringToByteList(InitialEncryptionKey->GetValue()));

    return eSuccess;
}
