#include <git2.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>

#define MAX_FILE_LEN 131072 
#define MAX_FILES 4096
#define MAX_HEADER_SIZE 512
#define FLAG_NOGIT (1 << 0)
#define MAX_REPOS 128
#define MAX_SELECTORS 4096

#ifdef DEBUG
#define debugprint(...) printf("[DEBUG]: " __VA_ARGS__)
#else
#define debugprint(...) ;
#endif

static int reposc = 0;
static git_repository *repos[MAX_REPOS];
static unsigned int flags;
static int totalignored;

typedef struct {
  const char *bytes;
  int len;
} sig_t;
#define sig(t) (sig_t) { .bytes = t, .len = sizeof(t)-1 }

static const sig_t sigs[] = {
    sig("\x7F\x45\x4C\x46"),                  // ELF
    sig("\x4D\x5A"),                          // EXE/DLL
    sig("\xFF\xD8\xFF"),                      // JPEG
    sig("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"),  // PNG
    sig("\x49\x44\x33"),                      // MP3 (ID3 tag)
    sig("\xFF\xFB"),                          // MP3 (raw frame, common)
    sig("\x4F\x67\x67\x53"),                  // OGG
    sig("\x25\x50\x44\x46\x2D"),              // PDF ("%PDF-")
};

static int isnontext(char* path) {
  FILE *file = fopen(path,"rb");
  if (!file) {fprintf(stderr,"selector.c: Unable to find file when checking isnontext: %s\n", path); return 1;}
  char fbytes[16];
  int nread = fread(fbytes,1,sizeof(fbytes),file);
  fclose(file);
  
  for (unsigned int i = 0; i < sizeof(sigs) / sizeof(sig_t); i++) {
    sig_t badsign = sigs[i];
    if (nread >= badsign.len && !memcmp(badsign.bytes,fbytes,badsign.len)) {
      return 1;
    }
  }

  return 0;
}

static void getfiles(int *pathsc, char **paths, int *selectorsc, char **selectors, char *currentpath) {
  DIR *d = opendir(currentpath);
  if (!d) {fprintf(stderr,"selector.c: Couldn't find dir while processing selectors: %s\n", currentpath); return;}
  struct dirent *dir;
  
  char gitpath[PATH_MAX];
  snprintf(gitpath,sizeof(gitpath),"%s/.git",currentpath);
  DIR *git = opendir(gitpath);
  if (git) {
    int err = git_repository_open(&repos[reposc++], currentpath);
    if (err) {
      const git_error *e = git_error_last();
      fprintf(stderr,"selector.c: Error opening repo: %s\n", e ? e->message : gitpath);
    } else {
      debugprint("selector.c: Succesfully opened repo: %s\n", gitpath);
    }
  }

  while ((dir = readdir(d))) {
    char filepath[PATH_MAX];
    if (!strcmp(currentpath,".")) {
      strcpy(filepath,dir->d_name);
    } else {
      snprintf(filepath,sizeof(filepath),"%s/%s",currentpath,dir->d_name);
    }

    int isignored = 0;
    for (int i = 0; i < reposc; i++) {
      int err = git_ignore_path_is_ignored(&isignored, repos[i], filepath);
      if (err) {
        const git_error *e = git_error_last();
        fprintf(stderr,"selector.c: Error checking ignore: %s\n", e ? e->message : "unknown");
      }
      if (isignored) {break;}
    }
    if (isignored && !(flags & FLAG_NOGIT)) {
      totalignored++;
      debugprint("selector.c: Skipping gitignored file: %s\n", filepath);
      continue;
    }

    switch(dir->d_type) {
      case DT_DIR:
        if (!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,".git")) {continue;}
        char dirfilepath[PATH_MAX+1];
        snprintf(dirfilepath,sizeof(dirfilepath),"%s/",filepath);

        for (int i = 0; i < *selectorsc; i++) {
          if (!fnmatch(selectors[i],dirfilepath,0) || selectors[i][0] == '*' || strstr(selectors[i], dirfilepath)) { // Only grab dirs whose content could match
            getfiles(pathsc,paths,selectorsc,selectors,filepath);
            break;
          }
        }
        debugprint("selector.c: Skipping directory %s/\n", filepath);
        break;
      case DT_REG:;
        if (isnontext(filepath)) {continue;}
        
        for (int i = 0; i < *selectorsc; i++) {
          if (!fnmatch(selectors[i],filepath,0)) {
            debugprint("selector.c: Appending file: %s\n", filepath);
            paths[(*pathsc)++] = strdup(filepath);
            break;
          }
        }
    }
  }
  closedir(d);
}

char **selectfiles(int argc, char **argv) {
  git_libgit2_init();

  int i;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
        case 'a':
          flags |= FLAG_NOGIT;
          break;
        default:
          fprintf(stderr,"selector.c: Flag -%c not found! Valid flags: [-a: Ignore gitignore]\n",argv[i][1]);
          return NULL;
      }
    } else {break;}
  }
  argv += i; argc -= i;
  
  // If no selector args were provided we assume *.
  if (argc < 1 || argv[0] == NULL) {
    argc = 1; argv[0] = "*";
  }
  
  char **dirpatterns = malloc(sizeof(char*)*MAX_SELECTORS);
  int dirpatternsc = 0;
  for (int i = 0; i < argc; i++) {
    int argvilen = strlen(argv[i]);
    if (argv[i][argvilen-1] == '/') {
      dirpatterns[dirpatternsc] = malloc(argvilen+2);
      snprintf(dirpatterns[dirpatternsc],argvilen+2,"%s*",argv[i]);
      argv[i] = dirpatterns[dirpatternsc];
      dirpatternsc++;
    }
  }

  char **paths = malloc(sizeof(char*)*MAX_FILES+1);
  int pathsc = 0;
  
  #ifdef DEBUG
  for(int i = 0; i < argc; i++) {
    debugprint("selector.c: Select pattern detected: %s\n", argv[i]);
  }
  #endif

  getfiles(&pathsc,paths,&argc,argv,".");

  for (int i = 0; i < dirpatternsc; i++) {free(dirpatterns[i]);} free(dirpatterns);
  for (int i = 0; i < reposc; i++) {git_repository_free(repos[i]);}
  reposc = flags = totalignored = 0;
  git_libgit2_shutdown();
  
  #ifdef DEBUG
  paths[pathsc] = NULL;
  if (pathsc == 0 && totalignored > 0) {
    debugprint("selector.c: No files selected but ignored > 0. Did you forget the -a flag?\n");
  }
  #endif

  return paths;
}
