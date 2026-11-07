#pragma once

#include <climits>
#include <cstdint>
#include <numbers>
#include <span>
#include <vector>
#include <complex>
#include <cmath>

template<typename T>
void swap_halves(std::vector<T>& span)
{
    for(size_t i = 0; i < span.size() / 2; ++i)
    {
        std::swap(span[i], span[i + span.size() / 2]);
    }
}

template<typename T>
T round_up_to_pow_2(T value)
{
    value--;

    for(size_t shift = 1; shift < sizeof(T) * CHAR_BIT; ++shift)
    {
        value |= value >> shift;
    }

    value++;

    return value;
}

uint32_t reverse_bits(uint32_t value, uint32_t num_bits)
{
    uint32_t result = 0;

    for(uint8_t bit_idx = 0; bit_idx < num_bits - 1; bit_idx++)
    {
        result |= value & 1U;

        result <<= 1;
        value >>= 1;
    }

    result |= value & 1U;

    return result;
}

template<typename T>
std::vector<std::complex<double>> dft(const std::span<T>& time_domain_data, size_t max_frequency, size_t frequency_step = 1)
{
    using namespace std::complex_literals;

    size_t N = time_domain_data.size();

    std::vector<std::complex<double>> frequency_domain_data;
    frequency_domain_data.reserve(N);

    for(size_t k = 0; k < std::min(max_frequency, N); k += frequency_step)
    {
        std::complex<T> sum = 0;

        for(size_t n = 0; n < N; ++n)
        {
            sum += time_domain_data[n] * std::exp<double>((-1j * 2 * std::numbers::pi * k * n) / N);
        }

        frequency_domain_data.push_back(sum);
    }

    return frequency_domain_data;
}

// https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B
// https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm

// time_domain_data needs to have a size that is a power of 2
std::vector<std::complex<double>> fft(const std::span<double>& time_domain_data)
{
    using namespace std::complex_literals;

    uint32_t n = time_domain_data.size();
    uint32_t num_bits = log2(n);

    std::vector<std::complex<double>> result;
    result.resize(n);

    for(uint32_t idx = 0; idx < n; ++idx)
    {
        uint32_t bit_reversed_idx = reverse_bits(idx, num_bits);

        result[bit_reversed_idx] = time_domain_data[idx];
    }

    for(uint32_t s = 0; s <= num_bits; ++s)
    {
        uint32_t m = 1 << s;

        auto omega_m = std::exp<double>(-2 / static_cast<float>(m) * std::numbers::pi * 1i);

        for(uint32_t k = 0; k < n; k += m)
        {
            std::complex<double> omega = 1;

            for(uint32_t j = 0; j < m / 2; ++j)
            {
                auto t = omega * result[k + j + m / 2];
                auto u = result[k + j];

                result[k + j] = u + t;
                result[k + j + m / 2] = u - t;

                omega *= omega_m;
            }
        }
    }

    return result;
}
