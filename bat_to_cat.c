#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CYCS4540");
MODULE_DESCRIPTION("Proof-of-concept kernel module that rewrites 'bat' to 'cat' while reading text.txt");
MODULE_VERSION("0.1");

#define TARGET_NAME "text.txt"
#define SEARCH "bat"
#define REPLACE "cat"

struct read_ctx {
    struct file *file;
    char __user *buf;
    size_t count;
};

static bool is_target_file(struct file *file)
{
    if (!file || !file->f_path.dentry || !file->f_path.dentry->d_name.name)
        return false;

    return strcmp(file->f_path.dentry->d_name.name, TARGET_NAME) == 0;
}

static int replace_pattern(char *dst, size_t dst_size, const char *src, size_t src_len)
{
    size_t i = 0, j = 0;
    const size_t old_len = strlen(SEARCH);
    const size_t new_len = strlen(REPLACE);

    while (i < src_len) {
        if ((src_len - i) >= old_len && strncmp(src + i, SEARCH, old_len) == 0) {
            if (j + new_len > dst_size)
                return -EFAULT;

            memcpy(dst + j, REPLACE, new_len);
            i += old_len;
            j += new_len;
        } else {
            if (j + 1 > dst_size)
                return -EFAULT;

            dst[j++] = src[i++];
        }
    }

    return j;
}

static int bat_to_cat_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct read_ctx *ctx = (struct read_ctx *)ri->data;

    if (!ctx)
        return 0;

    ctx->file = (struct file *)regs->di;
    ctx->buf = (char __user *)regs->si;
    ctx->count = (size_t)regs->dx;
    return 0;
}

static int bat_to_cat_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct read_ctx *ctx = (struct read_ctx *)ri->data;
    long ret = (long)regs_return_value(regs);

    if (ret <= 0 || !ctx || !ctx->buf || !ctx->file)
        return 0;

    if (!is_target_file(ctx->file))
        return 0;

    size_t bytes = (size_t)ret;
    if (bytes > ctx->count)
        bytes = ctx->count;

    char *tmp = kmalloc(bytes + 1, GFP_KERNEL);
    if (!tmp)
        return 0;

    if (copy_from_user(tmp, ctx->buf, bytes)) {
        kfree(tmp);
        return 0;
    }
    tmp[bytes] = '\0';

    char *patched = kmalloc(bytes * 2 + 1, GFP_KERNEL);
    if (!patched) {
        kfree(tmp);
        return 0;
    }

    int out_len = replace_pattern(patched, bytes * 2 + 1, tmp, bytes);
    if (out_len < 0) {
        kfree(tmp);
        kfree(patched);
        return 0;
    }

    if (copy_to_user(ctx->buf, patched, out_len))
        pr_warn("bat_to_cat: failed to write rewritten buffer back to userspace\n");

    kfree(tmp);
    kfree(patched);
    return 0;
}

static struct kretprobe bat_to_cat_kretprobe = {
    .handler = bat_to_cat_ret,
    .entry_handler = bat_to_cat_entry,
    .data_size = sizeof(struct read_ctx),
    .maxactive = 32,
    .kp = {
        .symbol_name = "vfs_read",
    },
};

static int __init bat_to_cat_init(void)
{
    int rc = register_kretprobe(&bat_to_cat_kretprobe);

    if (rc < 0) {
        pr_err("bat_to_cat: register_kretprobe() failed for vfs_read, rc=%d\n", rc);
        return rc;
    }

    pr_info("bat_to_cat: kretprobe installed on vfs_read\n");
    return 0;
}

static void __exit bat_to_cat_exit(void)
{
    unregister_kretprobe(&bat_to_cat_kretprobe);
    pr_info("bat_to_cat: kretprobe removed\n");
}

module_init(bat_to_cat_init);
module_exit(bat_to_cat_exit);
