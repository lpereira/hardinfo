
#include "hardinfo.h"

/* requires COMPILE_FLAGS "-std=c99" */

const char *problem_marker() {
    static const char as_markup[] = "<big><b>\u26A0</b></big>";
    static const char as_text[] = "(!)";
    if (params.markup_ok)
        return as_markup;
    else
        return as_text;
}
