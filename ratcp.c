#define _DEFAULT_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


enum State {
    HAVE_NONE,
    HAVE_SRC,
    HAVE_DEST
};


size_t file_count;
size_t files_copied;

int    progress;
int    progress_max;


int
copy_file(char* src,
          char* dest)
{
    #define GOTO_CLEANUP(msg)   \
        do { exit_code = 0; printf(msg " %s" "\n", src); perror("copy_file"); goto cleanup; } while(0)

    char    exit_code;
    char    buffer[BUFSIZ];

    int     source_fd;
    int     dest_fd;
    int     open_flags;

    mode_t  permission_flags;
    ssize_t read_bytes;
    struct stat src_stat;

    exit_code = 1;

    open_flags       = O_TRUNC | O_CREAT  | O_WRONLY | O_EXCL;

    permission_flags = S_IRUSR | S_IWUSR  | S_IRGRP |
                       S_IWGRP | S_IROTH  | S_IWOTH;

    source_fd = open(src, O_RDONLY);

    if (source_fd == -1)
        GOTO_CLEANUP("Error opening source file");

    dest_fd = open(dest, open_flags, permission_flags);
    if (dest_fd == -1)
        GOTO_CLEANUP("Error opening destination file");


    while ((read_bytes = read(source_fd, buffer, BUFSIZ))) {
        if (read_bytes == -1)
            GOTO_CLEANUP("Error reading source file");

        if (write(dest_fd, buffer, read_bytes) != read_bytes)
            GOTO_CLEANUP("Error writing to destination file");
    }

    stat(src, &src_stat);
    chmod(dest, src_stat.st_mode);


cleanup:;
    if (source_fd != -1)
        close(source_fd);

    if (dest_fd != -1)
        close(dest_fd);

    return exit_code;
}


void
traverse_path(char*  src,
              size_t src_len,
              char*  dest,
              size_t dest_len,
              int    count_only,
              int    print_errors,
              size_t progress_print_interval)
{
    #define LOG(...)                \
    do {                            \
        if (print_errors)           \
            printf(__VA_ARGS__);    \
    } while(0)

    int i;

    DIR* dir;
    struct dirent* file;
    struct stat src_stat;

    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    strcpy(src_path, src);
    strcpy(dest_path, dest);

    dir = opendir(src_path);

    if (dir == NULL) {
        perror("traverse_path (opendir)");
        return;
    }

    while ((file = readdir(dir)) != NULL) {
        /* Ignore . and .. */
        if (file->d_name[0] == '.')
            if (file->d_name[1] == '\0')
                continue;
            else
                if (file->d_name[1] == '.' && file->d_name[2] == '\0')
                    continue;

        src_path[src_len]   = '/';
        dest_path[dest_len] = '/';

        /* Start at 1 to not overwrite the '/' we just appended */
        for (i = 1;; ++i) {
            src_path[src_len + i] = file->d_name[i - 1];
            dest_path[dest_len + i] = file->d_name[i - 1];

            if (file->d_name[i - 1] == '\0') {
                src_path[src_len + i] = '\0';
                dest_path[dest_len + i] = '\0';
                break;
            }
        }

        if (stat(src_path, &src_stat) != 0) {
            if (errno == ENOENT) {
                LOG("Skipping broken symbolic link at %s\n", src_path);
            } else {
                LOG("Error at %s\n", src_path);
                perror("stat");
            }

            continue;
        }

        if (file->d_type == DT_DIR) {
            if (!count_only)
                if (mkdir(dest_path, src_stat.st_mode) != 0) {
                    perror("traverse_path (mkdir)");
                    return;
                }

            traverse_path(src_path, src_len + i, dest_path, dest_len + i, count_only, print_errors, progress_print_interval);
        } else {
            if (file->d_type == DT_LNK) {
                /* I don't want symlinks, most of the drives I'm copying from don't
                   even support them, so might as well just axe 'em now. */
                LOG("Skipping symbolic link at %s\n", src_path);
                continue;
            }

            if (!count_only) {
                copy_file(src_path, dest_path);
                ++files_copied;

                if (files_copied >= progress_print_interval * (progress + 1)) {
                    ++progress;

                    /* Ignores -v, because I always want to see the progress. */
                    printf("%d/%d\n", progress, progress_max);
                }
            }

            ++file_count;
        }
    }

    closedir(dir);
}


size_t
count_files(char* dir)
{
    file_count = 0;
    traverse_path(dir, strlen(dir), "/dev/null", strlen("/dev/null"), 1, 0, 0);
    return file_count;
}


void
copy_files(char* src,
           char* dest,
           int   print_errors)
{
    size_t print_interval = count_files(src) / progress_max;

    file_count = 0;
    traverse_path(src, strlen(src), dest, strlen(dest), 0, print_errors, print_interval);
    printf("\nCopied %lu files.\n", file_count);
}


void
print_usage_and_exit(void)
{
    printf(
        "USAGE:\n"
        "   ratcp [OPTIONS] <SOURCE> <DEST> \n\n"
        "OPTIONS:\n"
        "   -v       | Print log messages.\n"
        "   -p <NUM> | Set the percentage interval at which progress is reported.\n"
    );

    exit(1);
}


int
main(int argc,
     char** argv)
{
    int i;
    int print_errors;

    enum State state;

    char src[PATH_MAX];
    char dest[PATH_MAX];

    print_errors = 0;
    progress_max = 100;
    state = HAVE_NONE;

    if (argc < 3)
        print_usage_and_exit();

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-v") == 0)
                print_errors = 1;

            if (strcmp(argv[i], "-p") == 0)
                progress_max = strtoul(argv[++i], NULL, 10);
        } else {
            if (isdigit(argv[i][0]))
                continue;

            switch (state)
            {
                case HAVE_NONE:
                    strcpy(src, argv[i]);
                    state = HAVE_SRC;
                    break;

                case HAVE_SRC:
                    strcpy(dest, argv[i]);
                    state = HAVE_DEST;
                    break;

                case HAVE_DEST:
                    print_usage_and_exit();
                    break;
            }
        }

    }

    copy_files(src, dest, print_errors);
    return 0;
}