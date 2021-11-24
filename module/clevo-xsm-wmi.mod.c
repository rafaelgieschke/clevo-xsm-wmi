#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x25a444ff, "module_layout" },
	{ 0x3bb8f120, "param_array_ops" },
	{ 0x2c88ae6b, "param_get_byte" },
	{ 0x38552a81, "param_ops_bool" },
	{ 0x8f98374f, "platform_driver_unregister" },
	{ 0x129bca13, "platform_device_unregister" },
	{ 0x758659ed, "device_remove_file" },
	{ 0xc3f4aedc, "hwmon_device_unregister" },
	{ 0x4ecc2f74, "sysfs_remove_group" },
	{ 0xac1a55be, "unregister_reboot_notifier" },
	{ 0x83eb21c, "rfkill_unregister" },
	{ 0x4c9a52ca, "input_unregister_device" },
	{ 0xb9830b3b, "led_classdev_unregister" },
	{ 0x3517383e, "register_reboot_notifier" },
	{ 0xc017a45e, "sysfs_create_group" },
	{ 0xb7a1adaa, "hwmon_device_register" },
	{ 0xf02216ad, "kmem_cache_alloc_trace" },
	{ 0x1148c8a, "kmalloc_caches" },
	{ 0xc29f047c, "device_create_file" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x6c9ae98c, "of_led_classdev_register" },
	{ 0xdf9208c0, "alloc_workqueue" },
	{ 0xcbeaf22a, "input_free_device" },
	{ 0x38648ece, "input_register_device" },
	{ 0x75f19332, "input_allocate_device" },
	{ 0x8a490c90, "rfkill_set_sw_state" },
	{ 0xdb68bbad, "rfkill_destroy" },
	{ 0xff282521, "rfkill_register" },
	{ 0xe5e1bd7, "rfkill_alloc" },
	{ 0x984b6d00, "__platform_create_bundle" },
	{ 0xc9d4d6d1, "wmi_has_guid" },
	{ 0xd4835ef8, "dmi_check_system" },
	{ 0xcc5005fe, "msleep_interruptible" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x55129c3, "current_task" },
	{ 0xfbef41e3, "input_event" },
	{ 0xf1569fd9, "wake_up_process" },
	{ 0xc0181fe2, "kthread_create_on_node" },
	{ 0x19494c64, "param_set_byte" },
	{ 0xaa00fdc0, "ec_transaction" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xc708f1fe, "ec_write" },
	{ 0xfc4152fc, "ec_read" },
	{ 0x76ae31fd, "wmi_remove_notify_handler" },
	{ 0xbca37f48, "kthread_stop" },
	{ 0xf18bdd75, "wmi_install_notify_handler" },
	{ 0x3b6c41ea, "kstrtouint" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0xc5850110, "printk" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0x837b7b09, "__dynamic_pr_debug" },
	{ 0x37a0cba, "kfree" },
	{ 0x6068bedf, "wmi_evaluate_method" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xbdfb6dbb, "__fentry__" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=rfkill,wmi";

MODULE_ALIAS("dmi*:pn*PC5x_7xHP_HR_HS*:");
MODULE_ALIAS("dmi*:pn*P870TM_TM1*:");
MODULE_ALIAS("dmi*:pn*P870DM*:");
MODULE_ALIAS("dmi*:pn*P7xxTM1*:");
MODULE_ALIAS("dmi*:pn*P7xxDM(-G)*:");
MODULE_ALIAS("dmi*:pn*P750ZM*:");
MODULE_ALIAS("dmi*:pn*P370SM-A*:");
MODULE_ALIAS("dmi*:pn*P17SM-A*:");
MODULE_ALIAS("dmi*:pn*P15SM1-A*:");
MODULE_ALIAS("dmi*:pn*P15SM-A*:");
MODULE_ALIAS("dmi*:pn*P17SM*:");
MODULE_ALIAS("dmi*:pn*P15SM*:");
MODULE_ALIAS("dmi*:pn*P150EM*:");
MODULE_ALIAS("dmi*:pn*Deimos/Phobos1x15S*:");
MODULE_ALIAS("dmi*:pn*P5ProSE*:");
MODULE_ALIAS("dmi*:svn*ECT*:rn*P750ZM*:");
MODULE_ALIAS("dmi*:pn*P65_67RSRP*:");
MODULE_ALIAS("dmi*:pn*P65xRP*:");
MODULE_ALIAS("dmi*:pn*P15xEMx*:");
MODULE_ALIAS("dmi*:pn*P95_HP,HR,HQ*:");

MODULE_INFO(srcversion, "72104BA1F30861E6A400F5B");
