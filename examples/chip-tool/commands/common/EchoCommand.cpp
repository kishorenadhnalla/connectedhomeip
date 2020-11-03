/*
 *   Copyright (c) 2020 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include "EchoCommand.h"

#include <messaging/ExchangeContext.h>

using namespace ::chip;
using namespace ::chip::DeviceController;

#define SEND_DELAY 5
static const char * PAYLOAD = "Message from Standalone CHIP echo client!";

void EchoCommand::SendEcho(ExchangeContext * ec) const
{
    size_t payloadLen = strlen(PAYLOAD);
    if (payloadLen > UINT16_MAX)
    {
        ChipLogError(chipTool, "PAYLOAD length too big for PacketBuffer (> UINT16_MAX)");
        return;
    }

    // Reallocate buffer on each run, as the secure transport encrypts and
    // overwrites the buffer from previous iteration.
    auto * buffer = PacketBuffer::NewWithAvailableSize(static_cast<uint16_t>(payloadLen));
    if (buffer == nullptr)
    {
        ChipLogError(chipTool, "Failed to allocate memory for packet data.");
        return;
    }

    memcpy(buffer->Start(), PAYLOAD, payloadLen);
    buffer->SetDataLength(static_cast<uint16_t>(payloadLen));

    CHIP_ERROR err = ec->SendMessage(5, 0, buffer);
    if (err == CHIP_NO_ERROR)
    {
        ChipLogProgress(chipTool, "Echo (%s): Message sent to server", GetNetworkName());
    }
    else
    {
        ChipLogError(chipTool, "Echo (%s): %s", GetNetworkName(), ErrorStr(err));
    }
}

void EchoCommand::ReceiveEcho(PacketBuffer * buffer) const
{
    // attempt to print the incoming message
    size_t data_len = buffer->DataLength();
    char msg_buffer[data_len];
    msg_buffer[data_len] = 0; // Null-terminate whatever we received and treat like a string...
    memcpy(msg_buffer, buffer->Start(), data_len);

    bool isEchoIdenticalToMessage = strncmp(msg_buffer, PAYLOAD, data_len) == 0;
    if (isEchoIdenticalToMessage)
    {
        ChipLogProgress(chipTool, "Echo (%s): Received expected message !", GetNetworkName());
    }
    else
    {
        ChipLogError(chipTool, "Echo: (%s): Error \nSend: %s \nRecv: %s", GetNetworkName(), PAYLOAD, msg_buffer);
    }
}

CHIP_ERROR EchoCommand::Run(ChipDeviceController * dc, NodeId remoteId)
{
    CHIP_ERROR err       = CHIP_NO_ERROR;
    ExchangeManager * em = nullptr;
    ExchangeContext * ec = nullptr;

    err = NetworkCommand::Run(dc, remoteId);
    SuccessOrExit(err);

    em = dc->GetExchangeManager();
    VerifyOrExit(em != nullptr, err = CHIP_ERROR_NO_MEMORY);

    ec = em->NewContext(remoteId, this);
    VerifyOrExit(ec != nullptr, err = CHIP_ERROR_NO_MEMORY);

    // Run command until the user exits the process
    while (1)
    {
        if (mController != nullptr)
        {
            SendEcho(ec);
        }
        sleep(SEND_DELAY);
    }

exit:
    if (ec != nullptr)
        ec->Close();

    return err;
}

void EchoCommand::OnMessageReceived(ExchangeContext * ec, const PacketHeader & packetHeader, uint32_t protocolId, uint8_t msgType,
                                    System::PacketBuffer * payload)
{
    ReceiveEcho(payload);
}
