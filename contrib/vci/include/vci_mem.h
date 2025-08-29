/*-------------------------------------------------------------------------
 *
 * vci_mem.h
 *	  Definitions of on-memmory structures
 *
 * Portions Copyright (c) 2025, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		contrib/vci/include/vci_mem.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef VCI_MEM_H
#define VCI_MEM_H

#include "postgres.h"

#include <float.h>

#include "lib/ilist.h"
#include "portability/instr_time.h"
#include "storage/lwlock.h"
#include "utils/palloc.h"

#include "vci.h"
#include "vci_ros.h"
#include "vci_memory_entry.h"

/*-------------------------------------------------------------------------
 * START: Copied from include/vci_port.h
 *
 * XXX - This is a WIP cleanup to remove some unnecessary include files.
 * Some of this is known to be redundant, and will be removed later.
 *-------------------------------------------------------------------------
 */

#ifndef VCI_PORT_H
#define VCI_PORT_H

/*
 * key for vci_devload_t
 */
#define VCI_PSEUDO_UNMONITORED_DEVICE ""

#define VCI_FREE_DEVLOAD_INFO_NAME "~"
#define VCI_FREE_DEVLOAD_AWAIT (-1.0)
#define VCI_UNMONITORED_DEV_AWAIT DBL_MAX

#ifndef WIN32
#define VCI_PATH_MAX    PATH_MAX
#else
#define VCI_PATH_MAX    MAX_PATH
#endif

/*
 * Memory entry on the each device
 *
 * * head is the actual list, link is used to track unused entries
 */
typedef struct
{
	dlist_head	head;
	dlist_node	link;
} vci_memory_entry_list_t;

/*
 * IO statistics, mount information, etc for each devices
 */
typedef struct
{
	/**
	 * device name for example, sdb, sdb. Not the form like /dev/sda.
	 * devname[] = "" means that this vci_devload_t type value is used for
	 * non-monitored devices. devname "~" means this is free. Because
	 * any device name starts with an alphabet, "~" is greater than any
	 * device name. Then when we sort vci_devload_t list, free space
	 * is accumulated.
	 */
	char		devname[VCI_PATH_MAX];

	bool		alive;

	/**
	 * The average time(in milliseconds) for I/O requests issued to
	 * this device in the last interval.
	 * DBL_MAX for non monitored device(this is allocated last).
	 * for free object, place on free space
	 */
	double		await;

	/**
	 * await of the interval before last.
	 */
	double		prev_await;

	/**
	 * average of the await of the last VCI_DISKSTAT_RECORD_NUM intervals.
	 * A negative number means that the number of monitoring has not reached
	 * the minimum to calculate IO loads, VCI_DISKSTAT_RECORD_NUM.
	 */
	double		await_avg;

	/**
	 * coefficient of variation of await.
	 */
	double		await_cv;

	/*
	 * How many entries in devstat?
	 *
	 * - If this reaches VCI_DISKSTAT_RECORD_NUM, won't be changed.
	 */
	unsigned int devstat_num;

	bool		contain_vci;

	vci_memory_entry_list_t *memory_entry_queue;

	/*
	 * Next position when memory entry would be traced. NULL means there are
	 * no entries to be seen.
	 */
	dlist_node *memory_entry_pos;
} vci_devload_t;

#endif							/* VCI_PORT_H */

/*-------------------------------------------------------------------------
 * END: Copied from include/vci_port.h
 *-------------------------------------------------------------------------
 */

typedef struct VciGucStruct
{
	bool		have_loaded_postgresql_conf;

	bool		enable;

	bool		log_query;

	int			cost_threshold;

	int			table_scan_policy;

	/* GUC parameters read from postgresq.conf */
	int			maintenance_work_mem;
	int			max_devices;	/* max device num for storage */

	/* ROS control worker/daemon */
	int			control_max_workers;
	int			control_naptime;

	/* command thresholds */
	int			wosros_conv_threshold;
	int			cdr_threshold;

	/* for custom plan execution */
	int			max_local_ros_size;

	/* for parallel processing */
	int			table_rows_threshold;

	bool		enable_seqscan;
	bool		enable_indexscan;
	bool		enable_bitmapheapscan;
	bool		enable_sort;
	bool		enable_hashagg;
	bool		enable_sortagg;
	bool		enable_plainagg;
	bool		enable_hashjoin;
	bool		enable_nestloop;

	/* GUC parameters for internal use */
	bool		enable_ros_control_daemon;

} VciGucStruct;

extern PGDLLEXPORT VciGucStruct VciGuc;

/*
 * Data structure on shared memory
 *
 * The instance would be allocated on the shared memory and can be accessed via
 * VciShmemAddr.
 */
typedef struct VciShmemStruct
{
	/* --- ROS Control Daemon --- */

	/* Attributes for passing attributes to a worker */

	vci_wosros_conv_worker_arg_t *worker_args_array;

	/** vci_memory_entries_t is defined in vci_ros.h
	 * That keeps information of VCI indices kept in memory.
	 * The life is the same with PostgreSQL instance.
	 */
	vci_memory_entries_t *memory_entries;

	dlist_head	memory_entry_device_unknown_list;

	/* Standby server controller */
	LWLock	   *standby_exec_loc;
	int			num_standby_exec_queries;

	/* IO statistics */

	vci_devload_t *devload_array;

	vci_memory_entry_list_t *memory_entry_queue_array;

	dlist_head	free_memory_entry_queue_list;	/**list of memory_entry_queue_array */
	int			num_devload_info;	/* monitored device numbers + 1(for
									 * unmonitored devices) */
	int			max_devices;	/* max device num for storage */
	int			translated_dev_pos; /* index of a device VCIs on which is to
									 * be translated */
	LWLock	   *io_load_lock;

	/* Additional Lwlocks used by various modules */
	LWLock	   *vci_memory_entries_lock;
	LWLock	   *vci_query_context_lock;
	LWLock	   *vci_mnt_point2dev_lock;
} VciShmemStruct;

extern PGDLLEXPORT VciShmemStruct *VciShmemAddr;

#endif							/* VCI_MEM_H */
