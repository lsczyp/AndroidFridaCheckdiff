
#ifndef ELFINFO_H_
#define ELFINFO_H_
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#if __LP64__
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

// Android uses RELA for aarch64 and x86_64. mips64 still uses REL.
#if defined(__aarch64__) || defined(__x86_64__)
#define USE_RELA 1
#endif

#define DT_GNU_HASH 0x6ffffef5

enum GETELFTYPE
{
	IN_VM = 0,
	IN_FILE
};

struct ELF_INFO
{
	ElfW(Addr) base;
	char name[2];
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *Phdr;
	ElfW(Dyn) *Pdyn;

	ElfW(Sym) * symtab;
	size_t symsz;

	char* strtab;
#ifdef __LP64__
	Elf64_Xword strsz;
#else
	Elf32_Sword strsz;
#endif

	ElfW(Rel) *rel;
#ifdef __LP64__
	Elf64_Xword rel_count;
#else
	Elf32_Sword rel_count;
#endif

	ElfW(Rel) *plt_rel;
#ifdef __LP64__
	Elf64_Xword plt_rel_count;
#else
	Elf32_Sword plt_rel_count;
#endif

#ifdef USE_RELA
	ElfW(Rela) *rela;
	Elf64_Xword rela_count;

	ElfW(Rela) *plt_rela;
	Elf64_Xword plt_rela_count;
#endif

	uint32_t nbucket;
	uint32_t nchain;

	uint32_t * bucket;
	uint32_t * chain;

	size_t gnu_nbucket_;
	uint32_t* gnu_bucket_;
	uint32_t* gnu_chain_;
	uint32_t gnu_maskwords_;
	uint32_t gnu_shift2_;
	ElfW(Addr)* gnu_bloom_filter_;

	ElfW(Addr) load_bias;
};



struct API_uAddr_pAddr
{
	ElfW(Addr) uAddr;
	ElfW(Addr) pAddr;
};


extern "C" {
unsigned long getapi_addr_REL(void *base, const char *sapiname,GETELFTYPE bvf);

int  GetsoAPIaddr(void * base,const char *sapiname,GETELFTYPE bvf,API_uAddr_pAddr &aup);

}
#endif /* ELFINFO_H_ */
