#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace EngineDoctor {

struct Event {
	std::size_t sequence = 0;
	std::string topic;
	std::string message;
	std::unordered_map<std::string, std::string> attributes;
};

using EventHandler = std::function<void(const Event&)>;

class EventBus {
public:
	using SubscriptionId = std::size_t;

	SubscriptionId subscribe(std::string topic, EventHandler handler);
	bool unsubscribe(SubscriptionId id);

	void publish(std::string topic,
			 std::string message,
			 std::unordered_map<std::string, std::string> attributes = {});
	void publish(Event event);

	bool has_subscribers(std::string_view topic) const;
	std::vector<Event> history(std::string_view topic = {}, std::size_t limit = 0) const;
	std::optional<Event> latest(std::string_view topic = {}) const;
	void clear_history();

private:
	struct Subscription {
		SubscriptionId id = 0;
		std::string topic;
		EventHandler handler;
	};

	mutable std::mutex mutex_;
	std::vector<Subscription> subscriptions_;
	std::vector<Event> history_;
	SubscriptionId next_subscription_id_ = 1;
	std::size_t next_sequence_ = 1;
};

} // namespace EngineDoctor
