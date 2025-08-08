// **************************************************************************
// File       [ tm_usage.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ functions to calculate CPU time and memory usage ]
// Date       [ Ver 3.0 started 2010/12/20 ]
// **************************************************************************

#include "tm_usage.h"

#include <sys/resource.h>  // for getrusage()
#include <sys/time.h>      // for gettimeofday()

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace CommonNs;

TmUsage::TmUsage() {
    t_start_.u_time  = 0;
    t_start_.s_time  = 0;
    t_start_.r_time  = 0;
    t_start_.vm_size = 0;
    t_start_.vm_peak = 0;
    t_start_.vm_diff = 0;
    p_start_.u_time  = 0;
    p_start_.s_time  = 0;
    p_start_.r_time  = 0;
    p_start_.vm_size = 0;
    p_start_.vm_peak = 0;
    p_start_.vm_diff = 0;
}

TmUsage::~TmUsage() {}

bool TmUsage::totalStart() { return checkUsage(t_start_); }

bool TmUsage::periodStart() { return checkUsage(p_start_); }

bool TmUsage::getTotalUsage(TmStat &st) const {
    if (!checkUsage(st)) return false;
    st.u_time  -= t_start_.u_time;
    st.s_time  -= t_start_.s_time;
    st.r_time  -= t_start_.r_time;
    st.vm_diff  = st.vm_size - t_start_.vm_size;
    st.vm_peak  = st.vm_peak > t_start_.vm_peak ? st.vm_peak : t_start_.vm_peak;
    return true;
}

bool TmUsage::getPeriodUsage(TmStat &st) const {
    if (!checkUsage(st)) return false;
    st.u_time  -= p_start_.u_time;
    st.s_time  -= p_start_.s_time;
    st.r_time  -= p_start_.r_time;
    st.vm_diff  = st.vm_size - p_start_.vm_size;
    st.vm_peak  = st.vm_peak > p_start_.vm_peak ? st.vm_peak : p_start_.vm_peak;
    return true;
}

// **************************************************************************
// Function   [ checkUsage(TmStat &) ]
// Author     [ littleshamoo ]
// Synopsis   [ get user time and system time using getrusage() function and
//              get real time using gettimeofday() function and read
//              "/proc/self/status" to get memory usage ]
// **************************************************************************
bool TmUsage::checkUsage(TmStat &st) const {
    // check user time and system time
    rusage tUsg;
    timeval tReal;
    getrusage(RUSAGE_SELF, &tUsg);
    gettimeofday(&tReal, NULL);
    st.u_time = tUsg.ru_utime.tv_sec * 1000000 + tUsg.ru_utime.tv_usec;
    st.s_time = tUsg.ru_stime.tv_sec * 1000000 + tUsg.ru_stime.tv_usec;
    st.r_time = tReal.tv_sec * 1000000 + tReal.tv_usec;

    // check current memory and peak memory
    FILE *fmem = fopen("/proc/self/status", "r");
    if (!fmem) {
        fprintf(stderr, "**ERROR TmUsage::checkUsage(): cannot get memory usage\n");
        st.vm_size = 0;
        st.vm_peak = 0;
        return false;
    }
    char membuf[128];
    while (fgets(membuf, 128, fmem)) {
        char *ch;
        if ((ch = strstr(membuf, "VmPeak:"))) {
            st.vm_peak = atol(ch + 7);
            continue;
        }
        if ((ch = strstr(membuf, "VmSize:"))) {
            st.vm_size = atol(ch + 7);
            break;
        }
    }
    fclose(fmem);
    return true;
}
