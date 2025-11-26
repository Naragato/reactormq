//  SPDX-License-Identifier: MPL-2.0
//  Copyright 2025 Simon Balarabe
//  Project: ReactorMQ — https://github.com/Naragato/reactormq

#pragma once

#include "property_identifier.h"
#include "serialize/bytes.h"
#include "serialize/serializable.h"

#include <concepts>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace reactormq::mqtt::packets::properties
{
    /**
     * @brief Compile-time check whether a property identifier is one of a given set.
     * This variable template evaluates to @c true if @p Id is equal to any of the
     * property identifiers in the parameter pack @p Ids..., and @c false otherwise.
     * It is used to drive the selection of value types for MQTT properties at
     * compile time.
     * @tparam Id   The property identifier to test.
     * @tparam Ids  The list of property identifiers to compare against.
     */
    template<PropertyIdentifier Id, PropertyIdentifier... Ids>
    inline constexpr bool id_is_one_of = ((Id == Ids) || ...);

    /**
     * @brief Compile-time mapping from a property identifier to its value type.
     *
     * This traits struct maps a given MQTT @c PropertyIdentifier to the C++ type
     * used to store its value. The mapping follows the MQTT 5.0 specification:
     * - Some identifiers map to @c uint32_t
     * - Others to @c uint16_t or @c uint8_t
     * - String-like properties map to @c std::string
     * - Binary properties map to @c std::vector<uint8_t>
     * - User properties map to @c std::pair<std::string, std::string>
     *
     * For identifiers that are not supported, the @c type alias resolves to
     * @c void. Use the alias template ::property_value_type_t for convenience.
     *
     * @tparam Id The property identifier whose associated value type is queried.
     */
    template<PropertyIdentifier Id>
    struct property_value_type
    {
        using type = std::conditional_t<
            id_is_one_of<
                Id,
                PropertyIdentifier::MessageExpiryInterval,
                PropertyIdentifier::SubscriptionIdentifier,
                PropertyIdentifier::SessionExpiryInterval,
                PropertyIdentifier::WillDelayInterval,
                PropertyIdentifier::MaximumPacketSize>,
            uint32_t,
            std::conditional_t<
                id_is_one_of<
                    Id,
                    PropertyIdentifier::ServerKeepAlive,
                    PropertyIdentifier::ReceiveMaximum,
                    PropertyIdentifier::TopicAliasMaximum,
                    PropertyIdentifier::TopicAlias>,
                uint16_t,
                std::conditional_t<
                    id_is_one_of<
                        Id,
                        PropertyIdentifier::PayloadFormatIndicator,
                        PropertyIdentifier::RequestProblemInformation,
                        PropertyIdentifier::RequestResponseInformation,
                        PropertyIdentifier::MaximumQoS,
                        PropertyIdentifier::RetainAvailable,
                        PropertyIdentifier::WildcardSubscriptionAvailable,
                        PropertyIdentifier::SubscriptionIdentifierAvailable,
                        PropertyIdentifier::SharedSubscriptionAvailable>,
                    uint8_t,
                    std::conditional_t<
                        id_is_one_of<
                            Id,
                            PropertyIdentifier::ContentType,
                            PropertyIdentifier::ResponseTopic,
                            PropertyIdentifier::AssignedClientIdentifier,
                            PropertyIdentifier::AuthenticationMethod,
                            PropertyIdentifier::ResponseInformation,
                            PropertyIdentifier::ServerReference,
                            PropertyIdentifier::ReasonString>,
                        std::string,
                        std::conditional_t<
                            id_is_one_of<Id, PropertyIdentifier::CorrelationData, PropertyIdentifier::AuthenticationData>,
                            std::vector<uint8_t>,
                            std::conditional_t<Id == PropertyIdentifier::UserProperty, std::pair<std::string, std::string>, void>>>>>>;
    };

    template<PropertyIdentifier Id>
    using property_value_type_t = property_value_type<Id>::type;

    template<PropertyIdentifier Id, typename T>
    concept PropertyValueFor = std::same_as<std::decay_t<T>, property_value_type_t<Id>>
        || (std::same_as<property_value_type_t<Id>, std::string> && std::convertible_to<T, std::string>);

    bool operator==(
        const std::
            variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::string>>&
                lhs,
        const std::
            variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::string>>&
                rhs);

    /**
     * @brief Wrapper for MQTT properties.
     * MQTT 5.0: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc3901027
     */
    class Property final : public serialize::ISerializable
    {
    private:
        using PropertyData = std::
            variant<std::monostate, uint8_t, uint16_t, uint32_t, std::vector<uint8_t>, std::string, std::pair<std::string, std::string>>;

        template<typename TData>
        explicit Property(const PropertyIdentifier inIdentifier, TData&& inValue)
            : m_data{ std::forward<TData>(inValue) }
            , m_identifier{ inIdentifier }
        {
        }

    public:
        static constexpr auto kInvalidPropertyIdentifier = "Invalid property identifier";
        static constexpr auto kFailedToReadPropertyError = "Failed to read property.";

        Property() = delete;

        /**
         * @brief Factory method to create a Property from a specific identifier and value.
         * @return a Property for the of the specified value
         */
        template<PropertyIdentifier TIdentifier, typename TData>
            requires PropertyValueFor<TIdentifier, TData>
        static Property create(TData&& inValue)
        {
            using Value = property_value_type_t<TIdentifier>;
            return Property(TIdentifier, Value(std::forward<TData>(inValue)));
        }

        /**
         * @brief Factory method to create a Property from a specific identifier and value.
         * @return a Property for the of the specified value
         */
        template<PropertyIdentifier TIdentifier, typename TData>
            requires PropertyValueFor<TIdentifier, TData>
        static Property create(const TData& inValue)
        {
            using Value = property_value_type_t<TIdentifier>;
            return Property(TIdentifier, Value(inValue));
        }

        /**
         * @brief Constructor from ByteReader for deserialization.
         */
        explicit Property(serialize::ByteReader& inReader);

        bool operator==(const Property& other) const
        {
            return m_identifier == other.m_identifier && m_data == other.m_data;
        }

        /**
         * @brief Encode the property to a ByteWriter.
         */
        void encode(serialize::ByteWriter& w) const override;

        /**
         * @brief Decode the property from a ByteReader.
         */
        bool decode(serialize::ByteReader& r) override;

        /**
         * @brief Get the property identifier.
         */
        [[nodiscard]] PropertyIdentifier getIdentifier() const
        {
            return m_identifier;
        }

        /**
         * @brief Get the total length of the property (identifier + data).
         */
        [[nodiscard]] uint32_t getLength() const;

        /**
         * @brief Try to get the property value as uint8_t.
         */
        bool tryGetValue(uint8_t& outValue) const;

        /**
         * @brief Try to get the property value as uint16_t.
         */
        bool tryGetValue(uint16_t& outValue) const;

        /**
         * @brief Try to get the property value as uint32_t.
         */
        bool tryGetValue(uint32_t& outValue) const;

        /**
         * @brief Try to get the property value as binary data.
         */
        bool tryGetValue(std::vector<uint8_t>& outValue) const;

        /**
         * @brief Try to get the property value as string.
         */
        bool tryGetValue(std::string& outValue) const;

        /**
         * @brief Try to get the property value as string pair (user property).
         */
        bool tryGetValue(std::pair<std::string, std::string>& outValue) const;

        /**
         * @brief Convert property to string for debugging.
         */
        [[nodiscard]] std::string toString() const;

    private:
        PropertyData m_data;
        PropertyIdentifier m_identifier;

        /**
         * @brief Decode property identifier from reader.
         */
        static PropertyIdentifier decodePropertyIdentifier(serialize::ByteReader& reader);
    };
} // namespace reactormq::mqtt::packets::properties