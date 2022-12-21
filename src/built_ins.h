#ifndef BUILT_INS_H
#define BUILT_INS_H

void idle_process();

void invalid_cmd_process();

void busy_process();

void sleep_process();

void zombify();

void orphanify();

void ps_process();

void kill_process(char **args);

void echo_wrapper(char **args);

void ls_wrapper(char **args);

void cat_wrapper(char **args);

void touch_wrapper(char **args);

void mv_wrapper(char **args);

void rm_wrapper(char **args);

void cp_wrapper(char **args);

void chmod_wrapper(char **args);

char **parse_script(char **args);

#endif // !BUILT_INS_H