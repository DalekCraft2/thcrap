/**
  * Touhou Community Reliant Automatic Patcher
  * Main DLL
  *
  * ----
  *
  * Abstractions and approximations of Win32 API features that are not
  * available on Windows XP.
  */

#include "thcrap.h"

/// Slim Reader/Writer Locks
/// ------------------------
THCRAP_API srwlock_func_t *srwlock_funcs[4];

#define SPIN_UNTIL(cond) while(!(cond)) { Sleep(1); }

// XP approximations. We simply treat the SRW lock
// value as an integer with the following values:
// • -1: exclusive
// •  0: released
// • >0: shared

void TH_STDCALL XP_AcquireSRWLockExclusive(PSRWLOCK SRWLock)
{
	long *val = (long *)&SRWLock->Ptr;
	SPIN_UNTIL(*val == 0);
	while(*val != -1) {
		InterlockedCompareExchange(val, -1, 0);
	}
};

void TH_STDCALL XP_ReleaseSRWLockExclusive(PSRWLOCK SRWLock)
{
	long *val = (long *)&SRWLock->Ptr;
	assert(*val == -1);
	while(*val != 0) {
		InterlockedCompareExchange(val, 0, -1);
	}
};

void TH_STDCALL XP_AcquireSRWLockShared(PSRWLOCK SRWLock)
{
	long *val = (long *)&SRWLock->Ptr;
	SPIN_UNTIL(*val >= 0);
	InterlockedIncrement(val);
};

void TH_STDCALL XP_ReleaseSRWLockShared(PSRWLOCK SRWLock)
{
	long *val = (long *)&SRWLock->Ptr;
	assert(*val >= 1);
	InterlockedDecrement(val);
};

static int swrlock_init = []
{
	auto kernel32 = GetModuleHandleA("kernel32.dll");
	AcquireSRWLockExclusive = (srwlock_func_t *)GetProcAddress(kernel32, "AcquireSRWLockExclusive");
	ReleaseSRWLockExclusive = (srwlock_func_t *)GetProcAddress(kernel32, "ReleaseSRWLockExclusive");
	AcquireSRWLockShared = (srwlock_func_t *)GetProcAddress(kernel32, "AcquireSRWLockShared");
	ReleaseSRWLockShared = (srwlock_func_t *)GetProcAddress(kernel32, "ReleaseSRWLockShared");
	if(
		srwlock_funcs[0] == nullptr || srwlock_funcs[1] == nullptr ||
		srwlock_funcs[2] == nullptr || srwlock_funcs[3] == nullptr
	) {
	  AcquireSRWLockExclusive = XP_AcquireSRWLockExclusive;
	  ReleaseSRWLockExclusive = XP_ReleaseSRWLockExclusive;
	  AcquireSRWLockShared = XP_AcquireSRWLockShared;
	  ReleaseSRWLockShared = XP_ReleaseSRWLockShared;
	}
	return 0;
}();
/// ------------------------

/// Thread-local structures
/// -----------------------
inline void tlsstruct_default_ctor(void *instance, size_t struct_size)
{
	ZeroMemory(instance, struct_size);
}

void* tlsstruct_get(DWORD slot, size_t struct_size, tlsstruct_ctor_t *ctor)
{
	void *ret = TlsGetValue(slot);
	if(!ret) {
		ret = malloc(struct_size);
		if(ret) {
			if(ctor == nullptr) {
				tlsstruct_default_ctor(ret, struct_size);
			} else {
				ctor(ret, struct_size);
			}
		}
		TlsSetValue(slot, ret);
	}
	return ret;
}

void tlsstruct_free(DWORD slot, tlsstruct_dtor_t *dtor)
{
	void *ret = TlsGetValue(slot);
	if(ret) {
		if(dtor != nullptr) {
			dtor(ret);
		}
		free(ret);
		TlsSetValue(slot, nullptr);
	}
}
/// -----------------------
