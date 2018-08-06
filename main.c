#include <ccan/opt/opt.h>
#include "mb_node.h"
#include "utils-inl.h"
#include "rule.h"
#include "cut.h"
#include <stdarg.h>

static const char *filename;
static char *rule_file(const char *optarg, void *unused)
{
    filename = optarg;
    printf("filename: %s\n", filename);
    return NULL;
}

static void err_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	/* Check return, for fascist gcc */
    vprintf(fmt, ap);
	va_end(ap);
    vprintf("\n", ap);
}

int main(int argc, char *argv[])
{
    int ret;
    opt_register_arg("-f", rule_file, NULL, NULL, "");
    ret = opt_parse(&argc, argv, err_printf);
    if(!ret) {
        exit(-1);
    } 

    if(filename == NULL) {
        exit(-1);
    }
    
    rule_set_t ruleset;
    ReadFilterFile(&ruleset, filename);

    struct cnode root;
    memset(&root, 0, sizeof(root));
    root.ruleset = ruleset;
    cut(&root);

    return 0;
}
