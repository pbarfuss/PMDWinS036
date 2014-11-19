#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "ntapi.h"

/*
 * some good values for block size and count
 */
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 16

/*
 * module level variables
 */
static WAVEHDR* waveBlocks;
volatile LONG waveFreeBlockCount;
static int waveCurrentBlock;
static HWAVEOUT hWaveOut;

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1,DWORD dwParam2){
	/*
	 * pointer to free block counter
	*/
	LONG* freeBlockCounter = (LONG*)dwInstance;
	/*
	 * ignore calls that occur due to openining and closing the
	 * device.
	 */
	if(uMsg != WOM_DONE)
	    return;
    InterlockedIncrement(freeBlockCounter);
}

static WAVEHDR* allocateBlocks(unsigned int size, unsigned int count){
	unsigned char* buffer = NULL;
	int i;
	WAVEHDR* blocks;
	SIZE_T totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	 * allocate memory for the entire set in one go
	 */
    NtAllocateVirtualMemory(((HANDLE)-1), (void**)&buffer, 0, &totalBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

	/*
	 * and set up the pointers to each bit
	 */
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for(i = 0; i < count; i++) {
	   blocks[i].dwBufferLength = size;
	   blocks[i].lpData = (char*)buffer;
	   buffer += size;
	}
	return blocks;
}

#define WORD2INT(x) ((x) < -32766.5f ? -32767 : ((x) > 32766.5f ? 32767 : lrintf(x)))
void audio_winmm_write_flt(void *_ao, float *data, unsigned int size){
    HWAVEOUT hWaveOut = (HWAVEOUT)_ao;
	WAVEHDR* current;
	current = &waveBlocks[waveCurrentBlock];
	unsigned int i, k = 0, remain;
    int16_t *outbuf;
	while(size > 0) {
    	/* 
    	 * first make sure the header we're going to use is unprepared
    	 */
    	if(current->dwFlags & WHDR_PREPARED) 
    	    waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));
    	if(size < (size_t)((BLOCK_SIZE - current->dwUser) >> 1)) {
    	    //memcpy(current->lpData + current->dwUser, data, size);
            outbuf = (int16_t*)(current->lpData + current->dwUser);
            for (i = 0; i < size; i++) {
                outbuf[i] = WORD2INT(data[i+k]*32768.0f);
            }
    	    current->dwUser += (size * sizeof(int16_t));
    	    break;
    	}
    	remain = ((BLOCK_SIZE - current->dwUser) >> 1);
        outbuf = (int16_t*)(current->lpData + current->dwUser);
    	//memcpy(current->lpData + current->dwUser, data, remain);
        for (i = 0; i < remain; i++) {
            outbuf[i] = WORD2INT(data[i+k]*32768.0f);
        }
    	size -= (remain >> 1);
    	k += (remain >> 1);
    	current->dwBufferLength = BLOCK_SIZE;
    	waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
    	waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
    	InterlockedExchangeAdd(&waveFreeBlockCount, -1);
    
    	while(!waveFreeBlockCount) {
            LARGE_INTEGER TimeOut;
            TimeOut.QuadPart = -1000000;
            NtDelayExecution(FALSE, &TimeOut);
        }
    
    	waveCurrentBlock++;
    	waveCurrentBlock %= BLOCK_COUNT;
    	current = &waveBlocks[waveCurrentBlock];
    	current->dwUser = 0;
	}
}

void audio_winmm_write_s16(void *_ao, int16_t *data, unsigned int size){
    HWAVEOUT hWaveOut = (HWAVEOUT)_ao;
	WAVEHDR* current;
	current = &waveBlocks[waveCurrentBlock];
	unsigned int i, k = 0, remain;
    int16_t *outbuf;
	while(size > 0) {
    	/* 
    	 * first make sure the header we're going to use is unprepared
    	 */
    	if(current->dwFlags & WHDR_PREPARED) 
    	    waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));
    	if(size < (size_t)((BLOCK_SIZE - current->dwUser) >> 1)) {
    	    //memcpy(current->lpData + current->dwUser, data, size);
            outbuf = (int16_t*)(current->lpData + current->dwUser);
            for (i = 0; i < size; i++) {
                outbuf[i] = data[i+k];
            }
    	    current->dwUser += (size * sizeof(int16_t));
    	    break;
    	}
    	remain = ((BLOCK_SIZE - current->dwUser) >> 1);
        outbuf = (int16_t*)(current->lpData + current->dwUser);
    	//memcpy(current->lpData + current->dwUser, data, remain);
        for (i = 0; i < remain; i++) {
            outbuf[i] = data[i+k];
        }
    	size -= (remain >> 1);
    	k += (remain >> 1);
    	current->dwBufferLength = BLOCK_SIZE;
    	waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
    	waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));
    	InterlockedExchangeAdd(&waveFreeBlockCount, -1);
    
    	while(!waveFreeBlockCount) {
            LARGE_INTEGER TimeOut;
            TimeOut.QuadPart = -1000000;
            NtDelayExecution(FALSE, &TimeOut);
        }
    
    	waveCurrentBlock++;
    	waveCurrentBlock %= BLOCK_COUNT;
    	current = &waveBlocks[waveCurrentBlock];
    	current->dwUser = 0;
	}
}

void *audio_winmm_open(unsigned int *sfreq) {
    DWORD r = MMSYSERR_NOERROR;
	WAVEFORMATEX wfx;
	waveBlocks = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount = BLOCK_COUNT;
	waveCurrentBlock= 0;
    if(waveBlocks == NULL) {
        return NULL;
    }
	wfx.nSamplesPerSec = *sfreq;
	wfx.wBitsPerSample = 16;
	wfx.nChannels = 1; 
	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if ((r = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)&waveFreeBlockCount, CALLBACK_FUNCTION)) == MMSYSERR_NOERROR) {
        return &hWaveOut;
    }
    return NULL;
}

void audio_winmm_close(HWAVEOUT hWaveOut) {
	int i;
	while(waveFreeBlockCount < BLOCK_COUNT) {
        LARGE_INTEGER TimeOut;
        TimeOut.QuadPart = -1000000;
        NtDelayExecution(FALSE, &TimeOut);
    }

	for(i = 0; i < waveFreeBlockCount; i++) 
	    if(waveBlocks[i].dwFlags & WHDR_PREPARED)
	        waveOutUnprepareHeader(hWaveOut, &waveBlocks[i], sizeof(WAVEHDR));
    NtFreeVirtualMemory(((HANDLE)-1), (void**)&waveBlocks, 0, MEM_RELEASE); 
	waveOutClose(hWaveOut);
}

