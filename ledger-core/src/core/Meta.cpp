#include <core/api/PoolConfiguration.hpp>
#include <core/Meta.hpp>

namespace ledger {
    namespace core {
        Meta::Meta(
            const std::string &name,
            const std::string &password,
            const std::shared_ptr<api::HttpClient> &httpClient,
            const std::shared_ptr<api::WebSocketClient> &webSocketClient,
            const std::shared_ptr<api::PathResolver> &pathResolver,
            const std::shared_ptr<api::LogPrinter> &logPrinter,
            const std::shared_ptr<api::ThreadDispatcher> &dispatcher,
            const std::shared_ptr<api::RandomNumberGenerator> &rng,
            const std::shared_ptr<api::DatabaseBackend> &backend,
            const std::shared_ptr<api::DynamicObject> &configuration
        ): DedicatedContext(dispatcher->getSerialExecutionContext(fmt::format("pool_queue_{}", name))) {
            // General
            _poolName = name;

            _configuration = std::static_pointer_cast<DynamicObject>(configuration);

            // File system management
            _pathResolver = pathResolver;

            // HTTP management
            _httpEngine = httpClient;

            // WS management
            _wsClient = std::make_shared<WebSocketClient>(webSocketClient);

            // Preferences management
            _externalPreferencesBackend = std::make_shared<PreferencesBackend>(
                fmt::format("/{}/preferences.db", _poolName),
                getContext(),
                _pathResolver
            );
            _internalPreferencesBackend = std::make_shared<PreferencesBackend>(
                fmt::format("/{}/__preferences__.db", _poolName),
                getContext(),
                _pathResolver
            );

            _rng = rng;
            // Encrypt the preferences, if needed
            _password = password;
            if (!_password.empty()) {
                _externalPreferencesBackend->setEncryption(_rng, _password);
                _internalPreferencesBackend->setEncryption(_rng, _password);
            }

            // Logger management
            _logPrinter = logPrinter;
            auto enableLogger = _configuration->getBoolean(api::PoolConfiguration::ENABLE_INTERNAL_LOGGING).value_or(true);
            _logger = logger::create(
                    name + "-l",
                    dispatcher->getSerialExecutionContext(fmt::format("logger_queue_{}", name)),
                    pathResolver,
                    logPrinter,
                    logger::DEFAULT_MAX_SIZE,
                    enableLogger
            );

            // Database management
            _database = std::make_shared<DatabaseSessionPool>(
               std::static_pointer_cast<DatabaseBackend>(backend),
               pathResolver,
               _logger,
               Option<std::string>(configuration->getString(api::PoolConfiguration::DATABASE_NAME)).getValueOr(name),
               password
            );

            // Threading management
            _threadDispatcher = dispatcher;

            _publisher = std::make_shared<EventPublisher>(getContext());
        }

        std::shared_ptr<Meta> Meta::newInstance(
            const std::string &name,
            const std::string &password,
            const std::shared_ptr<api::HttpClient> &httpClient,
            const std::shared_ptr<api::WebSocketClient> &webSocketClient,
            const std::shared_ptr<api::PathResolver> &pathResolver,
            const std::shared_ptr<api::LogPrinter> &logPrinter,
            const std::shared_ptr<api::ThreadDispatcher> &dispatcher,
            const std::shared_ptr<api::RandomNumberGenerator> &rng,
            const std::shared_ptr<api::DatabaseBackend> &backend,
            const std::shared_ptr<api::DynamicObject> &configuration
        ) {
            auto meta = std::shared_ptr<Meta>(new Meta(
                name,
                password,
                httpClient,
                webSocketClient,
                pathResolver,
                logPrinter,
                dispatcher,
                rng,
                backend,
                configuration
            ));

            return meta;
        }

        std::shared_ptr<Preferences> Meta::getExternalPreferences() const {
            return _externalPreferencesBackend->getPreferences("pool");
        }

        std::shared_ptr<Preferences> Meta::getInternalPreferences() const {
            return _internalPreferencesBackend->getPreferences("pool");
        }

        std::shared_ptr<spdlog::logger> Meta::logger() const {
            return _logger;
        }

        std::shared_ptr<DatabaseSessionPool> Meta::getDatabaseSessionPool() const {
            return _database;
        }

        std::shared_ptr<DynamicObject> Meta::getConfiguration() const {
            return _configuration;
        }

        const std::string &Meta::getName() const {
            return _poolName;
        }

        const std::string Meta::getPassword() const {
            return _password;
        }

        std::shared_ptr<api::PathResolver> Meta::getPathResolver() const {
            return _pathResolver;
        }

        std::shared_ptr<api::RandomNumberGenerator> Meta::rng() const {
            return _rng;
        }

        std::shared_ptr<api::ThreadDispatcher> Meta::getDispatcher() const {
            return _threadDispatcher;
        }

        std::shared_ptr<HttpClient> Meta::getHttpClient(const std::string &baseUrl) {
            auto it = _httpClients.find(baseUrl);
            if (it == _httpClients.end() || !it->second.lock()) {
                auto client = std::make_shared<HttpClient>(
                    baseUrl,
                    _httpEngine,
                    getDispatcher()->getThreadPoolExecutionContext(fmt::format("http_clients"))
                );
                _httpClients[baseUrl] = client;
                client->setLogger(logger());
                return client;
            }
            auto client = _httpClients[baseUrl].lock();
            if (!client) {
                throw make_exception(api::ErrorCode::NULL_POINTER, "HttpClient was released.");
            }
            return client;
        }

        std::shared_ptr<api::EventBus> Meta::getEventBus() const {
            return _publisher->getEventBus();
        }

        std::shared_ptr<WebSocketClient> Meta::getWebSocketClient() const {
            return _wsClient;
        }

        Future<api::ErrorCode> Meta::changePassword(
            const std::string& oldPassword,
            const std::string& newPassword
        ) {
            auto self = shared_from_this();

            return async<api::ErrorCode>([=]() {
                self->getDatabaseSessionPool()->performChangePassword(oldPassword, newPassword);
                self->_externalPreferencesBackend->resetEncryption(_rng, oldPassword, newPassword);
                self->_internalPreferencesBackend->resetEncryption(_rng, oldPassword, newPassword);
                return api::ErrorCode::FUTURE_WAS_SUCCESSFULL;
            });
        }
    }
}
