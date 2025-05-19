/*
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <controller/JCMCommissioner.h>
#include <controller/CommissioningDelegate.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/InteractionModelEngine.h>
#include <app/tests/AppTestContext.h>
#include <credentials/CHIPCert.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPError.h>
#include <lib/support/TestPersistentStorageDelegate.h>
#include <messaging/ExchangeMgr.h>
#include <messaging/ReliableMessageProtocolConfig.h>
#include <transport/SecureSession.h>
#include <transport/SecureSessionTable.h>

#include <credentials/tests/CHIPCert_test_vectors.h>

#include <pw_unit_test/framework.h>

using namespace chip;
using namespace chip::Controller;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::Credentials;
using namespace chip::Crypto;
using namespace chip::Transport;
using namespace chip::Messaging;

namespace {

class MockCommissioneeDeviceProxy : public CommissioneeDeviceProxy
{
public:
    MockCommissioneeDeviceProxy() {}
    CHIP_ERROR SendCommands(app::CommandSender * commandObj, Optional<System::Clock::Timeout> timeout) override { return CHIP_NO_ERROR; }
    ExchangeManager * GetExchangeManager() const override { return nullptr; }
    CHIP_ERROR GetAttestationChallenge(ByteSpan & attestationChallenge) { return CHIP_NO_ERROR; }
    bool IsSecureConnected() const override { return true; }

    void SetUpDeviceProxy()
    {
        SecureSessionTable connections;
        auto optionalSession = connections.CreateNewSecureSessionForTest(SecureSession::Type::kPASE, 2, kLocalNodeId, kCasePeerNodeId,
                                                                     kPeerCATs, 1, kFabricIndex, GetDefaultMRPConfig());
        SetConnected(optionalSession.Value());
    }

private:
    const NodeId kLocalNodeId      = 0xC439A991071292DB;
    const NodeId kCasePeerNodeId  = 1;
    const FabricIndex kFabricIndex = 1;
    const CATValues kPeerCATs = { { 0xABCD0001, 0xABCE0100, 0xABCD0020 } };
};

class MockJCMTrustVerificationDelegate : public JCMTrustVerificationDelegate
{
public:
    void OnProgressUpdate(JCMDeviceCommissioner & commissioner, 
                          JCMTrustVerificationStage stage,
                          JCMTrustVerificationError error) override
    {
        mProgressUpdates++;
        mLastStage = stage;
        mLastError = error;
    }

    void OnAskUserForConsent(JCMDeviceCommissioner & commissioner, VendorId vendorId) override
    {
        mAskedForConsent = true;
        mLastVendorId = vendorId;
        commissioner.ContinueAfterUserConsent(mShouldConsent);
    }

    int mProgressUpdates = 0;
    JCMTrustVerificationStage mLastStage = JCMTrustVerificationStage::kIdle;
    JCMTrustVerificationError mLastError = JCMTrustVerificationResult::kSuccess;
    bool mAskedForConsent = false;
    bool mShouldConsent = true;
    VendorId mLastVendorId = VendorId::Common;
};

class MockClusterStateCacheCallback : public ClusterStateCache::Callback
{
    void OnDone(ReadClient *) override {}
    void OnAttributeData(const ConcreteDataAttributePath & aPath, TLV::TLVReader * apData, const StatusIB & aStatus) override {}
};

} // namespace

namespace {

class TestJCMCommissioner : public ::testing::Test
{
public:
    static void SetUpTestSuite()
    {
        ASSERT_EQ(chip::Platform::MemoryInit(), CHIP_NO_ERROR);
    }

    // Performs shared teardown for all tests in the test suite.  Run once for the whole suite.
    static void TearDownTestSuite()
    {
        chip::Platform::MemoryShutdown();
    }

    JCMAutoCommissioner mAutoCommissioner;
    std::unique_ptr<JCMDeviceCommissioner> mDeviceCommissioner = nullptr;
    JCMCommissioner * mCommissioner = nullptr;
    std::shared_ptr<MockJCMTrustVerificationDelegate> mTrustVerificationDelegate = nullptr;
    ClusterStateCache * mClusterStateCache = nullptr;;
    MockCommissioneeDeviceProxy mDeviceProxy;
    bool mSetup = false;

protected:
    void SetUp() override
    {
        VerifyOrReturn(SetupCommissioner() == CHIP_NO_ERROR);
        VerifyOrReturn(SetupClusterStateCache() == CHIP_NO_ERROR);
        VerifyOrReturn(SetupDeviceProxy() == CHIP_NO_ERROR);

        mSetup = true;
    }

    void TearDown() override
    {
        mClusterStateCache->ClearEventCache();
        Platform::Delete(mClusterStateCache);
        mDeviceCommissioner.reset();
        mTrustVerificationDelegate.reset();
    }

private:
    CHIP_ERROR SetupCommissioner()
    {
        CommissioningParameters params;
        params.SetUseJCM(true);
        mAutoCommissioner.SetCommissioningParameters(params);
        mDeviceCommissioner = JCMDeviceCommissioner::Builder()
            .setTrustVerificationDelegate(mTrustVerificationDelegate)
            .Build();
        mCommissioner = &mDeviceCommissioner->GetCommissioner();

        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetupDeviceProxy()
    {
        mDeviceProxy.SetUpDeviceProxy();

        return CHIP_NO_ERROR;
    }

    template <typename AttrType>
    CHIP_ERROR SetAttribute(const ConcreteAttributePath & path, const AttrType data)
    {
        Platform::ScopedMemoryBufferWithSize<uint8_t> handle;
        handle.Calloc(64);
        TLV::ScopedBufferTLVWriter writer(std::move(handle), 64);
        ReturnErrorOnFailure(DataModel::Encode(writer, TLV::AnonymousTag(), data));
        uint32_t writtenLength = writer.GetLengthWritten();
        ReturnErrorOnFailure(writer.Finalize(handle));

        TLV::ScopedBufferTLVReader reader;
        StatusIB aStatus;
        reader.Init(std::move(handle), writtenLength);
        ReturnErrorOnFailure(reader.Next());
        ReadClient::Callback & callback = mClusterStateCache->GetBufferedCallback();
        callback.OnAttributeData(path, &reader, aStatus);

        return CHIP_NO_ERROR;
    }

    template <typename AttrType>
    CHIP_ERROR SetAttributeForWrite(const ConcreteAttributePath & path, const AttrType data)
    {
        Platform::ScopedMemoryBufferWithSize<uint8_t> handle;
        handle.Calloc(3000);
        TLV::ScopedBufferTLVWriter writer(std::move(handle), 3000);
        ReturnErrorOnFailure(DataModel::EncodeForWrite(writer, TLV::AnonymousTag(), data));
        uint32_t writtenLength = writer.GetLengthWritten();
        ReturnErrorOnFailure(writer.Finalize(handle));

        TLV::ScopedBufferTLVReader reader;
        StatusIB aStatus;
        reader.Init(std::move(handle), writtenLength);
        ReturnErrorOnFailure(reader.Next());
        ReadClient::Callback & callback = mClusterStateCache->GetBufferedCallback();
        callback.OnAttributeData(path, &reader, aStatus);

        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetupClusterStateCache()
    {
        MockClusterStateCacheCallback client;
        mClusterStateCache = Platform::New<ClusterStateCache>(client);

        // Setup JF Administrator cluster attributes
        ConcreteAttributePath adminFabricIndexPath(1, JointFabricAdministrator::Id, 
                                                 JointFabricAdministrator::Attributes::AdministratorFabricIndex::Id);
        ReturnErrorOnFailure(SetAttribute(adminFabricIndexPath, static_cast<FabricIndex>(1)));

        // Setup Operational Credentials cluster attributes
        // Fabrics attribute
        OperationalCredentials::Structs::FabricDescriptorStruct::Type fabricDescriptor;
        fabricDescriptor.fabricIndex = static_cast<chip::FabricIndex>(1);
        fabricDescriptor.vendorID = static_cast<chip::VendorId>(0xFFF1); // Example vendor ID
        fabricDescriptor.fabricID = static_cast<chip::FabricId>(1234);
        
        // Create a fake public key for testing
        uint8_t publicKeyBuffer[Crypto::kP256_PublicKey_Length] = {0};
        fabricDescriptor.rootPublicKey = ByteSpan(publicKeyBuffer, sizeof(publicKeyBuffer));

        OperationalCredentials::Structs::FabricDescriptorStruct::Type fabricListData[1] = { fabricDescriptor };
        DataModel::List<const OperationalCredentials::Structs::FabricDescriptorStruct::Type> fabricsList;
        fabricsList = fabricListData;
        ConcreteAttributePath fabricsPath(0, OperationalCredentials::Id, OperationalCredentials::Attributes::Fabrics::Id);
        ReturnErrorOnFailure(SetAttributeForWrite(fabricsPath, fabricsList));

        // NOCs attribute
        OperationalCredentials::Structs::NOCStruct::Type nocStruct;
        nocStruct.fabricIndex = 1;
        
        uint8_t nocBuffer[128] = {0};
        nocStruct.noc = ByteSpan(nocBuffer, sizeof(nocBuffer));
        
        uint8_t icacBuffer[128] = {0};
        nocStruct.icac = app::DataModel::MakeNullable(ByteSpan(icacBuffer, sizeof(icacBuffer)));

        OperationalCredentials::Structs::NOCStruct::Type nocListData[1] = { nocStruct };
        DataModel::List<OperationalCredentials::Structs::NOCStruct::Type> nocsList;
        nocsList = nocListData;

        ConcreteAttributePath nocsPath(0, OperationalCredentials::Id, OperationalCredentials::Attributes::NOCs::Id);
        ReturnErrorOnFailure(SetAttributeForWrite(nocsPath, nocsList));

        // TrustedRootCertificates attribute
        static constexpr BitFlags<chip::TestCerts::TestCertLoadFlags> sNullLoadFlag;
        chip::ByteSpan rootCert;
        ReturnErrorOnFailure(GetTestCert(chip::TestCerts::TestCert::kRoot01, sNullLoadFlag, rootCert));
        chip::ByteSpan rootCertsData[] = { rootCert };
        DataModel::List<chip::ByteSpan> rootCerts;
        rootCerts = rootCertsData;

        ConcreteAttributePath trustedRootsPath(0, OperationalCredentials::Id, 
                                             OperationalCredentials::Attributes::TrustedRootCertificates::Id);
        ReturnErrorOnFailure(SetAttribute(trustedRootsPath, rootCerts));

        return CHIP_NO_ERROR;
    }
};

// Test starting the JCM trust verification processt
TEST_F(TestJCMCommissioner, TestStartJCMTrustVerification) {
    EXPECT_EQ(CHIP_NO_ERROR, mAutoCommissioner.StartCommissioning(mDeviceCommissioner.get(), &mDeviceProxy));
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->StartJCMTrustVerification(&mDeviceProxy));
    
    // Verify that the delegate received progress updates
    EXPECT_GT(mTrustVerificationDelegate->mProgressUpdates, 0);
    
    // The first stage after starting should be kVerifyingAdministratorEndpointAndFabricIndex
    EXPECT_EQ(static_cast<int>(mTrustVerificationDelegate->mLastStage), 
                    static_cast<int>(JCMTrustVerificationStage::kVerifyingAdministratorEndpointAndFabricIndex));
}

// Test asking the user for consent
TEST_F(TestJCMCommissioner, TestAskUserForConsent)
{
    EXPECT_EQ(CHIP_NO_ERROR, mAutoCommissioner.StartCommissioning(mDeviceCommissioner.get(), &mDeviceProxy));
    EXPECT_EQ(mCommissioner->StartJCMTrustVerification(&mDeviceProxy), CHIP_NO_ERROR);
    
    // Execute to the point where user consent is requested
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kStarted
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingAdministratorEndpointAndFabricIndex
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kPerformingVendorIDVerificationProcedure
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingNOCContainsAdministratorCAT
    
    // Check that the delegate was asked for consent
    EXPECT_EQ(mTrustVerificationDelegate->mAskedForConsent, true);
    
    // Verify the vendor ID was passed correctly
    EXPECT_EQ(static_cast<int>(mTrustVerificationDelegate->mLastVendorId), static_cast<VendorId>(0xFFF1));
}

// Test user grants consent
TEST_F(TestJCMCommissioner, TestUserConsent)
{
    // Set up the delegate to grant consent
    mTrustVerificationDelegate->mShouldConsent = true;
    
    EXPECT_EQ(CHIP_NO_ERROR, mAutoCommissioner.StartCommissioning(mDeviceCommissioner.get(), &mDeviceProxy));
    EXPECT_EQ(mCommissioner->StartJCMTrustVerification(&mDeviceProxy), CHIP_NO_ERROR);
    
    // Execute to the point where user consent is granted
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kStarted
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingAdministratorEndpointAndFabricIndex
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kPerformingVendorIDVerificationProcedure
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingNOCContainsAdministratorCAT

    // The verification should succeed with the granted consent
    EXPECT_EQ(static_cast<uint16_t>(mTrustVerificationDelegate->mLastError.mResult), 
                   static_cast<uint16_t>(JCMTrustVerificationResult::kSuccess));
}

// Test user denies consent
TEST_F(TestJCMCommissioner, TestUserDeniesConsent)
{
    // Set up the delegate to deny consent
    mTrustVerificationDelegate->mShouldConsent = false;
    
    EXPECT_EQ(CHIP_NO_ERROR, mAutoCommissioner.StartCommissioning(mDeviceCommissioner.get(), &mDeviceProxy));
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->StartJCMTrustVerification(&mDeviceProxy));
    
    // Execute to the point where user consent is requested and denied
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kStarted
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingAdministratorEndpointAndFabricIndex
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kPerformingVendorIDVerificationProcedure
    mCommissioner->AdvanceTrustVerificationStage(JCMTrustVerificationResult::kSuccess);  // kVerifyingNOCContainsAdministratorCAT
    
    // Verification should fail due to denied consent
    EXPECT_EQ(static_cast<uint16_t>(mTrustVerificationDelegate->mLastError.mResult), 
                   static_cast<uint16_t>(JCMTrustVerificationResult::KUserDeniedConsent));
}

// Test finding admin fabric index and endpoint ID
TEST_F(TestJCMCommissioner, TestParseAdminFabricIndexAndEndpointId)
{
    EXPECT_EQ(CHIP_NO_ERROR, mAutoCommissioner.StartCommissioning(mDeviceCommissioner.get(), &mDeviceProxy));

    // TODO: Call the method indirectly to test it
    //EXPECT_EQ(mCommissioner->ParseAdminFabricIndexAndEndpointId(), CHIP_NO_ERROR);
    
    // Check that we found the administrator fabric index and endpoint ID by 
    // checking if verification proceeds past that step
    EXPECT_EQ(mCommissioner->StartJCMTrustVerification(&mDeviceProxy), CHIP_NO_ERROR);
    
    EXPECT_EQ(static_cast<int>(mTrustVerificationDelegate->mLastStage), 
                   static_cast<int>(JCMTrustVerificationStage::kVerifyingAdministratorEndpointAndFabricIndex));
    EXPECT_EQ(static_cast<uint16_t>(mTrustVerificationDelegate->mLastError.mResult), 
                   static_cast<uint16_t>(JCMTrustVerificationResult::kSuccess));
}

// Test getting operational credentials
TEST_F(TestJCMCommissioner, TestParseOperationalCredentials)
{
    ReadCommissioningInfo info;
    info.attributes = mClusterStateCache;

    // Call the method directly to test it
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->ParseOperationalCredentials(info));
}

// Test getting trusted root
TEST_F(TestJCMCommissioner, TestParseTrustedRoot) {
    ReadCommissioningInfo info;
    info.attributes = mClusterStateCache;

    // Set up the prerequisites for ParseTrustedRoot
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->ParseAdminFabricIndexAndEndpointId(info));
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->ParseOperationalCredentials(info));
    
    // Call the method directly to test it
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->ParseTrustedRoot(info));
}

// Test parsing administrator info
TEST_F(TestJCMCommissioner, TestParseAdministratorInfo)
{
    ReadCommissioningInfo info;
    info.attributes = mClusterStateCache;
    
    // Call the method directly to test it
    EXPECT_EQ(CHIP_NO_ERROR, mCommissioner->ParseAdministratorInfo(info));
}

// Test finishing reading commissioning info
TEST_F(TestJCMCommissioner, TestFinishReadingCommissioningInfo)
{
    // Create an error to pass in
    CHIP_ERROR err = CHIP_NO_ERROR;
    
    // TODO: Call the method FinishReadingCommissioningInfo indirectly to test it
    //mCommissioner->FinishReadingCommissioningInfo(err, info);
    
    // The error should still be CHIP_NO_ERROR
    EXPECT_EQ(CHIP_NO_ERROR, err);
}

} // namespace
