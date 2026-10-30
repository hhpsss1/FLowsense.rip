#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class CThreadPool {
	friend struct ThreadPoolTestAccess;
private:
	using CreateThread_t = void* (__fastcall*)(void*, void*, void*, void*, void*, void*, void*);
	using JoinThread_t = bool (__fastcall*)(void*, unsigned long);
	using ReleaseThread_t = bool (__fastcall*)(void*);

	CreateThread_t create_thread = nullptr;
	JoinThread_t join_thread = nullptr;
	ReleaseThread_t release_thread = nullptr;
	std::vector<void*> workers;
	std::mutex run_mutex;
	std::mutex state_mutex;
	std::condition_variable work_ready;
	std::condition_variable work_finished;
	std::function<void(size_t)> job;
	std::exception_ptr exception;
	std::atomic<size_t> next_index = 0;
	size_t item_count = 0;
	size_t pending_workers = 0;
	uint64_t generation = 0;
	bool stopping = false;

	void Worker();
	static uintptr_t __fastcall ThreadEntry(void* context);
	void Execute();

public:
	bool HasWorkers() const { return !workers.empty(); }
	size_t WorkerCount() const { return workers.size(); }
	void Init();
	// Jobs use independent data and must not call this pool's methods.
	void Run(size_t count, const std::function<void(size_t)>& function);
	void Clear();
};

extern CThreadPool* ThreadPool;
