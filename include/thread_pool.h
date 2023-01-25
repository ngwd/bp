#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "queue_ts.h"

class thread_pool 
{
private:
	class thread_worker 
	{
	private:
		int id_;
		thread_pool * pool_;
	public:
		thread_worker(thread_pool * pool, const int id) : pool_(pool), id_(id) { }

		void operator()() 
		{
			std::function<void()> func;
			bool popped = false;
			while (!pool_->shutdown_) 
			{
				{
					std::unique_lock<std::mutex> lock(pool_->conditional_mutex_);
					if (pool_->queue_.empty()) {
						pool_->conditional_lock_.wait(lock);
					}
					popped = pool_->queue_.pop(func);
				}
				if (popped) { func(); }
			}
		}
	}; // end of thread_worker

	bool shutdown_;
	queue_ts<std::function<void()>> queue_;
	std::vector<std::thread> threads_;
	std::mutex conditional_mutex_;
	std::condition_variable conditional_lock_;

	void init() 
	{
		for (int i = 0; i < threads_.size(); ++i) 
		{
			threads_[i] = std::thread(thread_worker(this, i));
		}
	}

	void shutdown() 
	{
		shutdown_ = true;
		conditional_lock_.notify_all();
		
		for (int i = 0; i < threads_.size(); ++i) 
		{
			// wait until thread finish their current task, then join 
			if(threads_[i].joinable()) 
			{
				threads_[i].join(); 
			}
		}
	}
public:
	thread_pool(const int n_threads) : threads_(std::vector<std::thread>(n_threads)), shutdown_(false) 
	{
		init();
	}

	// submit a function to be executed asynchronously by the pool
	template<typename F, typename...Args>
	auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> 
	{
		// Create a function with bounded parameters ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

		// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		// Wrap packaged task into void function
		std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };

		// Enqueue generic wrapper function
		queue_.push(wrapper_func);

		// Wake up one thread if its waiting
		conditional_lock_.notify_one();

		// Return future from promise
		return task_ptr->get_future();
	};

/*
	// submit a class member function to be executed asynchronously by the pool
	template<typename F, typename Obj, typename...Args>
	auto submit2(F&& f,  Obj&& o, Args&&... args) 
		-> std::future<decltype((&Obj::(std::forward<F>(f))(args...))> 
	{
		// Create a function with bounded parameters ready to execute
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), o, std::forward<Args>(args)...);

		// Encapsulate it into a shared ptr in order to be able to copy construct / assign 
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

		// Wrap packaged task into void function
		std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };

		// Enqueue generic wrapper function
		queue_.push(wrapper_func);

		// Wake up one thread if its waiting
		conditional_lock_.notify_one();

		// Return future from promise
		return task_ptr->get_future();
	};
*/
	thread_pool(const thread_pool &) = delete;
	thread_pool & operator=(const thread_pool &) = delete;
	thread_pool(thread_pool &&) = delete;
	thread_pool & operator=(thread_pool &&) = delete;
	~thread_pool() { shutdown(); }
};
