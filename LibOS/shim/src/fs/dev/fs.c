/* Copyright (C) 2014 Stony Brook University
   This file is part of Graphene Library OS.

   Graphene Library OS is free software: you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Graphene Library OS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/*!
 * \file
 *
 * This file contains the implementation of `/dev` pseudo-filesystem.
 */

#include "shim_fs.h"

extern const struct pseudo_fs_ops dev_null_fs_ops;
extern const struct pseudo_fs_ops dev_tty_fs_ops;
extern const struct pseudo_fs_ops dev_zero_fs_ops;
extern const struct pseudo_fs_ops dev_random_fs_ops;
extern const struct pseudo_fs_ops dev_urandom_fs_ops;
extern const struct pseudo_fs_ops dev_stdin_fs_ops;
extern const struct pseudo_fs_ops dev_stdout_fs_ops;
extern const struct pseudo_fs_ops dev_stderr_fs_ops;

static const struct pseudo_dir dev_root_dir = {
    .size = 8,
    .ent =  {
              { .name   = "null",
                .fs_ops = &dev_null_fs_ops,
                .type   = LINUX_DT_CHR },
              { .name   = "tty",
                .fs_ops = &dev_tty_fs_ops,
                .type   = LINUX_DT_CHR },
              { .name   = "zero",
                .fs_ops = &dev_zero_fs_ops,
                .type   = LINUX_DT_CHR },
              { .name   = "random",
                .fs_ops = &dev_random_fs_ops,
                .type   = LINUX_DT_CHR },
              { .name   = "urandom",
                .fs_ops = &dev_urandom_fs_ops,
                .type   = LINUX_DT_CHR },
              { .name   = "stdin",
                .fs_ops = &dev_stdin_fs_ops,
                .type   = LINUX_DT_LNK },
              { .name   = "stdout",
                .fs_ops = &dev_stdout_fs_ops,
                .type   = LINUX_DT_LNK },
              { .name   = "stderr",
                .fs_ops = &dev_stderr_fs_ops,
                .type   = LINUX_DT_LNK },
        },
};

static const struct pseudo_fs_ops dev_root_fs = {
    .open = &pseudo_dir_open,
    .mode = &pseudo_dir_mode,
    .stat = &pseudo_dir_stat,
};

static const struct pseudo_ent dev_root_ent = {
    .name   = "",
    .fs_ops = &dev_root_fs,
    .dir    = &dev_root_dir,
};

static int dev_open(struct shim_handle* hdl, struct shim_dentry* dent, int flags) {
    return pseudo_open(hdl, dent, flags, &dev_root_ent);
}

static int dev_lookup(struct shim_dentry* dent) {
    return pseudo_lookup(dent, &dev_root_ent);
}

static int dev_mode(struct shim_dentry* dent, mode_t* mode) {
    return pseudo_mode(dent, mode, &dev_root_ent);
}

static int dev_readdir(struct shim_dentry* dent, struct shim_dirent** dirent) {
    return pseudo_readdir(dent, dirent, &dev_root_ent);
}

static int dev_stat(struct shim_dentry* dent, struct stat* buf) {
    return pseudo_stat(dent, buf, &dev_root_ent);
}

static int dev_hstat(struct shim_handle* hdl, struct stat* buf) {
    return pseudo_hstat(hdl, buf, &dev_root_ent);
}

static int dev_follow_link(struct shim_dentry* dent, struct shim_qstr* link) {
    return pseudo_follow_link(dent, link, &dev_root_ent);
}

static ssize_t dev_read(struct shim_handle* hdl, void* buf, size_t count) {
    if (!hdl->info.dev.dev_ops.read)
        return -EACCES;
    return hdl->info.dev.dev_ops.read(hdl, buf, count);
}

static ssize_t dev_write(struct shim_handle* hdl, const void* buf, size_t count) {
    if (!hdl->info.dev.dev_ops.write)
        return -EACCES;
    return hdl->info.dev.dev_ops.write(hdl, buf, count);
}

static off_t dev_seek(struct shim_handle* hdl, off_t offset, int wence) {
    if (!hdl->info.dev.dev_ops.seek)
        return -EACCES;
    return hdl->info.dev.dev_ops.seek(hdl, offset, wence);
}

static int dev_truncate(struct shim_handle* hdl, off_t len) {
    if (!hdl->info.dev.dev_ops.truncate)
        return -EACCES;
    return hdl->info.dev.dev_ops.truncate(hdl, len);
}

static int dev_flush(struct shim_handle* hdl) {
    if (!hdl->info.dev.dev_ops.flush)
        return 0;
    return hdl->info.dev.dev_ops.flush(hdl);
}

static int dev_close(struct shim_handle* hdl) {
    if (!hdl->info.dev.dev_ops.close)
        return 0;
    return hdl->info.dev.dev_ops.close(hdl);
}

static off_t dev_poll(struct shim_handle* hdl, int poll_type) {
    if (poll_type == FS_POLL_SZ)
        return 0;

    off_t ret = 0;
    if ((poll_type & FS_POLL_RD) && hdl->info.dev.dev_ops.read)
        ret |= FS_POLL_RD;
    if ((poll_type & FS_POLL_WR) && hdl->info.dev.dev_ops.write)
        ret |= FS_POLL_WR;

    return ret;
}

int dev_update_dev_ops(struct shim_handle* hdl) {
    struct shim_dentry* dent = hdl->dentry;
    assert(dent);

    /* simply reopen pseudo-file, this will update dev_ops function pointers to correct values */
    return pseudo_open(hdl, dent, /*flags=*/0, &dev_root_ent);
}

struct shim_fs_ops dev_fs_ops = {
    .mount    = &pseudo_mount,
    .unmount  = &pseudo_unmount,
    .flush    = &dev_flush,
    .close    = &dev_close,
    .read     = &dev_read,
    .write    = &dev_write,
    .seek     = &dev_seek,
    .hstat    = &dev_hstat,
    .poll     = &dev_poll,
    .truncate = &dev_truncate,
};

struct shim_d_ops dev_d_ops = {
    .open        = &dev_open,
    .lookup      = &dev_lookup,
    .mode        = &dev_mode,
    .readdir     = &dev_readdir,
    .stat        = &dev_stat,
    .follow_link = &dev_follow_link,
};
