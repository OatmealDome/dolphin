#include "HelperServer.h"

#include <steam/steam_api.h>

#include "SteamHelperCommon/MessageType.h"

namespace Steam
{
void HelperServer::Receive(sf::Packet &packet)
{
    uint8_t rawType;
    packet >> rawType;

    MessageType type = static_cast<MessageType>(rawType);

    uint32_t call_id;
    packet >> call_id;

    fprintf(stderr, "server received message %hhu\n", type);

    switch (type)
    {
        case MessageType::InitRequest:
            ReceiveInitRequest(call_id);
            break;
        case MessageType::FetchUsernameRequest:
            ReceiveFetchUsernameRequest(call_id);
            break;
        case MessageType::SetRichPresenceRequest:
        {
            std::string rp_key;
            packet >> rp_key;

            std::string rp_value;
            packet >> rp_value;

            ReceiveSetRichPresenceRequest(call_id, rp_key, rp_value);

            break;
        }
        case MessageType::ShutdownRequest:
            RequestStop();
            break;
        default:
            fprintf(stderr, "server unknown message\n");
            break;
    }
}

void HelperServer::ReceiveInitRequest(uint32_t call_id)
{
    sf::Packet replyPacket;

    replyPacket << static_cast<uint8_t>(MessageType::InitReply);
    replyPacket << call_id;

    replyPacket << static_cast<uint8_t>(SteamAPI_Init());
    
    Send(replyPacket);
}

void HelperServer::ReceiveFetchUsernameRequest(uint32_t call_id)
{
    sf::Packet replyPacket;

    replyPacket << static_cast<uint8_t>(MessageType::FetchUsernameReply);
    replyPacket << call_id;

    const char* persona_name = SteamFriends()->GetPersonaName();
    const std::string persona_name_str = persona_name;

    replyPacket << persona_name_str;

    Send(replyPacket);
}

void HelperServer::ReceiveSetRichPresenceRequest(uint32_t call_id, const std::string& key, const std::string& value)
{
    sf::Packet replyPacket;

    replyPacket << static_cast<uint8_t>(MessageType::SetRichPresenceReply);
    replyPacket << call_id;

    bool result = SteamFriends()->SetRichPresence(key.c_str(), value.c_str());


    replyPacket << static_cast<uint8_t>(result);

    Send(replyPacket);
}

void HelperServer::ReceiveShutdownRequest()
{
    SteamAPI_Shutdown();

    RequestStop();
}
} // namespace Steam
