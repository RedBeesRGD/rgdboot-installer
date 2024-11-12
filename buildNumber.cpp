//
// BuildNumber.cpp : Defines the entry point for the console application.
//
// BuildNumber 1.0 - © S. Gregory 2016
// BuildNumber 1.1 - © S. nitr8   2024
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>


#define UNUSED(x)       ((x) = (x))

#define ALTERNATIVE     0


int main(int argc, char* argv[])
{
    FILE *fp;
    char *linebuf;

#if ALTERNATIVE
    const char *magic = "#define BUILDNUMBER";
#else
    const char *magic = "char *buildNumber = ";
#endif

    int nBuild = 1;
    size_t len = 0;
    ssize_t read = 0;
    char *line = NULL;

    UNUSED(argc);

    if (argc < 2)
    {
        printf("[ERROR]: No filename given\n");

        return -6;
    }

    fp = fopen(argv[1], "r+");

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        linebuf = (char *)malloc(len);

        if (linebuf)
        {
            memset(linebuf, 0, len);

            while ((read = getline(&line, &len, fp)) != -1)
            {
                len = strlen(magic);

                if (strncmp(line, magic, len) == 0)
                {
                    nBuild = atol(line + len + 1) + 1;
                    fclose(fp);
                    fp = fopen(argv[1], "w");

                    if (fp)
                    {
                        fprintf(fp, "#ifndef __VERSION_H__\r\n");
                        fprintf(fp, "#define __VERSION_H__\r\n\r\n\r\n");
#if ALTERNATIVE
                        fprintf(fp, "%s %d\r\n", magic, nBuild);
#else
                        fprintf(fp, "%s\"%d\";\r\n\r\n\r\n", magic, nBuild);
#endif

#if ALTERNATIVE
                        fprintf(fp, "%s_STR \"%04d\"\r\n\r\n\r\n", magic, nBuild);
#endif
                        fprintf(fp, "#endif /* __VERSION_H__ */\r\n\r\n");

                        fclose(fp);

                        return 0;
                    }
                    else
                    {
                        printf("[ERROR]: Couldn't open file %s\n", argv[1]);

                        return -1;
                    }
                }
                else
                {
                    const char *str = "#define __VERSION_H__";

                    if (strncmp(line, str, strlen(str)) == 0)
                    {
                        if (argv[2])
                        {
                            if (atoi(argv[2]) == 1)
                            {
                                if (access(argv[1], F_OK) == 0)
                                {
                                    remove(argv[1]);
                                    goto recreate;
                                }
                            }
                        }
                    }

                    continue;
                }
            }

            free(linebuf);
        }
        else
        {
            printf("[ERROR]: Couldn't allocate %lu bytes of memory for file %s\n", len, argv[1]);

            return -5;
        }

        printf("[ERROR]: File %s is not valid\n", argv[1]);
        printf("         Recreation can be done by calling 'buildnumber %s 1'\n", argv[1]);
        fclose(fp);

        return -2;
    }
    else
    {
recreate:
        fp = fopen(argv[1], "w");

        if (fp)
        {
            fprintf(fp, "#ifndef __VERSION_H__\r\n");
            fprintf(fp, "#define __VERSION_H__\r\n\r\n\r\n");
#if ALTERNATIVE
            fprintf(fp, "%s %d\r\n", magic, nBuild);
#else
            fprintf(fp, "%s\"%d\";\r\n\r\n\r\n", magic, nBuild);
#endif

#if ALTERNATIVE
            fprintf(fp, "%s_STR \"%04d\"\r\n\r\n\r\n", magic, nBuild);
#endif
            fprintf(fp, "#endif /* __VERSION_H__ */\r\n\r\n");

            fclose(fp);
        }
        else
        {
            printf("[ERROR]: Couldn't open file %s\n", argv[1]);

            return -4;
        }

        return 0;
    }

    return -3;
}

