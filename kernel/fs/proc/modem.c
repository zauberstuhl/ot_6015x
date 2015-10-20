#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <../arch/arm/mach-msm/smd_private.h>

#define VERSION_MAX_LEN 16

static struct {
	char ver[VERSION_MAX_LEN];
} smem_version;


typedef enum {
	MODEM =0x01,
	MODEM_SBL =0x02,
	MODEM_TZ =0x04,
	MODEM_RPM =0x08,
	VER_TYPE_END
}ver_type_t;

static int proc_show(struct seq_file *m, void *v,ver_type_t type)
{
	uint8_t *data=NULL;
	unsigned int size;
	memset(&smem_version,0,sizeof(smem_version));
	if(type & MODEM_SBL){
		data = smem_get_entry(SMEM_ID_VENDOR0, &size);
		if (!data) {
			printk( KERN_ERR "can't get SMEM_ID_VENDOR0 %d",SMEM_ID_VENDOR0);
			return 1;
		}
	}else if(type & MODEM_TZ){
		data = smem_get_entry(SMEM_ID_VENDOR1, &size);
		if (!data) {
			printk( KERN_ERR "can't get SMEM_ID_VENDOR1 %d",SMEM_ID_VENDOR1);
			return 1;
		}
	}else if(type & MODEM_RPM){
		data = smem_get_entry(SMEM_ID_VENDOR2, &size);
		if (!data) {
			printk( KERN_ERR "can't get SMEM_ID_VENDOR2 %d",SMEM_ID_VENDOR2);
			return 1;
		}
	}

	if (size == sizeof(smem_version)) {
		memcpy(&smem_version, data, sizeof(smem_version));
	}else{
		printk( KERN_ERR "size of SMEM_ID_VENDOR2 is wrong %d not %d", size, sizeof(smem_version) );
		return 1;
	}
	seq_printf(m, "%s\n", smem_version.ver);
	return 0;
}


static int modem_sbl_proc_show(struct seq_file *m, void *v)
{
	return proc_show(m, v,MODEM_SBL);
}

static int modem_tz_proc_show(struct seq_file *m, void *v)
{
	return proc_show(m, v,MODEM_TZ);
}

static int modem_rpm_proc_show(struct seq_file *m, void *v)
{
	return proc_show(m, v,MODEM_RPM);
}

static int proc_open(struct inode *inode, struct file *file)
{
	printk( KERN_DEBUG "the file path is : %s", file->f_dentry->d_iname );
	if (strstr( file->f_dentry->d_iname, "modem_sbl")) {
		return single_open(file, modem_sbl_proc_show, NULL);
	}else if (strstr( file->f_dentry->d_iname, "modem_tz")) {
		return single_open(file, modem_tz_proc_show, NULL);
	}else if (strstr( file->f_dentry->d_iname, "modem_rpm")) {
		return single_open(file, modem_rpm_proc_show, NULL);
	}
	printk( KERN_ERR "don't know this proc/ file path %s", file->f_dentry->d_iname );
	return 1;
}

static const struct file_operations all_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations modem_sbl_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations modem_tz_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations modem_rpm_proc_fops = {
	.open		= proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_modem_init(void)
{
	proc_create("modem_sbl", 0, NULL, &modem_sbl_proc_fops);
	proc_create("modem_tz", 0, NULL, &modem_tz_proc_fops);
	proc_create("modem_rpm", 0, NULL, &modem_rpm_proc_fops);
	memset(&smem_version,0,sizeof(smem_version));
	return 0;
}

module_init(proc_modem_init);
/** add file (/proc/modem_sbl) for sbl1 version number by czb@tcl.com */
/** add file (/proc/modem_tz,/proc/modem_rpm ) for tz,rpm version number by czb@tcl.com */
