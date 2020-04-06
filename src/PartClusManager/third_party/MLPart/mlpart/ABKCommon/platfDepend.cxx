/**************************************************************************
***
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation
***  the rights to use, copy, modify, merge, publish, distribute, sublicense,
***  and/or sell copies of the Software, and to permit persons to whom the
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/

// August   22, 1997   created   by Igor Markov  VLSI CAD UCLA ABKGROUP
// November 30, 1997   additions by Max Moroz,   VLSI CAD UCLA ABKGROUP

// note: not thread-safe

// This file to be included into all projects in the group
// it contains platform-specific code portability of which relies
// on symbols defined by compilers

// 970825 mro made corrections to conditional compilation in ctors for
//            Platform and User:
//            i)   _MSC_VER not __MSC_VER (starts with single underscore)
//            ii)  allocated more space for _infoLines
//            iii) changed nested #ifdefs to #if ... #elif ... #else
// 970923 ilm added abk_dump_stack()
// 971130 ilm added Max Moroz code for memory estimate and reworked
//            class MemUsage()
// 980822 mm  corrected MemUsage() to work with Solaris 2.6

#include "infolines.h"
#include "abktypes.h"
#include "abkassert.h"
#include "uofm_alloc.h"

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#define _X86_
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>

#if defined(sun) || defined(__SUNPRO_CC)
#include <sys/systeminfo.h>
#endif

#ifdef linux
#include <sys/utsname.h>
#endif

#if defined(linux) || defined(sun) || defined(__SUNPRO_CC)
#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <iomanip>

#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/resource.h>
#endif

#ifdef _MSC_VER
#include <windows.h>
#include <psapi.h>
#endif

using std::ifstream;

/* ======================== IMPLEMENTATIONS ======================== */

#ifdef _MSC_VER

double PrintWindowsMemoryInfo(DWORD processID) {
        const unsigned kMegaByte = 1024 * 1024;
        HANDLE hProcess;
        PROCESS_MEMORY_COUNTERS pmc;

        // Print the process identifier.

        // printf( "\nProcess ID: %u\n", processID );

        // Print information about the memory usage of the process.

        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
        if (NULL == hProcess) return -1.0;

        double memUsage = -1.0;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                // printf( "\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount );
                // printf( "\tPeakWorkingSetSize: 0x%08X\n",
                //          pmc.PeakWorkingSetSize );
                // printf( "\tWorkingSetSize: 0x%08X\n", pmc.WorkingSetSize );
                // printf( "\tQuotaPeakPagedPoolUsage: 0x%08X\n",
                //          pmc.QuotaPeakPagedPoolUsage );
                // printf( "\tQuotaPagedPoolUsage: 0x%08X\n",
                //          pmc.QuotaPagedPoolUsage );
                // printf( "\tQuotaPeakNonPagedPoolUsage: 0x%08X\n",
                //          pmc.QuotaPeakNonPagedPoolUsage );
                // printf( "\tQuotaNonPagedPoolUsage: 0x%08X\n",
                //          pmc.QuotaNonPagedPoolUsage );
                // printf( "\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage );
                // printf( "\tPeakPagefileUsage: 0x%08X\n",
                //          pmc.PeakPagefileUsage );
                memUsage = pmc.WorkingSetSize;
        }

        CloseHandle(hProcess);
        return memUsage / kMegaByte;
}

double winMemUsage(void) { return PrintWindowsMemoryInfo(GetCurrentProcessId()); }

#endif

void abk_dump_stack() {
        return;
#if defined(linux) || defined(sun) || defined(__SUNPRO_CC)
        printf("\n  --- ABK GROUP DELIBERATE STACK DUMP  ----------- \n\n");
        fflush(stdout);
        char s[160];
        unsigned ProcId = getpid();
#if defined(linux)
        sprintf(s,
                "/bin/echo \"bt   \nquit   \\n\" | "
                "gdb -q /proc/%d/exe %d",
                ProcId, ProcId);
#else
        sprintf(s, "/bin/echo \"where\\ndetach \\n\" | dbx - %d", ProcId);
#endif
        system(s);
        printf("  ------------------ END STACK DUMP -------------- \n");
        fflush(stdout);
#else
// fprintf(stderr," abk_dump_stack(): Can't dump stack on this platform\n");
// fflush(stderr);
#endif
        return;
}

void abk_call_debugger() {
#if defined(linux) || defined(sun) || defined(__SUNPRO_CC)
        unsigned ProcId = getpid();
        printf("\n  --- ATTACHING DEBUGGER to process %d ", ProcId);
        printf(" (an ABKGROUP utility) --- \n\n");
        fflush(stdout);
        char s[160];
#if defined(linux)
        sprintf(s, "gdb -q /proc/%d/exe %d", ProcId, ProcId);
#else  // must be Solaris
        sprintf(s, "dbx - %d", ProcId);
#endif
        system(s);
        printf("  ------------------ CONTINUING -------------- \n");
        fflush(stdout);
#else
        fprintf(stderr, " abk_call_debugger(): Can't call debugger on this platform\n");
        fflush(stderr);
#endif
        return;
}

double getPeakMemoryUsage() {
#if defined(_MSC_VER)
        return winMemUsage();
#endif

#if !(defined(sun) || defined(linux) || defined(__SUNPRO_CC))
        return -1;
#endif

#if defined(sun) || defined(__SUNPRO_CC)
        char procFileName[20];
        unsigned pid = getpid();
        sprintf(procFileName, "/proc/%d/as", pid);
        struct stat buf;
        if (stat(procFileName, &buf))  // no such file on Solaris 2.5 and earlier
        {                              // so we stat another file now
                char procFileNameOld[20];
                sprintf(procFileNameOld, "/proc/%d", pid);
                if (stat(procFileNameOld, &buf)) return -1.0;
        }
        return (1. / (1024. * 1024.)) * static_cast<double>(buf.st_size);
#elif defined(linux)
        char buf[1000];
        ifstream ifs("/proc/self/stat");
        for (unsigned k = 0; k != 23; k++) ifs >> buf;
        //  cout << k << ": " << buf << endl; }
        return (1.0 / (1024. * 1024.)) * atof(buf);
#else
        return -1;
#endif
}

static double getMemoryUsageEstimate();

MemUsage::MemUsage() {
        _peak = getPeakMemoryUsage();
#if (defined(sun) || defined(__SUNPRO_CC) || defined(linux))
        _estimate = _peak;
        return;
#endif
        _estimate = getMemoryUsageEstimate();
}

static double getVmRSS();
static double getTotalPhysMemSize();
static double getAvailablePhysMemSize();
static double getMinorPageFaults();
static double getMajorPageFaults();

double VMemUsage::_phys_total = getTotalPhysMemSize();
double VMemUsage::measureMinPageFaults() { return getMinorPageFaults(); }
double VMemUsage::measureMajPageFaults() { return getMajorPageFaults(); }
double VMemUsage::measureRSS() { return getVmRSS(); }
double VMemUsage::measurePhysAvailable() { return getAvailablePhysMemSize(); }

VMemUsage::VMemUsage() {
        _rss = getVmRSS();
        _minflt = getMinorPageFaults();
        _majflt = getMajorPageFaults();
        _phys = getAvailablePhysMemSize();
}

void MemChange::resetMark() { _memUsageMark = getMemoryUsageEstimate(); }

MemChange::MemChange() { resetMark(); }

MemChange::operator double() const {
#if defined(sun) || defined(linux) || defined(__SUNPRO_CC)
        return getPeakMemoryUsage() - _memUsageMark;
#else
        return -1.0;
#endif
}

Platform::Platform() {
#if defined(sun) || defined(__SUNPRO_CC)
        Str31 sys, rel, arch, platf;
        sysinfo(SI_SYSNAME, sys, 31);
        sysinfo(SI_RELEASE, rel, 31);
        sysinfo(SI_ARCHITECTURE, arch, 31);
        sysinfo(SI_PLATFORM, platf, 31);
        _infoLine = new char[strlen(sys) + strlen(rel) + strlen(arch) + strlen(platf) + 30];
        sprintf(_infoLine, "# Platform     : %s %s %s %s \n", sys, rel, arch, platf);
#elif defined(linux)
        struct utsname buf;
        uname(&buf);
        _infoLine = new char[strlen(buf.sysname) + strlen(buf.release) + strlen(buf.version) + strlen(buf.machine) + 30];
        sprintf(_infoLine, "# Platform     : %s %s %s %s \n", buf.sysname, buf.release, buf.version, buf.machine);
#elif defined(_MSC_VER)
        _infoLine = new char[40];
        strcpy(_infoLine, "# Platform     : MS Windows \n");
#else
        _infoLine = new char[40];
        strcpy(_infoLine, "# Platform     : unknown \n");
#endif
}

#if !defined(sun) && !defined(linux) && !defined(_MSC_VER)
extern int gethostname(char *, int);
#endif

User::User() {

//#if defined(linux) || defined(sun) || defined(__SUNPRO_CC)
#if defined(linux)
        char host[100];
        gethostname(host, 31);
        struct passwd *pwr = getpwuid(getuid());
        if (pwr == NULL) {
                _infoLine = new char[40];
                strcpy(_infoLine, "# User         : unknown \n");
                return;
        }

        //_infoLine= new char[strlen(pwr->pw_name)+strlen(pwr->pw_gecos)+130];
        _infoLine = new char[strlen(pwr->pw_name) + 130];
        // sprintf(_infoLine,"# User         : %s@%s (%s) \n",
        //                    pwr->pw_name,host,pwr->pw_gecos);
        sprintf(_infoLine, "# User         : %s@%s \n", pwr->pw_name, host);
#elif defined(_MSC_VER)
        _infoLine = new char[40];
        strcpy(_infoLine, "# User         : unknown \n");
#else
        _infoLine = new char[40];
        strcpy(_infoLine, "# User         : unknown \n");
#endif
}

// code by Max Moroz

const unsigned kMegaByte = 1024 * 1024;
const int kSmallChunks = 1000;
const unsigned kMaxAllocs = 20000;
const double MemUsageEps = 3;

// everything in bytes

inline long memused() {
        // cout << " Peak memory " << getPeakMemoryUsage() << endl;
        return static_cast<long>(getPeakMemoryUsage() * kMegaByte);
}

double getMemoryUsageEstimate() {
#if defined(_MSC_VER)
        return winMemUsage();
#endif

#if !(defined(linux) || defined(sun) || defined(__SUNPRO_CC))
        return -1;
#endif
        static long prevMem = 0;
        static long extra;
        static int fail = 0;

        if (fail) return -1.;

        //      new_handler oldHndl;
        //      oldHndl=set_new_handler(mH);

        void *ptr[kMaxAllocs];
        unsigned numAllocs = 0;

        long last = memused();
        //       cout << "Memused : " << last << endl;
        if (last <= 0) return -1.;
        //      cerr<<"memused()="<<memused()<<endl;
        unsigned chunk = 8192;  // system allocates 8K chunks
        unsigned allocated = 0;
        int countSmallChunks = kSmallChunks;
        // if we allocate <8K, we'd get memused()+=8K, and allocated<8K - error
        while (1) {
                //              abkfatal(numAllocs<kMaxAllocs, "too many
                // allocs");
                if (numAllocs >= kMaxAllocs) {
                        abkwarn(0, "too many allocs (may be okay for 64-bit builds)");
                        // FIFO destruction
                        for (unsigned i = 0; i != numAllocs; ++i) free(ptr[i]);
                        return -1.;
                }
                //              cerr<<"old: "<<memused()<<"; allocating
                // "<<chunk<<"; now: ";
                if (!(ptr[numAllocs++] = malloc(chunk))) {
                        fail = 1;
                        return -1.;
                }
                //              cerr<<memused()<<endl;
                allocated += chunk;
                if (memused() > last + MemUsageEps) break;
                if (chunk <= kMegaByte && !countSmallChunks--) {
                        chunk *= 2;
                        countSmallChunks = kSmallChunks;
                }
        }

        //     int result=memused()-allocated-prevMem;

        /* LIFO destruction
                while (numAllocs) free(ptr[--numAllocs]); */

        // FIFO destruction
        for (unsigned i = 0; i != numAllocs; ++i) free(ptr[i]);

        extra = memused() - last;
        // handle extra correctly:
        // in some cases we need to add its prev value to current,
        // in some just store the new value

        prevMem = memused() - allocated;
        //      set_new_handler(oldHndl);
        return static_cast<double>(prevMem) / static_cast<double>(kMegaByte);
}

UserHomeDir::UserHomeDir() {
#if defined(sun) || defined(linux)
        struct passwd *pwrec;
        pwrec = getpwuid(getuid());
        if (pwrec == NULL) {
                _infoLine = new char[8];
                strcpy(_infoLine, "unknown");
        } else {
                _infoLine = new char[strlen(pwrec->pw_dir) + 1];
                strcpy(_infoLine, pwrec->pw_dir);
        }
#elif defined(_MSC_VER)
        char *homedr = getenv("HOMEDRIVE");
        char *homepath = getenv("HOMEPATH");
        char *pathbuf = new char[_MAX_PATH];
        if (homedr == NULL || homepath == NULL)
                strcpy(pathbuf, "c:\\users\\default");
        else
                _makepath(pathbuf, homedr, homepath, NULL, NULL);
        unsigned len = strlen(pathbuf);
        if (pathbuf[len - 1] == '\\') pathbuf[len - 1] = '\0';
        _infoLine = new char[len];
        strcpy(_infoLine, pathbuf);
        delete[] pathbuf;
#endif
}

ExecLocation::ExecLocation() {
        char buf[1000] = "";
#ifdef linux
        readlink("/proc/self/exe", buf, 1000);
        int pos = strlen(buf) - 1;
        for (; pos >= 0; pos--) {
                if (buf[pos] == '/') {
                        buf[pos] = '\0';
                        break;
                }
        }
#endif

#if defined(sun) || defined(__SUNPRO_CC)
        pid_t pid = getpid();
        char tempfname[100] = "";
        sprintf(tempfname, "/tmp/atempf%d", static_cast<int>(pid));
        sprintf(buf, "/usr/proc/bin/ptree %d | tail -4 > %s", static_cast<int>(pid), tempfname);
        system(buf);
        {
                ifstream ifs(tempfname);
                ifs >> buf;
                ifs >> buf;
        }
        unlink(tempfname);
        if (buf[0] != '/') {
                if (strrchr(buf, '/') == NULL) {
                        getcwd(buf, 1000);
                        _infoLine = new char[strlen(buf) + 1];
                        strcpy(_infoLine, buf);
                        return;
                } else {
                        char buf1[500] = "", buf2[1000] = "";
                        getcwd(buf1, 500);
                        strncpy(buf2, buf, 500);
                        sprintf(buf, "%s/%s", buf1, buf2);
                }
        }
        char *lastDelim = strrchr(buf, '/');
        if (lastDelim == NULL)
                sprintf(buf, "Cannot find");
        else
                *lastDelim = '\0';
#elif defined(_MSC_VER)
        ::GetModuleFileName(NULL, buf, 995);
        char drv[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
        _splitpath(buf, drv, dir, fname, ext);
        _makepath(buf, drv, dir, "", "");
        int len = strlen(buf);
        if (buf[len - 1] == '\\') buf[len - 1] = '\0';
#endif

        _infoLine = new char[strlen(buf) + 1];
        strcpy(_infoLine, buf);
}

// <aaronnn>
static double getTotalPhysMemSize() {
#ifdef linux
#define AARON_BUFSZ 500
        char buf[AARON_BUFSZ];
        int totalPhysMem = -1;

        FILE *f;
        if ((f = fopen("/proc/meminfo", "r")) == NULL) return double(totalPhysMem);
        fgets(buf, AARON_BUFSZ, f);
        while (!feof(f) && !ferror(f)) {
                if (sscanf(buf, "MemTotal:%d", &totalPhysMem) == 1) break;

                // flush the pipe if buf was too small
                while (!feof(f) && !ferror(f) && strlen(buf) >= AARON_BUFSZ - 1) fgets(buf, AARON_BUFSZ, f);

                fgets(buf, AARON_BUFSZ, f);
        }
        fclose(f);

        return double(totalPhysMem) / 1024.;
#undef AARON_BUFSZ
#else
        return -1.0;
#endif
}

static double getAvailablePhysMemSize() {
#ifdef linux
        struct stat s;
        if (stat("/proc/kcore", &s)) return -1.0;

        return s.st_size / (1024. * 1024.);
#else
        return -1.0;
#endif
}

static double getVmRSS() {
#ifdef linux
#define AARON_BUFSZ 500
        char buf[AARON_BUFSZ];
        int vmRSS = -1;

        FILE *f;
        if ((f = fopen("/proc/self/status", "r")) == NULL) return double(vmRSS);
        fgets(buf, AARON_BUFSZ, f);
        while (!feof(f) && !ferror(f)) {
                if (sscanf(buf, "VmRSS:%d", &vmRSS) == 1) break;

                // flush the pipe if buf was too small
                while (!feof(f) && !ferror(f) && strlen(buf) == AARON_BUFSZ - 1) fgets(buf, AARON_BUFSZ, f);

                fgets(buf, AARON_BUFSZ, f);
        }
        fclose(f);

        return double(vmRSS) / 1024.;
#undef AARON_BUFSZ
#else
        return -1.0;
#endif
}

static double getMinorPageFaults() {
#ifdef linux
        struct rusage ru;

        if (getrusage(RUSAGE_SELF, &ru)) return -1.0;

        return ru.ru_minflt;
#else
        return -1.0;
#endif
}

static double getMajorPageFaults() {
#ifdef linux
        struct rusage ru;

        if (getrusage(RUSAGE_SELF, &ru)) return -1.0;

        return ru.ru_majflt;
#else
        return -1.0;
#endif
}

#ifdef linux
static void getProcSymlink(const char *elm, char *out, int outsz)
    // find out where elm points to in /proc
    // 'out' must have enough space allocated
{
        char path[MAXPATHLEN];

        if (snprintf(path, MAXPATHLEN, "/proc/self/%s", elm) >= MAXPATHLEN) {
                out[0] = '\0';
                return;
        }

        int written = readlink(path, out, outsz);

        if (written == -1)
                out[0] = '\0';
        else if (written == outsz)
                out[outsz - 1] = '\0';
        else
                out[written] = '\0';
}
#endif

#ifdef linux
static void getCPUInfo(char *out, int outsz)
    /*
     * get cpu info and copy that in to buffer
     */
{
#define AARON_BUFSZ 500
        char buf[AARON_BUFSZ];
        char cpuID[AARON_BUFSZ];
        double clock;
        unsigned numCPU = 1;
        char cpuInfo[AARON_BUFSZ];

        out[0] = '\0';

        FILE *f;
        if ((f = fopen("/proc/cpuinfo", "r")) == NULL) {
                out[0] = '\0';
                return;
        }
        bool modelNameFound, clockFound;
        modelNameFound = clockFound = false;
        fgets(buf, AARON_BUFSZ, f);
        while (!feof(f) && !ferror(f)) {
                if (sscanf(buf, "model name %*s %499[-0-9_a-zA-z():$#+=. ]", cpuID) == 1) {  // (sizeof(cpuID) = 499+1)
                        modelNameFound = true;
                        clockFound = false;
                } else if (sscanf(buf, "cpu MHz %*s %lf", &clock) == 1) {
                        clockFound = true;
                }

                if (modelNameFound && clockFound) {
                        cpuID[AARON_BUFSZ - 1] = '\0';
                        if (numCPU > 1)
                                snprintf(cpuInfo, AARON_BUFSZ, "\n# CPU%d : %.2fMHz %s", numCPU, clock, cpuID);
                        else
                                snprintf(cpuInfo, AARON_BUFSZ, "# CPU%d : %.2fMHz %s", numCPU, clock, cpuID);
                        cpuInfo[AARON_BUFSZ - 1] = '\0';
                        strncat(out, cpuInfo, outsz - 1);
                        out[outsz - 1] = '\0';
                        modelNameFound = clockFound = false;
                        numCPU++;
                }

                // flush the pipe if buf was too small
                while (!feof(f) && !ferror(f) && strlen(buf) == AARON_BUFSZ - 1) fgets(buf, AARON_BUFSZ, f);

                fgets(buf, AARON_BUFSZ, f);
        }
        fclose(f);
#undef AARON_BUFSZ
}
#endif

ExecInfo::ExecInfo()
    //: Prints starting execution info
{
#ifdef linux
        uofm::string infoLine;
        char buf[MAXPATHLEN];

        infoLine += "# Working directory : ";
        getProcSymlink("cwd", buf, MAXPATHLEN);
        infoLine += buf;
        infoLine += "\n";

        infoLine += "# Executable : ";
        getProcSymlink("exe", buf, MAXPATHLEN);  // WARNING: duplicate
                                                 // implementation in
                                                 // ExecLocation()
        infoLine += buf;
        infoLine += "\n";

        if (!gethostname(buf, MAXPATHLEN)) {
                infoLine += "# Hostname : ";
                infoLine += buf;
                infoLine += "\n";
        }

        getCPUInfo(buf, MAXPATHLEN);
        infoLine += buf;
        infoLine += "\n";

        _infoLine = new char[infoLine.size() + 1];
        strcpy(_infoLine, infoLine.c_str());
#else
        _infoLine = new char[1];
        _infoLine[0] = '\0';
#endif
}
// </aaronnn>
