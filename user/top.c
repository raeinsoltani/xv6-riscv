#include "user/user.h"


int main() {
    struct top *info = (struct top *)malloc(sizeof(struct top));
    top(info);
    info->uptime = uptime();

    printf("\033[0;32m\nuptime: %d seconds\n\033[0m", info->uptime);

    printf("\033[1;33mtotal process: %d\n\033[0m", info->total_process);

    printf("\033[0;36mrunning process: %d\n", info->running_process);
    printf("sleeping process: %d\n\033[0m", info->sleeping_process);

    printf("\033[0;34mprocess data:\n");
    printf("Name      PID  PPID   State\n\033[0m");

    char *state;
    for (int i = 0; i < info->total_process; i++) {
        // Assign state based on process state
        switch (info->p_list[i].state) {
            case SLEEPING:
                state = "sleeping";
                break;
            case RUNNING:
                state = "running";
                break;
            case RUNNABLE:
                state = "runnable";
                break;
            case USED:
                state = "used";
                break;
            case UNUSED:
                state = "unused";
                break;
            default:
                state = "zombie";
        }

        printf("%s        %d     %d       \033[0;35m%s\033[0m\n", 
            info->p_list[i].name, info->p_list[i].pid, info->p_list[i].ppid, state);
    }

    printf("\n");
    return 0;
}