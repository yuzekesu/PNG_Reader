#pragma once
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

/// <summary>
/// Virtual base class for polymorphism.
/// </summary>
class RawMemory {
public:
	virtual std::string what() = 0;
protected:
	RawMemory() = default;
};

/// <summary>
/// Container for the memory that you want to monitor. 
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class Memory : public RawMemory {
public:
	Memory() = delete;
	Memory(T& ref, const char* description = "", const char* suffix = "");
	std::string what() override;
private:
	std::string m_description;
	T* m_p_value;
	std::string m_suffix;
};

/// <summary>
/// Class that creates a terminal and updates the monitored memory in a certain interval.
/// </summary>
class MemoryTracker {
public:
	template <typename ...Memory_T>
	MemoryTracker(unsigned int interval_milli, Memory_T& ...Memories);
	~MemoryTracker();
	template <typename ...Memory_T>
	void Add(Memory_T& ...Memories);
	void Stop();
private:
	void Update();
private:
	std::atomic_bool m_running = true;
	std::chrono::duration<unsigned int, std::milli> m_interval_milli;
	std::mutex m_queue_mutex;
	std::vector<std::unique_ptr<RawMemory>> m_queue;
	std::thread m_thread;
	FILE* m_new_output = nullptr;
};

//**********************************************
// Template Implementation
//**********************************************
/// <summary> 
/// Constructor of Memory. It defines how the memory will be displayed in the terminal.
/// </summary>
template<typename T>
inline Memory<T>::Memory(T& ref, const char* description, const char* suffix) {
	m_description = description;
	m_p_value = &ref;
	m_suffix = suffix;
}
/// <summary>
/// Formats the string to be displayed in the terminal. 
/// </summary>
template<typename T>
inline std::string Memory<T>::what() {
	std::stringstream ss;
	ss << m_description << ": " << *m_p_value << m_suffix << "\n";
	return ss.str();
}
/// <summary>
/// Special template for uint8_t so it display number instead of ASCII character. 
/// </summary>
template<>
inline std::string Memory<uint8_t>::what() {
	std::stringstream ss;
	ss << m_description << ": " << static_cast<unsigned int>(*m_p_value) << m_suffix << "\n";
	return ss.str();
}
/// <summary>
/// Constructor of MemoryTracker. It creates a terminal in a thread and updates the monitored memory in a certain interval. 
/// </summary>
template<typename ...Memory_T>
inline MemoryTracker::MemoryTracker(unsigned int interval_milli, Memory_T& ...Memories) {
	(m_queue.push_back(std::make_unique<Memory_T>(Memories)), ...);
	m_interval_milli = std::chrono::duration<unsigned int, std::milli>(interval_milli);
	if (GetConsoleWindow() == NULL) {
		AllocConsole();
		freopen_s(&m_new_output, "CONOUT$", "w", stdout);
	}
	m_thread = std::thread(&MemoryTracker::Update, this);
}

template<typename ...Memory_T>
inline void MemoryTracker::Add(Memory_T & ...Memories) {
	std::lock_guard<std::mutex> lock(m_queue_mutex);
	(m_queue.push_back(std::make_unique<Memory_T>(Memories)), ...);
}
