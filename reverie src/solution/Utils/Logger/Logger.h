#pragma once

struct Logger_t {
    void Attach(const char* title);
    void Detach();

    template <typename... Args>
    void Print(std::string_view fmt, Args&&... args) {
        try {
            if constexpr (sizeof...(args) > 0) {
                const auto message = std::vformat(fmt, std::make_format_args(args...));
                std::cout << message << std::endl;
            } else {
                std::cout << fmt << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[error] " << e.what() << std::endl;
        }
    }
private:
    FILE* file_out{ nullptr };
    FILE* file_in{ nullptr };
};

inline Logger_t* Logger = new Logger_t();

#define Log(fmt, ...) Logger->Print(fmt, ##__VA_ARGS__)
