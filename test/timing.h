/* timing.h

*/

#ifndef TIMING_H
#define TIMING_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <intrin.h>
#include <windows.h>
typedef unsigned __int64 usCount;
static usCount GetUsCount()
{
  static LARGE_INTEGER ticksPerSec;
  static double scalefactor;
  LARGE_INTEGER val;
  if(!scalefactor)
  {
    if(QueryPerformanceFrequency(&ticksPerSec))
      scalefactor = ticksPerSec.QuadPart / 1000000000000.0;
    else
      scalefactor = 1;
  }
  if(!QueryPerformanceCounter(&val))
    return (usCount) GetTickCount() * 1000000000;
  return (usCount)(val.QuadPart / scalefactor);
}
#else
#include <sys/time.h>
#include <time.h>
typedef unsigned long long usCount;
static usCount GetUsCount()
{
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((usCount) ts.tv_sec * 1000000000000LL) + ts.tv_nsec * 1000LL;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  return ((usCount) tv.tv_sec * 1000000000000LL) + tv.tv_usec * 1000000LL;
#endif
}
#endif


static uint64_t ticksclock()
{
#ifdef _MSC_VER
  auto rdtscp = [] {
    unsigned x;
    return (uint64_t) __rdtscp(&x);
  };
#else
#ifdef __rdtscp
  return (uint64_t) __rdtscp();
#elif defined(__x86_64__)
  auto rdtscp = [] {
    unsigned lo, hi;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi));
    return (uint64_t) lo | ((uint64_t) hi << 32);
  };
#elif defined(__i386__)
  auto rdtscp = [] {
    unsigned count;
    asm volatile("rdtscp" : "=a"(count));
    return (uint64_t) count;
  };
#endif
#if __ARM_ARCH >= 6
  auto rdtscp = [] {
    unsigned count;
    asm volatile("MRC p15, 0, %0, c9, c13, 0" : "=r"(count));
    return (uint64_t) count * 64;
  };
#endif
#endif
  return rdtscp();
}

static uint64_t nanoclock()
{
  static uint16_t ticks_per_sec;
  static uint64_t offset;
  if(ticks_per_sec == 0)
  {
    auto end = std::chrono::high_resolution_clock::now(), begin = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - begin);
    uint64_t _begin = ticksclock(), _end;
    do
    {
      end = std::chrono::high_resolution_clock::now();
    } while(std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() < 1);
    _end = ticksclock();
    uint64_t x = _end - _begin;
    x /= (1000000000 / 128);
    ticks_per_sec = (uint16_t) x;
    volatile uint64_t a = (uint64_t)((128 * ticksclock()) / ticks_per_sec);
    volatile uint64_t b = (uint64_t)((128 * ticksclock()) / ticks_per_sec);
    offset = b - a;
#if 1
    std::cout << "There are " << (ticks_per_sec / 128.0) << " TSCs in 1 nanosecond and it takes " << offset << " nanoseconds per nanoclock()." << std::endl;
#endif
  }
  return (uint64_t)((128 * ticksclock()) / ticks_per_sec) - offset;
}

#endif
