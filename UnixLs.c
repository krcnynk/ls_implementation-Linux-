#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>

enum status{onlyCMD, CMDoption, CMDdirs, CMDoptionsdirs, invalid}; 

bool checkOptions(char* str,bool flags[])
{
  if(str[0] != '-')
  {
    return false;
  }
  int i = 1;
  while(str[i] != '\0')
  {
    if(str[i] == 'l' || str[i] == 'R' ||str[i] == 'i')
    {
      if(str[i] == 'l')
      {
        flags[0] = true;
      }
      if(str[i] == 'R')
      {
        flags[1] = true;
      }
      if(str[i] == 'i')
      {
        flags[2] = true;
      }
    }
    else
    {
      printf("UnixLs: invalid option -- '%c'\n",str[i]);
      exit(1);
    }
    ++i;
  }
  return true;
}

void swapVectors(char**v, int index1, int index2)
{
  char* temp;
  temp = v[index1];
  v[index1] = v[index2];
  v[index2] = temp;
}

void parse(int c, char** v,enum status *statusP,bool flags[])
{
  if(c == 1)
  {
    *statusP = onlyCMD;
  }
  else if(c == 2)
  {
    if(checkOptions(v[1],flags) == true)
    {
        *statusP = CMDoption;
    }
    else
    {
        *statusP = CMDdirs;
    }
  }
  else
  {
    *statusP = CMDdirs;
    for(int i = 1; i < c;++i)
    {
      if(checkOptions(v[i],flags) == true)
      {
        *statusP = CMDoptionsdirs;
      }
    }
  }
}

void readDirOption(char* path,bool flags[])//Option flags l R i
{
  DIR *dir;
  struct dirent *dp;

  if((dir = opendir(path)) == NULL)
  {
    printf("UnixLs: cannot access '%s': No such file or directory\n",path);
    return;
  }

    printf("%s:\n",path);

  while((dp = readdir(dir)))
  {
    if(dp->d_name[0] == '.')
    {
      continue;
    }
    struct stat dps;
    bool symbolic = false;
    if(strcmp(dp->d_name,".") && strcmp(dp->d_name,".."))
    {
        char statStr[1024] = {};
        strcat(statStr,path);
        strcat(statStr,"/");
        strcat(statStr,dp->d_name);

        lstat(statStr,&dps);

      if(flags[2])
      {
        printf("%-5lu",dps.st_ino);
      }

      if(flags[0])
      {
        if(S_ISLNK(dps.st_mode) != 0)
        {
          printf("l");
          symbolic = true;
        }
        else if(S_ISDIR(dps.st_mode) != 0)
        {
          printf("d");
        }
        else
        {
          printf("-");
        }
        
        printf( (dps.st_mode & S_IRUSR) ? "r" : "-");
        printf( (dps.st_mode & S_IWUSR) ? "w" : "-");
        printf( (dps.st_mode & S_IXUSR) ? "x" : "-");
        printf( (dps.st_mode & S_IRGRP) ? "r" : "-");
        printf( (dps.st_mode & S_IWGRP) ? "w" : "-");
        printf( (dps.st_mode & S_IXGRP) ? "x" : "-");
        printf( (dps.st_mode & S_IROTH) ? "r" : "-");
        printf( (dps.st_mode & S_IWOTH) ? "w" : "-");
        printf( (dps.st_mode & S_IXOTH) ? "x" : "-");
        
        printf("%5lu",dps.st_nlink);
        
        struct passwd *pw = NULL;
        pw = getpwuid(dps.st_uid);
        assert(pw);
        printf("%10s",pw->pw_name);

        struct group *grp = NULL;
        grp = getgrgid(dps.st_gid);
        assert(grp); 
        printf("%10s",grp->gr_name);

        printf("%7lu",dps.st_size);

        struct tm * timeinfo = NULL;
        char month[6];
        char day[6];
        char hourmin[6];
        timeinfo = localtime(&dps.st_mtime);
        strftime(month,sizeof(month),"%b",timeinfo);
        strftime(day,sizeof(day),"%d",timeinfo);
        if(day[0] == '0')
        {
          day[0] = day[1];
          day[1] = '\0';
        }
        strftime(hourmin,sizeof(hourmin),"%R",timeinfo);
        printf("%4s%3s%7s ", month,day,hourmin);
      }
      printf("%s",dp->d_name);

      if(symbolic == true)
      {
        char buf[PATH_MAX] = {};
        ssize_t nbytes;

        nbytes = readlink(statStr, buf, sizeof(buf));
        if (nbytes == -1) {
            perror("readlink");
            exit(EXIT_FAILURE);
        }
        printf(" -> %s",buf);
      }

      printf("\n");
    }
  }
  printf("\n");
  closedir(dir);

    if(flags[1])
    {
        if((dir = opendir(path)) == NULL)
        {
            printf("Unable to read directory\n");
            return;
        }
        while((dp = readdir(dir)))
        {
            struct stat dps;
            char statStr[1024] = {};
            strcat(statStr,path);
            strcat(statStr,"/");
            strcat(statStr,dp->d_name);

            lstat(statStr,&dps);

            if(strcmp(dp->d_name,".") && strcmp(dp->d_name,".."))
            {
              //printf("KKK: %d %s %u %u %u %u %u\n",S_ISDIR(dps.st_mode),dp->d_name,dp->d_type,DT_DIR,DT_UNKNOWN,DT_LNK,DT_CHR);
                if((S_ISDIR(dps.st_mode)) != 0)
                {
                    char str[1024] = {};
                    strcat(str,path);
                    strcat(str,"/");
                    strcat(str,dp->d_name);
                    readDirOption(str,flags);
                }
            }
        }
        closedir(dir);
    }
}

void readDir(char* path)
{
  DIR *dir;
  struct dirent *dp;

  if((dir = opendir(path)) == NULL)
  {
    printf("UnixLs: cannot access '%s': No such file or directory\n",path);
    return;
  }

  while((dp = readdir(dir)))
  {
    if(strcmp(dp->d_name,".") && strcmp(dp->d_name,".."))
    {
      printf("%s",dp->d_name);
      printf("\n");
    }
  }
  printf("\n");
  closedir(dir);
}

int main(int argc, char *argv[]) {

  enum status state;
  bool flags[3] = {false};//Option flags l R i

  parse(argc,argv,&state,flags);

  if(state == invalid)
  {
    printf("Couldn't parse the input\n");
    return 1;
  }
  else if(state == onlyCMD)
  {
      readDir("./");
  }
  else if(state == CMDoption)
  {
      readDirOption(".",flags);
  }
  else if(state == CMDoptionsdirs)
  {
    bool flag = false;
      for(int i = 1; i < argc; ++i)
      {
        if(argv[i][0] != '-')
        {
          flag = true;
          char str[1024] = {};
          strcat(str,"./");
          strcat(str,argv[i]);
          readDirOption(str,flags);
        }
      }
    if(flag == false)
    {
      readDirOption(".",flags);
    }
  }
  else if(state == CMDdirs)
  {
    for(int i = 1; i < argc; ++i)
    {
      if(argv[i][0] != '-')
      {
      char str[1024] = {};
      strcat(str,"./");
      strcat(str,argv[i]);
      readDir(str);
      }
    }
  }

	return 0;
}
