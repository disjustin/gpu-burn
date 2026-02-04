/*
 * Structured Output and Error Classification for GPU-Burn
 * Provides JSON logging, error categorization, and standardized exit codes
 */

#ifndef STRUCTURED_OUTPUT_H
#define STRUCTURED_OUTPUT_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <sys/utsname.h>

// Standardized exit codes
namespace ExitCode {
    const int SUCCESS = 0;
    const int ERROR_NO_GPUS = 1;
    const int ERROR_CUDA_INIT = 2;
    const int ERROR_ALL_CLIENTS_DEAD = 3;
    const int ERROR_GPU_FAULTY = 4;
    const int ERROR_INVALID_ARGS = 5;
    const int ERROR_CONFIG_FILE = 6;
    const int ERROR_KERNEL_LOAD = 7;
}

// Error classification
enum class ErrorType {
    COMPUTE,      // Computation mismatch
    MEMORY,       // Memory allocation/access error
    THERMAL,      // Temperature related
    TIMEOUT,      // Process timeout
    CUDA_API,     // CUDA API call failure
    PROCESS,      // Process management error
    UNKNOWN
};

struct ErrorDetail {
    ErrorType type;
    int count;
    std::string message;
    std::string timestamp;
};

// Simple JSON writer (lightweight, no external dependencies)
class JSONWriter {
private:
    std::stringstream ss;
    std::vector<bool> needsComma;
    int indentLevel;

    void indent() {
        for (int i = 0; i < indentLevel * 2; i++) ss << " ";
    }

    std::string escapeString(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }

    void writeComma() {
        if (!needsComma.empty() && needsComma.back()) {
            ss << ",\n";
        } else if (!needsComma.empty()) {
            ss << "\n";
            needsComma.back() = true;
        }
    }

public:
    JSONWriter() : indentLevel(0) {}

    void startObject() {
        writeComma();
        indent();
        ss << "{";
        indentLevel++;
        needsComma.push_back(false);
    }

    void endObject() {
        indentLevel--;
        needsComma.pop_back();
        ss << "\n";
        indent();
        ss << "}";
    }

    void startArray(const std::string& key) {
        writeComma();
        indent();
        ss << "\"" << key << "\": [";
        indentLevel++;
        needsComma.push_back(false);
    }

    void endArray() {
        indentLevel--;
        needsComma.pop_back();
        ss << "\n";
        indent();
        ss << "]";
    }

    void writeKey(const std::string& key, const std::string& value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": \"" << escapeString(value) << "\"";
    }

    void writeKey(const std::string& key, int value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": " << value;
    }

    void writeKey(const std::string& key, long value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": " << value;
    }

    void writeKey(const std::string& key, float value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": " << std::fixed << std::setprecision(2) << value;
    }

    void writeKey(const std::string& key, double value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": " << std::fixed << std::setprecision(2) << value;
    }

    void writeKey(const std::string& key, bool value) {
        writeComma();
        indent();
        ss << "\"" << key << "\": " << (value ? "true" : "false");
    }

    std::string toString() {
        return ss.str();
    }
};

// Structured logger for test results
class StructuredLogger {
private:
    std::string outputFile;
    std::string startTime;
    std::vector<std::map<std::string, std::string>> gpuInfo;
    std::map<int, std::vector<ErrorDetail>> errorsByGpu;
    std::map<int, std::vector<std::map<std::string, float>>> iterationData;
    bool enabled;

    std::string getCurrentTimestamp() {
        time_t now = time(0);
        char buf[100];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", gmtime(&now));
        return std::string(buf);
    }

public:
    StructuredLogger() : enabled(false) {}

    void init(const std::string& file) {
        outputFile = file;
        enabled = !file.empty();
        startTime = getCurrentTimestamp();
    }

    bool isEnabled() const { return enabled; }

    void addGPU(int id, const std::string& name, size_t memoryMB) {
        std::map<std::string, std::string> gpu;
        gpu["id"] = std::to_string(id);
        gpu["name"] = name;
        gpu["memory_mb"] = std::to_string(memoryMB);
        gpuInfo.push_back(gpu);
    }

    void logError(int gpuId, ErrorType type, const std::string& message) {
        if (!enabled) return;

        ErrorDetail error;
        error.type = type;
        error.count = 1;
        error.message = message;
        error.timestamp = getCurrentTimestamp();

        errorsByGpu[gpuId].push_back(error);
    }

    void logIteration(int gpuId, int iteration, float gflops, int errors, int temp) {
        if (!enabled) return;

        std::map<std::string, float> data;
        data["iteration"] = iteration;
        data["gflops"] = gflops;
        data["errors"] = errors;
        data["temperature_c"] = temp;

        iterationData[gpuId].push_back(data);
    }

    std::string errorTypeToString(ErrorType type) {
        switch (type) {
            case ErrorType::COMPUTE: return "COMPUTE";
            case ErrorType::MEMORY: return "MEMORY";
            case ErrorType::THERMAL: return "THERMAL";
            case ErrorType::TIMEOUT: return "TIMEOUT";
            case ErrorType::CUDA_API: return "CUDA_API";
            case ErrorType::PROCESS: return "PROCESS";
            default: return "UNKNOWN";
        }
    }

    void writeResults(const std::map<int, bool>& gpuFaulty, int duration,
                     const std::string& precision, bool tensorCores) {
        if (!enabled) return;

        JSONWriter json;

        // Root object
        json.startObject();

        // Test metadata
        json.writeKey("test_name", "gpu-burn");
        json.writeKey("version", "1.0.0");
        json.writeKey("start_time", startTime);
        json.writeKey("end_time", getCurrentTimestamp());
        json.writeKey("duration_seconds", duration);

        // System info
        struct utsname sysinfo;
        if (uname(&sysinfo) == 0) {
            json.writeKey("hostname", sysinfo.nodename);
            json.writeKey("os", std::string(sysinfo.sysname) + " " + sysinfo.release);
            json.writeKey("architecture", sysinfo.machine);
        }

        // Test configuration
        json.writeKey("precision", precision);
        json.writeKey("tensor_cores", tensorCores);
        json.writeKey("matrix_size", 8192);

        // GPU information
        json.startArray("gpus");
        for (size_t i = 0; i < gpuInfo.size(); i++) {
            json.startObject();
            json.writeKey("id", std::stoi(gpuInfo[i]["id"]));
            json.writeKey("name", gpuInfo[i]["name"]);
            json.writeKey("memory_mb", std::stol(gpuInfo[i]["memory_mb"]));

            // Status
            auto faultyIt = gpuFaulty.find(i);
            bool isFaulty = (faultyIt != gpuFaulty.end() && faultyIt->second);
            json.writeKey("status", isFaulty ? "FAULTY" : "OK");

            // Errors for this GPU
            auto errIt = errorsByGpu.find(i);
            if (errIt != errorsByGpu.end() && !errIt->second.empty()) {
                json.startArray("errors");
                for (const auto& err : errIt->second) {
                    json.startObject();
                    json.writeKey("type", errorTypeToString(err.type));
                    json.writeKey("count", err.count);
                    json.writeKey("message", err.message);
                    json.writeKey("timestamp", err.timestamp);
                    json.endObject();
                }
                json.endArray();
            }

            // Performance data
            auto iterIt = iterationData.find(i);
            if (iterIt != iterationData.end() && !iterIt->second.empty()) {
                json.startArray("iterations");
                for (const auto& iter : iterIt->second) {
                    json.startObject();
                    json.writeKey("iteration", (int)iter.at("iteration"));
                    json.writeKey("gflops", iter.at("gflops"));
                    json.writeKey("errors", (int)iter.at("errors"));
                    json.writeKey("temperature_c", (int)iter.at("temperature_c"));
                    json.endObject();
                }
                json.endArray();
            }

            json.endObject();
        }
        json.endArray();

        // Overall result
        bool anyFaulty = false;
        for (const auto& kv : gpuFaulty) {
            if (kv.second) anyFaulty = true;
        }
        json.writeKey("overall_result", anyFaulty ? "FAIL" : "PASS");

        json.endObject();

        // Write to file
        std::ofstream out(outputFile);
        if (out.is_open()) {
            out << json.toString() << std::endl;
            out.close();
        }
    }
};

#endif // STRUCTURED_OUTPUT_H
