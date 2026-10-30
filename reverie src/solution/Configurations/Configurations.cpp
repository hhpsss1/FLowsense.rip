#include <shared.h>

namespace Binds {
    void SaveToConfig();
    void LoadFromConfig();
    void ResetAll();
}

namespace fs = std::filesystem;

static constexpr std::size_t max_configs = 5;

static std::unordered_map<int, std::pair<std::string, std::string>> legacy_bind_targets;

static std::size_t CountCfgFiles() {
    std::size_t count{};

    const fs::path dir = Config::base_path / xs("configs");
    if (!fs::exists(dir))
        return 0;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == xs(".cfg"))
            ++count;
    }

    return count;
}

static void ProcessBufferXor(std::vector<BYTE>& data) {
    static const BYTE xor_key[] = { 0x5E, 0x8A, 0x11, 0x93, 0x3F, 0xB2, 0x71, 0xC4 };
    const size_t key_size = sizeof(xor_key);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] ^= xor_key[i % key_size];
}

static bool ProtectText(const std::string& plain, std::vector<BYTE>& encrypted) {
    encrypted.assign(plain.begin(), plain.end());
    ProcessBufferXor(encrypted);
    return true;
}

static bool UnprotectText(const std::vector<BYTE>& encrypted, std::string& plain) {
    std::vector<BYTE> temp = encrypted;
    ProcessBufferXor(temp);
    plain.assign(temp.begin(), temp.end());
    return true;
}

static std::vector<BYTE> ReadBinaryFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return {};

    return std::vector<BYTE>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static bool WriteBinaryFile(const fs::path& path, const std::vector<BYTE>& data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return false;

    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return true;
}

static std::string ToLower(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static std::string Trim(std::string s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
        s.erase(s.begin());

    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.pop_back();

    return s;
}

static std::uint64_t UnixNow() {
    return static_cast<std::uint64_t>(std::time(nullptr));
}

static std::uint64_t FileCreationTime(const fs::path& path) {
    HANDLE file = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return 0;

    FILETIME creation{};
    if (!GetFileTime(file, &creation, nullptr, nullptr)) {
        CloseHandle(file);
        return 0;
    }

    CloseHandle(file);

    ULARGE_INTEGER ui{};
    ui.LowPart = creation.dwLowDateTime;
    ui.HighPart = creation.dwHighDateTime;

    if (ui.QuadPart < 116444736000000000ULL)
        return 0;

    return (ui.QuadPart - 116444736000000000ULL) / 10000000ULL;
}

static void* GetVariableRawPtr(VariableObject_t& var) {
    if (var.type_name == xs("bool")) return std::any_cast<bool>(&var.value);
    if (var.type_name == xs("int")) return std::any_cast<int>(&var.value);
    if (var.type_name == xs("float")) return std::any_cast<float>(&var.value);
    if (var.type_name == xs("Color_t") || var.type_name == xs("color_t")) return std::any_cast<Color_t>(&var.value);
    if (var.type_name == xs("std::string")) return std::any_cast<std::string>(&var.value);
    if (var.type_name == xs("std::vector<bool>")) return std::any_cast<std::vector<bool>>(&var.value);
    if (var.type_name == xs("std::vector<int>")) return std::any_cast<std::vector<int>>(&var.value);
    if (var.type_name == xs("std::vector<float>")) return std::any_cast<std::vector<float>>(&var.value);
    return nullptr;
}

int Config::GetIndexFromPtr(const void* ptr) {
    if (!ptr)
        return -1;

    for (std::size_t i = 0; i < variables.size(); ++i) {
        if (GetVariableRawPtr(variables[i]) == ptr)
            return static_cast<int>(i);
    }

    return -1;
}

void* Config::GetPtrFromName(const std::string& name, const std::string& type_name) {
    VariableObject_t* match = nullptr;
    for (auto& var : variables) {
        if (var.name != name)
            continue;

        if (match)
            return nullptr;
        match = &var;
    }

    return match && match->type_name == type_name ? GetVariableRawPtr(*match) : nullptr;
}

void* Config::GetLegacyBindPtr(int idx, const std::string& type_name) {
    const auto it = legacy_bind_targets.find(idx);
    if (it == legacy_bind_targets.end())
        return nullptr;

    const bool legacy_stop = it->second.first.ends_with(xs("_AutoStop")) && it->second.second == xs("int") && type_name == xs("std::vector<bool>");
    if (it->second.second != type_name && !legacy_stop)
        return nullptr;

    return GetPtrFromName(it->second.first, type_name);
}

static std::string SerializeBool(const bool val) {
    return val ? xs("1") : xs("0");
}

static std::string SerializeFloat(const float val) {
    std::ostringstream ss;
    ss << val;
    return ss.str();
}

static std::string SerializeInt(const int val) {
    return std::to_string(val);
}

static std::string SerializeColor(const Color_t& val) {
    std::ostringstream ss;
    ss << static_cast<int>(val.r) << xs(",") << static_cast<int>(val.g) << xs(",") << static_cast<int>(val.b) << xs(",") << static_cast<int>(val.a);
    return ss.str();
}

static std::string SerializeVectorBool(const std::vector<bool>& val) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (i)
            ss << xs(",");
        ss << (val[i] ? 1 : 0);
    }
    return ss.str();
}

static std::string SerializeVectorInt(const std::vector<int>& val) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (i)
            ss << xs(",");
        ss << val[i];
    }
    return ss.str();
}

static std::string SerializeVectorFloat(const std::vector<float>& val) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (i)
            ss << xs(",");
        ss << val[i];
    }
    return ss.str();
}

static std::string SerializeVectorVecBool(const std::vector<std::vector<bool>>& val) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (i)
            ss << xs(";");
        ss << SerializeVectorBool(val[i]);
    }
    return ss.str();
}

static std::string SerializeVectorVecInt(const std::vector<std::vector<int>>& val) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (i)
            ss << xs(";");
        ss << SerializeVectorInt(val[i]);
    }

    return ss.str();
}

static bool ParseBool(const std::string& val) {
    std::string s = ToLower(Trim(val));
    return s == xs("1") || s == xs("true");
}

static float ParseFloat(const std::string& val, float def = 0.f) {
    if (val.empty()) 
        return def;

    try {
        return std::stof(val);
    } catch (...) {
        return def;
    }
}

static int ParseInt(const std::string& val, int def = 0) {
    if (val.empty())
        return def;

    try {
        return std::stoi(val);
    } catch (...) {
        return def;
    }
}

static Color_t ParseColor(const std::string& val) {
    std::vector<int> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ','))
        out.push_back(ParseInt(token, 255));

    if (out.size() == 4)
        return Color_t(out[0], out[1], out[2], out[3]);

    return Color_t(255, 255, 255, 255);
}

static std::vector<bool> ParseVectorBool(const std::string& val) {
    std::vector<bool> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ','))
        out.push_back(ParseBool(token));

    return out;
}

static std::vector<int> ParseVectorInt(const std::string& val) {
    std::vector<int> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ','))
        out.push_back(ParseInt(token, 0));

    return out;
}

static std::vector<float> ParseVectorFloat(const std::string& val) {
    std::vector<float> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ','))
        out.push_back(ParseFloat(token, 0.f));

    return out;
}

static std::vector<std::vector<bool>> ParseVectorVecBool(const std::string& val) {
    std::vector<std::vector<bool>> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ';'))
        out.push_back(ParseVectorBool(token));

    return out;
}

static std::vector<std::vector<int>> ParseVectorVecInt(const std::string& val) {
    std::vector<std::vector<int>> out;
    std::istringstream ss(val);
    std::string token;

    while (std::getline(ss, token, ';'))
        out.push_back(ParseVectorInt(token));

    return out;
}

static std::string ReadTextFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in.is_open())
        return {};

    std::string out((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return Trim(out);
}

static bool WriteTextFile(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
        return false;

    out << text;
    return true;
}

static std::string SerializeValue(const VariableObject_t& var) {
    if (var.type_name == xs("bool"))
        return SerializeBool(std::any_cast<bool>(var.value));

    if (var.type_name == xs("float"))
        return SerializeFloat(std::any_cast<float>(var.value));

    if (var.type_name == xs("int"))
        return SerializeInt(std::any_cast<int>(var.value));

    if (var.type_name == xs("color_t") || var.type_name == xs("Color_t"))
        return SerializeColor(std::any_cast<Color_t>(var.value));

    if (var.type_name == xs("std::vector<bool>"))
        return SerializeVectorBool(std::any_cast<std::vector<bool>>(var.value));

    if (var.type_name == xs("std::vector<int>"))
        return SerializeVectorInt(std::any_cast<std::vector<int>>(var.value));

    if (var.type_name == xs("std::vector<float>"))
        return SerializeVectorFloat(std::any_cast<std::vector<float>>(var.value));

    if (var.type_name == xs("std::vector<std::vector<bool>>"))
        return SerializeVectorVecBool(std::any_cast<std::vector<std::vector<bool>>>(var.value));

    if (var.type_name == xs("std::vector<std::vector<int>>"))
        return SerializeVectorVecInt(std::any_cast<std::vector<std::vector<int>>>(var.value));

    if (var.type_name == xs("std::string"))
        return std::any_cast<std::string>(var.value);

    return {};
}

static void ResetValue(VariableObject_t& var) {
    if (var.type_name == xs("bool")) {
        var.Get<bool>() = std::any_cast<bool>(var.default_value);
        return;
    }

    if (var.type_name == xs("float")) {
        var.Get<float>() = std::any_cast<float>(var.default_value);
        return;
    }

    if (var.type_name == xs("int")) {
        var.Get<int>() = std::any_cast<int>(var.default_value);
        return;
    }

    if (var.type_name == xs("color_t") || var.type_name == xs("Color_t")) {
        var.Get<Color_t>() = std::any_cast<Color_t>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::vector<bool>")) {
        var.Get<std::vector<bool>>() = std::any_cast<std::vector<bool>>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::vector<int>")) {
        var.Get<std::vector<int>>() = std::any_cast<std::vector<int>>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::vector<float>")) {
        var.Get<std::vector<float>>() = std::any_cast<std::vector<float>>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::vector<std::vector<bool>>")) {
        var.Get<std::vector<std::vector<bool>>>() = std::any_cast<std::vector<std::vector<bool>>>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::vector<std::vector<int>>")) {
        var.Get<std::vector<std::vector<int>>>() = std::any_cast<std::vector<std::vector<int>>>(var.default_value);
        return;
    }

    if (var.type_name == xs("std::string")) {
        var.Get<std::string>() = std::any_cast<std::string>(var.default_value);
        return;
    }
}

static bool ApplyValue(VariableObject_t& var, const std::string& val) {
    if (var.type_name == xs("bool")) {
        var.Get<bool>() = ParseBool(val);
        return true;
    }

    if (var.type_name == xs("float")) {
        var.Get<float>() = ParseFloat(val);
        return true;
    }

    if (var.type_name == xs("int")) {
        var.Get<int>() = ParseInt(val);
        return true;
    }

    if (var.type_name == xs("color_t") || var.type_name == xs("Color_t")) {
        var.Get<Color_t>() = ParseColor(val);
        return true;
    }

    if (var.type_name == xs("std::vector<bool>")) {
        auto parsed = ParseVectorBool(val);
        const auto& def = std::any_cast<std::vector<bool>>(var.default_value);
        if (parsed.size() < def.size()) {
            size_t old_sz = parsed.size();
            parsed.resize(def.size());
            for (size_t i = old_sz; i < def.size(); ++i)
                parsed[i] = def[i];
        }

        var.Get<std::vector<bool>>() = parsed;
        return true;
    }

    if (var.type_name == xs("std::vector<int>")) {
        auto parsed = ParseVectorInt(val);
        const auto& def = std::any_cast<std::vector<int>>(var.default_value);
        if (parsed.size() < def.size()) {
            size_t old_sz = parsed.size();
            parsed.resize(def.size());
            for (size_t i = old_sz; i < def.size(); ++i)
                parsed[i] = def[i];
        }

        var.Get<std::vector<int>>() = parsed;
        return true;
    }

    if (var.type_name == xs("std::vector<float>")) {
        auto parsed = ParseVectorFloat(val);
        const auto& def = std::any_cast<std::vector<float>>(var.default_value);
        if (parsed.size() < def.size()) {
            size_t old_sz = parsed.size();
            parsed.resize(def.size());
            for (size_t i = old_sz; i < def.size(); ++i)
                parsed[i] = def[i];
        }

        var.Get<std::vector<float>>() = parsed;
        return true;
    }

    if (var.type_name == xs("std::vector<std::vector<bool>>")) {
        const auto& def = std::any_cast<std::vector<std::vector<bool>>>(var.default_value);
        std::vector<std::vector<bool>> parsed;

        if (val.find(';') != std::string::npos) {
            parsed = ParseVectorVecBool(val);
        } else {
            parsed = def;
            if (!val.empty())
                parsed[0] = ParseVectorBool(val);
        }

        if (parsed.size() < def.size()) {
            size_t old_sz = parsed.size();
            parsed.resize(def.size());
            for (size_t i = old_sz; i < def.size(); ++i)
                parsed[i] = def[i];
        }

        for (size_t i = 0; i < parsed.size() && i < def.size(); ++i) {
            if (parsed[i].size() < def[i].size()) {
                size_t old_inner = parsed[i].size();
                parsed[i].resize(def[i].size());
                for (size_t j = old_inner; j < def[i].size(); ++j)
                    parsed[i][j] = def[i][j];
            }
        }

        var.Get<std::vector<std::vector<bool>>>() = parsed;
        return true;
    }

    if (var.type_name == xs("std::vector<std::vector<int>>")) {
        const auto& def = std::any_cast<std::vector<std::vector<int>>>(var.default_value);
        std::vector<std::vector<int>> parsed;

        if (val.find(';') != std::string::npos) {
            parsed = ParseVectorVecInt(val);
        } else {
            parsed = def;
            if (!val.empty())
                parsed[0] = ParseVectorInt(val);
        }

        if (parsed.size() < def.size()) {
            size_t old_sz = parsed.size();
            parsed.resize(def.size());
            for (size_t i = old_sz; i < def.size(); ++i)
                parsed[i] = def[i];
        }
        
        for (size_t i = 0; i < parsed.size() && i < def.size(); ++i) {
            if (parsed[i].size() < def[i].size()) {
                size_t old_inner = parsed[i].size();
                parsed[i].resize(def[i].size());
                for (size_t j = old_inner; j < def[i].size(); ++j)
                    parsed[i][j] = def[i][j];
            }
        }

        var.Get<std::vector<std::vector<int>>>() = parsed;
        return true;
    }

    if (var.type_name == xs("std::string")) {
        var.Get<std::string>() = val;
        return true;
    }

    return false;
}

fs::path Config::GetConfigPath(const std::string& name) {
    return base_path / xs("configs") / (name + xs(".cfg"));
}

fs::path Config::GetAutoloadPath() {
    return base_path / xs("autoload.txt");
}

std::string Config::SanitizeName(std::string name) {
    name = Trim(name);

    if (name.size() >= 4) {
        std::string ext = ToLower(name.substr(name.size() - 4));
        if (ext == xs(".cfg"))
            name = name.substr(0, name.size() - 4);
    }

    for (auto& c : name) {
        const bool invalid = c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
        if (invalid)
            c = '_';
    }

    while (!name.empty() && (name.front() == '.' || name.front() == ' '))
        name.erase(name.begin());

    while (!name.empty() && (name.back() == '.' || name.back() == ' '))
        name.pop_back();

    return name;
}

std::string Config::FormatDate(std::uint64_t unix_time) {
    if (!unix_time)
        return xs("unknown");

    std::time_t tt = static_cast<std::time_t>(unix_time);
    std::tm tm{};

    localtime_s(&tm, &tt);

    char buf[32]{};
    std::snprintf(buf, sizeof(buf), xs("%02d/%02d/%04d"), tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    return buf;
}

bool Config::IsDefaultConfig(const std::string& name) {
    return ToLower(SanitizeName(name)) == xs("default");
}

bool Config::Exists(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty())
        return false;

    return fs::exists(GetConfigPath(normalized));
}

static void EnsureDirectories() {
    fs::create_directories(Config::base_path / xs("configs"));
    fs::create_directories(Config::base_path / xs("scripts/libs"));
}

static bool WriteConfigFile(const std::string& raw_name, const bool allow_default) {
    std::string name = Config::SanitizeName(raw_name);
    if (name.empty())
        return false;

    if (!allow_default && Config::IsDefaultConfig(name))
        return false;

    const fs::path file_path = Config::GetConfigPath(name);

    if (!allow_default && !fs::exists(file_path) && CountCfgFiles() >= max_configs)
        return false;

    Binds::SaveToConfig();

    std::ostringstream plain;

    for (std::size_t i = 0; i < Config::variables.size(); ++i) {
        const auto& var = Config::variables[i];
        const std::string value = SerializeValue(var);

        if (value.empty() && var.type_name != xs("std::string"))
            continue;

        plain << var.name << xs("|") << var.type_name << xs("|") << value << "\n";
    }

    std::vector<BYTE> encrypted;
    if (!ProtectText(plain.str(), encrypted))
        return false;

    return WriteBinaryFile(file_path, encrypted);
}

void Config::Init() {
    char module_path[MAX_PATH]{};
    GetModuleFileNameA(GetModuleHandleA("client.dll"), module_path, MAX_PATH);

    fs::path client_dir = fs::path(module_path).parent_path().parent_path().parent_path();
    base_path = client_dir / xs("reverie");

    EnsureDirectories();

    if (!fs::exists(GetConfigPath(xs("default"))))
        WriteConfigFile(xs("default"), true);

    if (fs::exists(GetAutoloadPath())) {
        startup_config = SanitizeName(ReadTextFile(GetAutoloadPath()));
        if (startup_config.empty())
            startup_config = xs("default");
    } else {
        startup_config = xs("default");
        WriteTextFile(GetAutoloadPath(), startup_config);
    }

    Refresh();
    LoadStartup();
    Refresh();
}

void Config::Refresh() {
    items.clear();

    const fs::path dir = base_path / xs("configs");
    if (!fs::exists(dir))
        return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != xs(".cfg"))
            continue;

        ConfigItem_t item{};
        item.name = entry.path().stem().string();
        item.is_default = IsDefaultConfig(item.name);

        item.created_at = FileCreationTime(entry.path());
        if (!item.created_at)
            item.created_at = UnixNow();

        item.created_text = FormatDate(item.created_at);
        items.push_back(item);
    }

    std::sort(items.begin(), items.end(), [](const ConfigItem_t& a, const ConfigItem_t& b) {
        if (a.is_default != b.is_default)
            return a.is_default > b.is_default;

        if (a.created_at != b.created_at)
            return a.created_at > b.created_at;

        return a.name < b.name;
    });
}

bool Config::Save(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty() || IsDefaultConfig(normalized))
        return false;

    const bool ok = WriteConfigFile(normalized, false);
    if (ok) {
        active_config = normalized;
        Refresh();
    }

    return ok;
}

static bool IsLegacyAutoStop(const VariableObject_t& var, const std::string& type_name) {
    return var.name.ends_with(xs("_AutoStop")) && var.type_name == xs("std::vector<bool>") && type_name == xs("int");
}

bool Config::Load(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty())
        return false;

    const fs::path file_path = GetConfigPath(normalized);
    if (!fs::exists(file_path))
        return false;

    const std::vector<BYTE> encrypted = ReadBinaryFile(file_path);
    if (encrypted.empty())
        return false;

    std::string decrypted;
    if (!UnprotectText(encrypted, decrypted))
        return false;

    Binds::ResetAll();
    for (auto& var : variables)
        ResetValue(var);

    legacy_bind_targets.clear();
    bool ordered_schema_matches = true;
    std::size_t unindexed_rows = 0;
    std::unordered_map<std::string, std::size_t> saved_name_counts;

    std::unordered_map<std::string, std::size_t> name_to_idx;
    name_to_idx.reserve(variables.size());
    for (std::size_t i = 0; i < variables.size(); ++i)
        name_to_idx[variables[i].name] = i;
    
    std::istringstream in(decrypted);
    std::string line;

    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty())
            continue;

        const auto p1 = line.find('|');
        if (p1 == std::string::npos) {
            ordered_schema_matches = false;
            continue;
        }

        const auto p2 = line.find('|', p1 + 1);
        if (p2 == std::string::npos) {
            ordered_schema_matches = false;
            continue;
        }

        const auto p3 = line.find('|', p2 + 1);

        std::string var_name;
        std::string type_name;
        std::string value;

        if (p3 == std::string::npos) {
            var_name = line.substr(0, p1);
            type_name = line.substr(p1 + 1, p2 - p1 - 1);
            value = line.substr(p2 + 1);

            if (unindexed_rows >= variables.size() ||
                variables[unindexed_rows].name != var_name ||
                (variables[unindexed_rows].type_name != type_name && !IsLegacyAutoStop(variables[unindexed_rows], type_name))) {
                ordered_schema_matches = false;
            }
            ++unindexed_rows;
        } else {
            ordered_schema_matches = false;
            var_name = line.substr(p1 + 1, p2 - p1 - 1);
            type_name = line.substr(p2 + 1, p3 - p2 - 1);
            value = line.substr(p3 + 1);

            try {
                std::size_t consumed = 0;
                const std::string saved_index = line.substr(0, p1);
                const int index = std::stoi(saved_index, &consumed);
                if (index >= 0 && consumed == saved_index.size()) {
                    const auto [it, inserted] = legacy_bind_targets.emplace(index, std::make_pair(var_name, type_name));
                    if (!inserted)
                        it->second = {};
                }
            } catch (...) {}
        }

        const std::size_t saved_name_count = ++saved_name_counts[var_name];
        auto it = name_to_idx.find(var_name);
        if (it != name_to_idx.end()) {
            auto& var = variables[it->second];

            if (saved_name_count > 1) {
                ResetValue(var);
                continue;
            }

            if (IsLegacyAutoStop(var, type_name)) {
                const int mode = std::clamp(ParseInt(value), 0, 2);
                var.Get<std::vector<bool>>() = { mode == 2, mode == 1, false, false };
                continue;
            }

            bool type_valid = (var.type_name == type_name);
            if (!type_valid) {
                if ((var.type_name == xs("Color_t") || var.type_name == xs("color_t")) &&
                    (type_name == xs("Color_t") || type_name == xs("color_t"))) {
                    type_valid = true;
                }
            }

            if (type_valid) {
                ApplyValue(var, value);
            }
        }
    }

    if (ordered_schema_matches && unindexed_rows == variables.size()) {
        for (std::size_t i = 0; i < variables.size(); ++i)
            legacy_bind_targets.emplace(static_cast<int>(i), std::make_pair(variables[i].name, variables[i].type_name));
    }

    for (auto& [index, target] : legacy_bind_targets) {
        if (saved_name_counts[target.first] != 1)
            target = {};
    }

    Binds::LoadFromConfig();
    legacy_bind_targets.clear();

    active_config = normalized;
    return true;
}

bool Config::Create(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty() || IsDefaultConfig(normalized))
        return false;

    if (fs::exists(GetConfigPath(normalized)))
        return false;

    const bool ok = WriteConfigFile(normalized, false);
    if (ok) {
        active_config = normalized;
        Refresh();
    }

    return ok;
}

bool Config::Remove(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty() || IsDefaultConfig(normalized))
        return false;

    const fs::path cfg_path = GetConfigPath(normalized);

    if (!fs::exists(cfg_path))
        return false;

    fs::remove(cfg_path);

    if (active_config == normalized)
        Load(xs("default"));

    if (startup_config == normalized) {
        startup_config = xs("default");
        WriteTextFile(GetAutoloadPath(), startup_config);
    }

    Refresh();
    return true;
}

bool Config::SetStartupConfig(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty())
        return false;

    if (!fs::exists(GetConfigPath(normalized)))
        return false;

    startup_config = normalized;
    return WriteTextFile(GetAutoloadPath(), startup_config);
}

void Config::LoadStartup() {
    if (startup_config.empty())
        startup_config = xs("default");

    if (!Load(startup_config)) {
        startup_config = xs("default");
        WriteTextFile(GetAutoloadPath(), startup_config);
        Load(xs("default"));
    }
}

bool Config::ShowInExplorer(const std::string& name) {
    const std::string normalized = SanitizeName(name);
    if (normalized.empty())
        return false;

    const fs::path file_path = GetConfigPath(normalized);
    if (!fs::exists(file_path))
        return false;

    const std::wstring params = L"/select,\"" + file_path.wstring() + L"\"";
    const auto result = reinterpret_cast<std::uintptr_t>(ShellExecuteW(nullptr, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL));
    return result > 32;
}
