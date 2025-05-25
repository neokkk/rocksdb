#include <controller.h>

/*
 * KVSSD Device Controller
 */

int Controller::Open(char* devpath)
{
    fd_ = open(devpath, O_RDWR);
    if (fd_ < 0) {
        std::cerr << "can't open a device : " << devpath << std::endl;
        return fd_;
    }

    nsid_ = ioctl(fd_, NVME_IOCTL_ID);
    if (nsid_ == (unsigned)-1) {
        std::cerr << "can't get an ID" << std::endl;
        return -1;
    }

    //check //////////////////////////////

    // async or ipc
    int efd = eventfd(0, 0);
    if (efd < 0) {
        std::cerr << "fail to create an event." << std::endl;
        return -1;
    }

    aioctx_.ctxid = 0;
    aioctx_.eventfd = efd;

    if (ioctl(fd_, NVME_IOCTL_SET_AIOCTX, &aioctx_) < 0) {
        std::cerr << "fail to set_aioctx" << std::endl;
        return -1;
    }
    /////////////////////////////////////////

    return 0;
}

int Controller::Close()
{
    if (fd_ > 0) {
        if (ioctl(fd_, NVME_IOCTL_DEL_AIOCTX, &aioctx_) < 0) {
            std::cerr << "KV device is closed error!" << std::endl;
            return -1;
        }
        close((int)aioctx_.eventfd);
        close(fd_);

        std::cerr << "KV device is closed: fd " << fd_ << std::endl;
        fd_ = -1;
    }

    return 0;
}

int Controller::Store(const char* key, int ksize, const char* value, int vsize)
{
    struct nvme_passthru_kv_cmd cmd;
    memset((void*)&cmd, 0, sizeof(struct nvme_passthru_kv_cmd));

    cmd.opcode = NVME_KV_STORE;
    cmd.nsid = nsid_;

    memcpy((void*)cmd.key, (void*)key, ksize);
    cmd.value_addr = (__u64)value;
    cmd.key_size = ksize;
    cmd.value_size = vsize;

    cmd.ctxid = aioctx_.ctxid;
    cmd.reqid = index_;

    int ret;
    ret = ioctl(fd_, NVME_IOCTL_AIO_CMD, &cmd);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int Controller::Retrieve(const char* key, int ksize, /*OUT*/ char* value)
{
    struct nvme_passthru_kv_cmd cmd;
    memset((void*)&cmd, 0, sizeof(struct nvme_passthru_kv_cmd));

    cmd.opcode = NVME_KV_RETRIEVE;
    cmd.nsid = nsid_;

    memcpy((void*)cmd.key, (void*)key, ksize);
    cmd.value_addr = (__u64)value;
    cmd.key_size = ksize;

    cmd.ctxid = aioctx_.ctxid;
    cmd.reqid = index_;

    int ret = ioctl(fd_, NVME_IOCTL_IO_KV_CMD, &cmd);
    if (ret == 0) {
        ret = cmd.result;
    }

    return ret;
}

int Controller::Exist(const char* key, int ksize)
{
    struct nvme_passthru_kv_cmd cmd;
    memset((void*)&cmd, 0, sizeof(struct nvme_passthru_kv_cmd));

    cmd.opcode = NVME_KV_EXIST;
    cmd.nsid = nsid_;

    memcpy((void*)cmd.key, (void*)key, ksize);
    cmd.key_size = ksize;

    cmd.ctxid = aioctx_.ctxid;
    cmd.reqid = index_;

    int ret = ioctl(fd_, NVME_IOCTL_AIO_CMD, &cmd);

    return (ret == 0) ? true : false;
}

// Controller::aio_cmd_ctx* Controller::get_cmd_ctx(const kv_postprocess_function* cb)
// {
//     bool print_log_flag = true;
//     std::unique_lock<std::mutex> lock(cmdctx_lock);
//     while(free_cmdctxs.empty()) {
//         if(cmdctx_cond.wait_for(lock, std::chrono::seconds(5)) == std::cv_status::timeout && print_log_flag == true) {
//             std::cerr << "max queue depth has reached. wait..." << std::endl;
//             print_log_flag = false;
//         }
//     }
//
//     aio_cmd_ctx* p = free_cmdctxs.back();
//     free_cmdctxs.pop_back();
//     if(cb) {
//         p->post_fn = cb->post_fn;
//         p->post_data = cb->private_data;
//     } else {
//         p->post_fn = NULL;
//         p->post_data = NULL;
//     }
//     pending_cmdctxs.insert(std::make_pair(p->index, p));
//     return p;
// }
//
// void KADI::release_cmd_ctx(aio_cmd_ctx* p)
// {
//     std::lock_guard<std::mutex> lock(cmdctx_lock);
//
//     pending_cmdctxs.erase(p->index);
//     free_cmdctxs.push_back(p);
//     cmdctx_cond.notify_one();
// }
//
// void Controller::Complete(kvs_postprocess_context* ioctx)
// {
//     switch(ioctx->context) {
//         case KVS_CMD_DELETE:
//             delete[] (char*)(ioctx->key)->key;
//             delete ioctx->key;
//             break;
//         case KVS_CMD_STORE:
//             delete[] (char*)(ioctx->key)->key;
//             delete[] (char*)(ioctx->value)->value;
//             delete ioctx->key;
//             delete ioctx->value;
//             break;
//     }
// }

/*
bool Controller::iterator(int dnum, string pattern, vector<string>& klist)
{
	kvs_result ret;

	// Open iterator
	kvs_iterator_handle iter_hd;
	kvs_option_iterator iter_op = {KVS_ITERATOR_KEY};
	kvs_key_group_filter iter_fltr;

	// array key set
	int bitmask = 0xffffffff;
	memcpy(iter_fltr.bitmask, &bitmask, sizeof(iter_fltr.bitmask));
	memcpy(iter_fltr.bit_pattern, pattern.c_str(), sizeof(iter_fltr.bit_pattern));

	ret = kvs_create_iterator(*ks_hd[dnum], &iter_op, &iter_fltr, &iter_hd);
	if(ret != KVS_SUCCESS) {
		fprintf(stderr, "iterator open fails with error 0x%x\n", ret);
		return -1;
	}

	// Do iteration
	kvs_iterator_list* iter_list = new kvs_iterator_list;

	char* buffer = new char[ITER_BUFFER_SIZE];
	memset(buffer, 0, ITER_BUFFER_SIZE);

	iter_list->it_list = (uint8_t*)buffer;
	iter_list->size = ITER_BUFFER_SIZE;
	iter_list->end = 0;
	iter_list->num_entries = 0;

	while(1) {
		iter_list->size = ITER_BUFFER_SIZE;
		memset(iter_list->it_list, 0, ITER_BUFFER_SIZE);

		ret = kvs_iterate_next(*ks_hd[dnum], iter_hd, iter_list);
		//ret = kvs_iterate_next_async(*ks_hd[dnum], iter_hd, iter_list, NULL, &data, complete);
		if (ret != KVS_SUCCESS) {
			fprintf(stderr, "iterator next fails with error 0x%x\n", ret);
			if((ret = kvs_delete_iterator(*ks_hd[dnum], iter_hd)) != KVS_SUCCESS)
				fprintf(stderr, "delete iterator after next fails with error 0x%x\n", ret);
			delete[] buffer;
			delete iter_list;
			return -1;
		}

		// key list save
		char *it_buffer = (char*)iter_list->it_list;
		for(int i=0; i<iter_list->num_entries; ++i) {
			int ksize = *(int*)it_buffer;
			it_buffer += sizeof(ksize);

			string skey(it_buffer, ksize);
			it_buffer += ksize;

			klist.push_back(skey);
		}

		if(iter_list->end) break;
	}

	// Close iteration
	ret = kvs_delete_iterator(*ks_hd[dnum], iter_hd);
	if(ret != KVS_SUCCESS) {
		fprintf(stderr, "Failed to close iterator\n");
		delete[] buffer;
		delete iter_list;
		return -1;
	}

	delete[] buffer;
	delete iter_list;

	return 0;
}
*/

// bool Controller::Erase(int dnum, const char* key, bool sync)
// {
//     int ksize = (int)strlen(key);
//     char* key_ = new char[ksize];
//     memcpy(key_, key, ksize);
//
//     kvs_option_delete option = {false};
//
//     kvs_key* kvskey = new kvs_key {key_, (uint16_t)ksize};
//
//     kvs_result ret;
//     if(sync) {
//         ret = kvs_delete_kvp(*ks_[dnum], kvskey, &option);
//
//         delete[] key_;
//         delete kvskey;
//     } else {
//         ret = kvs_delete_kvp_async(*ks_[dnum], kvskey, &option, 0, 0, Complete);
//     }
//     if(ret != KVS_SUCCESS) {
//         fprintf(stderr, "erase failed with error 0x%x\n", ret);
//         return -1;
//     }
//
//     return 0;
// }
