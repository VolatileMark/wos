/*
 * tools/elftool.c
 *
 * Copyright (C) 2017 bzt (bztsrc@gitlab)
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * A művet szabadon:
 *
 * - Megoszthatod — másolhatod és terjesztheted a művet bármilyen módon
 *     vagy formában
 * - Átdolgozhatod — származékos műveket hozhatsz létre, átalakíthatod
 *     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza
 *     ezen engedélyeket míg betartod a licensz feltételeit.
 *
 * Az alábbi feltételekkel:
 *
 * - Nevezd meg! — A szerzőt megfelelően fel kell tüntetned, hivatkozást
 *     kell létrehoznod a licenszre és jelezned, ha a művön változtatást
 *     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve
 *     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a
 *     felhasználásod körülményeit.
 * - Ne add el! — Nem használhatod a művet üzleti célokra.
 * - Így add tovább! — Ha feldolgozod, átalakítod vagy gyűjteményes művet
 *     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-
 *     feltételek mellett kell terjesztened, mint az eredetit.
 *
 * @subsystem eszközök
 * @brief Segédprogram ELF64 fájlok tartalmának listázására
 *
 */

#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>   /* chmod */

int hu = 0;             /* magyar nyelvű kimenet */
long int fsiz;
char devdb[] = "../../../../bin/initrd/sys/drivers";

/* segédfüggvény egy fájl memóriába olvasására */
char* readfileall(char *file, int err)
{
    char *data=NULL;
    FILE *f;
    fsiz = 0;
    f=fopen(file,"rb");
    if(f){
        fseek(f,0L,SEEK_END);
        fsiz=ftell(f);
        fseek(f,0L,SEEK_SET);
        data=(char*)malloc(fsiz+1);
        if(data==NULL) {
memerr:     printf(hu?"Nem tudok lefoglalni %ld bájtnyi memóriát\n":"Unable to allocate %ld memory\n", fsiz+1);
            exit(2);
        }
        if(fsiz && !fread(data, fsiz, 1, f)) {
            printf(hu?"Nem lehet olvasni\n":"Unable to read\n");
            exit(1);
        }
        fclose(f);
        data[fsiz] = 0;
    } else {
        if(err) {
            printf(hu?"Nincs ilyen fájl\n":"File not found\n");
            exit(1);
        } else {
            fsiz = 0;
            data=(char*)malloc(1);
            if(!data) goto memerr;
            data[0] = 0;
        }
    }
    return data;
}

/* számkonvertálás */
unsigned long int hextoi(char *s)
{
    uint64_t v = 0;
    if(*s=='0' && *(s+1)=='x')
        s+=2;
    do{
        v <<= 4;
        if(*s>='0' && *s<='9')
            v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f')
            v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F')
            v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    return v;
}

/* funkció keresztreferencia készítés
 * na jó, ez nem igazán ide tartozik, de nem akartam mégegy toolt csinálni */
typedef struct {
    int sub;
    char *ifn;
    char *brief;
    char *link;
} files_t;
typedef struct {
    int sub;
    int line;
    int file;
    char *name;
    char *proto;
    char *desc;
} func_t;
int srtcmp(const void *a, const void *b) { return strcmp(*(char * const *)a, *(char * const *)b); }
int fnccmp(const void *a, const void *b) {
    func_t *A = (func_t*)a;
    func_t *B = (func_t*)b;
    if(A->sub != B->sub) return B->sub - A->sub;
    return strcmp(A->name, B->name);
}
void mkref(char *outmd, int argc, char **argv)
{
    int i, j, nfunc = 0, nsub = 0, ol = 1;
    char *data, *c, *e, *d, *g, *n, *m, **subs = NULL;
    files_t *files = NULL;
    func_t *funcs = NULL;
    FILE *f;

    qsort(argv, argc, sizeof(char*), srtcmp);
    files = (files_t*)malloc(argc * sizeof(files_t));
    memset(files, 0, argc * sizeof(files_t));
    for(i=0;i<argc;i++) {
        data = readfileall(argv[i], 1);
        /* fájllista */
        for(e = c = data + 6; *e != '\n'; e++);
        files[i].ifn = malloc(e - c + 1);
        memcpy(files[i].ifn, c, e - c);
        files[i].ifn[e - c] = 0;
        if(strlen(argv[i]) < (size_t)(e - c) || strcmp(argv[i] + strlen(argv[i]) - (int)(e - c), files[i].ifn)) {
            printf("Hibás fájlnév a kommentben: %s != %s\n", argv[i], files[i].ifn);
            exit(1);
        }
        for(c = e; *c && memcmp(c, "*/", 2) && memcmp(c, "@brief ", 7); c++);
        if(*c != '@') {
            printf("Nincs @brief a kommentben: %s\n", argv[i]);
            exit(1);
        }
        c += 7;
        for(e = c; *e != '\n'; e++);
        files[i].brief = malloc(e - c + 1);
        memcpy(files[i].brief, c, e - c);
        files[i].brief[e - c] = 0;
        for(c = data; *c && memcmp(c, "*/", 2) && memcmp(c, "@link ", 6); c++);
        if(*c == '@') {
            c += 6;
            for(e = c; *e != '\n'; e++);
            files[i].link = malloc(e - c + 1);
            memcpy(files[i].link, c, e - c);
            files[i].link[e - c] = 0;
        } else
            files[i].link = NULL;
        for(c = data; *c && memcmp(c, "*/", 2) && memcmp(c, "@subsystem ", 11); c++);
        if(*c != '@') {
            printf("Nincs @subsystem a kommentben: %s\n", argv[i]);
            exit(1);
        }
        c += 11;
        for(e = c; *e != '\n'; e++);
        for(j = 0; j < nsub && !(strlen(subs[j]) == (size_t)(e - c) && !memcmp(c, subs[j], e - c)); j++);
        if(j >= nsub) {
            j = nsub++;
            subs = (char**)realloc(subs, nsub * sizeof(char*));
            subs[j] = malloc(e - c + 1);
            memcpy(subs[j], c, e - c);
            subs[j][e - c] = 0;
        }
        files[i].sub = j;
        /* funkciólista */
        for(c = d = data, ol = 1; *c; c++)
            if(!memcmp(c, "/**\n", 4)) {
                for(; d < c; d++) if(d[0] == '\n') ol++;
                n = c + 4;
                while(*c && memcmp(c, "*/", 2) && memcmp(c-1, "\n *\n", 4) && memcmp(c - 1, "\n * \n", 5)) c++;
                m = c;
                while(*c && *c != '\n') c++;
                while(*c && *c == '\n') c++;
                if(!memcmp(c, " * ", 3)) c += 3;
                if(c[0] == '/' || !memcmp(c, "private", 7) || !memcmp(c, "extern", 6)) continue;
                if(!memcmp(c, "public", 6)) c += 7;
                for(e = c; *e && *e != '\n' && *e != ';' && *e != '{' && memcmp(e-1,") (",3); e++);
                for(g = c; *g && *g != '('; g++);
                if(*g=='(') {
                    funcs = (func_t*)realloc(funcs, (nfunc+1)*sizeof(func_t));
                    funcs[nfunc].sub = j;
                    funcs[nfunc].line = ol;
                    funcs[nfunc].file = i;
                    funcs[nfunc].desc = malloc(m - n + 1);
                    funcs[nfunc].proto = g = malloc(e - c + 3);
                    if(!memcmp(c, "func ", 5)) {
                        memcpy(g, "void ", 5); g += 5;
                        memcpy(g, c + 5, e - c - 5);
                        memcpy(g + (int)(e - c - 5), "()", 3);
                    } else {
                        for(; c < e; c++) {
                            if(!memcmp(c, "__attribute__((malloc)) ", 24)) c += 23; else
                            if(!memcmp(c, "__attribute__((unused)) ", 24)) c += 23; else
                            if(!memcmp(c, "unused ", 7)) c += 6; else {
                                if(*c == '[' || *c == ']') *g++ = '\\';
                                *g++ = *c;
                            }
                        }
                        while(g > funcs[nfunc].proto && g[-1] == ' ') g--;
                        *g = 0;
                        for(g = funcs[nfunc].proto; *g && *g != '('; g++);
                        for(; g > funcs[nfunc].proto && g[-1] != ' ' && g[-1] != '*'; g--);
                    }
                    funcs[nfunc].name = g;
                    for(g = funcs[nfunc].desc; n < m; n++)
                        if(!memcmp(n, " * ", 3)) { *g++ = ' '; n++; } else *g++ = *n;
                    *g = 0;
                    nfunc++;

                }
                c = e;
            }
    }
    if(!funcs) { printf("Nincs egy funkció se???\n"); exit(1); }
    qsort(funcs, nfunc, sizeof(func_t), fnccmp);
    /* ez csak magyarul van, mivel a forrásban a kommentek úgyis magyarul vannak */
    f = fopen(outmd,"w");
    if(f) {
        fprintf(f, "OS/Z Függvényreferenciák\n========================\n\nPrototípusok\n------------\n"
            "Összesen %d függvény van definiálva.\n\n", nfunc);
        for(j = -1, i = 0; i < nfunc; i++) {
            if(j != funcs[i].sub) { j = funcs[i].sub; fprintf(f, "### %c%s\n",
                subs[j][0] >= 'a' && subs[j][0] <= 'z' ? subs[j][0] - 32 : subs[j][0],
                subs[j] + 1); if(files[funcs[i].file].link) fprintf(f, "Linkelés: %s\n",files[funcs[i].file].link); }
            fprintf(f, "[%s](https://gitlab.com/bztsrc/osz/blob/master/src/%s#L%d)\n%s\n",
                funcs[i].proto, files[funcs[i].file].ifn, funcs[i].line, funcs[i].desc);
        }
        fprintf(f, "Fájlok\n------\n\n| Fájl | Alrendszer | Leírás |\n| ---- | ---------- | ------ |\n");
        for(i = 0; i < argc; i++)
            fprintf(f, "| [%s](https://gitlab.com/bztsrc/osz/blob/master/src/%s) | %s | %s |\n",
                files[i].ifn, files[i].ifn, subs[files[i].sub], files[i].brief);
        fprintf(f, "\n\n");
        fclose(f);
    } else {
        printf("%s: %s\n", outmd, hu?"Nem lehet írni\n":"Unable to write\n");
    }
}

/* belépési pont */
int main(int argc,char** argv)
{
    FILE *f;
    char *binds[] = { "L", "G", "W", "N" };
    char *types[] = { "NOT", "OBJ", "FNC", "SEC", "FLE", "COM", "TLS", "NUM", "LOS", "IFC", "HOS", "LPR", "HPR" };
    char *elf, *strtable = NULL, *lang, *n, *c, *e, *data, *dev;
    int i=0, dump=0, chk=0, strsz, syment, dtrelasz = 0, dtjmpsz = 0, dtrela = 0 , dtjmprel = 0;
    unsigned long int reloc=-1;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr, *phdr_c, *phdr_d, *phdr_l;
    Elf64_Sym *sym = NULL, *s = NULL;
    Elf64_Dyn *d;
    lang=getenv("LANG");
    if(lang && lang[0]=='h' && lang[1]=='u') hu=1;
    if(argc<2) {
        if(hu) {
            printf("./elftool [elf]                 dunamikus szekció C headerré formázása\n"
                   "./elftool -b [elf]              hozzáadás a meghajtóprogram adatbázishoz\n"
                   "./elftool -c [elf]              ELF ellenőrzése és hedör beállítása\n"
                   "./elftool -d [elf]              szekciók listázása ember számára érhetően\n"
                   "./elftool -D [elf]              dynszekciók listázása ember számára érhetően\n"
                   "./elftool -s [reloff] [elf]     szimbólumok listázása relokált címekkel\n");
            printf("./elftool -r <kimd> [be1... ]   keresztreferencia doksi készítése\n");
        } else {
            printf("./elftool [elf]                 dumps dynamic section into C header\n"
                   "./elftool -b [elf]              add to the device driver database\n"
                   "./elftool -c [elf]              check ELF and set header\n"
                   "./elftool -d [elf]              dumps sections in human readable format\n"
                   "./elftool -D [elf]              dumps dynsections in human readable format\n"
                   "./elftool -s [reloff] [elf]     dumps symbols with relocated addresses\n"
                   "./elftool -r <outmd> [in1... ]  createing cross-refernce documentation\n");
        }
        return 1;
    }
    if(argc>2 && argv[1][0]=='-' && argv[1][1]=='s') {
        reloc=hextoi(argv[2]);
    } else if(argv[1][0]=='-' && argv[1][1]=='d') {
        dump=1;
    } else if(argv[1][0]=='-' && argv[1][1]=='D') {
        dump=2;
    } else if(argv[1][0]=='-' && argv[1][1]=='c') {
        chk=1;
    } else if(argv[1][0]=='-' && argv[1][1]=='b') {
        n = argv[argc-1];
        c = strrchr(n, '/');
        if(!c) exit(0);
        dev = readfileall("devices", 0);
        data = readfileall(devdb, 0);
        for(e = data + 1; *e && (e[-1] != '\t' || memcmp(e, n, strlen(n))); e++);
        if(*e) exit(0);
        data = realloc(data, fsiz+1024);
        if(!data) { printf(hu?"Nem tudok lefoglalni %ld bájtnyi memóriát\n":"Unable to allocate %ld memory\n", fsiz+1024); exit(1); }
        if(!memcmp(n, "fs/", c - n + 1)) fsiz += sprintf(data + fsiz, "FS\t%s\n",n); else
        if(!memcmp(n, "ui/", c - n + 1)) fsiz += sprintf(data + fsiz, "UI\t%s\n",n); else
        if(!memcmp(n, "inet/", c - n + 1)) fsiz += sprintf(data + fsiz, "inet\t%s\n",n); else
        if(!memcmp(n, "print/", c - n + 1)) fsiz += sprintf(data + fsiz, "print\t%s\n",n); else
        if(!memcmp(n, "sound/", c - n + 1)) fsiz += sprintf(data + fsiz, "sound\t%s\n",n); else
        if(!memcmp(n, "syslog/", c - n + 1)) fsiz += sprintf(data + fsiz, "syslog\t%s\n",n); else {
            if(!dev) exit(0);
            for(c = dev; *c; c++) {
                e = c; while(*c && *c!='\n') c++;
                *c = 0;
                data = realloc(data, fsiz+1024);
                if(!data) { printf(hu?"Nem tudok lefoglalni %ld bájtnyi memóriát\n":"Unable to allocate %ld memory\n", fsiz+1024); exit(1); }
                fsiz += sprintf(data + fsiz, "%s\t%s\n",e,n);
            }
        }
        f = fopen(devdb,"wb");
        if(f) {
            fwrite(data, fsiz, 1, f);
            fclose(f);
        } else {
            printf("%s: %s\n", devdb, hu?"Nem lehet írni":"Unable to write\n");
            exit(1);
        }
        exit(0);
    } else if(argv[1][0]=='-' && argv[1][1]=='r') {
        if(argc < 4) {
            printf("./elftool -r <kimenet.md> <.c/.h/.s> [.c/.h/.s [.c/.h/.s [ ... ]]]\n");
            exit(1);
        }
        /* átmásoljuk, mert az argv átrendezése UB lenne */
        mkref(argv[2], argc - 3, argv + 3);
        exit(0);
    }
    /* ha van paraméter, aszt használjuk, egyébként magunkat */
    elf = readfileall(argv[argc-1], 1);

    /* mutatók különféle elf struktúrákra */
    /* elf fájl, fő fejléc */
    ehdr=(Elf64_Ehdr *)(elf);

    /* szegmensek beolvasása */
    phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);

    /* Program header */
    if(dump)
        printf("ELF %s %s, %s: %08lx\n\n%s\n",
            ehdr->e_machine==62?"x86_64":"aarch64",
            ehdr->e_type==2?(hu?"futtatható":"executable"):(hu?"függvénykönyvtár":"shared library"),
            hu?"belépési pont":"entry point", ehdr->e_entry, hu?"Szegmensek":"Segments");
    for(i = 0; i < ehdr->e_phnum; i++){
        if(dump)
            printf(" %2d %08lx %6ld %s\n",(int)phdr->p_type,phdr->p_offset,phdr->p_filesz,
                (phdr->p_type==PT_LOAD?(hu?"betöltendő":"load"):(phdr->p_type==PT_DYNAMIC?"dynamic":"")));
        /* csak a Dynamic header-re van szükségünk */
        if(phdr->p_type==PT_DYNAMIC) {
            /* Dynamic szekció beolvasása */
            d = (Elf64_Dyn *)(elf + phdr->p_offset);
            while(d->d_tag != DT_NULL) {
                if(dump && d->d_tag<1000)
                    printf("     %3d %08lx %8ld %s\n",(int)d->d_tag,d->d_un.d_ptr,d->d_un.d_ptr,
                        (d->d_tag==DT_STRTAB?"strtab":(d->d_tag==DT_SYMTAB?"symtab":
                        (d->d_tag==DT_RELA?"rela":(d->d_tag==DT_RELASZ?"relasz":
                        (d->d_tag==DT_RELAENT?"relaent":(d->d_tag==DT_JMPREL?"jmprel":
                        (d->d_tag==DT_SYMENT?"syment":(d->d_tag==DT_PLTRELSZ?"jmprelsz":"")))))))));
                switch(d->d_tag) {
                    case DT_STRTAB: strtable = elf + (d->d_un.d_ptr&0xFFFFFFFF); break;
                    case DT_STRSZ: strsz = d->d_un.d_val; break;
                    case DT_SYMTAB: sym = (Elf64_Sym *)(elf + (d->d_un.d_ptr&0xFFFFFFFF)); break;
                    case DT_SYMENT: syment = d->d_un.d_val; break;
                }
                /* mutató növelése a következő dynamic elemre */
                d++;
            }
            break;
        }
        /* mutató növelése a következő szegmensre */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }

    /* ha szekciólistázást kértek a parancssorból */
    if(dump || reloc!=(unsigned long int)-1 || chk) {
        /* szekciók beolvasása */
        Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
        /* sztringtáblamutató és egyéb szekció fejlécek */
        Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)ehdr->e_shstrndx*(uint64_t)ehdr->e_shentsize);
        Elf64_Shdr *rela_sh=NULL;
        Elf64_Shdr *relat_sh=NULL;
        Elf64_Shdr *dyn_sh=NULL;
        Elf64_Shdr *got_sh=NULL;
        Elf64_Shdr *dynsym_sh=NULL;
        Elf64_Shdr *dynstr_sh=NULL;
        Elf64_Shdr *sym_sh=NULL;
        Elf64_Shdr *str_sh=NULL;
        /* a sztringtábla tényleges címe */
        strtable = elf + strt->sh_offset;

        if(dump)
            printf("\nPhdr %lx Shdr %lx Strt %lx Sstr %lx\n\n%s\n",
                (uint64_t)ehdr->e_phoff,
                (uint64_t)ehdr->e_shoff,
                (uint64_t)strt-(uint64_t)ehdr,
                (uint64_t)strt->sh_offset,
                hu?"Szekciók":"Sections"
            );

        /* Szekciók */
        for(i = 0; i < ehdr->e_shnum; i++){
            if(dump)
                printf(" %2d %08lx %6ld %3d %s\n",i,
                    (uint64_t)shdr->sh_offset, (uint64_t)shdr->sh_size,
                    shdr->sh_type,strtable+shdr->sh_name);
            if(!strcmp(strtable+shdr->sh_name, ".rela.plt")){
                rela_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".rela.text")||
               !strcmp(strtable+shdr->sh_name, ".rela.dyn")){
                relat_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynamic")){
                dyn_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".got.plt")){
                got_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynsym")){
                dynsym_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".symtab")){
                sym_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynstr")){
                dynstr_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".strtab")){
                str_sh = shdr;
            }
            /* mutató növelése a következő szekcióra */
            shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
        }
        if(dump || reloc!=(unsigned long int)-1) {
            if(str_sh!=NULL) {
                strtable = elf + str_sh->sh_offset;
                strsz = (int)str_sh->sh_size;
            }
            if(sym_sh!=NULL) {
                sym = (Elf64_Sym *)(elf + (sym_sh->sh_offset));
                syment = (int)sym_sh->sh_entsize;
            }
        }
        if(chk) {
            ehdr->e_ident[EI_OSABI] = 5;
            if(ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
                fprintf(stderr, "%s: %s (e_ident[4] = %d)\n", argv[argc-1], hu?"nem 64 bites\n":"not 64 bit\n",
                    ehdr->e_ident[EI_CLASS]);
                exit(1);
            }
            if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
                fprintf(stderr, "%s: %s (e_ident[5] = %d)\n", argv[argc-1], hu?"nem little endian\n":"not little endian\n",
                    ehdr->e_ident[EI_DATA]);
                exit(1);
            }
            if(ehdr->e_machine != EM_X86_64 && ehdr->e_machine != EM_AARCH64) {
                fprintf(stderr, "%s: %s (e_machine = %d)\n", argv[argc-1], hu?"ismeretlen arch?\n":"unknown arch?\n",
                    ehdr->e_machine);
                /* this is just a warning exit(1); */
            }
            phdr_c = (Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);
            phdr_d = (Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff + 1*ehdr->e_phentsize);
            phdr_l = (Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff + 2*ehdr->e_phentsize);
            if( (uint64_t)ehdr->e_entry < (uint64_t)phdr_c->p_vaddr + 64 ||
                (uint64_t)ehdr->e_entry > (uint64_t)phdr_c->p_vaddr + (uint64_t)phdr_c->p_filesz) {
                    fprintf(stderr,"%s: %s (entry %lx vaddr %lx filesz %lx)\n", argv[argc-1],
                        hu?"Hibás belépési pont":"Invalid entry point",(uint64_t)ehdr->e_entry,
                        (uint64_t)phdr_c->p_vaddr, (uint64_t)phdr_c->p_filesz);
                    exit(1);
            }
            if(phdr_c->p_vaddr==0xffffffffffe02000) {
                /* biztosra megyünk, hogy a sys/core ne legyen direktben futtatható */
#if !defined(DEBUG) || !DEBUG
                memcpy(ehdr, "OS/Z", 4);
#endif
                ehdr->e_type = ET_DYN;
                if(phdr_c->p_type!=PT_LOAD || phdr_c->p_offset!=0) {
                    fprintf(stderr,"%s: %s (p_type %x p_offset %lx)\n", argv[argc-1], hu?"Hibás boot szegmens":"Invalid boot segment",
                        phdr_c->p_type,phdr_c->p_offset);
                    exit(1);
                }
                if(str_sh==NULL || sym_sh==NULL) {
                    fprintf(stderr, "%s: %s\n", argv[argc-1], hu?"nincs symtab vagy strtab?\n":"no symtab or strtab?\n");
                    exit(1);
                }
                strtable = elf + str_sh->sh_offset;
                strsz = (int)str_sh->sh_size;
                sym = (Elf64_Sym *)(elf + (sym_sh->sh_offset));
                syment = (int)sym_sh->sh_entsize;
                s = sym;
                for(i=0;i<(strtable-(char*)sym)/syment;i++) {
                    if(s->st_name > (unsigned int)strsz) break;
                    if(!strcmp(strtable + s->st_name, "bootboot") && s->st_value != 0xffffffffffe00000) {
                        fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Hibás bootboot szimbólumcím":"Invalid bootboot symbol address");
                        exit(1);
                    }
                    if(!strcmp(strtable + s->st_name, "environment") && s->st_value != 0xffffffffffe01000) {
                        fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Hibás environment szimbólumcím":"Invalid environment symbol address");
                        exit(1);
                    }
                    s++;
                }
            } else {
                if(ehdr->e_phnum > 2) {
                    d = (Elf64_Dyn *)(elf + phdr_d->p_offset);
                    while(d->d_tag != DT_NULL) {
                        switch(d->d_tag) {
                            case DT_RELASZ:   dtrelasz = d->d_un.d_val; break;
                            case DT_PLTRELSZ: dtjmpsz = d->d_un.d_val; break;
                            case DT_RELA:     dtrela = d->d_un.d_ptr; break;
                            case DT_JMPREL:   dtjmprel = d->d_un.d_ptr; break;
                        }
                        d++;
                    }
                }
                /* ha külön van az adat és funkció relokáció, akkor csak akkor jó, ha egymás után van a két tábla */
                if(ehdr->e_phnum != 3 || (dtrela && dtjmprel && dtrela != dtjmprel && (dtrela + dtrelasz != dtjmprel ||
                    dtjmprel + dtjmpsz != dtrela))) {
                    fprintf(stderr,"%s: %s (rela %x %d bytes, jmprel %x %d bytes)\n", argv[argc-1],
                        hu?"Hibás relokációs tábla":"Invalid relocation table", dtrela, dtrelasz, dtjmprel, dtjmpsz);
                    exit(1);
                }
                /* újabb linker bug, néha muszáj shared libnek fordítani, különben nem kerülnek a szimbólumok a dynsym-be
                 * a sys/driver pedig azért lett so-nak fordítva, mert máskülönben az ld rinyál a feloldatlan szimbólumokra,
                 * amik meg persze, hogy feloldatlanok, mert futásidőben szerkesztjük hozzá valamelyik eszközmeghajtót */
                c = strrchr(argv[argc-1], '.');
                ehdr->e_type = c && !strcmp(c, ".so") ? ET_DYN : ET_EXEC;
                /* kódszegmens, betöltendő, olvasható, futtatható, de nem írható */
                if(phdr_c->p_type!=PT_LOAD || phdr_c->p_offset!=0 || phdr_c->p_vaddr!=0 || (phdr_c->p_flags&PF_W)!=0) {
                    fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Hibás kódszegmens":"invalid code segment");
                    exit(1);
                }
                /* adatszegmens, lapcímhatárra igazított, olvasható, írható, de nem futtatható */
                if(phdr_d->p_type!=PT_LOAD || (phdr_d->p_vaddr&4095)!=0 || (phdr_d->p_flags&PF_X)!=0) {
                    fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Hibás adatszegmens":"invalid data segment");
                    exit(1);
                }
                /* dinamikus linkelési adatok */
                if(phdr_l->p_type!=PT_DYNAMIC) {
                    fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Hibás dinamikus szegmens":"invalid dynamic segment");
                    exit(1);
                }
            }
            f=fopen(argv[argc-1],"wb");
            if(f){
                if(!fwrite(elf, fsiz, 1, f)) {
                    printf("%s: %s\n", argv[argc-1], hu?"Nem lehet írni":"Unable to write");
                    exit(1);
                }
                fclose(f);
            } else {
                fprintf(stderr,"%s: %s\n", argv[argc-1], hu?"Nem lehet írni":"Unable to write\n");
                exit(1);
            }
            chmod(argv[argc-1], ehdr->e_type == ET_EXEC ? 0755 : 0644);
            exit(0);
        }
        if(!dump) {
            goto output;
        }
        if(dynstr_sh==NULL)
            dynstr_sh=str_sh;
        if(dynsym_sh==NULL)
            dynsym_sh=sym_sh;
        if(dump == 2) {
            strtable = elf + dynstr_sh->sh_offset;
            strsz = (int)dynstr_sh->sh_size;
            sym = (Elf64_Sym *)(elf + (dynsym_sh->sh_offset));
            syment = (int)dynsym_sh->sh_entsize;
        }
        /* dynstr tábla */
        printf("\n%s %08lx (%d %s %08lx (%d %s %d) %08lx\n\n",
            hu?"Sztringtábla":"Stringtable",
            dynstr_sh->sh_offset, (int)dynstr_sh->sh_size, hu?"bájt), szimbólumok":"bytes), symbols",
            dynsym_sh->sh_offset, (int)dynsym_sh->sh_size, hu?"bájt, egy bejegyzés":"bytes, one entry",
            (int)dynsym_sh->sh_entsize, sym_sh ? sym_sh->sh_offset : 0
        );

        printf("--- IMPORT ---\n");

        /* dynamic tábla */
        if(dyn_sh!=NULL) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)(elf + dyn_sh->sh_offset);
            printf("Dynamic %08lx (%d %s %d):\n",
                dyn_sh->sh_offset, (int)dyn_sh->sh_size, hu?"bájt, egy bejegyzés":"bytes, one entry", (int)dyn_sh->sh_entsize
            );
            for(i = 0; i < (int)(dyn_sh->sh_size / dyn_sh->sh_entsize); i++){
                /* megosztott függvénykönyvtár hivatkozás? */
                if(dyn->d_tag == DT_NEEDED) {
                    printf("%3d. /lib/%s\n", i,
                        ((char*)elf + (uint64_t)dynstr_sh->sh_offset + (uint64_t)dyn->d_un.d_ptr)
                    );
                }
                /* mutató növelése a következő dyanmic bejegyzésre */
                dyn = (Elf64_Dyn *)((uint8_t *)dyn + dyn_sh->sh_entsize);
            }

            /* GOT plt bejegyzések */
            if(got_sh)
                printf("\nGOT %08lx (%d %s)",
                    got_sh->sh_offset, (int)got_sh->sh_size, hu?"bájt":"bytes"
                );
            /* adatrelokáció */
            if(rela_sh) {
                Elf64_Rela *rela=(Elf64_Rela *)(elf + rela_sh->sh_offset);
                printf(", Rela plt %08lx (%d %s %d):\n",
                    rela_sh->sh_offset, (int)rela_sh->sh_size, hu?"bájt, egy bejegyzés":"bytes, one entry",
                    (int)rela_sh->sh_entsize
                );

                for(i = 0; i < (int)(rela_sh->sh_size / rela_sh->sh_entsize); i++){
                    /* szimbólum keresése, kicsit szörnyű, de lényegében csak offszetszámítás */
                    Elf64_Sym *sym = (Elf64_Sym *)(
                        elf + dynsym_sh->sh_offset +                        /* alap cím */
                        ELF64_R_SYM(rela->r_info) * dynsym_sh->sh_entsize   /* eltolás */
                    );
                    /* a sym-hez tartozó sztring kikeresése a sztringtáblából */
                    char *symbol = (elf + dynstr_sh->sh_offset + sym->st_name);
                    printf("%3d. %08lx +%lx %s\n", i, rela->r_offset,
                        rela->r_addend, symbol
                    );
                    /* mutató növelése a következő rela bejegyzésre */
                    rela = (Elf64_Rela *)((uint8_t *)rela + rela_sh->sh_entsize);
                }
            }
            /* kódrelokáció (text) */
            if(relat_sh) {
                Elf64_Rela *rela=(Elf64_Rela *)(elf + relat_sh->sh_offset);
                printf("\nRela text %08lx (%d %s %d):\n",
                    relat_sh->sh_offset, (int)relat_sh->sh_size, hu?"bájt, egy bejegyzés":"bytes, one entry",
                    (int)relat_sh->sh_entsize
                );

                for(i = 0; i < (int)(relat_sh->sh_size / relat_sh->sh_entsize); i++){
                    /* szimbólum keresése, kicsit szörnyű, de lényegében csak offszetszámítás */
                    Elf64_Sym *sym = (Elf64_Sym *)(
                        elf + dynsym_sh->sh_offset +                        /* alap cím */
                        ELF64_R_SYM(rela->r_info) * dynsym_sh->sh_entsize   /* eltolás */
                    );
                    /* a sym-hez tartozó sztring kikeresése a sztringtáblából */
                    char *symbol = (elf + dynstr_sh->sh_offset + sym->st_name);
                    printf("%3d. %08lx +%lx %s\n", i, rela->r_offset,
                        rela->r_addend, symbol
                    );
                    /* mutató növelése a következő relat bejegyzésre */
                    rela = (Elf64_Rela *)((uint8_t *)rela + relat_sh->sh_entsize);
                }
            }
        }
        printf("\n--- EXPORT ---\n");

        s = sym;
        for(i=0;i<(strtable-(char*)sym)/syment;i++) {
            if(s->st_name > (unsigned int)strsz) break;
            printf("%3d. %08lx %x %s %x %s %s\n", i, s->st_value,
                ELF64_ST_BIND(s->st_info), binds[ELF64_ST_BIND(s->st_info)],
                ELF64_ST_TYPE(s->st_info), types[ELF64_ST_TYPE(s->st_info)],
                strtable + s->st_name);
            s++;
        }
        return 0;
    }

output:
    if(strtable==NULL || sym==NULL) {
        printf(hu?"nincs symtab vagy strtab?\n":"no symtab or strtab?\n");
        return 1;
    }

    if(reloc==(unsigned long int)-1) {
        char *name = strrchr(argv[argc-1], '/');
        if(name!=NULL && name[0]=='/') name++;
        printf("/*\n"
            " * include/osZ/bits/%s.h - EZT A FÁJLT AZ elftool GENERÁLTA. NE MÓDOSÍTSD\n"
            " *\n"
            " * Copyright (c) 2019 bzt (bztsrc@gitlab)\n"
            " * https://creativecommons.org/licenses/by-nc-sa/4.0/\n"
            " * \n", name);
        printf(" * A művet szabadon:\n"
            " *\n"
            " * - Megoszthatod — másolhatod és terjesztheted a művet bármilyen módon\n"
            " *     vagy formában\n"
            " * - Átdolgozhatod — származékos műveket hozhatsz létre, átalakíthatod\n"
            " *     és új művekbe építheted be. A jogtulajdonos nem vonhatja vissza\n"
            " *     ezen engedélyeket míg betartod a licensz feltételeit.\n"
            " *\n");
        printf(" * Az alábbi feltételekkel:\n"
            " *\n"
            " * - Nevezd meg! — A szerzőt megfelelően fel kell tüntetned, hivatkozást\n"
            " *     kell létrehoznod a licenszre és jelezned, ha a művön változtatást\n"
            " *     hajtottál végre. Ezt bármilyen ésszerű módon megteheted, kivéve\n"
            " *     oly módon ami azt sugallná, hogy a jogosult támogat téged vagy a\n"
            " *     felhasználásod körülményeit.\n");
        printf(" * - Ne add el! — Nem használhatod a művet üzleti célokra.\n"
            " * - Így add tovább! — Ha feldolgozod, átalakítod vagy gyűjteményes művet\n"
            " *     hozol létre a műből, akkor a létrejött művet ugyanazon licensz-\n"
            " *     feltételek mellett kell terjesztened, mint az eredetit.\n"
            " *\n"
            " * @brief OS/Z %s szolgáltatások (rendszerhíváskódok)\n"
            " */\n\n", name);
    }

    s = sym;
    for(i=0;i<(strtable-(char*)sym)/syment;i++) {
        if(s->st_name > (unsigned int)strsz) break;
        /* ebben az objektumban van definiálva és nem bináris import? */
        if( s->st_value && *(strtable + s->st_name)!=0 && (reloc!=(unsigned long int)-1 || *(strtable + s->st_name)!='_') ) {
            if(reloc!=(unsigned long int)-1)
                printf("%08lx %s\n", s->st_value+reloc, strtable + s->st_name);
            else
            /* belépési pont kimarad... GNU ld nem tud privát belépési pontot értelmezni */
            if(ELF64_ST_TYPE(s->st_info)==STT_FUNC && s->st_size &&
                ELF64_ST_BIND(s->st_info)==STB_GLOBAL && ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT &&
                strcmp(strtable + s->st_name, "mq_dispatch") && strcmp(strtable + s->st_name, "main"))
                printf("#define SYS_%s\t%s(0x%02x)\n",
                    strtable + s->st_name, strlen(strtable + s->st_name)<4?"\t":"", i);
        }
        s++;
    }
    if(reloc==(unsigned long int)-1)
        printf("\n");
    return 0;
}
