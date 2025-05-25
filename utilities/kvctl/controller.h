#ifndef CONTROLLER_H
#define CONTROLLER_H

// #include <Node.h>
#include <fcntl.h>
#include <iostream>
#include <linux_nvme_ioctl.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>

// KV COMMANDS
#define NVME_KV_STORE     0x81
#define NVME_KV_APPEND    0x83
#define NVME_KV_BATCH     0x85
#define NVME_KV_RETRIEVE  0x90
#define NVME_KV_DELETE    0xA1
#define NVME_KV_ITER_REQ  0xB1
#define NVME_KV_ITER_READ 0xB2
#define NVME_KV_EXIST     0xB3

#define KB                1024
#define MB                (KB * 1024)
#define GB                (MB * 1024)

#define READ_SIZE         (2 * MB)
// #define ITER_BUFFER_SIZE (32 * KB)

// class Controller;

// class CBThread {
//   private:
//     pthread_t tid_;
//
//     std::atomic_bool start_;
//     std::atomic_bool stop_;
//
//     Controller* dev_;
//
//   public:
//     CBThread(Controller* dev)
//         : dev_(dev), start_(false), stop_(false) {}
//
//     void Start()
//     {
//         int ret = pthread_create(&tid_, NULL, _entry_func, (void*)this);
//         if(ret != 0) {
//             fprintf(stderr, "failed to create a interrupt thread\n");
//             return;
//         }
//         start_ = true;
//     }
//
//     void Stop()
//     {
//         stop_ = true;
//         Join(0);
//     }
//
//     int Join(void** prval)
//     {
//         if(!start_) {
//             return 0;
//         }
//         if(tid_ == 0) {
//             return -EINVAL;
//         }
//
//         int status = pthread_join(tid_, prval);
//         if(status != 0) {
//             fprintf(stderr, "interrupt thread: failed to join\n");
//         }
//         start_ = false;
//         tid_ = 0;
//
//         return status;
//     }
//
//     static void* _entry_func(void* arg)
//     {
//         return ((CBThread*)arg)->entry();
//     }
//
//     void* entry()
//     {
//         uint32_t num_events = 2048;
//         while(!stop_) {
//             dev_->poll_completion(num_events, 500000);
//         }
//
//         return 0;
//     }
// };

class Controller {
  private:
    int fd_ = -1;
    unsigned int nsid_;

    struct nvme_aioctx aioctx_;

  public:
    uint64_t capacity_;
    unsigned int index_;

    Controller()
        : capacity_(0) {}
    ~Controller() {}

    int Open(char* devpath);
    int Close();
    int Store(const char* key, int ksize, const char* value, int vsize);
    int Retrieve(const char* key, int ksize, /*OUT*/ char* value);
    int Exist(const char* key, int ksize);
};

#endif
