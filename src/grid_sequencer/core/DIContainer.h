#pragma once
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <stdexcept>

namespace GridSequencer {
namespace Core {

// Result type for error handling
template<typename T>
class Result {
public:
    static Result success(T value) {
        return Result(std::move(value), true);
    }

    static Result error(std::string message) {
        return Result(std::move(message), false);
    }

    bool isSuccess() const { return hasValue_; }
    bool isError() const { return !hasValue_; }

    const T& value() const {
        if (!hasValue_) throw std::runtime_error("Accessing value of error result");
        return value_;
    }

    const std::string& error() const {
        if (hasValue_) throw std::runtime_error("Accessing error of success result");
        return error_;
    }

private:
    Result(T value, bool success) : value_(std::move(value)), hasValue_(success) {}
    Result(std::string error, bool success) : error_(std::move(error)), hasValue_(success) {}

    T value_;
    std::string error_;
    bool hasValue_;
};

// Simple dependency injection container
class DIContainer {
public:
    // Register a singleton instance
    template<typename Interface, typename Implementation, typename... Args>
    void registerSingleton(Args&&... args) {
        auto instance = std::make_shared<Implementation>(std::forward<Args>(args)...);
        services_[std::type_index(typeid(Interface))] = instance;
    }

    // Register a factory function
    template<typename Interface>
    void registerFactory(std::function<std::shared_ptr<Interface>()> factory) {
        factories_[std::type_index(typeid(Interface))] = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
    }

    // Resolve a service
    template<typename T>
    std::shared_ptr<T> resolve() {
        auto typeIndex = std::type_index(typeid(T));

        // Try singleton first
        auto it = services_.find(typeIndex);
        if (it != services_.end()) {
            return std::static_pointer_cast<T>(it->second);
        }

        // Try factory
        auto factoryIt = factories_.find(typeIndex);
        if (factoryIt != factories_.end()) {
            auto instance = factoryIt->second();
            return std::static_pointer_cast<T>(instance);
        }

        return nullptr;
    }

    // Check if service is registered
    template<typename T>
    bool isRegistered() {
        auto typeIndex = std::type_index(typeid(T));
        return services_.find(typeIndex) != services_.end() ||
               factories_.find(typeIndex) != factories_.end();
    }

    // Clear all registrations
    void clear() {
        services_.clear();
        factories_.clear();
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> factories_;
};

} // namespace Core
} // namespace GridSequencer