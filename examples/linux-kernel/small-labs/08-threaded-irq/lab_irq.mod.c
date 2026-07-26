#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x058c185a, "jiffies" },
	{ 0x32feeafc, "mod_timer" },
	{ 0x2cf3d298, "__irq_alloc_descs" },
	{ 0x89053761, "handle_simple_irq" },
	{ 0x5e80e781, "irq_set_chip_and_handler_name" },
	{ 0x9126ce86, "request_threaded_irq" },
	{ 0x02f9bbf0, "timer_init_key" },
	{ 0x6de270a3, "handle_bad_irq" },
	{ 0x432534e8, "irq_free_descs" },
	{ 0x2352b148, "timer_delete_sync" },
	{ 0x9dd4105e, "free_irq" },
	{ 0x44339294, "irq_set_chip_data" },
	{ 0x4d8419c6, "param_ops_uint" },
	{ 0xd272d446, "__fentry__" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xe8213e80, "_printk" },
	{ 0xff7fbdd1, "___ratelimit" },
	{ 0x74d54026, "generic_handle_irq" },
	{ 0x534ed5f3, "__msecs_to_jiffies" },
	{ 0x814e12e5, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x058c185a,
	0x32feeafc,
	0x2cf3d298,
	0x89053761,
	0x5e80e781,
	0x9126ce86,
	0x02f9bbf0,
	0x6de270a3,
	0x432534e8,
	0x2352b148,
	0x9dd4105e,
	0x44339294,
	0x4d8419c6,
	0xd272d446,
	0xd272d446,
	0xe8213e80,
	0xff7fbdd1,
	0x74d54026,
	0x534ed5f3,
	0x814e12e5,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"jiffies\0"
	"mod_timer\0"
	"__irq_alloc_descs\0"
	"handle_simple_irq\0"
	"irq_set_chip_and_handler_name\0"
	"request_threaded_irq\0"
	"timer_init_key\0"
	"handle_bad_irq\0"
	"irq_free_descs\0"
	"timer_delete_sync\0"
	"free_irq\0"
	"irq_set_chip_data\0"
	"param_ops_uint\0"
	"__fentry__\0"
	"__x86_return_thunk\0"
	"_printk\0"
	"___ratelimit\0"
	"generic_handle_irq\0"
	"__msecs_to_jiffies\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C6BE5713FB4A4D4BB597FD3");
