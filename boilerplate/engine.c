
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define MAX_CONTAINERS 100

typedef enum {
    RUNNING,
    STOPPED
} state_t;

typedef struct {
    char id[32];
    pid_t pid;
    state_t state;
    time_t start_time;
} container_t;

container_t containers[MAX_CONTAINERS];
int container_count = 0;

void usage(char *prog) {
    printf("Usage:\n");
    printf("%s supervisor <rootfs>\n", prog);
    printf("%s start <id> <rootfs> <command>\n", prog);
    printf("%s run <id> <rootfs> <command>\n", prog);
    printf("%s ps\n", prog);
    printf("%s stop <id>\n", prog);
}

void add_container(char *id, pid_t pid) {
    strcpy(containers[container_count].id, id);
    containers[container_count].pid = pid;
    containers[container_count].state = RUNNING;
    containers[container_count].start_time = time(NULL);
    container_count++;
}

void update_container(pid_t pid) {
    for (int i = 0; i < container_count; i++) {
        if (containers[i].pid == pid) {
            containers[i].state = STOPPED;
        }
    }
}

void list_containers() {
    printf("ID\tPID\tSTATE\n");
    for (int i = 0; i < container_count; i++) {
        printf("%s\t%d\t%s\n",
               containers[i].id,
               containers[i].pid,
               containers[i].state == RUNNING ? "running" : "stopped");
    }
}

void stop_container(char *id) {
    for (int i = 0; i < container_count; i++) {
        if (strcmp(containers[i].id, id) == 0) {
            kill(containers[i].pid, SIGKILL);
            containers[i].state = STOPPED;
            printf("Stopped container %s\n", id);
            return;
        }
    }
    printf("Container not found\n");
}

void run_container(char *id, char *rootfs, char *cmd, int wait_flag) {
    pid_t pid = fork();

    if (pid == 0) {
        // CHILD
        printf("[Container %s] Starting...\n", id);

        if (chroot(rootfs) != 0) {
            perror("chroot failed");
            exit(1);
        }

        chdir("/");

        mkdir("/proc", 0555);
        mount("proc", "/proc", "proc", 0, NULL);

        execl(cmd, cmd, NULL);

        perror("exec failed");
        exit(1);
    } else {
        // PARENT
        printf("[Supervisor] Container %s started with PID %d\n", id, pid);
        add_container(id, pid);

        if (wait_flag) {
            waitpid(pid, NULL, 0);
            update_container(pid);
        }
    }
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        update_container(pid);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    signal(SIGCHLD, sigchld_handler);

    if (strcmp(argv[1], "supervisor") == 0) {
        printf("Supervisor running...\n");
        while (1) pause();
    }

    if (strcmp(argv[1], "start") == 0) {
        if (argc < 5) {
            usage(argv[0]);
            return 1;
        }
        run_container(argv[2], argv[3], argv[4], 0);
    }

    else if (strcmp(argv[1], "run") == 0) {
        if (argc < 5) {
            usage(argv[0]);
            return 1;
        }
        run_container(argv[2], argv[3], argv[4], 1);
    }

    else if (strcmp(argv[1], "ps") == 0) {
        list_containers();
    }

    else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            usage(argv[0]);
            return 1;
        }
        stop_container(argv[2]);
    }

    else {
        usage(argv[0]);
        }
        
        return 0;
        }
