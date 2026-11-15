#include <mach/mach_types.h>
kern_return_t HP250Dmic_start(kmod_info_t *ki, void *data) { return KERN_SUCCESS; }
kern_return_t HP250Dmic_stop(kmod_info_t *ki, void *data) { return KERN_SUCCESS; }
KMOD_EXPLICIT_DECL(com.tarekwasfy.HP250Dmic, "0.1.0", HP250Dmic_start, HP250Dmic_stop)
