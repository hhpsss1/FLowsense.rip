#include "ThreadPool.h"
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max

void CThreadPool::Init() {
	std::lock_guard lock(run_mutex);
	if (!workers.empty())
		return;

	auto tier0 = GetModuleHandleW(L"tier0.dll");
	create_thread = reinterpret_cast<CreateThread_t>(GetProcAddress(tier0, "CreateSimpleThread"));
	join_thread = reinterpret_cast<JoinThread_t>(GetProcAddress(tier0, "ThreadJoin"));
	release_thread = reinterpret_cast<ReleaseThread_t>(GetProcAddress(tier0, "ReleaseThreadHandle"));
	if (!create_thread || !join_thread || !release_thread)
		return;

	unsigned int available = std::thread::hardware_concurrency();
	if (available <= 1)
		return;

	size_t count = std::min(available - 1, 4u);
	try {
		workers.reserve(count);
		for (size_t i = 0; i < count; i++) {
			auto thread = create_thread(reinterpret_cast<void*>(&ThreadEntry), this, nullptr, nullptr, nullptr, nullptr, nullptr);
			if (thread)
				workers.emplace_back(thread);
		}
	}
	catch (...) {	}
}

uintptr_t __fastcall CThreadPool::ThreadEntry(void* context) {
	static_cast<CThreadPool*>(context)->Worker();
	return 0;
}

void CThreadPool::Execute() {
	for (;;) {
		size_t index = next_index.fetch_add(1, std::memory_order_relaxed);
		if (index >= item_count)
			return;

		try {
			job(index);
		}
		catch (...) {
			std::lock_guard lock(state_mutex);
			if (!exception)
				exception = std::current_exception();
		}
	}
}

void CThreadPool::Worker() {
	uint64_t previous = 0;
	for (;;) {
		std::unique_lock lock(state_mutex);
		work_ready.wait(lock, [&] { return stopping || generation != previous; });
		if (stopping)
			return;

		previous = generation;
		if (next_index.load(std::memory_order_relaxed) >= item_count)
			continue;
		++pending_workers;
		lock.unlock();
		Execute();
		lock.lock();
		if (--pending_workers == 0)
			work_finished.notify_one();
	}
}

void CThreadPool::Run(size_t count, const std::function<void(size_t)>& function) {
	if (!count)
		return;

	std::lock_guard run_lock(run_mutex);
	bool parallel = count >= 2 && !workers.empty();
	{
		std::lock_guard lock(state_mutex);
		job = function;
		exception = nullptr;
		item_count = count;
		next_index.store(0, std::memory_order_relaxed);
		pending_workers = 0;
		if (parallel)
			generation++;
	}

	if (parallel)
		work_ready.notify_all();

	Execute();
	std::unique_lock lock(state_mutex);
	work_finished.wait(lock, [&] { return pending_workers == 0; });
	job = {};
	auto failure = exception;
	exception = nullptr;
	lock.unlock();
	if (failure)
		std::rethrow_exception(failure);
}

void CThreadPool::Clear() {
	std::lock_guard run_lock(run_mutex);
	{
		std::lock_guard lock(state_mutex);
		stopping = true;
	}
	work_ready.notify_all();

	for (auto worker : workers) {
		join_thread(worker, 0xFFFFFFFF);
		release_thread(worker);
	}

	workers.clear();
	std::lock_guard lock(state_mutex);
	stopping = false;
	generation = 0;
	item_count = 0;
	pending_workers = 0;
	job = {};
	exception = nullptr;
}

CThreadPool* ThreadPool = new CThreadPool;
