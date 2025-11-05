#include "client_factory.h"
#include "client_impl.h"

namespace reactormq::mqtt::client
{
    std::shared_ptr<IClient> createClient(const ConnectionSettingsPtr& settings)
    {
        return std::make_shared<ClientImpl>(settings);
    }
} // namespace reactormq::mqtt::client