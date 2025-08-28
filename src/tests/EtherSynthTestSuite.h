#pragma once
#include <cassert>
#include <chrono>
#include <vector>
#include <functional>
#include <string>
#include <iostream>

/**
 * EtherSynthTestSuite - Comprehensive test framework for EtherSynth components
 * Generated with input from local LLM for systematic testing coverage
 */

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "ASSERTION FAILED: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define TEST_TIMING_START() auto _start_time = std::chrono::high_resolution_clock::now()
#define TEST_TIMING_END_US(max_us) \
    auto _end_time = std::chrono::high_resolution_clock::now(); \
    auto _duration = std::chrono::duration_cast<std::chrono::microseconds>(_end_time - _start_time).count(); \
    TEST_ASSERT(_duration < max_us, "Timing requirement failed: " + std::to_string(_duration) + "us > " + std::to_string(max_us) + "us")

class EtherSynthTestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string message;
        long execution_time_us;
    };
    
    using TestFunction = std::function<bool()>;
    
    void addTest(const std::string& name, TestFunction test) {
        tests_.push_back({name, test});
    }
    
    std::vector<TestResult> runAllTests() {
        std::vector<TestResult> results;
        
        for (const auto& test : tests_) {
            std::cout << "Running test: " << test.name << "..." << std::endl;
            
            auto start = std::chrono::high_resolution_clock::now();
            bool passed = false;
            std::string message = "";
            
            try {
                passed = test.function();
                message = passed ? "PASSED" : "FAILED";
            } catch (const std::exception& e) {
                passed = false;
                message = std::string("EXCEPTION: ") + e.what();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            long execution_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            
            results.push_back({test.name, passed, message, execution_time});
            std::cout << "  Result: " << message << " (" << execution_time << "us)" << std::endl;
        }
        
        return results;
    }
    
    void printSummary(const std::vector<TestResult>& results) {
        int passed = 0;
        int failed = 0;
        long total_time = 0;
        
        for (const auto& result : results) {
            if (result.passed) passed++;
            else failed++;
            total_time += result.execution_time_us;
        }
        
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Tests run: " << results.size() << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Total execution time: " << total_time << "us" << std::endl;
        
        if (failed > 0) {
            std::cout << "\nFailed tests:" << std::endl;
            for (const auto& result : results) {
                if (!result.passed) {
                    std::cout << "  - " << result.name << ": " << result.message << std::endl;
                }
            }
        }
    }
    
private:
    struct Test {
        std::string name;
        TestFunction function;
    };
    
    std::vector<Test> tests_;
};

// Forward declarations for test classes
class Classic4OpFMEngineests;
class SlideAccentBassTests;
class FMOperatorTests;
class AdvancedParameterSmootherTests;
class HTMParameterTests;
class IntegrationTests;
class PerformanceTests;
class RealTimeSafetyTests;

// Test result aggregator
class TestSuiteRunner {
public:
    static void runAllTestSuites();
    static void runUnitTests();
    static void runIntegrationTests();
    static void runPerformanceTests();
    static void runRealTimeSafetyTests();
};