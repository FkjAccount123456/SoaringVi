#include "editor.h"

void filemgr_open(filemgr *fm, rawstr name, char *name_mbs, char *path_mbs, FILE *f) {
    do_if_exist(free, fm->name.v);
    do_if_exist(free, fm->name_mbs);
    do_if_exist(free, fm->path_mbs);
    fm->name = name;
    fm->name_mbs = name_mbs;
    fm->path_mbs = path_mbs;
    if (!f) {
        fm->is_sync = false;
    } else {
        if (!fm->path_mbs || !text_read(&fm->mgr, f)) {
            editor_sendmsg_charp(fm->e, "Failed to read file");
            fm->is_sync = false;
        } else {
            fm->is_sync = true;
            fm->sync_time = get_file_updtime(fm->path_mbs);
        }
    }
    fm->ver = fm->mgr.undo_cur;
}

void filemgr_reopen(filemgr *fm) {
    if (!fm->path_mbs)
        fm->path_mbs = get_abspath(fm->name_mbs);
    if (!fm->path_mbs) {
        editor_sendmsg_charp(fm->e, "File name not given");
        return;
    }
    FILE *f = fopen(fm->name_mbs, "r");
    if (!f || !text_read(&fm->mgr, f)) {
        editor_sendmsg_charp(fm->e, "Failed to read file");
        fm->is_sync = false;
    } else {
        fm->is_sync = true;
        fm->sync_time = get_file_updtime(fm->path_mbs);
    }
    fm->ver = fm->mgr.undo_cur;
}

void filemgr_write(filemgr *fm, FILE *f, char *name_mbs) {
    if (text_write(&fm->mgr, f)) {
        fm->is_sync = true;
        fm->sync_time = get_file_updtime(name_mbs);
    } else {
        editor_sendmsg_charp(fm->e, "Failed to write file");
        fm->is_sync = false;
    }
}

void filemgr_setname(filemgr *fm, rawstr name, char *name_mbs, char *path_mbs) {
    fm->name = name;
    do_if_exist(free, fm->name_mbs);
    do_if_exist(free, fm->path_mbs);
    fm->path_mbs = path_mbs;
    fm->name_mbs = name_mbs;
}

void filemgr_free(filemgr *fm) {
    do_if_exist(free, fm->name.v);
    do_if_exist(free, fm->name_mbs);
    do_if_exist(free, fm->path_mbs);
    text_free(&fm->mgr);
}

bool filemgr_is_sync(filemgr *fm) {
    return fm->path_mbs && fm->is_sync && get_file_updtime(fm->name_mbs) <= fm->sync_time;
}

void buffer_openfile_byfm(buffer *buf, filemgr *fm) {
    buf->fm = fm;
    buf->mgr = &fm->mgr;
    buf->x = buf->y = buf->ideal_x = 0;
    buf->sel.x = buf->sel.y = 0;
    drawer_set_mgr(&buf->dr, buf->mgr);
}

void buffer_openfile(buffer *buf, rawstr name, bool force) {
    if (!name.len && !buf->fm->path_mbs) {
        editor_sendmsg_charp(buf->e, "No file name");
        return;
    }
    char *name_mbs = NULL, *path_mbs = NULL;
    if (name.len) {
        name_mbs = (char *)rawmbs_from_c32(name.v, name.len).v;
        path_mbs = get_abspath(name_mbs);
    }
    if (!name.len || path_mbs && buf->fm->path_mbs && !strcmp(buf->fm->path_mbs, path_mbs)) {
        if (force)
            filemgr_reopen(buf->fm), buf->x = buf->y = buf->ideal_x = 0;
        else
            editor_sendmsg_charp(buf->e, "Use :e! to reopen file");
    } else if (path_mbs) {
        filemgr *fm = editor_find_fm(buf->e, path_mbs);
        if (fm) {
            buffer_openfile_byfm(buf, fm);
        } else {
            FILE *f = fopen(name_mbs, "r");
            if (!f) {
                editor_sendmsg_charp(buf->e, "Failed to read file");
                name_mbs = path_mbs = NULL, name.v = NULL;
            } else {
                buf->fm = editor_add_file(buf->e);
                filemgr_open(buf->fm, name, name_mbs, path_mbs, f);
                buffer_openfile_byfm(buf, buf->fm);
                name_mbs = path_mbs = NULL, name.v = NULL;
            }
        }
    } else {
        buf->fm = editor_add_file(buf->e);
        buf->fm->name = name, buf->fm->name_mbs = name_mbs, buf->fm->path_mbs = NULL;
        name.v = NULL, name_mbs = NULL;
    }
    do_if_exist(free, name.v);
    do_if_exist(free, name_mbs);
    do_if_exist(free, path_mbs);
}

// 获取name的所有权
void buffer_writefile(buffer *buf, rawstr name, bool force) {
    char *name_mbs = 0, *path_mbs = 0;
    if (name.len) {
        name_mbs = rawmbs_from_c32(name.v, name.len).v;
        path_mbs = get_abspath(name_mbs);
    }
    do_if_exist(free, buf->fm->path_mbs);
    buf->fm->path_mbs = get_abspath(buf->fm->name_mbs);
    if (!name.len || path_mbs && buf->fm->path_mbs && !strcmp(path_mbs, buf->fm->path_mbs)) {
        if (!buf->fm->name.len) {
            editor_sendmsg_charp(buf->e, "No file name");
        } else {
            if (force || !path_mbs || filemgr_is_sync(buf->fm)) {
                FILE *f = fopen(buf->fm->name_mbs, "w");
                if (!f)
                    editor_sendmsg_charp(buf->e, "Failed to open file");
                else
                    filemgr_write(buf->fm, f, buf->fm->name_mbs);
            } else {
                editor_sendmsg_charp(buf->e, "File written since last change, use :w! to force write");
            }
        }
    } else {
        if (!path_mbs || force) {
            filemgr *fm;
            if (path_mbs && (fm = editor_find_fm(buf->e, path_mbs))) { // 此时force必定成立
                text_free(&fm->mgr);
                fm->mgr = buf->fm->mgr;
                do_if_exist(free, buf->fm->name.v);
                do_if_exist(free, buf->fm->name_mbs);
                do_if_exist(free, buf->fm->path_mbs);
                for (size_t i = 0; i < buf->e->files.len; i++)
                    if (buf->e->files.v[i] == buf->fm)
                        seq_remove(buf->e->files, i);
                for (size_t i = 0; i < buf->e->bufs.len; i++)
                    if (buf->e->bufs.v[i]->fm == fm || buf->e->bufs.v[i]->fm == buf->fm)
                        buffer_openfile_byfm(buf->e->bufs.v[i], fm);
                buf->fm = fm;
                FILE *f = fopen(buf->fm->name_mbs, "w");
                if (!f)
                    editor_sendmsg_charp(buf->e, "Failed to open file");
                else
                    filemgr_write(buf->fm, f, buf->fm->name_mbs);
            } else {
                FILE *f = fopen(name_mbs, "w");
                if (!f) {
                    editor_sendmsg_charp(buf->e, "Failed to open file");
                } else {
                    filemgr_write(buf->fm, f, name_mbs);
                    path_mbs = get_abspath(name_mbs);
                    filemgr_setname(buf->fm, name, name_mbs, path_mbs);
                    name.v = NULL, name_mbs = path_mbs = NULL;
                }
            }
        } else {
            editor_sendmsg_charp(buf->e, "File exists, use :w! to force write");
        }
    }
    do_if_exist(free, name.v);
    do_if_exist(free, name_mbs);
    do_if_exist(free, path_mbs);
}
