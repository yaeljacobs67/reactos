/*
 * Wine server USER definitions
 *
 * Copyright (C) 2001 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SERVER_USER_H
#define __WINE_SERVER_USER_H

#include "wine/server_protocol.h"

struct thread;
struct region;
struct window;
struct msg_queue;
struct hook_table;
struct window_class;
struct atom_table;
struct clipboard;

enum user_object
{
    USER_WINDOW = 1,
    USER_HOOK,
    USER_CLIENT  /* arbitrary client handle */
};

#define DESKTOP_ATOM  ((atom_t)32769)

struct winstation
{
    struct object      obj;                /* object header */
    unsigned int       flags;              /* winstation flags */
    struct list        entry;              /* entry in global winstation list */
    struct list        desktops;           /* list of desktops of this winstation */
    struct clipboard  *clipboard;          /* clipboard information */
    struct atom_table *atom_table;         /* global atom table */
};

struct desktop
{
    struct object        obj;            /* object header */
    unsigned int         flags;          /* desktop flags */
    struct winstation   *winstation;     /* winstation this desktop belongs to */
    struct list          entry;          /* entry in winstation list of desktops */
    struct window       *top_window;     /* desktop window for this desktop */
    struct window       *msg_window;     /* HWND_MESSAGE top window */
    struct hook_table   *global_hooks;   /* table of global hooks on this desktop */
    struct timeout_user *close_timeout;  /* timeout before closing the desktop */
    unsigned int         users;          /* processes and threads using this desktop */
    struct list          shell_hooks;    /* list of registered shell hooks */
};

struct region
{
    int size;
    int num_rects;
    rectangle_t *rects;
    rectangle_t extents;
};

/* user handles functions */

extern user_handle_t alloc_user_handle( void *ptr, enum user_object type );
extern void *get_user_object( user_handle_t handle, enum user_object type );
extern void *get_user_object_handle( user_handle_t *handle, enum user_object type );
extern user_handle_t get_user_full_handle( user_handle_t handle );
extern void *free_user_handle( user_handle_t handle );
extern void *next_user_handle( user_handle_t *handle, enum user_object type );
extern void free_process_user_handles( PPROCESSINFO process );

/* clipboard functions */

extern void cleanup_clipboard_thread( PTHREADINFO thread );

/* hook functions */

extern void remove_thread_hooks( PTHREADINFO thread );
extern unsigned int get_active_hooks(void);

/* queue functions */

extern void free_msg_queue( PTHREADINFO thread );
extern struct hook_table *get_queue_hooks( PTHREADINFO thread );
extern void set_queue_hooks( PTHREADINFO thread, struct hook_table *hooks );
extern void inc_queue_paint_count( PTHREADINFO thread, int incr );
extern void queue_cleanup_window( PTHREADINFO thread, user_handle_t win );
extern int init_thread_queue( PTHREADINFO thread );
extern int attach_thread_input( PTHREADINFO thread_from, PTHREADINFO thread_to );
extern void detach_thread_input( PTHREADINFO thread_from );
extern void post_message( user_handle_t win, unsigned int message,
                          lparam_t wparam, lparam_t lparam );
extern void post_win_event( PTHREADINFO thread, unsigned int event,
                            user_handle_t win, unsigned int object_id,
                            unsigned int child_id, client_ptr_t proc,
                            const WCHAR *module, data_size_t module_size,
                            user_handle_t handle );

/* region functions */

extern struct region *create_empty_region(void);
extern struct region *create_region_from_req_data( const void *data, data_size_t size );
extern struct region *create_region_from_rects( const void *data, unsigned long count );
extern void free_region( struct region *region );
extern void set_region_rect( struct region *region, const rectangle_t *rect );
extern rectangle_t *get_region_data( const struct region *region, data_size_t max_size,
                                     data_size_t *total_size );
extern rectangle_t *get_region_data_and_free( struct region *region, data_size_t max_size,
                                              data_size_t *total_size );
extern int is_region_empty( const struct region *region );
extern void get_region_extents( const struct region *region, rectangle_t *rect );
extern void offset_region( struct region *region, int x, int y );
extern void mirror_region( const rectangle_t *client_rect, struct region *region );
extern struct region *copy_region( struct region *dst, const struct region *src );
extern struct region *intersect_region( struct region *dst, const struct region *src1,
                                        const struct region *src2 );
extern struct region *subtract_region( struct region *dst, const struct region *src1,
                                       const struct region *src2 );
extern struct region *union_region( struct region *dst, const struct region *src1,
                                    const struct region *src2 );
extern struct region *xor_region( struct region *dst, const struct region *src1,
                                  const struct region *src2 );
extern int point_in_region( struct region *region, int x, int y );
extern int rect_in_region( struct region *region, const rectangle_t *rect );

/* window functions */

extern PPROCESSINFO get_top_window_owner( struct desktop *desktop );
extern void close_desktop_window( struct desktop *desktop );
extern void destroy_window( struct window *win );
extern void destroy_thread_windows( PTHREADINFO thread );
extern int is_child_window( user_handle_t parent, user_handle_t child );
extern int is_top_level_window( user_handle_t window );
extern int is_window_visible( user_handle_t window );
extern int is_window_transparent( user_handle_t window );
extern int make_window_active( user_handle_t window );
extern PTHREADINFO get_window_thread( user_handle_t handle );
extern user_handle_t window_from_point( struct desktop *desktop, int x, int y );
extern user_handle_t find_window_to_repaint( user_handle_t parent, PTHREADINFO thread );
extern struct window_class *get_window_class( user_handle_t window );

/* window class functions */

extern void destroy_process_classes( PPROCESSINFO process );
extern struct window_class *grab_class( PPROCESSINFO process, atom_t atom,
                                        mod_handle_t instance, int *extra_bytes );
extern void release_class( struct window_class *class );
extern int is_desktop_class( struct window_class *class );
extern int is_hwnd_message_class( struct window_class *class );
extern atom_t get_class_atom( struct window_class *class );
extern client_ptr_t get_class_client_ptr( struct window_class *class );

/* windows station functions */

extern struct desktop *get_desktop_obj( PPROCESSINFO process, obj_handle_t handle, unsigned int access );
extern struct winstation *get_process_winstation( PPROCESSINFO process, unsigned int access );
extern struct desktop *get_thread_desktop( PTHREADINFO thread, unsigned int access );
extern void connect_process_winstation( PPROCESSINFO process );
extern void set_process_default_desktop( PPROCESSINFO process, struct desktop *desktop,
                                         obj_handle_t handle );
extern void close_process_desktop( PPROCESSINFO process );
extern void close_thread_desktop( PTHREADINFO thread );

/* mirror a rectangle respective to the window client area */
static inline void mirror_rect( const rectangle_t *client_rect, rectangle_t *rect )
{
    int width = client_rect->right - client_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

#endif  /* __WINE_SERVER_USER_H */