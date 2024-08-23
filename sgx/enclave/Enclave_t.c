#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


typedef struct ms_handle_t {
	sgx_status_t ms_retval;
} ms_handle_t;

typedef struct ms_t_global_init_ecall_t {
	uint64_t ms_id;
	const uint8_t* ms_path;
	size_t ms_len;
} ms_t_global_init_ecall_t;

typedef struct ms_u_thread_set_event_ocall_t {
	int ms_retval;
	int* ms_error;
	const void* ms_tcs;
} ms_u_thread_set_event_ocall_t;

typedef struct ms_u_thread_wait_event_ocall_t {
	int ms_retval;
	int* ms_error;
	const void* ms_tcs;
	const struct timespec* ms_timeout;
} ms_u_thread_wait_event_ocall_t;

typedef struct ms_u_thread_set_multiple_events_ocall_t {
	int ms_retval;
	int* ms_error;
	const void** ms_tcss;
	int ms_total;
} ms_u_thread_set_multiple_events_ocall_t;

typedef struct ms_u_thread_setwait_events_ocall_t {
	int ms_retval;
	int* ms_error;
	const void* ms_waiter_tcs;
	const void* ms_self_tcs;
	const struct timespec* ms_timeout;
} ms_u_thread_setwait_events_ocall_t;

typedef struct ms_u_clock_gettime_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_clk_id;
	struct timespec* ms_tp;
} ms_u_clock_gettime_ocall_t;

typedef struct ms_u_read_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	void* ms_buf;
	size_t ms_count;
} ms_u_read_ocall_t;

typedef struct ms_u_pread64_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	void* ms_buf;
	size_t ms_count;
	int64_t ms_offset;
} ms_u_pread64_ocall_t;

typedef struct ms_u_readv_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const struct iovec* ms_iov;
	int ms_iovcnt;
} ms_u_readv_ocall_t;

typedef struct ms_u_preadv64_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const struct iovec* ms_iov;
	int ms_iovcnt;
	int64_t ms_offset;
} ms_u_preadv64_ocall_t;

typedef struct ms_u_write_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const void* ms_buf;
	size_t ms_count;
} ms_u_write_ocall_t;

typedef struct ms_u_pwrite64_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const void* ms_buf;
	size_t ms_count;
	int64_t ms_offset;
} ms_u_pwrite64_ocall_t;

typedef struct ms_u_writev_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const struct iovec* ms_iov;
	int ms_iovcnt;
} ms_u_writev_ocall_t;

typedef struct ms_u_pwritev64_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd;
	const struct iovec* ms_iov;
	int ms_iovcnt;
	int64_t ms_offset;
} ms_u_pwritev64_ocall_t;

typedef struct ms_u_sendfile_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_out_fd;
	int ms_in_fd;
	int64_t* ms_offset;
	size_t ms_count;
} ms_u_sendfile_ocall_t;

typedef struct ms_u_copy_file_range_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd_in;
	int64_t* ms_off_in;
	int ms_fd_out;
	int64_t* ms_off_out;
	size_t ms_len;
	unsigned int ms_flags;
} ms_u_copy_file_range_ocall_t;

typedef struct ms_u_splice_ocall_t {
	size_t ms_retval;
	int* ms_error;
	int ms_fd_in;
	int64_t* ms_off_in;
	int ms_fd_out;
	int64_t* ms_off_out;
	size_t ms_len;
	unsigned int ms_flags;
} ms_u_splice_ocall_t;

typedef struct ms_u_fcntl_arg0_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int ms_cmd;
} ms_u_fcntl_arg0_ocall_t;

typedef struct ms_u_fcntl_arg1_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int ms_cmd;
	int ms_arg;
} ms_u_fcntl_arg1_ocall_t;

typedef struct ms_u_ioctl_arg0_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int ms_request;
} ms_u_ioctl_arg0_ocall_t;

typedef struct ms_u_ioctl_arg1_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int ms_request;
	int* ms_arg;
} ms_u_ioctl_arg1_ocall_t;

typedef struct ms_u_close_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
} ms_u_close_ocall_t;

typedef struct ms_u_isatty_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
} ms_u_isatty_ocall_t;

typedef struct ms_u_dup_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_oldfd;
} ms_u_dup_ocall_t;

typedef struct ms_u_eventfd_ocall_t {
	int ms_retval;
	int* ms_error;
	unsigned int ms_initval;
	int ms_flags;
} ms_u_eventfd_ocall_t;

typedef struct ms_u_futimens_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	const struct timespec* ms_times;
} ms_u_futimens_ocall_t;

typedef struct ms_u_malloc_ocall_t {
	void* ms_retval;
	int* ms_error;
	size_t ms_size;
} ms_u_malloc_ocall_t;

typedef struct ms_u_free_ocall_t {
	void* ms_p;
} ms_u_free_ocall_t;

typedef struct ms_u_mmap_ocall_t {
	void* ms_retval;
	int* ms_error;
	void* ms_start;
	size_t ms_length;
	int ms_prot;
	int ms_flags;
	int ms_fd;
	int64_t ms_offset;
} ms_u_mmap_ocall_t;

typedef struct ms_u_munmap_ocall_t {
	int ms_retval;
	int* ms_error;
	void* ms_start;
	size_t ms_length;
} ms_u_munmap_ocall_t;

typedef struct ms_u_msync_ocall_t {
	int ms_retval;
	int* ms_error;
	void* ms_addr;
	size_t ms_length;
	int ms_flags;
} ms_u_msync_ocall_t;

typedef struct ms_u_mprotect_ocall_t {
	int ms_retval;
	int* ms_error;
	void* ms_addr;
	size_t ms_length;
	int ms_prot;
} ms_u_mprotect_ocall_t;

typedef struct ms_u_open_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_pathname;
	int ms_flags;
} ms_u_open_ocall_t;

typedef struct ms_u_open64_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	int ms_oflag;
	int ms_mode;
} ms_u_open64_ocall_t;

typedef struct ms_u_openat_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_dirfd;
	const char* ms_pathname;
	int ms_flags;
} ms_u_openat_ocall_t;

typedef struct ms_u_fstat_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	struct stat_t* ms_buf;
} ms_u_fstat_ocall_t;

typedef struct ms_u_fstat64_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	struct stat64_t* ms_buf;
} ms_u_fstat64_ocall_t;

typedef struct ms_u_stat_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	struct stat_t* ms_buf;
} ms_u_stat_ocall_t;

typedef struct ms_u_stat64_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	struct stat64_t* ms_buf;
} ms_u_stat64_ocall_t;

typedef struct ms_u_lstat_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	struct stat_t* ms_buf;
} ms_u_lstat_ocall_t;

typedef struct ms_u_lstat64_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	struct stat64_t* ms_buf;
} ms_u_lstat64_ocall_t;

typedef struct ms_u_lseek_ocall_t {
	uint64_t ms_retval;
	int* ms_error;
	int ms_fd;
	int64_t ms_offset;
	int ms_whence;
} ms_u_lseek_ocall_t;

typedef struct ms_u_lseek64_ocall_t {
	int64_t ms_retval;
	int* ms_error;
	int ms_fd;
	int64_t ms_offset;
	int ms_whence;
} ms_u_lseek64_ocall_t;

typedef struct ms_u_ftruncate_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int64_t ms_length;
} ms_u_ftruncate_ocall_t;

typedef struct ms_u_ftruncate64_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	int64_t ms_length;
} ms_u_ftruncate64_ocall_t;

typedef struct ms_u_truncate_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	int64_t ms_length;
} ms_u_truncate_ocall_t;

typedef struct ms_u_truncate64_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	int64_t ms_length;
} ms_u_truncate64_ocall_t;

typedef struct ms_u_fsync_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
} ms_u_fsync_ocall_t;

typedef struct ms_u_fdatasync_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
} ms_u_fdatasync_ocall_t;

typedef struct ms_u_fchmod_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_fd;
	uint32_t ms_mode;
} ms_u_fchmod_ocall_t;

typedef struct ms_u_unlink_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_pathname;
} ms_u_unlink_ocall_t;

typedef struct ms_u_link_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_oldpath;
	const char* ms_newpath;
} ms_u_link_ocall_t;

typedef struct ms_u_unlinkat_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_dirfd;
	const char* ms_pathname;
	int ms_flags;
} ms_u_unlinkat_ocall_t;

typedef struct ms_u_linkat_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_olddirfd;
	const char* ms_oldpath;
	int ms_newdirfd;
	const char* ms_newpath;
	int ms_flags;
} ms_u_linkat_ocall_t;

typedef struct ms_u_rename_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_oldpath;
	const char* ms_newpath;
} ms_u_rename_ocall_t;

typedef struct ms_u_chmod_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path;
	uint32_t ms_mode;
} ms_u_chmod_ocall_t;

typedef struct ms_u_readlink_ocall_t {
	size_t ms_retval;
	int* ms_error;
	const char* ms_path;
	char* ms_buf;
	size_t ms_bufsz;
} ms_u_readlink_ocall_t;

typedef struct ms_u_symlink_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_path1;
	const char* ms_path2;
} ms_u_symlink_ocall_t;

typedef struct ms_u_realpath_ocall_t {
	char* ms_retval;
	int* ms_error;
	const char* ms_pathname;
} ms_u_realpath_ocall_t;

typedef struct ms_u_mkdir_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_pathname;
	uint32_t ms_mode;
} ms_u_mkdir_ocall_t;

typedef struct ms_u_rmdir_ocall_t {
	int ms_retval;
	int* ms_error;
	const char* ms_pathname;
} ms_u_rmdir_ocall_t;

typedef struct ms_u_fdopendir_ocall_t {
	void* ms_retval;
	int* ms_error;
	int ms_fd;
} ms_u_fdopendir_ocall_t;

typedef struct ms_u_opendir_ocall_t {
	void* ms_retval;
	int* ms_error;
	const char* ms_pathname;
} ms_u_opendir_ocall_t;

typedef struct ms_u_readdir64_r_ocall_t {
	int ms_retval;
	void* ms_dirp;
	struct dirent64_t* ms_entry;
	struct dirent64_t** ms_result;
} ms_u_readdir64_r_ocall_t;

typedef struct ms_u_closedir_ocall_t {
	int ms_retval;
	int* ms_error;
	void* ms_dirp;
} ms_u_closedir_ocall_t;

typedef struct ms_u_dirfd_ocall_t {
	int ms_retval;
	int* ms_error;
	void* ms_dirp;
} ms_u_dirfd_ocall_t;

typedef struct ms_u_fstatat64_ocall_t {
	int ms_retval;
	int* ms_error;
	int ms_dirfd;
	const char* ms_pathname;
	struct stat64_t* ms_buf;
	int ms_flags;
} ms_u_fstatat64_ocall_t;

typedef struct ms_sgx_oc_cpuidex_t {
	int* ms_cpuinfo;
	int ms_leaf;
	int ms_subleaf;
} ms_sgx_oc_cpuidex_t;

typedef struct ms_sgx_thread_wait_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_self;
} ms_sgx_thread_wait_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_set_untrusted_event_ocall_t {
	int ms_retval;
	const void* ms_waiter;
} ms_sgx_thread_set_untrusted_event_ocall_t;

typedef struct ms_sgx_thread_setwait_untrusted_events_ocall_t {
	int ms_retval;
	const void* ms_waiter;
	const void* ms_self;
} ms_sgx_thread_setwait_untrusted_events_ocall_t;

typedef struct ms_sgx_thread_set_multiple_untrusted_events_ocall_t {
	int ms_retval;
	const void** ms_waiters;
	size_t ms_total;
} ms_sgx_thread_set_multiple_untrusted_events_ocall_t;

static sgx_status_t SGX_CDECL sgx_handle(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_handle_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_handle_t* ms = SGX_CAST(ms_handle_t*, pms);
	ms_handle_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_handle_t), ms, sizeof(ms_handle_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	sgx_status_t _in_retval;


	_in_retval = handle();
	if (memcpy_verw_s(&ms->ms_retval, sizeof(ms->ms_retval), &_in_retval, sizeof(_in_retval))) {
		status = SGX_ERROR_UNEXPECTED;
		goto err;
	}

err:
	return status;
}

static sgx_status_t SGX_CDECL sgx_t_global_init_ecall(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_t_global_init_ecall_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_t_global_init_ecall_t* ms = SGX_CAST(ms_t_global_init_ecall_t*, pms);
	ms_t_global_init_ecall_t __in_ms;
	if (memcpy_s(&__in_ms, sizeof(ms_t_global_init_ecall_t), ms, sizeof(ms_t_global_init_ecall_t))) {
		return SGX_ERROR_UNEXPECTED;
	}
	sgx_status_t status = SGX_SUCCESS;
	const uint8_t* _tmp_path = __in_ms.ms_path;
	size_t _tmp_len = __in_ms.ms_len;
	size_t _len_path = _tmp_len;
	uint8_t* _in_path = NULL;

	CHECK_UNIQUE_POINTER(_tmp_path, _len_path);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_path != NULL && _len_path != 0) {
		if ( _len_path % sizeof(*_tmp_path) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_path = (uint8_t*)malloc(_len_path);
		if (_in_path == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_path, _len_path, _tmp_path, _len_path)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	t_global_init_ecall(__in_ms.ms_id, (const uint8_t*)_in_path, _tmp_len);

err:
	if (_in_path) free(_in_path);
	return status;
}

static sgx_status_t SGX_CDECL sgx_t_global_exit_ecall(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	t_global_exit_ecall();
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[3];
} g_ecall_table = {
	3,
	{
		{(void*)(uintptr_t)sgx_handle, 0, 0},
		{(void*)(uintptr_t)sgx_t_global_init_ecall, 0, 0},
		{(void*)(uintptr_t)sgx_t_global_exit_ecall, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[71][3];
} g_dyn_entry_table = {
	71,
	{
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
		{0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL u_thread_set_event_ocall(int* retval, int* error, const void* tcs)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_thread_set_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_thread_set_event_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_thread_set_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_thread_set_event_ocall_t));
	ocalloc_size -= sizeof(ms_u_thread_set_event_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_tcs, sizeof(ms->ms_tcs), &tcs, sizeof(tcs))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_thread_wait_event_ocall(int* retval, int* error, const void* tcs, const struct timespec* timeout)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_timeout = sizeof(struct timespec);

	ms_u_thread_wait_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_thread_wait_event_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(timeout, _len_timeout);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (timeout != NULL) ? _len_timeout : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_thread_wait_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_thread_wait_event_ocall_t));
	ocalloc_size -= sizeof(ms_u_thread_wait_event_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_tcs, sizeof(ms->ms_tcs), &tcs, sizeof(tcs))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (timeout != NULL) {
		if (memcpy_verw_s(&ms->ms_timeout, sizeof(const struct timespec*), &__tmp, sizeof(const struct timespec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, timeout, _len_timeout)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_timeout);
		ocalloc_size -= _len_timeout;
	} else {
		ms->ms_timeout = NULL;
	}

	status = sgx_ocall(1, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_thread_set_multiple_events_ocall(int* retval, int* error, const void** tcss, int total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_tcss = total * sizeof(void*);

	ms_u_thread_set_multiple_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_thread_set_multiple_events_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(tcss, _len_tcss);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (tcss != NULL) ? _len_tcss : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_thread_set_multiple_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_thread_set_multiple_events_ocall_t));
	ocalloc_size -= sizeof(ms_u_thread_set_multiple_events_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (tcss != NULL) {
		if (memcpy_verw_s(&ms->ms_tcss, sizeof(const void**), &__tmp, sizeof(const void**))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_tcss % sizeof(*tcss) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, tcss, _len_tcss)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_tcss);
		ocalloc_size -= _len_tcss;
	} else {
		ms->ms_tcss = NULL;
	}

	if (memcpy_verw_s(&ms->ms_total, sizeof(ms->ms_total), &total, sizeof(total))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(2, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_thread_setwait_events_ocall(int* retval, int* error, const void* waiter_tcs, const void* self_tcs, const struct timespec* timeout)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_timeout = sizeof(struct timespec);

	ms_u_thread_setwait_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_thread_setwait_events_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(timeout, _len_timeout);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (timeout != NULL) ? _len_timeout : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_thread_setwait_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_thread_setwait_events_ocall_t));
	ocalloc_size -= sizeof(ms_u_thread_setwait_events_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_waiter_tcs, sizeof(ms->ms_waiter_tcs), &waiter_tcs, sizeof(waiter_tcs))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_self_tcs, sizeof(ms->ms_self_tcs), &self_tcs, sizeof(self_tcs))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (timeout != NULL) {
		if (memcpy_verw_s(&ms->ms_timeout, sizeof(const struct timespec*), &__tmp, sizeof(const struct timespec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, timeout, _len_timeout)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_timeout);
		ocalloc_size -= _len_timeout;
	} else {
		ms->ms_timeout = NULL;
	}

	status = sgx_ocall(3, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_clock_gettime_ocall(int* retval, int* error, int clk_id, struct timespec* tp)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_tp = sizeof(struct timespec);

	ms_u_clock_gettime_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_clock_gettime_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_tp = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(tp, _len_tp);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (tp != NULL) ? _len_tp : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_clock_gettime_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_clock_gettime_ocall_t));
	ocalloc_size -= sizeof(ms_u_clock_gettime_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_clk_id, sizeof(ms->ms_clk_id), &clk_id, sizeof(clk_id))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (tp != NULL) {
		if (memcpy_verw_s(&ms->ms_tp, sizeof(struct timespec*), &__tmp, sizeof(struct timespec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_tp = __tmp;
		memset_verw(__tmp_tp, 0, _len_tp);
		__tmp = (void *)((size_t)__tmp + _len_tp);
		ocalloc_size -= _len_tp;
	} else {
		ms->ms_tp = NULL;
	}

	status = sgx_ocall(4, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (tp) {
			if (memcpy_s((void*)tp, _len_tp, __tmp_tp, _len_tp)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_read_ocall(size_t* retval, int* error, int fd, void* buf, size_t count)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_read_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_read_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_read_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_read_ocall_t));
	ocalloc_size -= sizeof(ms_u_read_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_buf, sizeof(ms->ms_buf), &buf, sizeof(buf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_count, sizeof(ms->ms_count), &count, sizeof(count))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(5, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_pread64_ocall(size_t* retval, int* error, int fd, void* buf, size_t count, int64_t offset)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_pread64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_pread64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_pread64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_pread64_ocall_t));
	ocalloc_size -= sizeof(ms_u_pread64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_buf, sizeof(ms->ms_buf), &buf, sizeof(buf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_count, sizeof(ms->ms_count), &count, sizeof(count))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(6, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_readv_ocall(size_t* retval, int* error, int fd, const struct iovec* iov, int iovcnt)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_iov = iovcnt * sizeof(struct iovec);

	ms_u_readv_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_readv_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(iov, _len_iov);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iov != NULL) ? _len_iov : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_readv_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_readv_ocall_t));
	ocalloc_size -= sizeof(ms_u_readv_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (iov != NULL) {
		if (memcpy_verw_s(&ms->ms_iov, sizeof(const struct iovec*), &__tmp, sizeof(const struct iovec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, iov, _len_iov)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iov);
		ocalloc_size -= _len_iov;
	} else {
		ms->ms_iov = NULL;
	}

	if (memcpy_verw_s(&ms->ms_iovcnt, sizeof(ms->ms_iovcnt), &iovcnt, sizeof(iovcnt))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(7, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_preadv64_ocall(size_t* retval, int* error, int fd, const struct iovec* iov, int iovcnt, int64_t offset)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_iov = iovcnt * sizeof(struct iovec);

	ms_u_preadv64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_preadv64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(iov, _len_iov);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iov != NULL) ? _len_iov : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_preadv64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_preadv64_ocall_t));
	ocalloc_size -= sizeof(ms_u_preadv64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (iov != NULL) {
		if (memcpy_verw_s(&ms->ms_iov, sizeof(const struct iovec*), &__tmp, sizeof(const struct iovec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, iov, _len_iov)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iov);
		ocalloc_size -= _len_iov;
	} else {
		ms->ms_iov = NULL;
	}

	if (memcpy_verw_s(&ms->ms_iovcnt, sizeof(ms->ms_iovcnt), &iovcnt, sizeof(iovcnt))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(8, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_write_ocall(size_t* retval, int* error, int fd, const void* buf, size_t count)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_write_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_write_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_write_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_write_ocall_t));
	ocalloc_size -= sizeof(ms_u_write_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_buf, sizeof(ms->ms_buf), &buf, sizeof(buf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_count, sizeof(ms->ms_count), &count, sizeof(count))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(9, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_pwrite64_ocall(size_t* retval, int* error, int fd, const void* buf, size_t count, int64_t offset)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_pwrite64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_pwrite64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_pwrite64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_pwrite64_ocall_t));
	ocalloc_size -= sizeof(ms_u_pwrite64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_buf, sizeof(ms->ms_buf), &buf, sizeof(buf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_count, sizeof(ms->ms_count), &count, sizeof(count))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(10, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_writev_ocall(size_t* retval, int* error, int fd, const struct iovec* iov, int iovcnt)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_iov = iovcnt * sizeof(struct iovec);

	ms_u_writev_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_writev_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(iov, _len_iov);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iov != NULL) ? _len_iov : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_writev_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_writev_ocall_t));
	ocalloc_size -= sizeof(ms_u_writev_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (iov != NULL) {
		if (memcpy_verw_s(&ms->ms_iov, sizeof(const struct iovec*), &__tmp, sizeof(const struct iovec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, iov, _len_iov)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iov);
		ocalloc_size -= _len_iov;
	} else {
		ms->ms_iov = NULL;
	}

	if (memcpy_verw_s(&ms->ms_iovcnt, sizeof(ms->ms_iovcnt), &iovcnt, sizeof(iovcnt))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(11, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_pwritev64_ocall(size_t* retval, int* error, int fd, const struct iovec* iov, int iovcnt, int64_t offset)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_iov = iovcnt * sizeof(struct iovec);

	ms_u_pwritev64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_pwritev64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(iov, _len_iov);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (iov != NULL) ? _len_iov : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_pwritev64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_pwritev64_ocall_t));
	ocalloc_size -= sizeof(ms_u_pwritev64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (iov != NULL) {
		if (memcpy_verw_s(&ms->ms_iov, sizeof(const struct iovec*), &__tmp, sizeof(const struct iovec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, iov, _len_iov)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_iov);
		ocalloc_size -= _len_iov;
	} else {
		ms->ms_iov = NULL;
	}

	if (memcpy_verw_s(&ms->ms_iovcnt, sizeof(ms->ms_iovcnt), &iovcnt, sizeof(iovcnt))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(12, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_sendfile_ocall(size_t* retval, int* error, int out_fd, int in_fd, int64_t* offset, size_t count)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_offset = sizeof(int64_t);

	ms_u_sendfile_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_sendfile_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_offset = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(offset, _len_offset);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (offset != NULL) ? _len_offset : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_sendfile_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_sendfile_ocall_t));
	ocalloc_size -= sizeof(ms_u_sendfile_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_out_fd, sizeof(ms->ms_out_fd), &out_fd, sizeof(out_fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_in_fd, sizeof(ms->ms_in_fd), &in_fd, sizeof(in_fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (offset != NULL) {
		if (memcpy_verw_s(&ms->ms_offset, sizeof(int64_t*), &__tmp, sizeof(int64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_offset = __tmp;
		if (_len_offset % sizeof(*offset) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, offset, _len_offset)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_offset);
		ocalloc_size -= _len_offset;
	} else {
		ms->ms_offset = NULL;
	}

	if (memcpy_verw_s(&ms->ms_count, sizeof(ms->ms_count), &count, sizeof(count))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(13, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (offset) {
			if (memcpy_s((void*)offset, _len_offset, __tmp_offset, _len_offset)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_copy_file_range_ocall(size_t* retval, int* error, int fd_in, int64_t* off_in, int fd_out, int64_t* off_out, size_t len, unsigned int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_off_in = sizeof(int64_t);
	size_t _len_off_out = sizeof(int64_t);

	ms_u_copy_file_range_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_copy_file_range_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_off_in = NULL;
	void *__tmp_off_out = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(off_in, _len_off_in);
	CHECK_ENCLAVE_POINTER(off_out, _len_off_out);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (off_in != NULL) ? _len_off_in : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (off_out != NULL) ? _len_off_out : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_copy_file_range_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_copy_file_range_ocall_t));
	ocalloc_size -= sizeof(ms_u_copy_file_range_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd_in, sizeof(ms->ms_fd_in), &fd_in, sizeof(fd_in))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (off_in != NULL) {
		if (memcpy_verw_s(&ms->ms_off_in, sizeof(int64_t*), &__tmp, sizeof(int64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_off_in = __tmp;
		if (_len_off_in % sizeof(*off_in) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, off_in, _len_off_in)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_off_in);
		ocalloc_size -= _len_off_in;
	} else {
		ms->ms_off_in = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd_out, sizeof(ms->ms_fd_out), &fd_out, sizeof(fd_out))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (off_out != NULL) {
		if (memcpy_verw_s(&ms->ms_off_out, sizeof(int64_t*), &__tmp, sizeof(int64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_off_out = __tmp;
		if (_len_off_out % sizeof(*off_out) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, off_out, _len_off_out)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_off_out);
		ocalloc_size -= _len_off_out;
	} else {
		ms->ms_off_out = NULL;
	}

	if (memcpy_verw_s(&ms->ms_len, sizeof(ms->ms_len), &len, sizeof(len))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(14, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (off_in) {
			if (memcpy_s((void*)off_in, _len_off_in, __tmp_off_in, _len_off_in)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (off_out) {
			if (memcpy_s((void*)off_out, _len_off_out, __tmp_off_out, _len_off_out)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_splice_ocall(size_t* retval, int* error, int fd_in, int64_t* off_in, int fd_out, int64_t* off_out, size_t len, unsigned int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_off_in = sizeof(int64_t);
	size_t _len_off_out = sizeof(int64_t);

	ms_u_splice_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_splice_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_off_in = NULL;
	void *__tmp_off_out = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(off_in, _len_off_in);
	CHECK_ENCLAVE_POINTER(off_out, _len_off_out);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (off_in != NULL) ? _len_off_in : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (off_out != NULL) ? _len_off_out : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_splice_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_splice_ocall_t));
	ocalloc_size -= sizeof(ms_u_splice_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd_in, sizeof(ms->ms_fd_in), &fd_in, sizeof(fd_in))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (off_in != NULL) {
		if (memcpy_verw_s(&ms->ms_off_in, sizeof(int64_t*), &__tmp, sizeof(int64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_off_in = __tmp;
		if (_len_off_in % sizeof(*off_in) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, off_in, _len_off_in)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_off_in);
		ocalloc_size -= _len_off_in;
	} else {
		ms->ms_off_in = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd_out, sizeof(ms->ms_fd_out), &fd_out, sizeof(fd_out))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (off_out != NULL) {
		if (memcpy_verw_s(&ms->ms_off_out, sizeof(int64_t*), &__tmp, sizeof(int64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_off_out = __tmp;
		if (_len_off_out % sizeof(*off_out) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, off_out, _len_off_out)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_off_out);
		ocalloc_size -= _len_off_out;
	} else {
		ms->ms_off_out = NULL;
	}

	if (memcpy_verw_s(&ms->ms_len, sizeof(ms->ms_len), &len, sizeof(len))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(15, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (off_in) {
			if (memcpy_s((void*)off_in, _len_off_in, __tmp_off_in, _len_off_in)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (off_out) {
			if (memcpy_s((void*)off_out, _len_off_out, __tmp_off_out, _len_off_out)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fcntl_arg0_ocall(int* retval, int* error, int fd, int cmd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fcntl_arg0_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fcntl_arg0_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fcntl_arg0_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fcntl_arg0_ocall_t));
	ocalloc_size -= sizeof(ms_u_fcntl_arg0_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_cmd, sizeof(ms->ms_cmd), &cmd, sizeof(cmd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(16, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fcntl_arg1_ocall(int* retval, int* error, int fd, int cmd, int arg)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fcntl_arg1_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fcntl_arg1_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fcntl_arg1_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fcntl_arg1_ocall_t));
	ocalloc_size -= sizeof(ms_u_fcntl_arg1_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_cmd, sizeof(ms->ms_cmd), &cmd, sizeof(cmd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_arg, sizeof(ms->ms_arg), &arg, sizeof(arg))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(17, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_ioctl_arg0_ocall(int* retval, int* error, int fd, int request)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_ioctl_arg0_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_ioctl_arg0_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_ioctl_arg0_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_ioctl_arg0_ocall_t));
	ocalloc_size -= sizeof(ms_u_ioctl_arg0_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_request, sizeof(ms->ms_request), &request, sizeof(request))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(18, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_ioctl_arg1_ocall(int* retval, int* error, int fd, int request, int* arg)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_arg = sizeof(int);

	ms_u_ioctl_arg1_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_ioctl_arg1_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_arg = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(arg, _len_arg);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (arg != NULL) ? _len_arg : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_ioctl_arg1_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_ioctl_arg1_ocall_t));
	ocalloc_size -= sizeof(ms_u_ioctl_arg1_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_request, sizeof(ms->ms_request), &request, sizeof(request))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (arg != NULL) {
		if (memcpy_verw_s(&ms->ms_arg, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_arg = __tmp;
		if (_len_arg % sizeof(*arg) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, arg, _len_arg)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_arg);
		ocalloc_size -= _len_arg;
	} else {
		ms->ms_arg = NULL;
	}

	status = sgx_ocall(19, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (arg) {
			if (memcpy_s((void*)arg, _len_arg, __tmp_arg, _len_arg)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_close_ocall(int* retval, int* error, int fd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_close_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_close_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_close_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_close_ocall_t));
	ocalloc_size -= sizeof(ms_u_close_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(20, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_isatty_ocall(int* retval, int* error, int fd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_isatty_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_isatty_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_isatty_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_isatty_ocall_t));
	ocalloc_size -= sizeof(ms_u_isatty_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(21, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_dup_ocall(int* retval, int* error, int oldfd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_dup_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_dup_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_dup_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_dup_ocall_t));
	ocalloc_size -= sizeof(ms_u_dup_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_oldfd, sizeof(ms->ms_oldfd), &oldfd, sizeof(oldfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(22, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_eventfd_ocall(int* retval, int* error, unsigned int initval, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_eventfd_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_eventfd_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_eventfd_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_eventfd_ocall_t));
	ocalloc_size -= sizeof(ms_u_eventfd_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_initval, sizeof(ms->ms_initval), &initval, sizeof(initval))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(23, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_futimens_ocall(int* retval, int* error, int fd, const struct timespec* times)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_times = 2 * sizeof(struct timespec);

	ms_u_futimens_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_futimens_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(times, _len_times);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (times != NULL) ? _len_times : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_futimens_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_futimens_ocall_t));
	ocalloc_size -= sizeof(ms_u_futimens_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (times != NULL) {
		if (memcpy_verw_s(&ms->ms_times, sizeof(const struct timespec*), &__tmp, sizeof(const struct timespec*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, times, _len_times)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_times);
		ocalloc_size -= _len_times;
	} else {
		ms->ms_times = NULL;
	}

	status = sgx_ocall(24, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_malloc_ocall(void** retval, int* error, size_t size)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_malloc_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_malloc_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_malloc_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_malloc_ocall_t));
	ocalloc_size -= sizeof(ms_u_malloc_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_size, sizeof(ms->ms_size), &size, sizeof(size))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(25, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_free_ocall(void* p)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_u_free_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_free_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_free_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_free_ocall_t));
	ocalloc_size -= sizeof(ms_u_free_ocall_t);

	if (memcpy_verw_s(&ms->ms_p, sizeof(ms->ms_p), &p, sizeof(p))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(26, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_mmap_ocall(void** retval, int* error, void* start, size_t length, int prot, int flags, int fd, int64_t offset)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_mmap_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_mmap_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_mmap_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_mmap_ocall_t));
	ocalloc_size -= sizeof(ms_u_mmap_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_start, sizeof(ms->ms_start), &start, sizeof(start))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_prot, sizeof(ms->ms_prot), &prot, sizeof(prot))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(27, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_munmap_ocall(int* retval, int* error, void* start, size_t length)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_munmap_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_munmap_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_munmap_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_munmap_ocall_t));
	ocalloc_size -= sizeof(ms_u_munmap_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_start, sizeof(ms->ms_start), &start, sizeof(start))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(28, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_msync_ocall(int* retval, int* error, void* addr, size_t length, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_msync_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_msync_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_msync_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_msync_ocall_t));
	ocalloc_size -= sizeof(ms_u_msync_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_addr, sizeof(ms->ms_addr), &addr, sizeof(addr))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(29, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_mprotect_ocall(int* retval, int* error, void* addr, size_t length, int prot)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_mprotect_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_mprotect_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_mprotect_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_mprotect_ocall_t));
	ocalloc_size -= sizeof(ms_u_mprotect_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_addr, sizeof(ms->ms_addr), &addr, sizeof(addr))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_prot, sizeof(ms->ms_prot), &prot, sizeof(prot))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(30, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_open_ocall(int* retval, int* error, const char* pathname, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_open_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_open_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_open_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_open_ocall_t));
	ocalloc_size -= sizeof(ms_u_open_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(31, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_open64_ocall(int* retval, int* error, const char* path, int oflag, int mode)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;

	ms_u_open64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_open64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_open64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_open64_ocall_t));
	ocalloc_size -= sizeof(ms_u_open64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (memcpy_verw_s(&ms->ms_oflag, sizeof(ms->ms_oflag), &oflag, sizeof(oflag))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_mode, sizeof(ms->ms_mode), &mode, sizeof(mode))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(32, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_openat_ocall(int* retval, int* error, int dirfd, const char* pathname, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_openat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_openat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_openat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_openat_ocall_t));
	ocalloc_size -= sizeof(ms_u_openat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_dirfd, sizeof(ms->ms_dirfd), &dirfd, sizeof(dirfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(33, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fstat_ocall(int* retval, int* error, int fd, struct stat_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_buf = sizeof(struct stat_t);

	ms_u_fstat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fstat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fstat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fstat_ocall_t));
	ocalloc_size -= sizeof(ms_u_fstat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat_t*), &__tmp, sizeof(struct stat_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(34, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fstat64_ocall(int* retval, int* error, int fd, struct stat64_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_buf = sizeof(struct stat64_t);

	ms_u_fstat64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fstat64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fstat64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fstat64_ocall_t));
	ocalloc_size -= sizeof(ms_u_fstat64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat64_t*), &__tmp, sizeof(struct stat64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(35, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_stat_ocall(int* retval, int* error, const char* path, struct stat_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;
	size_t _len_buf = sizeof(struct stat_t);

	ms_u_stat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_stat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_stat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_stat_ocall_t));
	ocalloc_size -= sizeof(ms_u_stat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat_t*), &__tmp, sizeof(struct stat_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(36, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_stat64_ocall(int* retval, int* error, const char* path, struct stat64_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;
	size_t _len_buf = sizeof(struct stat64_t);

	ms_u_stat64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_stat64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_stat64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_stat64_ocall_t));
	ocalloc_size -= sizeof(ms_u_stat64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat64_t*), &__tmp, sizeof(struct stat64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(37, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_lstat_ocall(int* retval, int* error, const char* path, struct stat_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;
	size_t _len_buf = sizeof(struct stat_t);

	ms_u_lstat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_lstat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_lstat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_lstat_ocall_t));
	ocalloc_size -= sizeof(ms_u_lstat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat_t*), &__tmp, sizeof(struct stat_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(38, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_lstat64_ocall(int* retval, int* error, const char* path, struct stat64_t* buf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;
	size_t _len_buf = sizeof(struct stat64_t);

	ms_u_lstat64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_lstat64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_lstat64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_lstat64_ocall_t));
	ocalloc_size -= sizeof(ms_u_lstat64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat64_t*), &__tmp, sizeof(struct stat64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	status = sgx_ocall(39, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_lseek_ocall(uint64_t* retval, int* error, int fd, int64_t offset, int whence)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_lseek_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_lseek_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_lseek_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_lseek_ocall_t));
	ocalloc_size -= sizeof(ms_u_lseek_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_whence, sizeof(ms->ms_whence), &whence, sizeof(whence))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(40, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_lseek64_ocall(int64_t* retval, int* error, int fd, int64_t offset, int whence)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_lseek64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_lseek64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_lseek64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_lseek64_ocall_t));
	ocalloc_size -= sizeof(ms_u_lseek64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_offset, sizeof(ms->ms_offset), &offset, sizeof(offset))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_whence, sizeof(ms->ms_whence), &whence, sizeof(whence))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(41, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_ftruncate_ocall(int* retval, int* error, int fd, int64_t length)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_ftruncate_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_ftruncate_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_ftruncate_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_ftruncate_ocall_t));
	ocalloc_size -= sizeof(ms_u_ftruncate_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(42, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_ftruncate64_ocall(int* retval, int* error, int fd, int64_t length)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_ftruncate64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_ftruncate64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_ftruncate64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_ftruncate64_ocall_t));
	ocalloc_size -= sizeof(ms_u_ftruncate64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(43, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_truncate_ocall(int* retval, int* error, const char* path, int64_t length)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;

	ms_u_truncate_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_truncate_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_truncate_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_truncate_ocall_t));
	ocalloc_size -= sizeof(ms_u_truncate_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(44, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_truncate64_ocall(int* retval, int* error, const char* path, int64_t length)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;

	ms_u_truncate64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_truncate64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_truncate64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_truncate64_ocall_t));
	ocalloc_size -= sizeof(ms_u_truncate64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (memcpy_verw_s(&ms->ms_length, sizeof(ms->ms_length), &length, sizeof(length))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(45, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fsync_ocall(int* retval, int* error, int fd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fsync_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fsync_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fsync_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fsync_ocall_t));
	ocalloc_size -= sizeof(ms_u_fsync_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(46, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fdatasync_ocall(int* retval, int* error, int fd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fdatasync_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fdatasync_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fdatasync_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fdatasync_ocall_t));
	ocalloc_size -= sizeof(ms_u_fdatasync_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(47, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fchmod_ocall(int* retval, int* error, int fd, uint32_t mode)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fchmod_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fchmod_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fchmod_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fchmod_ocall_t));
	ocalloc_size -= sizeof(ms_u_fchmod_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_mode, sizeof(ms->ms_mode), &mode, sizeof(mode))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(48, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_unlink_ocall(int* retval, int* error, const char* pathname)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_unlink_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_unlink_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_unlink_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_unlink_ocall_t));
	ocalloc_size -= sizeof(ms_u_unlink_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	status = sgx_ocall(49, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_link_ocall(int* retval, int* error, const char* oldpath, const char* newpath)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_oldpath = oldpath ? strlen(oldpath) + 1 : 0;
	size_t _len_newpath = newpath ? strlen(newpath) + 1 : 0;

	ms_u_link_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_link_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(oldpath, _len_oldpath);
	CHECK_ENCLAVE_POINTER(newpath, _len_newpath);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (oldpath != NULL) ? _len_oldpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (newpath != NULL) ? _len_newpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_link_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_link_ocall_t));
	ocalloc_size -= sizeof(ms_u_link_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (oldpath != NULL) {
		if (memcpy_verw_s(&ms->ms_oldpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_oldpath % sizeof(*oldpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, oldpath, _len_oldpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_oldpath);
		ocalloc_size -= _len_oldpath;
	} else {
		ms->ms_oldpath = NULL;
	}

	if (newpath != NULL) {
		if (memcpy_verw_s(&ms->ms_newpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_newpath % sizeof(*newpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, newpath, _len_newpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_newpath);
		ocalloc_size -= _len_newpath;
	} else {
		ms->ms_newpath = NULL;
	}

	status = sgx_ocall(50, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_unlinkat_ocall(int* retval, int* error, int dirfd, const char* pathname, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_unlinkat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_unlinkat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_unlinkat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_unlinkat_ocall_t));
	ocalloc_size -= sizeof(ms_u_unlinkat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_dirfd, sizeof(ms->ms_dirfd), &dirfd, sizeof(dirfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(51, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_linkat_ocall(int* retval, int* error, int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_oldpath = oldpath ? strlen(oldpath) + 1 : 0;
	size_t _len_newpath = newpath ? strlen(newpath) + 1 : 0;

	ms_u_linkat_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_linkat_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(oldpath, _len_oldpath);
	CHECK_ENCLAVE_POINTER(newpath, _len_newpath);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (oldpath != NULL) ? _len_oldpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (newpath != NULL) ? _len_newpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_linkat_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_linkat_ocall_t));
	ocalloc_size -= sizeof(ms_u_linkat_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_olddirfd, sizeof(ms->ms_olddirfd), &olddirfd, sizeof(olddirfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (oldpath != NULL) {
		if (memcpy_verw_s(&ms->ms_oldpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_oldpath % sizeof(*oldpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, oldpath, _len_oldpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_oldpath);
		ocalloc_size -= _len_oldpath;
	} else {
		ms->ms_oldpath = NULL;
	}

	if (memcpy_verw_s(&ms->ms_newdirfd, sizeof(ms->ms_newdirfd), &newdirfd, sizeof(newdirfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (newpath != NULL) {
		if (memcpy_verw_s(&ms->ms_newpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_newpath % sizeof(*newpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, newpath, _len_newpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_newpath);
		ocalloc_size -= _len_newpath;
	} else {
		ms->ms_newpath = NULL;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(52, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_rename_ocall(int* retval, int* error, const char* oldpath, const char* newpath)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_oldpath = oldpath ? strlen(oldpath) + 1 : 0;
	size_t _len_newpath = newpath ? strlen(newpath) + 1 : 0;

	ms_u_rename_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_rename_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(oldpath, _len_oldpath);
	CHECK_ENCLAVE_POINTER(newpath, _len_newpath);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (oldpath != NULL) ? _len_oldpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (newpath != NULL) ? _len_newpath : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_rename_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_rename_ocall_t));
	ocalloc_size -= sizeof(ms_u_rename_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (oldpath != NULL) {
		if (memcpy_verw_s(&ms->ms_oldpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_oldpath % sizeof(*oldpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, oldpath, _len_oldpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_oldpath);
		ocalloc_size -= _len_oldpath;
	} else {
		ms->ms_oldpath = NULL;
	}

	if (newpath != NULL) {
		if (memcpy_verw_s(&ms->ms_newpath, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_newpath % sizeof(*newpath) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, newpath, _len_newpath)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_newpath);
		ocalloc_size -= _len_newpath;
	} else {
		ms->ms_newpath = NULL;
	}

	status = sgx_ocall(53, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_chmod_ocall(int* retval, int* error, const char* path, uint32_t mode)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;

	ms_u_chmod_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_chmod_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_chmod_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_chmod_ocall_t));
	ocalloc_size -= sizeof(ms_u_chmod_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (memcpy_verw_s(&ms->ms_mode, sizeof(ms->ms_mode), &mode, sizeof(mode))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(54, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_readlink_ocall(size_t* retval, int* error, const char* path, char* buf, size_t bufsz)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path = path ? strlen(path) + 1 : 0;
	size_t _len_buf = bufsz;

	ms_u_readlink_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_readlink_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path, _len_path);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path != NULL) ? _len_path : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_readlink_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_readlink_ocall_t));
	ocalloc_size -= sizeof(ms_u_readlink_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path != NULL) {
		if (memcpy_verw_s(&ms->ms_path, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path % sizeof(*path) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path, _len_path)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path);
		ocalloc_size -= _len_path;
	} else {
		ms->ms_path = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(char*), &__tmp, sizeof(char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		if (_len_buf % sizeof(*buf) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	if (memcpy_verw_s(&ms->ms_bufsz, sizeof(ms->ms_bufsz), &bufsz, sizeof(bufsz))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(55, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_symlink_ocall(int* retval, int* error, const char* path1, const char* path2)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_path1 = path1 ? strlen(path1) + 1 : 0;
	size_t _len_path2 = path2 ? strlen(path2) + 1 : 0;

	ms_u_symlink_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_symlink_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(path1, _len_path1);
	CHECK_ENCLAVE_POINTER(path2, _len_path2);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path1 != NULL) ? _len_path1 : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (path2 != NULL) ? _len_path2 : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_symlink_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_symlink_ocall_t));
	ocalloc_size -= sizeof(ms_u_symlink_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (path1 != NULL) {
		if (memcpy_verw_s(&ms->ms_path1, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path1 % sizeof(*path1) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path1, _len_path1)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path1);
		ocalloc_size -= _len_path1;
	} else {
		ms->ms_path1 = NULL;
	}

	if (path2 != NULL) {
		if (memcpy_verw_s(&ms->ms_path2, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_path2 % sizeof(*path2) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, path2, _len_path2)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_path2);
		ocalloc_size -= _len_path2;
	} else {
		ms->ms_path2 = NULL;
	}

	status = sgx_ocall(56, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_realpath_ocall(char** retval, int* error, const char* pathname)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_realpath_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_realpath_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_realpath_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_realpath_ocall_t));
	ocalloc_size -= sizeof(ms_u_realpath_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	status = sgx_ocall(57, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_mkdir_ocall(int* retval, int* error, const char* pathname, uint32_t mode)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_mkdir_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_mkdir_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_mkdir_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_mkdir_ocall_t));
	ocalloc_size -= sizeof(ms_u_mkdir_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	if (memcpy_verw_s(&ms->ms_mode, sizeof(ms->ms_mode), &mode, sizeof(mode))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(58, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_rmdir_ocall(int* retval, int* error, const char* pathname)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_rmdir_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_rmdir_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_rmdir_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_rmdir_ocall_t));
	ocalloc_size -= sizeof(ms_u_rmdir_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	status = sgx_ocall(59, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fdopendir_ocall(void** retval, int* error, int fd)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_fdopendir_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fdopendir_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fdopendir_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fdopendir_ocall_t));
	ocalloc_size -= sizeof(ms_u_fdopendir_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_fd, sizeof(ms->ms_fd), &fd, sizeof(fd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(60, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_opendir_ocall(void** retval, int* error, const char* pathname)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;

	ms_u_opendir_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_opendir_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_opendir_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_opendir_ocall_t));
	ocalloc_size -= sizeof(ms_u_opendir_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	status = sgx_ocall(61, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_readdir64_r_ocall(int* retval, void* dirp, struct dirent64_t* entry, struct dirent64_t** result)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_entry = sizeof(struct dirent64_t);
	size_t _len_result = sizeof(struct dirent64_t*);

	ms_u_readdir64_r_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_readdir64_r_ocall_t);
	void *__tmp = NULL;

	void *__tmp_entry = NULL;
	void *__tmp_result = NULL;

	CHECK_ENCLAVE_POINTER(entry, _len_entry);
	CHECK_ENCLAVE_POINTER(result, _len_result);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (entry != NULL) ? _len_entry : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (result != NULL) ? _len_result : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_readdir64_r_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_readdir64_r_ocall_t));
	ocalloc_size -= sizeof(ms_u_readdir64_r_ocall_t);

	if (memcpy_verw_s(&ms->ms_dirp, sizeof(ms->ms_dirp), &dirp, sizeof(dirp))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (entry != NULL) {
		if (memcpy_verw_s(&ms->ms_entry, sizeof(struct dirent64_t*), &__tmp, sizeof(struct dirent64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_entry = __tmp;
		if (memcpy_verw_s(__tmp, ocalloc_size, entry, _len_entry)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_entry);
		ocalloc_size -= _len_entry;
	} else {
		ms->ms_entry = NULL;
	}

	if (result != NULL) {
		if (memcpy_verw_s(&ms->ms_result, sizeof(struct dirent64_t**), &__tmp, sizeof(struct dirent64_t**))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_result = __tmp;
		if (_len_result % sizeof(*result) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_result, 0, _len_result);
		__tmp = (void *)((size_t)__tmp + _len_result);
		ocalloc_size -= _len_result;
	} else {
		ms->ms_result = NULL;
	}

	status = sgx_ocall(62, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (entry) {
			if (memcpy_s((void*)entry, _len_entry, __tmp_entry, _len_entry)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (result) {
			if (memcpy_s((void*)result, _len_result, __tmp_result, _len_result)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_closedir_ocall(int* retval, int* error, void* dirp)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_closedir_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_closedir_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_closedir_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_closedir_ocall_t));
	ocalloc_size -= sizeof(ms_u_closedir_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_dirp, sizeof(ms->ms_dirp), &dirp, sizeof(dirp))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(63, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_dirfd_ocall(int* retval, int* error, void* dirp)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);

	ms_u_dirfd_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_dirfd_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_dirfd_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_dirfd_ocall_t));
	ocalloc_size -= sizeof(ms_u_dirfd_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_dirp, sizeof(ms->ms_dirp), &dirp, sizeof(dirp))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(64, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL u_fstatat64_ocall(int* retval, int* error, int dirfd, const char* pathname, struct stat64_t* buf, int flags)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_error = sizeof(int);
	size_t _len_pathname = pathname ? strlen(pathname) + 1 : 0;
	size_t _len_buf = sizeof(struct stat64_t);

	ms_u_fstatat64_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_u_fstatat64_ocall_t);
	void *__tmp = NULL;

	void *__tmp_error = NULL;
	void *__tmp_buf = NULL;

	CHECK_ENCLAVE_POINTER(error, _len_error);
	CHECK_ENCLAVE_POINTER(pathname, _len_pathname);
	CHECK_ENCLAVE_POINTER(buf, _len_buf);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (error != NULL) ? _len_error : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (pathname != NULL) ? _len_pathname : 0))
		return SGX_ERROR_INVALID_PARAMETER;
	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (buf != NULL) ? _len_buf : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_u_fstatat64_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_u_fstatat64_ocall_t));
	ocalloc_size -= sizeof(ms_u_fstatat64_ocall_t);

	if (error != NULL) {
		if (memcpy_verw_s(&ms->ms_error, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_error = __tmp;
		if (_len_error % sizeof(*error) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_error, 0, _len_error);
		__tmp = (void *)((size_t)__tmp + _len_error);
		ocalloc_size -= _len_error;
	} else {
		ms->ms_error = NULL;
	}

	if (memcpy_verw_s(&ms->ms_dirfd, sizeof(ms->ms_dirfd), &dirfd, sizeof(dirfd))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (pathname != NULL) {
		if (memcpy_verw_s(&ms->ms_pathname, sizeof(const char*), &__tmp, sizeof(const char*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_pathname % sizeof(*pathname) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, pathname, _len_pathname)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_pathname);
		ocalloc_size -= _len_pathname;
	} else {
		ms->ms_pathname = NULL;
	}

	if (buf != NULL) {
		if (memcpy_verw_s(&ms->ms_buf, sizeof(struct stat64_t*), &__tmp, sizeof(struct stat64_t*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_buf = __tmp;
		memset_verw(__tmp_buf, 0, _len_buf);
		__tmp = (void *)((size_t)__tmp + _len_buf);
		ocalloc_size -= _len_buf;
	} else {
		ms->ms_buf = NULL;
	}

	if (memcpy_verw_s(&ms->ms_flags, sizeof(ms->ms_flags), &flags, sizeof(flags))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(65, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (error) {
			if (memcpy_s((void*)error, _len_error, __tmp_error, _len_error)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
		if (buf) {
			if (memcpy_s((void*)buf, _len_buf, __tmp_buf, _len_buf)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_oc_cpuidex(int cpuinfo[4], int leaf, int subleaf)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_cpuinfo = 4 * sizeof(int);

	ms_sgx_oc_cpuidex_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_oc_cpuidex_t);
	void *__tmp = NULL;

	void *__tmp_cpuinfo = NULL;

	CHECK_ENCLAVE_POINTER(cpuinfo, _len_cpuinfo);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (cpuinfo != NULL) ? _len_cpuinfo : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_oc_cpuidex_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_oc_cpuidex_t));
	ocalloc_size -= sizeof(ms_sgx_oc_cpuidex_t);

	if (cpuinfo != NULL) {
		if (memcpy_verw_s(&ms->ms_cpuinfo, sizeof(int*), &__tmp, sizeof(int*))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp_cpuinfo = __tmp;
		if (_len_cpuinfo % sizeof(*cpuinfo) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		memset_verw(__tmp_cpuinfo, 0, _len_cpuinfo);
		__tmp = (void *)((size_t)__tmp + _len_cpuinfo);
		ocalloc_size -= _len_cpuinfo;
	} else {
		ms->ms_cpuinfo = NULL;
	}

	if (memcpy_verw_s(&ms->ms_leaf, sizeof(ms->ms_leaf), &leaf, sizeof(leaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_subleaf, sizeof(ms->ms_subleaf), &subleaf, sizeof(subleaf))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(66, ms);

	if (status == SGX_SUCCESS) {
		if (cpuinfo) {
			if (memcpy_s((void*)cpuinfo, _len_cpuinfo, __tmp_cpuinfo, _len_cpuinfo)) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_wait_untrusted_event_ocall(int* retval, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_wait_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_wait_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_wait_untrusted_event_ocall_t);

	if (memcpy_verw_s(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(67, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_untrusted_event_ocall(int* retval, const void* waiter)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_set_untrusted_event_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_untrusted_event_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_untrusted_event_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_untrusted_event_ocall_t);

	if (memcpy_verw_s(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(68, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_setwait_untrusted_events_ocall(int* retval, const void* waiter, const void* self)
{
	sgx_status_t status = SGX_SUCCESS;

	ms_sgx_thread_setwait_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);
	void *__tmp = NULL;


	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_setwait_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_setwait_untrusted_events_ocall_t);

	if (memcpy_verw_s(&ms->ms_waiter, sizeof(ms->ms_waiter), &waiter, sizeof(waiter))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	if (memcpy_verw_s(&ms->ms_self, sizeof(ms->ms_self), &self, sizeof(self))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(69, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

sgx_status_t SGX_CDECL sgx_thread_set_multiple_untrusted_events_ocall(int* retval, const void** waiters, size_t total)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_waiters = total * sizeof(void*);

	ms_sgx_thread_set_multiple_untrusted_events_ocall_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(waiters, _len_waiters);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (waiters != NULL) ? _len_waiters : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_sgx_thread_set_multiple_untrusted_events_ocall_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t));
	ocalloc_size -= sizeof(ms_sgx_thread_set_multiple_untrusted_events_ocall_t);

	if (waiters != NULL) {
		if (memcpy_verw_s(&ms->ms_waiters, sizeof(const void**), &__tmp, sizeof(const void**))) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		if (_len_waiters % sizeof(*waiters) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_verw_s(__tmp, ocalloc_size, waiters, _len_waiters)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_waiters);
		ocalloc_size -= _len_waiters;
	} else {
		ms->ms_waiters = NULL;
	}

	if (memcpy_verw_s(&ms->ms_total, sizeof(ms->ms_total), &total, sizeof(total))) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}

	status = sgx_ocall(70, ms);

	if (status == SGX_SUCCESS) {
		if (retval) {
			if (memcpy_s((void*)retval, sizeof(*retval), &ms->ms_retval, sizeof(ms->ms_retval))) {
				sgx_ocfree();
				return SGX_ERROR_UNEXPECTED;
			}
		}
	}
	sgx_ocfree();
	return status;
}

