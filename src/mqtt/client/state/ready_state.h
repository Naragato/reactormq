//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "state.h"

namespace reactormq::mqtt::packets
{
    class IControlPacket;
}

namespace reactormq::mqtt::client
{
    struct PublishCommand;
    struct SubscribeCommand;
    struct SubscribesCommand;
    struct UnsubscribesCommand;

    /**
     * @brief State representing a connected client ready for MQTT operations.
     * Handles publish, subscribe, unsubscribe operations.
     * Manages keepalive (PINGREQ/PINGRESP), ACK tracking for QoS 1/2.
     */
    class ReadyState final : public IState
    {
    public:
        ReadyState() = default;

        ~ReadyState() override = default;

        StateTransition onEnter(Context& context) override;

        void onExit(Context& context) override;

        StateTransition handleCommand(Context& context, Command& command) override;

        StateTransition onSocketConnected(Context& context) override;

        StateTransition onSocketDisconnected(Context& context) override;

        StateTransition onDataReceived(Context& context, const uint8_t* data, uint32_t size) override;

        StateTransition onTick(Context& context) override;

        [[nodiscard]] const char* getStateName() const override
        {
            return "Ready";
        }

        [[nodiscard]] StateId getStateId() const override
        {
            return StateId::Ready;
        }

    private:
        /**
         * @brief Handle a publish command by encoding and sending a PUBLISH packet.
         * @param context Shared context.
         * @param sock Socket for sending data.
         * @param publishCmd The publish command containing the message and promise.
         * @return Optional state transition.
         */
        static StateTransition handlePublishCommand(Context& context, socket::Socket& sock, PublishCommand& publishCmd);

        /**
         * @brief Handle a subscribe command by encoding and sending a SUBSCRIBE packet.
         * @param context Shared context.
         * @param sock Socket for sending data.
         * @param subscribeCmd The subscribe command containing the topic filter and promise.
         * @return Optional state transition.
         */
        static StateTransition handleSubscribeCommand(Context& context, socket::Socket& sock, SubscribeCommand& subscribeCmd);

        /**
         * @brief Handle a multi-subscribe command by encoding and sending a SUBSCRIBE packet with multiple filters.
         * @param context Shared context.
         * @param sock Socket for sending data.
         * @param subscribesCmd The subscribes command containing multiple topic filters and promise.
         * @return Optional state transition.
         */
        static StateTransition handleSubscribesCommand(Context& context, socket::Socket& sock, SubscribesCommand& subscribesCmd);

        /**
         * @brief Handle an unsubscribe command by encoding and sending an UNSUBSCRIBE packet.
         * @param context Shared context.
         * @param sock Socket for sending data.
         * @param unsubscribesCmd The unsubscribes command containing topics and promise.
         * @return Optional state transition.
         */
        static StateTransition handleUnsubscribesCommand(Context& context, socket::Socket& sock, UnsubscribesCommand& unsubscribesCmd);

        /**
         * @brief Handle received PUBACK packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @return Optional state transition.
         */
        static StateTransition handlePubAck(Context& context, const packets::IControlPacket& packet);

        /**
         * @brief Handle received SUBACK packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @param protocolVersion MQTT protocol version.
         * @return Optional state transition.
         */
        static StateTransition handleSubAck(
            Context& context, const packets::IControlPacket& packet, packets::ProtocolVersion protocolVersion);

        /**
         * @brief Handle received UNSUBACK packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @param protocolVersion MQTT protocol version.
         * @return Optional state transition.
         */
        static StateTransition handleUnsubAck(
            Context& context, const packets::IControlPacket& packet, packets::ProtocolVersion protocolVersion);

        /**
         * @brief Handle received PUBREC packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @param protocolVersion MQTT protocol version.
         * @return Optional state transition.
         */
        static StateTransition handlePubRec(
            const Context& context, const packets::IControlPacket& packet, packets::ProtocolVersion protocolVersion);

        /**
         * @brief Handle received PUBREL packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @param protocolVersion MQTT protocol version.
         * @return Optional state transition.
         */
        static StateTransition handlePubRel(
            Context& context, const packets::IControlPacket& packet, packets::ProtocolVersion protocolVersion);

        /**
         * @brief Handle received PUBCOMP packet.
         * @param context Shared context.
         * @param packet The received packet.
         * @return Optional state transition.
         */
        static StateTransition handlePubComp(Context& context, const packets::IControlPacket& packet);

        /**
         * @brief Handle received PINGRESP packet.
         * @param context Shared context.
         * @return Optional state transition.
         */
        static StateTransition handlePingResp(Context& context);
    };
} // namespace reactormq::mqtt::client