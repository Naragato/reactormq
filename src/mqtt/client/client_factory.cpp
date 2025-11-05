#include "client_factory.h"
#include "client_impl.h"

namespace reactormq::mqtt::client
{
    std::shared_ptr<IClient> createClient(ConnectionSettingsPtr settings)
    {
        return std::make_shared<ClientImpl>(std::move(settings));
    }
} // namespace reactormq::mqtt::client