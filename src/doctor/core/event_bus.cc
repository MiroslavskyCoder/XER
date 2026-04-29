#include "core/event_bus.h"

#include <algorithm>
#include <utility>

namespace EngineDoctor {
namespace {

bool TopicMatches(std::string_view subscription_topic, std::string_view event_topic) {
	return subscription_topic.empty() || subscription_topic == "*" || subscription_topic == event_topic;
}

} // namespace

EventBus::SubscriptionId EventBus::subscribe(std::string topic, EventHandler handler) {
	std::lock_guard<std::mutex> lock(mutex_);
	const SubscriptionId id = next_subscription_id_++;
	subscriptions_.push_back(Subscription{id, std::move(topic), std::move(handler)});
	return id;
}

bool EventBus::unsubscribe(SubscriptionId id) {
	std::lock_guard<std::mutex> lock(mutex_);
	const auto previous_size = subscriptions_.size();
	subscriptions_.erase(
		std::remove_if(subscriptions_.begin(), subscriptions_.end(), [id](const Subscription& subscription) {
			return subscription.id == id;
		}),
		subscriptions_.end());
	return subscriptions_.size() != previous_size;
}

void EventBus::publish(std::string topic,
			      std::string message,
			      std::unordered_map<std::string, std::string> attributes) {
	Event event;
	event.topic = std::move(topic);
	event.message = std::move(message);
	event.attributes = std::move(attributes);
	publish(std::move(event));
}

void EventBus::publish(Event event) {
	std::vector<EventHandler> handlers;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		event.sequence = next_sequence_++;
		history_.push_back(event);
		for (const Subscription& subscription : subscriptions_) {
			if (TopicMatches(subscription.topic, event.topic) && subscription.handler) {
				handlers.push_back(subscription.handler);
			}
		}
	}
	for (const EventHandler& handler : handlers) {
		handler(event);
	}
}

bool EventBus::has_subscribers(std::string_view topic) const {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const Subscription& subscription : subscriptions_) {
		if (TopicMatches(subscription.topic, topic)) {
			return true;
		}
	}
	return false;
}

std::vector<Event> EventBus::history(std::string_view topic, std::size_t limit) const {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<Event> events;
	for (const Event& event : history_) {
		if (topic.empty() || event.topic == topic) {
			events.push_back(event);
		}
	}
	if (limit > 0 && events.size() > limit) {
		events.erase(events.begin(), events.end() - static_cast<std::ptrdiff_t>(limit));
	}
	return events;
}

std::optional<Event> EventBus::latest(std::string_view topic) const {
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto iterator = history_.rbegin(); iterator != history_.rend(); ++iterator) {
		if (topic.empty() || iterator->topic == topic) {
			return *iterator;
		}
	}
	return std::nullopt;
}

void EventBus::clear_history() {
	std::lock_guard<std::mutex> lock(mutex_);
	history_.clear();
}

} // namespace EngineDoctor
