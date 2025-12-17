#ifdef _WIN32
#pragma comment(lib, "Shlwapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "benchmark.lib")
#else
#pragma comment(lib, "benchmark.lib")
#endif
#endif

#include <benchmark/benchmark.h>
#include <vector>
#include <numeric>
#include <optional>
#include <memory>
#include <thread>
#include <atomic>
#include <variant>
#include <algorithm>
#include <random>
#include <execution>
#include <immintrin.h>

#ifdef _WIN32
#include <windows.h>

void pin_to_core(DWORD core)
{
    HANDLE hThread = GetCurrentThread();

    DWORD_PTR mask = (1ull << core);

    SetThreadAffinityMask(hThread, mask);
}
#endif

constexpr size_t MATRIX_SIZE = 128;
constexpr size_t DATA_SIZE = 1024 * 100000;

long long interleaved_sum(const std::vector<long long> &data)
{
    long long sum = 0;
    constexpr size_t stride = 16;

    for (size_t i = 0; i < data.size() / stride; ++i)
    {
        sum += data[i * stride];
    }
    return sum;
}

static void BM_InterleavedSum(benchmark::State &state)
{
    std::vector<long long> data(DATA_SIZE);
    std::iota(data.begin(), data.end(), 1);

    long long r = 0;

    for (auto _ : state)
    {
        r = interleaved_sum(data);

        benchmark::DoNotOptimize(r);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_InterleavedSum);


static void BM_DotProduct(benchmark::State &state)
{
    const size_t N = DATA_SIZE;

    static std::vector<float> a(N), b(N);
    static bool initialized = false;

    if (!initialized)
    {
        std::mt19937 rng(123);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (size_t i = 0; i < N; ++i)
        {
            a[i] = dist(rng);
            b[i] = dist(rng);
        }
        initialized = true;
    }

    for (auto _ : state)
    {
        float sum = 0.0f;

        for (size_t i = 0; i < N; ++i)
        {
            sum += a[i] * b[i];
        }

        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_DotProduct);

static void BM_process_data_nohint(benchmark::State &state)
{

    static std::vector<long long> data(DATA_SIZE);
    std::fill(data.begin(), data.begin() + DATA_SIZE * 95 / 100, 0);
    std::fill(data.begin() + DATA_SIZE * 95 / 100, data.end(), 1);

    for (auto _ : state)
    {
        long long sum = 0;
        for (int x : data)
        {
            if (x == 0)
            { // 95% true
                sum += 1;
            }
            else
            {
                sum += 2;
            }
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_process_data_nohint);

static void BM_process_data_withhint(benchmark::State &state)
{
    static std::vector<long long> data(DATA_SIZE);
    std::fill(data.begin(), data.begin() + DATA_SIZE * 95 / 100, 0);
    std::fill(data.begin() + DATA_SIZE * 95 / 100, data.end(), 1);

    for (auto _ : state)
    {
        long long sum = 0;
        for (int x : data)
        {
            if (x == 0)[[likely]]
            { // 95% true
                sum += 1;
            }
            else[[unlikely]]
            {
                sum += 2;
            }
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_process_data_withhint);


int main(int argc, char **argv)
{
#ifdef _WIN32
    pin_to_core(2); // Pin to physical core 2 affinity mask 4
#endif

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}

