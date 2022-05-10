extern int do_syscall(int num, ...);

int main(int argc, char** argv, char** envp)
{
    do_syscall(0);
    while (1);
    return 0;
}