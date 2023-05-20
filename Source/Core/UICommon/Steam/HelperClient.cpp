#include "UICommon/Steam/HelperClient.h"

namespace Steam
{
std::future<sf::Packet> HelperClient::SendRequest(const sf::Packet* data_packet, MessageType message_type)
{
    auto promise = std::make_shared<std::promise<sf::Packet>>();

    sf::Packet packet;
    packet << (uint8_t)message_type;

    {
        std::scoped_lock lock(m_promises_mutex);

        packet << ++m_last_call_id;

        m_promises[m_last_call_id] = promise;
    }

    if (data_packet != nullptr)
    {
        packet.append(data_packet->getData(), data_packet->getDataSize());
    }

    Send(packet);

    return promise->get_future();
}

void HelperClient::SendRequestNoReply(MessageType type, const sf::Packet* data_packet)
{
    sf::Packet packet;
    packet << static_cast<uint8_t>(type);
    packet << std::numeric_limits<uint32_t>::max(); // dummy call ID

    if (data_packet != nullptr)
    {
        packet.append(data_packet->getData(), data_packet->getDataSize());
    }

    Send(packet);
}

void HelperClient::Receive(sf::Packet& packet)
{
    uint8_t raw_type;
    packet >> raw_type;

    MessageType message_type = (MessageType)raw_type;

    uint32_t call_id;
    packet >> call_id;

    switch (message_type)
    {
        case MessageType::InitReply:
        case MessageType::FetchUsernameReply:
        case MessageType::SetRichPresenceReply:
            {
                std::scoped_lock lock(m_promises_mutex);

                m_promises[call_id].get()->set_value(packet);
                m_promises.erase(call_id);
            }

            break;
        default:
            fprintf(stderr, "invalid\n");
            break;
    }
}
} // namespace Steam
