#ifndef __SERIAL_H__
#define __SERIAL_H__

/* transfer->sample_count is normally 65536 */
//#define FFT_BUFFER_COPY_SIZE 65536

//typedef struct {
//    uint32_t index;
//    uint32_t size;
//    char data[FFT_BUFFER_COPY_SIZE * sizeof(float)];
//    pthread_mutex_t mutex;
//    pthread_cond_t signal;
//} lime_fft_buffer_t;

void *serial_thread(void *arg);

#endif /* __SERIAL_H__ */

