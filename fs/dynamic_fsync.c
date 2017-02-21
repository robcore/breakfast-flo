/*
 * Copyright (C) 2012-2017, Paul Reioux,
 *			    Alex Saiko <solcmdr@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/writeback.h>

#include <linux/earlysuspend.h>

#define TAG "[DFS]: "

#define DFS_VERSION_MAJOR 3
#define DFS_VERSION_MINOR 0

/*
 * DFS boolean triggers. They control all the routine.
 */
bool suspended __read_mostly = false;
bool dfs_active __read_mostly = false;

/*
 * fsync_mutex protects dfs_active during suspend / late resume
 * transitions.
 */
static DEFINE_MUTEX(fsync_mutex);

/*
 * File synchronization API.
 */
extern void sync_filesystems(int wait);

static void dfs_force_flush(void)
{
	sync_filesystems(0);
	sync_filesystems(1);
}

static void dfs_resume(void)
{
	mutex_lock(&fsync_mutex);
	suspended = false;
	mutex_unlock(&fsync_mutex);
}

static void dfs_suspend(void)
{
	mutex_lock(&fsync_mutex);
	if (dfs_active) {
		suspended = true;
		dfs_force_flush();
	}
	mutex_unlock(&fsync_mutex);
}

static struct early_suspend dfs_early_suspend_notifier = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
	.suspend = dfs_suspend,
	.resume = dfs_resume,
};

static int dfs_panic_event(struct notifier_block *this,
			   unsigned long event, void *ptr)
{
	suspended = true;
	dfs_force_flush();

	return NOTIFY_DONE;
}

static struct notifier_block dfs_panic_block = {
	.notifier_call	= dfs_panic_event,
	.priority	= INT_MAX,
};

static int dfs_notify_sys(struct notifier_block *this, unsigned long code,
			  void *unused)
{
	if (code == SYS_DOWN || code == SYS_HALT) {
		suspended = true;
		dfs_force_flush();
	}

	return NOTIFY_DONE;
}

static struct notifier_block dfs_notifier = {
	.notifier_call = dfs_notify_sys,
};

static ssize_t dfs_active_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", (dfs_active ? 1 : 0));
}

static ssize_t dfs_active_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int data;

	if (sscanf(buf, "%u\n", &data) == 1) {
		if (data == 1) {
			pr_info(TAG "Enabled!\n");
			dfs_active = true;
		} else if (data == 0) {
			pr_info(TAG "Disabled!\n");
			dfs_active = false;
		} else
			pr_info(TAG "Bad value!\n");
	} else
		pr_info(TAG "Unknown input!\n");

	return count;
}

static ssize_t dfs_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "DFS Version: %u.%u\n",
		DFS_VERSION_MAJOR,
		DFS_VERSION_MINOR);
}

static ssize_t dfs_suspended_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "DFS Suspended: %u\n", suspended);
}

static struct kobj_attribute dfs_active_attribute =
	__ATTR(Dyn_fsync_active, 0666,
		dfs_active_show,
		dfs_active_store);

static struct kobj_attribute dfs_version_attribute =
	__ATTR(Dyn_fsync_version, 0444, dfs_version_show, NULL);

static struct kobj_attribute dfs_suspended_attribute =
	__ATTR(Dyn_fsync_earlysuspend, 0444, dfs_suspended_show, NULL);

static struct attribute *dfs_active_attrs[] = {
	&dfs_active_attribute.attr,
	&dfs_version_attribute.attr,
	&dfs_suspended_attribute.attr,
	NULL,
};

static struct attribute_group dfs_active_attr_group = {
	.attrs = dfs_active_attrs,
};

static struct kobject *dfs_kobj;

static int dfs_init(void)
{
	int sysfs_result;

	register_early_suspend(&dfs_early_suspend_notifier);
	register_reboot_notifier(&dfs_notifier);
	atomic_notifier_chain_register(&panic_notifier_list,
		&dfs_panic_block);

	dfs_kobj = kobject_create_and_add("dyn_fsync", kernel_kobj);
	if (!dfs_kobj) {
		pr_err(TAG "kobject create failed!\n");
		return -ENOMEM;
	}

	sysfs_result = sysfs_create_group(dfs_kobj, &dfs_active_attr_group);

	if (sysfs_result) {
		pr_info(TAG "sysfs create failed!\n");
		kobject_put(dfs_kobj);
	}

	return sysfs_result;
}

static void dfs_exit(void)
{
	unregister_early_suspend(&dfs_early_suspend_notifier);
	unregister_reboot_notifier(&dfs_notifier);
	atomic_notifier_chain_unregister(&panic_notifier_list,
		&dfs_panic_block);

	if (dfs_kobj != NULL)
		kobject_put(dfs_kobj);
}

module_init(dfs_init);
module_exit(dfs_exit);

MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_DESCRIPTION("Dynamic fsync - automatic fsync trigger.");
MODULE_LICENSE("GPLv2");
