#include "utils.h"
#include "wayland_state.h"
#include <stdio.h>

int main() {
    struct wayland_state* state = wayland_state_new();
    UNUSE(state);
    printf("hello world!\n");
}
