
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <string.h>
#include "elfinfo.h"
#include "xylog.h"

extern "C" {

#define powerof2(x)     ((((x)-1)&(x))==0)

#define	R_ARM_JUMP_SLOT		22
#define R_ARM_GLOB_DAT		21
#define R_ARM_ABS32		2

#define R_AARCH64_JUMP_SLOT 1026
#define R_AARCH64_GLOB_DAT  1025
#define R_AARCH64_ABS64  257

#define R_386_JMP_SLOT 7
#define R_386_GLOB_DAT 6
#define R_386_32  1

#define R_X86_64_JUMP_SLOT  7
#define R_X86_64_GLOB_DAT   6

#define R_MIPS_JUMP_SLOT    127
#define R_MIPS_GLOB_DAT     51

#ifdef __LP64__
#define ELF_R_TYPE	ELF64_R_TYPE
#define ELF_R_SYM	ELF64_R_SYM
#else
#define ELF_R_TYPE	ELF32_R_TYPE
#define ELF_R_SYM	ELF32_R_SYM
#endif

#define PAGE_START(addr) (~(0x1000 - 1) & (addr))


static unsigned elfhash(const char *name) {
	const unsigned char *tmp = (const unsigned char *) name;
	unsigned h = 0, g;

	while (*tmp) {
		h = (h << 4) + *tmp++;
		g = h & 0xf0000000;
		h ^= g;
		h ^= g >> 24;
	}
	return h;
}

static uint32_t dl_new_hash (const char *s)
{
	uint32_t h = 5381;

	for (unsigned char c = *s; c != '\0'; c = *++s)
		h = h * 33 + c;

	return h;
}

int Get_ELF_Info(void *base,ELF_INFO *pei, GETELFTYPE bvf)
{
	pei->base = (ElfW(Addr))base;
	pei->name[0] = '*';
    ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)pei->base;
	pei->ehdr = ehdr;

	ElfW(Phdr) *Phdr = (ElfW(Phdr) *)((ElfW(Addr)) pei->base+ehdr->e_phoff);
	pei->Phdr = Phdr;

	if(memcmp(ehdr->e_ident,ELFMAG,SELFMAG) != 0) {
		return 1;
	}

	int bload = 0;
	for (int nc = 0; nc < ehdr->e_phnum; nc++) {
		if(Phdr->p_type == PT_LOAD  && bload == 0) {
			pei->load_bias = pei->base - Phdr->p_vaddr;
			bload = 1;
		}
		if(Phdr->p_type == PT_DYNAMIC) {
			if(bvf == IN_FILE) {
				pei->Pdyn = (ElfW(Dyn) *)(pei->base + Phdr->p_offset);
			} else if(bvf == IN_VM) {
				pei->Pdyn = (ElfW(Dyn) *)(pei->load_bias + Phdr->p_vaddr);
			}
			break;
		}

		Phdr++;
	}

	for(ElfW(Dyn)* d = pei->Pdyn; d->d_tag != DT_NULL; ++d) {
		switch (d->d_tag) {
			case DT_JMPREL:
#if defined(USE_RELA)
				pei->plt_rela = (ElfW(Rela) *)(pei->load_bias+ d->d_un.d_ptr);
#else
				pei->plt_rel = (ElfW(Rel) *) (pei->load_bias + d->d_un.d_ptr);
#endif
				break;
			case DT_PLTRELSZ:
#if defined(USE_RELA)
				pei->plt_rela_count = d->d_un.d_val / sizeof(ElfW(Rela));
#else
				pei->plt_rel_count = d->d_un.d_val / sizeof(ElfW(Rel));
#endif
				break;
#if defined(USE_RELA)
            case DT_RELA:
                pei->rela = reinterpret_cast<ElfW(Rela) *>(pei->load_bias + d->d_un.d_ptr);
            break;
            case DT_RELASZ:
                pei->rela_count = d->d_un.d_val / sizeof(ElfW(Rela));
                break;
#else
			case DT_REL:
				pei->rel = reinterpret_cast<ElfW(Rel) *>(pei->load_bias + d->d_un.d_ptr);
				break;
			case DT_RELSZ:
				pei->rel_count = d->d_un.d_val / sizeof(ElfW(Rel));
				break;
#endif
			case DT_SYMTAB:
				pei->symtab = (ElfW(Sym) *) (pei->load_bias + d->d_un.d_ptr);
				break;
			case DT_STRTAB:
				pei->strtab = (char *) (pei->load_bias + d->d_un.d_ptr);
				break;
			case DT_STRSZ:
				pei->strsz = d->d_un.d_val;
				break;
			case DT_HASH:
				pei->nbucket = reinterpret_cast<uint32_t*>(pei->load_bias + d->d_un.d_ptr)[0];
				pei->nchain = reinterpret_cast<uint32_t*>(pei->load_bias + d->d_un.d_ptr)[1];
				pei->bucket = reinterpret_cast<uint32_t*>(pei->load_bias + d->d_un.d_ptr + 8);
				pei->chain = reinterpret_cast<uint32_t*>(pei->load_bias + d->d_un.d_ptr + 8 + pei->nbucket * 4);

				pei->symsz = pei->nchain;
				break;
			case DT_GNU_HASH:
				pei->gnu_nbucket_ = reinterpret_cast<uint32_t *>(pei->load_bias + d->d_un.d_ptr)[0];
				// skip symndx
				pei->gnu_maskwords_ = reinterpret_cast<uint32_t *>(pei->load_bias + d->d_un.d_ptr)[2];
				pei->gnu_shift2_ = reinterpret_cast<uint32_t *>(pei->load_bias + d->d_un.d_ptr)[3];

				pei->gnu_bloom_filter_ = reinterpret_cast<ElfW(Addr) *>(pei->load_bias + d->d_un.d_ptr + 16);
				pei->gnu_bucket_ = reinterpret_cast<uint32_t *>(pei->gnu_bloom_filter_ +
															   pei->gnu_maskwords_);
				// amend chain for symndx = header[1]
				pei->gnu_chain_ = pei->gnu_bucket_ + pei->gnu_nbucket_ -
								 reinterpret_cast<uint32_t *>(pei->load_bias + d->d_un.d_ptr)[1];


				if (!powerof2(pei->gnu_maskwords_)) {
                    LOGE("invalid maskwords for gnu_hash = 0x%x, expecting power to two",
                         pei->gnu_maskwords_);
					return 2;
				}
				--pei->gnu_maskwords_;

				break;
			default:
				break;
		}
	}

	LOGD("pei->base = %p(%p), pei->strtab = %p, pei->symtab = %p, pei->bucket = %p, pei->gnu_bucket_ = %p",
		  reinterpret_cast<void*>(pei->base),reinterpret_cast<void*>(pei->load_bias),
		 pei->strtab, pei->symtab, pei->bucket, pei->gnu_bucket_);

	return 0;
}

static ElfW(Sym)* gnu_lookup(ELF_INFO* pei, uint32_t hash, const char* name, int &index){
	uint32_t h2 = hash >> pei->gnu_shift2_;

	uint32_t bloom_mask_bits = sizeof(ElfW(Addr))*8;
	uint32_t word_num = (hash / bloom_mask_bits) & pei->gnu_maskwords_;
	ElfW(Addr) bloom_word = pei->gnu_bloom_filter_[word_num];


	LOGD("SEARCH %s in %s@%p (gnu)",
			   name, pei->name, reinterpret_cast<void*>(pei->base));

	// test against bloom filter
	if ((1 & (bloom_word >> (hash % bloom_mask_bits)) & (bloom_word >> (h2 % bloom_mask_bits))) == 0) {
		LOGE("1 NOT FOUND %s in %s@%p",
				   name, pei->name, reinterpret_cast<void*>(pei->base));

		return NULL;
	}

	// bloom test says "probably yes"...
	uint32_t n = pei->gnu_bucket_[hash % pei->gnu_nbucket_];

	if (n == 0) {
		LOGE("2 NOT FOUND %s in %s@%p",
				   name, pei->name, reinterpret_cast<void*>(pei->base));

		return NULL;
	}


	do {
		ElfW(Sym)* s = pei->symtab + n;
		if (((pei->gnu_chain_[n] ^ hash) >> 1) == 0 && strcmp(pei->strtab + s->st_name, name) == 0) {
			LOGE("FOUND %s in %s (%p) %zd",
					   name, pei->name, reinterpret_cast<void*>(s->st_value),
					   static_cast<size_t>(s->st_size));
			index = n;
			return s;
		}
	} while ((pei->gnu_chain_[n++] & 1) == 0);

	LOGD("3 NOT FOUND %s in %s@%p",
			   name, pei->name, reinterpret_cast<void*>(pei->base));

	return NULL;
}

static ElfW(Sym)* elf_lookup(ELF_INFO *pei, unsigned hash, const char *name, int &index) {
	ElfW(Sym)* symtab = pei->symtab;
	const char* strtab = pei->strtab;

	LOGD("SEARCH %s in %s@%p %x %zd",
			   name, pei->name, reinterpret_cast<void*>(pei->base), hash, hash % pei->nbucket);

	for (unsigned n = pei->bucket[hash % pei->nbucket]; n != 0; n = pei->chain[n]) {
		ElfW(Sym)* s = symtab + n;
		if (strcmp(strtab + s->st_name, name)) continue;

		/* only concern ourselves with global and weak symbol definitions */
		switch (ELF_ST_BIND(s->st_info)) {
			case STB_GLOBAL:
			case STB_WEAK:
				if (s->st_shndx == SHN_UNDEF) {
					continue;
				}

				LOGD("FOUND %s in %s (%p) %zd",
						   name, pei->name, reinterpret_cast<void*>(s->st_value),
						   static_cast<size_t>(s->st_size));
				index = n;
				return s;
			case STB_LOCAL:
				continue;
			default:
				LOGW("ERROR: Unexpected ST_BIND value: %d for '%s' in '%s'",
					  ELF_ST_BIND(s->st_info), name, pei->name);
		}
	}

	LOGD("NOT FOUND %s in %s@%p %x %zd",
			   name, pei->name, reinterpret_cast<void*>(pei->base), hash, hash % pei->nbucket);


	return NULL;
}

static ElfW(Sym)* match_lookup(ELF_INFO *pei, const char *name, int &index)
{
    ElfW(Sym)* symtab = pei->symtab;
    const char* strtab = pei->strtab;
    int n = 0;
    ElfW(Sym)* s;
    do
    {
        s = symtab + n;
        if((strlen(strtab + s->st_name) + s->st_name + 1) >= pei->strsz)
            break;
        if (strcmp(strtab + s->st_name, name) == 0){
            index = n;
            return s;
        }
        n++;
    }while(s->st_name < pei->strsz);
    return NULL;
}

static ElfW(Sym)*  soinfo_do_lookup(void * base,const char *name, ELF_INFO * pei, int &index)
{
	unsigned elf_hash = elfhash(name);
    uint32_t gnu_hash = dl_new_hash(name);
	ElfW(Sym)* s = NULL;

	if(pei != NULL) {
		if(pei->bucket) {
			s = elf_lookup(pei, elf_hash, name, index);
		} else if(pei->gnu_bucket_) {
			s = gnu_lookup(pei, gnu_hash, name, index);
		}
        //mmap 在libart中gnu方式查找失败，暂时未搞清楚原因，只能进行先暴力搜索
		if(s == NULL){
            s = match_lookup(pei, name, index);
		}
	}
	return s;
}


unsigned long getapi_addr_REL(void *base, const char *sapiname,GETELFTYPE bvf)
{
	unsigned long pAddr = 0;
	bool bf  = false;
	ELF_INFO ei = {0};
	if(Get_ELF_Info(base,&ei,bvf) != 0) {
		return 0;
	}

	int index			 = 0;
	ElfW(Sym) *s	= soinfo_do_lookup(base,sapiname,&ei,index);

	LOGD("Sym =%p,index =%x",s,index);
	if(s == NULL) {
		return 0;
	}

#ifdef USE_RELA
	if(ei.plt_rela) {
		for (int i = 0; i < ei.plt_rela_count; i ++,ei.plt_rela++) {
			if (ELF_R_SYM(ei.plt_rela->r_info)  == index
#if defined(__aarch64__)
				&& ELF_R_TYPE(ei.plt_rela->r_info) == R_AARCH64_JUMP_SLOT
#elif defined(__x86_64__)
				&& ELF_R_TYPE(ei.plt_rela->r_info) == R_X86_64_JUMP_SLOT
#endif
					) {
				bf = true;
				pAddr = ElfW(Addr)(ei.load_bias + ei.plt_rela->r_offset);
				LOGD("plt_rela %d %lx",index,pAddr);
				break;
			}
		}
	}
#else
	if(ei.plt_rel) {
		for (int i = 0; i < ei.plt_rel_count; i ++,ei.plt_rel++) {
			if (ELF_R_SYM(ei.plt_rel->r_info)  == index
#if defined(__arm__)
				&& ELF_R_TYPE(ei.plt_rel->r_info) == R_ARM_JUMP_SLOT
#elif defined(__i386__)
				&& ELF_R_TYPE(ei.plt_rel->r_info) == R_386_JMP_SLOT
#elif defined(__mips__)
				&& ELF_R_TYPE(ei.plt_rel->r_info) == R_MIPS_JUMP_SLOT
#endif
					) {
				bf = true;
				pAddr = ElfW(Addr)(ei.load_bias + ei.plt_rel->r_offset);
				LOGD("plt_rel %d %lx",index,pAddr);
				break;
			}
		}
	}
#endif

	if(!bf) {
#ifdef USE_RELA
		if(ei.rela) {
			for (int i = 0; i < ei.rela_count; i ++,ei.rela++) {
				if (ELF_R_SYM(ei.rela->r_info)  == index
#if defined(__aarch64__)
				&&(ELF_R_TYPE(ei.rela->r_info) == R_AARCH64_ABS64
#elif defined(__x86_64__)
				&&(0
#endif

#if defined(__aarch64__)
				|| ELF_R_TYPE(ei.rela->r_info) == R_AARCH64_GLOB_DAT
#elif defined(__x86_64__)
				|| ELF_R_TYPE(ei.rela->r_info) == R_X86_64_GLOB_DAT
#endif
				)){
					pAddr = ElfW(Addr)(ei.load_bias +ei.rela->r_offset);
					LOGD("rela %d %lx",index,pAddr);
					break;
				}

			}
		}
#else
		if(ei.rel) {
			for (int i = 0; i < ei.rel_count; i ++,ei.rel++) {
				if (ELF_R_SYM(ei.rel->r_info)  == index

#if defined(__arm__)
				&&(ELF_R_TYPE(ei.rel->r_info) == R_ARM_ABS32
#elif defined(__i386__)
				&&(ELF_R_TYPE(ei.rel->r_info) == R_386_32
#elif defined(__mips__)
				&&(0
#endif

#if defined(__arm__)
				|| ELF_R_TYPE(ei.rel->r_info) == R_ARM_GLOB_DAT
#elif defined(__i386__)
				|| ELF_R_TYPE(ei.rel->r_info) == R_386_GLOB_DAT
#elif defined(__mips__)
				|| ELF_R_TYPE(ei.rel->r_info) == R_MIPS_GLOB_DAT
#endif
				)) {
					pAddr = ElfW(Addr)(ei.load_bias +ei.rel->r_offset);
					LOGD("rel %d %lx",index,pAddr);
					break;
			   }

			}
		}
#endif
	}
	return pAddr;
}





int  GetsoAPIaddr(void * base,const char *sapiname,GETELFTYPE bvf,API_uAddr_pAddr &aup)
{
	int index			 = 0;
	ELF_INFO ei = {0};
	if(Get_ELF_Info(base,&ei,bvf) != 0) {
		return 1;
	}
	ElfW(Sym) *target	= soinfo_do_lookup(base,sapiname,&ei,index);

	if(target) {
		aup.uAddr = ElfW(Addr)((ElfW(Addr))ei.load_bias + target->st_value);
		aup.pAddr = ElfW(Addr)(&(target->st_value));
	}
	return 0;
}

}
