/*
 * Copyright (C) 2009 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>	/* copy_from/to_user */
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/sec_export.h>
#include <soc/qcom/scm.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include "sec_core.h"

#define EFUSE_COMPAT_STR	"qcom,msm-efuse"

static int sec_init(void);
static void sec_exit(void);
static ssize_t sec_read(struct file *filp, char *buf,
		size_t count, loff_t *f_pos);
static int sec_open(struct inode *inode, struct file *filp);
static int sec_release(struct inode *inode, struct file *filp);
static long sec_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param);

static bool sec_alloc_buffer(void);
static bool sec_buf_updated(void);

#define TZBSP_SVC_OEM 254
#define SEC_BUF_SIZE 32

static u8 *sec_shared_mem;
static u32 sec_phy_mem;
static int sec_failures;

/* Structure that declares the usual file */
/* access functions */
const struct file_operations sec_fops = {
	.read = sec_read,
	.open = sec_open,
	.release = sec_release,
	.unlocked_ioctl = sec_ioctl
};

struct sec_cmd {
	int mot_cmd;
	int parm1;
	int parm2;
	int parm3;
};

static struct miscdevice sec_dev = {
	MISC_DYNAMIC_MINOR,
	"sec",
	&sec_fops
};

static DEFINE_MUTEX(sec_core_lock);

/****************************************************************************/
/*   KERNEL DRIVER APIs, ONLY IOCTL RESPONDS BACK TO USER SPACE REQUESTS    */
/****************************************************************************/

int sec_init(void)
{
	int result;

	result = misc_register(&sec_dev);

	if (result) {
		pr_err("sec: cannot obtain major number\n");
		return result;
	}

	return 0;
}

void sec_exit(void)
{
	/* Freeing the major number */
	misc_deregister(&sec_dev);
}

/* Mapping of the module init and exit functions */
module_init(sec_init);
module_exit(sec_exit);

int sec_open(struct inode *inode, struct file *filp)
{
	/* Not supported, return Success */
	return 0;
}

int sec_release(struct inode *inode, struct file *filp)
{

	/* Not supported, return Success */
	return 0;
}

ssize_t sec_read(struct file *filp, char *buf,
		 size_t count, loff_t *f_pos)
{
	/* Not supported, return Success */
	return 0;
}

static bool sec_alloc_buffer()
{
	sec_shared_mem = kzalloc(PAGE_ALIGN(SEC_BUF_SIZE), GFP_KERNEL);
	sec_phy_mem = (u32)virt_to_phys(sec_shared_mem);

	if (!sec_shared_mem || !sec_phy_mem) {
		kfree(sec_shared_mem);
		pr_err("sec: mem alloc fail!");
		return false;
	}

	memset(sec_shared_mem, 0xff, SEC_BUF_SIZE);
	scm_inv_range((u32)sec_shared_mem, (u32)sec_shared_mem + SEC_BUF_SIZE);

	return true;
}

static bool sec_buf_updated()
{
	int i;
	scm_inv_range((u32)sec_shared_mem, (u32)sec_shared_mem + SEC_BUF_SIZE);

	for (i = 0; i < SEC_BUF_SIZE; i++)
		if (sec_shared_mem[i] != 0xff)
			return true;

	pr_err("sec: buffer not updated!");
	return false;
}

long sec_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{
	struct sec_cmd my_cmd;
	struct SEC_EFUSE_PARM_T efuse_data;
	long ret_val = SEC_KM_FAIL;
	u32 ctr;

	mutex_lock(&sec_core_lock);

	if (sec_alloc_buffer() == false) {
		mutex_unlock(&sec_core_lock);
		return SEC_KM_FAIL;
	}

	switch (ioctl_num) {

	case SEC_IOCTL_READ_PROC_ID:

		for (ctr = 0; ctr < 5 && ret_val != SEC_KM_SUCCESS; ctr++) {

			my_cmd.mot_cmd = 10;
			my_cmd.parm1 = sec_phy_mem;
			my_cmd.parm2 = SEC_PROC_ID_SIZE;

			if (scm_call(TZBSP_SVC_OEM, 1, &my_cmd,
				sizeof(my_cmd), NULL, 0) != 0) {

				pr_err("sec: scm call failed!\n");
				continue;
			}

			/* check data was actually updated */
			if (sec_buf_updated() == false)
				continue;

			if (copy_to_user((void __user *)ioctl_param,
				(const void *) sec_shared_mem,
				SEC_PROC_ID_SIZE) == 0) {

				ret_val = SEC_KM_SUCCESS;
			}
		}

		break;

	case SEC_IOCTL_WRITE_FUSE:

		if (copy_from_user(&efuse_data, (void __user *)
			ioctl_param, sizeof(efuse_data)) != 0) {

			break;
		}

		my_cmd.mot_cmd = 2;
		my_cmd.parm1 = efuse_data.which_bank;
		my_cmd.parm2 = efuse_data.efuse_value;

		if (scm_call(TZBSP_SVC_OEM, 1, &my_cmd,
			sizeof(my_cmd), NULL, 0) == 0) {

			ret_val = SEC_KM_SUCCESS;
		}

		break;

	case SEC_IOCTL_READ_FUSE:

		if (copy_from_user(&efuse_data, (void *)ioctl_param,
			sizeof(efuse_data)) != 0) {

			break;
		}

		for (ctr = 0; ctr < 5 && ret_val != SEC_KM_SUCCESS; ctr++) {

			my_cmd.mot_cmd = 1;
			my_cmd.parm1 = efuse_data.which_bank;
			my_cmd.parm2 = sec_phy_mem;

			if (scm_call(TZBSP_SVC_OEM, 1, &my_cmd,
				sizeof(my_cmd), NULL, 0) != 0) {

				pr_err("sec: scm call failed!\n");
				continue;
			}

			/* check data was actually updated */
			if (sec_buf_updated() == false)
				continue;

			efuse_data.efuse_value = *(u32 *)sec_shared_mem;

			if (copy_to_user((void *)ioctl_param,
				&efuse_data, sizeof(efuse_data)) == 0) {

				ret_val = SEC_KM_SUCCESS;
			}
		}

		break;

	case SEC_IOCTL_GET_TZ_VERSION:

		my_cmd.mot_cmd = 11;
		my_cmd.parm1 = sec_phy_mem;

		if (scm_call(TZBSP_SVC_OEM, 1, &my_cmd,
			sizeof(my_cmd), NULL, 0) == 0) {

			if (copy_to_user((void *)ioctl_param,
				sec_shared_mem, 4) == 0) {

				ret_val = SEC_KM_SUCCESS;
			}
		}

		break;

	case SEC_IOCTL_GET_TZ_CODES:

		pr_err("sec: fail counter = %d\n", sec_failures);
		print_hab_fail_codes();
		break;

	default:
		pr_err("sec: invalid ioctl number\n");
		break;
	}

	if (ret_val != SEC_KM_SUCCESS)
		sec_failures++;


	kfree(sec_shared_mem);
	mutex_unlock(&sec_core_lock);

	return ret_val;
}

void print_hab_fail_codes(void)
{
	struct sec_cmd my_cmd;

	if (sec_alloc_buffer() == false)
		return;

	mutex_lock(&sec_core_lock);

	my_cmd.mot_cmd = 9;
	my_cmd.parm1 = 16;
	my_cmd.parm2 = sec_phy_mem;

	scm_call(254, 1, &my_cmd, sizeof(my_cmd), NULL, 0);
	sec_buf_updated();

	pr_err("HAB fail codes: 0x%x 0x%x 0x%x 0x%x\n",
	sec_shared_mem[0], sec_shared_mem[1],
	sec_shared_mem[2], sec_shared_mem[3]);

	mutex_unlock(&sec_core_lock);

	kfree(sec_shared_mem);
}
EXPORT_SYMBOL(print_hab_fail_codes);

static int msm_efuse_probe(struct platform_device *pdev)
{
	int result;

	result = misc_register(&sec_dev);

	if (result) {
		pr_err("sec: cannot obtain major number\n");
		return result;
	}

	return 0;
}

static struct of_device_id msm_match_table[] = {
	{.compatible = EFUSE_COMPAT_STR},
	{},
};
EXPORT_COMPAT(EFUSE_COMPAT_STR);

static int msm_efuse_remove(struct platform_device *pdev)
{
	/* Freeing the major number */
	misc_deregister(&sec_dev);
	return 0;
}

static struct platform_driver msm_efuse_driver = {
	.probe = msm_efuse_probe,
	.remove = msm_efuse_remove,
	.driver         = {
		.name = "msm_efuse",
		.owner = THIS_MODULE,
		.of_match_table = msm_match_table
	},
};

static int __init msm_efuse_init(void)
{
	return platform_driver_register(&msm_efuse_driver);
}

static void __exit msm_efuse_exit(void)
{
	platform_driver_unregister(&msm_efuse_driver);
}

module_init(msm_efuse_init)
module_exit(msm_efuse_exit)
/****************************************************************************/
/*Kernel Module License Information                                         */
/****************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MOTOROLA");
