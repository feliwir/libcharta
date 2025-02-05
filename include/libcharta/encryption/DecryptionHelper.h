/*
Source File : DecryptionHelper.h


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
#pragma once

#include "EStatusCode.h"
#include "encryption/XCryptionCommon.h"

#include <memory>
#include <string>

class PDFParser;
namespace charta
{
class IByteReader;
class PDFObject;
class PDFStreamInput;
}; // namespace charta

class DecryptionHelper
{

  public:
    DecryptionHelper(void);
    virtual ~DecryptionHelper(void);

    /*
        Setup will fail for case of bad password, for decryption that can be handled.
    */
    charta::EStatusCode Setup(PDFParser *inParser, const std::string &inPassword);

    /*
        Queries.

        1. IsEncrypted means PDF is encrypted
        2. SupportsDecryption means that DecryptionHelper supports the kind of encryption defined in the PDF
        3. CanDecryptDocument means that DecryptionHelper can decrypt document. means decryption is supported, and
       password is good
        4. DidFailPasswordVerification meams that password verification didnt went through. either decryption not
       supported, or password no good
        5. DidSucceedOwnerPasswordVerification means that password verification succeeded, and the user has owner
       priviliges
    */
    bool IsEncrypted() const;
    bool SupportsDecryption() const;
    bool CanDecryptDocument() const;
    bool DidFailPasswordVerification() const;
    bool DidSucceedOwnerPasswordVerification() const;

    std::string DecryptString(const std::string &inStringToDecrypt);

    void OnObjectStart(long long inObjectID, long long inGenerationNumber);
    void OnObjectEnd(const std::shared_ptr<charta::PDFObject> &inObject);
    // this should be used by parser to grab a default filter for stream. will return null if a stream-specific filter
    // is to be used, or that there's no encryption expected for this stream
    charta::IByteReader *CreateDefaultDecryptionFilterForStream(const std::shared_ptr<charta::PDFStreamInput> &inStream,
                                                                charta::IByteReader *inToWrapStream);

    // use this for creating a decryption filter for a stream that uses a stream-specific crypt filter
    charta::IByteReader *CreateDecryptionFilterForStream(const std::shared_ptr<charta::PDFStreamInput> &inStream,
                                                         charta::IByteReader *inToWrapStream,
                                                         const std::string &inCryptName);

    uint32_t GetLength() const;
    uint32_t GetV() const;
    uint32_t GetRevision() const;
    long long GetP() const;
    bool GetEncryptMetaData() const;
    const ByteList &GetFileIDPart1() const;
    const ByteList &GetO() const;
    const ByteList &GetU() const;
    const ByteList &GetInitialEncryptionKey() const;
    const StringToXCryptionCommonMap &GetXcrypts() const;
    XCryptionCommon *GetStreamXcrypt() const;
    XCryptionCommon *GetStringXcrypt() const;
    XCryptionCommon *GetAuthenticationXcrypt() const;

    // Reset after or before usage
    void Reset();

    // client can tell at times to halt encryption, when knowing that parsing content that's not encrypted (in encrypted
    // object stream, for instances)
    void PauseDecryption();
    // use this to flag that a previous pause of encryption may now be released. encryption can continue
    void ReleaseDecryption();

    /*
    IsEncrypting will return true if actually encrypting now. it may be false if encryption was not requested, not
    supported or that the encrytion mechanism is not currently in a state that encrypts (say when writing an encryption
    dictionary).
    */
    bool IsDecrypting() const;

    //	void HaltDecryption();
    //	void ContinueDecryption();
  private:
    PDFParser *mParser;

    // named xcrypts, for V4
    StringToXCryptionCommonMap mXcrypts;
    // xcrypt to use for streams
    XCryptionCommon *mXcryptStreams;
    // xcrypt to use for strings
    XCryptionCommon *mXcryptStrings;
    // xcrypt to use for password authentication
    XCryptionCommon *mXcryptAuthentication;

    bool mIsEncrypted;
    bool mSupportsDecryption;
    int mDecryptionPauseLevel;

    // Generic encryption
    uint32_t mV;
    uint32_t mLength; // mLength is in bytes!

    charta::IByteReader *CreateDecryptionReader(charta::IByteReader *inSourceStream, const ByteList &inEncryptionKey,
                                                bool inUsingAES);
    XCryptionCommon *GetCryptForStream(const std::shared_ptr<charta::PDFStreamInput> &inStream);

    // Standard filter specific
    bool mFailedPasswordVerification;
    bool mDidSucceedOwnerPasswordVerification;

    uint32_t mRevision;
    ByteList mO;
    ByteList mU;
    long long mP;
    bool mEncryptMetaData;
    ByteList mFileIDPart1;

    bool AuthenticateUserPassword(const ByteList &inPassword);
    bool AuthenticateOwnerPassword(const ByteList &inPassword);

    void Release();
};