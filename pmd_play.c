#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <getopt.h>
#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#endif
#include "pmdwin.h"
#include "libaout/audio_out.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

static uint32_t atoui(const uint8_t *str) {
    uint32_t c, num = 0;
    while(1) {
        c = *str++ - 0x30;
        if(c > 9) break;
        num = (num << 1) + (num << 3) + c;
    }
    return num;
}

static const unsigned char static_hdr_portion[20] = {
   0x52, 0x49, 0x46, 0x46, 0xFF, 0xFF, 0xFF, 0x7F,
   0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
   0x10, 0x00, 0x00, 0x00
};

typedef struct _WAVHeader {
    uint16_t wav_id;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t bitrate;
    uint32_t block_align;
    uint32_t pad0;
    uint32_t pad1;
} __attribute__((packed)) WAVHeader;

static void write_wav_header(int fd, uint32_t rate)
{
   WAVHeader w;
   write(fd, &static_hdr_portion, 20);

   w.wav_id = 1;
   w.channels = 1;
   w.samplerate = rate;
   w.bitrate = rate*2;
   w.block_align = 0x00100010;
   w.pad0 = 0x61746164;
   w.pad1 = 0x7fffffff;
   write(fd, &w, sizeof(WAVHeader));
}

//=============================================================================
// Load PMD data from file into memory. Uses mmap() on both Windows and *nix.
// Originally in pmdwin.cpp, moved here to minimize platform-specific code
// in pmdwin.cpp
//=============================================================================
#if defined(WIN32) || defined(WIN64)
static NTSTATUS map_file(const wchar_t *filename, VOID **addr, SIZE_T *sz_ret)
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING name;
    NTSTATUS r;
    HANDLE file, section;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER ofs;
    SIZE_T sz;

    RtlDosPathNameToNtPathName_U(filename, &name, NULL, NULL);
    memset(&oa, 0, sizeof(oa));
    oa.Length = sizeof(oa);
    oa.RootDirectory = NULL;
    oa.ObjectName = &name;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    r = NtOpenFile(&file, FILE_GENERIC_READ | 0x8F | SYNCHRONIZE, &oa, &iosb,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0);
    if (r != STATUS_SUCCESS) {
        return r;
    }

    memset(&oa, 0, sizeof(oa));
    oa.Length = sizeof(oa);
    oa.RootDirectory = NULL;
    oa.ObjectName = NULL;
    oa.Attributes = OBJ_CASE_INSENSITIVE;

    section = NULL;
    r = NtCreateSection(&section, SECTION_MAP_READ, &oa, 0, PAGE_READWRITE, SEC_COMMIT, file);
    if (r != STATUS_SUCCESS) {
        return r;
    }

    sz = 0;
    ofs.QuadPart = 0;
    r = NtMapViewOfSection(section, (HANDLE)-1, addr, 0, 0, &ofs, &sz, 1, 0, PAGE_READONLY);
    if (r != STATUS_SUCCESS) {
        return r;
    }

    if (sz_ret != NULL) {
        FILE_STANDARD_INFORMATION st;
        NtQueryInformationFile(file, &iosb, &st, sizeof(st), 5); // FileStandardInformation == 5
        *sz_ret = st.EndOfFile.QuadPart-1;
    }
    RtlFreeUnicodeString(&name);
    NtClose(section);
    NtClose(file);
    return r;
}

static int music_load(PMDWIN *pmd, wchar_t *filename)
{
    int     result;
    size_t size;
    uint8_t *musbuf;

    if (map_file(filename, (void**)&musbuf, &size) != NTSTATUS_SUCCESS) { 
        return ERR_OPEN_MUSIC_FILE;
    }
    result = music_load3(pmd, musbuf, size);
    return result;
}
#else
static int music_load(PMDWIN *pmd, char *filename)
{
    struct stat st;
    int fd = -1;
    int     result;
    size_t size;
    uint8_t musbuf[mdata_def*1024];

    if((fd = open(filename, O_RDONLY)) < 0) {
        return ERR_OPEN_MUSIC_FILE;
    }

    fstat(fd, &st);
    size = st.st_size;    
    read(fd, musbuf, size);
    close(fd);
    result = music_load3(pmd, musbuf, size);
    return result;
}
#endif

#if defined(WIN32) || defined(WIN64)
#define WriteToConsole(str, len) WriteFile(errh, (str), (len), &done, NULL)
#else
#define WriteToConsole(str, len) write(2, (str), (len))
#endif

static void usage(void) {
#if defined(WIN32) || defined(WIN64)
    DWORD done;
    HANDLE errh = GetStdHandle(STD_ERROR_HANDLE);
#endif
    const char *usage_str = 
"pmdwin -f <output file> -l <loop count> -m <device mask> -c <channel mask> -t <time> [PMD File]\n\
       -f - Write output to wavfile\n\
       -l - Loop n times (default is once - use 0 for infinite loop)\n\
       -m - Device mask: 1 - OPNA, 2 - PSG, 4 - Rhythm (or them together)\n\
       -c - Channel mask: 1,2,4,8,16,32 are OPNA channels 1-6, 64,128,256 are PSG channels 1-3. Once again, or them together\n\
       -t - Play song for n seconds\n\n";
    WriteToConsole(usage_str, strlen(usage_str));
}

#if defined(WIN32) || defined(WIN64)
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    uint32_t i, k, frameno, samplerate_out = SOUND_44K, infloop = 0;
    uint32_t pmd_length = 0, pmd_loopcount = 1;
    int opt, out_fd = -1;
    uint8_t tofile = 0;
    int16_t pcmbuf[8192];
    char pmd_title[1024];
    char pmd_compo[1024];
#if defined(WIN32) || defined(WIN64)
    DWORD done;
    HANDLE errh = GetStdHandle(STD_ERROR_HANDLE);
#endif
    char buf[1024];
    char *devpath = NULL, device = '\0';
    void *ao = NULL;
	PMDWIN *pmd = pmdwininit();

    while((opt = getopt(argc, argv, "f:l:m:c:d:t:r:")) != -1) {
        switch(opt) {
            case 'd':
              device = argv[++opt][0];
              break;
            case 'f':
              tofile = 1;
              out_fd = open(optarg, O_WRONLY|O_CREAT|O_APPEND|O_BINARY, 0644);
              break;
            case 'l':
              pmd_loopcount = atoui((uint8_t*)optarg);
              if(!pmd_loopcount) infloop = 1;
              break;
            case 'c':
              setchanmask(pmd, atoui((uint8_t*)optarg));
              break;
            case 'm':
              setdevmask(pmd, atoui((uint8_t*)optarg));
              break;
            case 't':
              pmd_length = atoui((uint8_t*)optarg)*1000;
              break;
	        case 'r':
	          samplerate_out = atoui((uint8_t*)optarg);
	          break;
            default:
              usage();
              break;
        }
    }

    if(!tofile) {
        if (!(ao = audio_open(device, devpath, &samplerate_out))) {
            WriteToConsole("Cannot open sound device, exiting.\n", 35);
            return -1;
        }
    } else {
        write_wav_header(out_fd, samplerate_out);
    }

#if defined(WIN32) || defined(WIN64)
    SetConsoleOutputCP(65001); // UTF-8
#endif

    for (k = optind; k < argc; k++) {
        music_load(pmd, argv[k]);
        memcpy(pmd_title, "Title = ", 8);
        _getmemo3(pmd, pmd_title+8, NULL, 0, 1);
        i = strlen(pmd_title); pmd_title[i++] = '\n';
        WriteToConsole(pmd_title, i);
        memcpy(pmd_compo, "Composer = ", 11);
        _getmemo3(pmd, pmd_compo+11, NULL, 0, 2);
        i = strlen(pmd_compo); pmd_compo[i++] = '\n';
        WriteToConsole(pmd_compo, i);
        setpcmrate(pmd, samplerate_out);
        music_start(pmd);
        i = 0;
        if(pmd_length != 0) {
            frameno = lrintf(((float)pmd_length*(float)samplerate_out)/(4096.0f*1000.0f));
        } else {
            frameno = 0;
        }
        buf[0] = '\r';
        while(getloopcount(pmd) < pmd_loopcount || infloop || i < frameno) {
            int ret = getstatus(pmd, buf+1, 1022);
            WriteToConsole(buf, ret+1);
            getpcmdata(pmd, pcmbuf, 4096);
            if (tofile) {
              write(out_fd, pcmbuf, 8192);
            } else {
              audio_write_s16(ao, pcmbuf, 4096);
            }
            i++;
        }
        WriteToConsole("\n", 1);
    }
    if (!tofile) {
        audio_close(ao);
    } else {
        close(out_fd);
    }
    return 0;
}

