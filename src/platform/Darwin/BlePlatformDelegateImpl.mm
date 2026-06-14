/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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

/**
 *    @file
 *          Provides an implementation of BlePlatformDelegate for Darwin platforms.
 */

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC. Use -fobjc-arc flag (or convert project to ARC).
#endif

#include <ble/Ble.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/Darwin/BlePlatformDelegateImpl.h>
#include <platform/Darwin/BleUtils.h>

#import <CoreBluetooth/CoreBluetooth.h>

using namespace chip::Ble;
using chip::System::PacketBufferHandle;

namespace chip {
namespace DeviceLayer {
    namespace Internal {

        namespace {
            CBCharacteristic * FindCharacteristic(CBPeripheral * peripheral, const ChipBleUUID * svcId, const ChipBleUUID * charId)
            {
                VerifyOrReturnValue(svcId != nullptr && charId != nullptr, nil);
                CBUUID * cbSvcId = CBUUIDFromBleUUID(*svcId);
                for (CBService * service in peripheral.services) {
                    if ([service.UUID isEqual:cbSvcId]) {
                        CBUUID * cbCharId = CBUUIDFromBleUUID(*charId);
                        for (CBCharacteristic * characteristic in service.characteristics) {
                            if ([characteristic.UUID isEqual:cbCharId]) {
                                return characteristic;
                            }
                        }
                        break;
                    }
                }
                return nil;
            }
        }

        CHIP_ERROR BlePlatformDelegateImpl::SubscribeCharacteristic(
            BLE_CONNECTION_OBJECT connObj, const ChipBleUUID * svcId, const ChipBleUUID * charId)
        {
            CBPeripheral * peripheral = CBPeripheralFromBleConnObject(connObj);
            CBCharacteristic * characteristic = FindCharacteristic(peripheral, svcId, charId);
            VerifyOrReturnError(characteristic != nil, BLE_ERROR_GATT_SUBSCRIBE_FAILED);

            // macOS Tahoe (26.x) blocks setNotifyValue:YES for 128-bit proprietary UUIDs
            // with CBErrorUUIDNotAllowed (code=8), preventing BTP subscribe from succeeding.
            //
            // Workaround: write the CCCD descriptor (UUID 0x2902) directly with the
            // "enable indications" value (0x0002).  The CCCD descriptor was eagerly
            // discovered in didDiscoverCharacteristicsForService: and is available on
            // characteristic.descriptors.
            //
            // On success didWriteValueForDescriptor: fires HandleSubscribeComplete.
            // If the CCCD descriptor is not available, fall back to setNotifyValue:.
            CBUUID * cccdUUID = [CBUUID UUIDWithString:@"2902"];
            CBDescriptor * cccdDescriptor = nil;
            for (CBDescriptor * desc in characteristic.descriptors) {
                if ([desc.UUID isEqual:cccdUUID]) {
                    cccdDescriptor = desc;
                    break;
                }
            }

            if (cccdDescriptor != nil) {
                // Write 0x0002 = enable indications
                uint8_t enableIndications[] = { 0x02, 0x00 };
                NSData * value = [NSData dataWithBytes:enableIndications length:sizeof(enableIndications)];
                ChipLogProgress(Ble, "BLE: Writing CCCD descriptor directly to enable indications (macOS Tahoe workaround)");
                [peripheral writeValue:value forDescriptor:cccdDescriptor];
            } else {
                ChipLogProgress(Ble, "BLE: CCCD descriptor not cached — falling back to setNotifyValue:YES");
                [peripheral setNotifyValue:YES forCharacteristic:characteristic];
            }

            return CHIP_NO_ERROR;
        }

        CHIP_ERROR BlePlatformDelegateImpl::UnsubscribeCharacteristic(
            BLE_CONNECTION_OBJECT connObj, const ChipBleUUID * svcId, const ChipBleUUID * charId)
        {
            CBPeripheral * peripheral = CBPeripheralFromBleConnObject(connObj);
            CBCharacteristic * characteristic = FindCharacteristic(peripheral, svcId, charId);
            VerifyOrReturnError(characteristic != nil, BLE_ERROR_GATT_UNSUBSCRIBE_FAILED);
            [peripheral setNotifyValue:NO forCharacteristic:characteristic];
            return CHIP_NO_ERROR;
        }

        CHIP_ERROR BlePlatformDelegateImpl::CloseConnection(BLE_CONNECTION_OBJECT connObj)
        {
            CBPeripheral * peripheral = CBPeripheralFromBleConnObject(connObj);

            // CoreBluetooth API requires a CBCentralManager to close a connection which is a property of the peripheral.
            CBCentralManager * manager = (CBCentralManager *) [peripheral valueForKey:@"manager"];
            if (manager != nil)
                [manager cancelPeripheralConnection:peripheral];

            // TODO: This does not cause our BleConnectionDelegateImpl to let go of the CBPeripheral
            // since MTRBleConnection does not implement centralManager:didDisconnectPeripheral:error:

            return CHIP_NO_ERROR;
        }

        uint16_t BlePlatformDelegateImpl::GetMTU(BLE_CONNECTION_OBJECT connObj) const
        {
            CBPeripheral * peripheral = CBPeripheralFromBleConnObject(connObj);

            // The negotiated mtu length is a property of the peripheral.
            uint16_t mtuLength = [[peripheral valueForKey:@"mtuLength"] unsignedShortValue];
            ChipLogProgress(Ble, "ATT MTU = %u", mtuLength);

            return mtuLength;
        }

        CHIP_ERROR BlePlatformDelegateImpl::SendIndication(
            BLE_CONNECTION_OBJECT connObj, const ChipBleUUID * svcId, const ChipBleUUID * charId, PacketBufferHandle pBuf)
        {
            return CHIP_ERROR_NOT_IMPLEMENTED;
        }

        CHIP_ERROR BlePlatformDelegateImpl::SendWriteRequest(
            BLE_CONNECTION_OBJECT connObj, const ChipBleUUID * svcId, const ChipBleUUID * charId, PacketBufferHandle pBuf)
        {
            CBPeripheral * peripheral = CBPeripheralFromBleConnObject(connObj);
            CBCharacteristic * characteristic = FindCharacteristic(peripheral, svcId, charId);
            VerifyOrReturnError(characteristic != nil && !pBuf.IsNull(), BLE_ERROR_GATT_WRITE_FAILED);
            NSData * data = [NSData dataWithBytes:pBuf->Start() length:pBuf->DataLength()]; // copies data, pBuf can be freed

            // On macOS Tahoe (and potentially earlier) CoreBluetooth rejects
            // CBCharacteristicWriteWithResponse to 128-bit proprietary UUIDs from
            // sandboxed apps with CBErrorUUIDNotAllowed (domain=CBErrorDomain code=8).
            //
            // The Matter C1 characteristic (18EE2EF5-...-9D11) is declared with both
            // Write (0x08) and WriteWithoutResponse (0x04) properties.  We prefer
            // WriteWithoutResponse when available because:
            //  1. It is never blocked by the macOS sandbox UUID policy.
            //  2. The BTPEngine write-confirmation flow still works: since CoreBluetooth
            //     does not call didWriteValueForCharacteristic: for no-response writes,
            //     we synthesise HandleWriteConfirmation ourselves on the CHIP work queue
            //     immediately after the write is submitted.
            bool useWriteWithoutResponse =
                (characteristic.properties & CBCharacteristicPropertyWriteWithoutResponse) != 0;

            if (useWriteWithoutResponse) {
                ChipLogDetail(Ble, "SendWriteRequest: using WriteWithoutResponse for char=%s",
                    characteristic.UUID.UUIDString.UTF8String);
                [peripheral writeValue:data forCharacteristic:characteristic type:CBCharacteristicWriteWithoutResponse];

                // Synthesise write confirmation so BTPEngine clears kGattOperationInFlight.
                // Copy the UUIDs by value for the lambda capture.
                ChipBleUUID capSvcId  = *svcId;
                ChipBleUUID capCharId = *charId;
                // Retain the peripheral so the conn object remains valid across the dispatch.
                __block CBPeripheral * capPeripheral = peripheral;
                dispatch_async(chip::DeviceLayer::PlatformMgrImpl().GetWorkQueue(), ^{
                    chip::Ble::BleLayer * bleLayer = chip::DeviceLayer::Internal::BLEMgr().GetBleLayer();
                    bleLayer->HandleWriteConfirmation(
                        chip::DeviceLayer::Internal::BleConnObjectFromCBPeripheral(capPeripheral),
                        &capSvcId, &capCharId);
                });
            } else {
                ChipLogDetail(Ble, "SendWriteRequest: using WriteWithResponse for char=%s",
                    characteristic.UUID.UUIDString.UTF8String);
                [peripheral writeValue:data forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
            }

            return CHIP_NO_ERROR;
        }

    } // namespace Internal
} // namespace DeviceLayer
} // namespace chip
