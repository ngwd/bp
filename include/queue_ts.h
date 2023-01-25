#pragma once
#include <mutex>
#include <queue>

// Thread safe implementation of a Queue using an std::queue
template <typename T>
class queue_ts
{
private:
	std::queue<T> queue_;
	std::mutex mutex_;
public:
	queue_ts() { }
	queue_ts(queue_ts& other) { } 
	~queue_ts() { }

	bool empty() 
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return queue_.empty();
	}
	
	int size() 
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return queue_.size();
	}

	void push(T& t) 
	{
		std::unique_lock<std::mutex> lock(mutex_);
		queue_.push(t);
	}
	
	bool pop(T& t) 
	{
		std::unique_lock<std::mutex> lock(mutex_);

		if (queue_.empty()) 
		{
			return false;
		}
		t = std::move(queue_.front());
		
		queue_.pop();
		return true;
	}
};
