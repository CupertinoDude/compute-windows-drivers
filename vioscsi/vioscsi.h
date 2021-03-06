/**********************************************************************
 * Copyright (c) 2012-2015  Red Hat, Inc.
 *
 * File: vioscsi.h
 *
 * Main include file
 * This file contains vrious routines and globals
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
**********************************************************************/

#ifndef ___VIOSCSI_H__
#define ___VIOSCSI_H__

#include <ntddk.h>
#include <ntddscsi.h>
#include <storport.h>
#include <scsiwmi.h>

#include "osdep.h"
#include "srbwrapper.h"
#include "virtio_pci.h"
#include "VirtIO.h"

typedef struct VirtIOBufferDescriptor VIO_SG, *PVIO_SG;

#define VIRTIO_SCSI_CDB_SIZE   32
#define VIRTIO_SCSI_SENSE_SIZE 96

#if (INDIRECT_SUPPORTED == 1)
#define MAX_PHYS_SEGMENTS       64
#else
#define MAX_PHYS_SEGMENTS       16
#endif

#define VIOSCSI_POOL_TAG        'SoiV'
#define VIRTIO_MAX_SG            (3+MAX_PHYS_SEGMENTS)

#define SECTOR_SIZE             512
#define IO_PORT_LENGTH          0x40
#define MAX_CPU                 256

/* See google3/cloud/vmm/guest/virtio_scsi/virtio_scsi_abi.h for abi
documentation. */

/* Feature Bits */
#define VIRTIO_SCSI_F_INOUT                        0
#define VIRTIO_SCSI_F_HOTPLUG                      1
#define VIRTIO_SCSI_F_CHANGE                       2
#define VIRTIO_SCSI_F_GOOGLE_SNAPSHOT              22
#define VIRTIO_SCSI_F_GOOGLE_REPORT_DRIVER_VERSION 23

/* Response codes */
#define VIRTIO_SCSI_S_OK                       0
#define VIRTIO_SCSI_S_UNDERRUN                 1
#define VIRTIO_SCSI_S_ABORTED                  2
#define VIRTIO_SCSI_S_BAD_TARGET               3
#define VIRTIO_SCSI_S_RESET                    4
#define VIRTIO_SCSI_S_BUSY                     5
#define VIRTIO_SCSI_S_TRANSPORT_FAILURE        6
#define VIRTIO_SCSI_S_TARGET_FAILURE           7
#define VIRTIO_SCSI_S_NEXUS_FAILURE            8
#define VIRTIO_SCSI_S_FAILURE                  9
#define VIRTIO_SCSI_S_FUNCTION_SUCCEEDED       10
#define VIRTIO_SCSI_S_FUNCTION_REJECTED        11
#define VIRTIO_SCSI_S_INCORRECT_LUN            12

/* Controlq type codes.  */
#define VIRTIO_SCSI_T_TMF                      0
#define VIRTIO_SCSI_T_AN_QUERY                 1
#define VIRTIO_SCSI_T_AN_SUBSCRIBE             2
#define VIRTIO_SCSI_T_GOOGLE                   0x80000000

/* Valid TMF subtypes.  */
#define VIRTIO_SCSI_T_TMF_ABORT_TASK           0
#define VIRTIO_SCSI_T_TMF_ABORT_TASK_SET       1
#define VIRTIO_SCSI_T_TMF_CLEAR_ACA            2
#define VIRTIO_SCSI_T_TMF_CLEAR_TASK_SET       3
#define VIRTIO_SCSI_T_TMF_I_T_NEXUS_RESET      4
#define VIRTIO_SCSI_T_TMF_LOGICAL_UNIT_RESET   5
#define VIRTIO_SCSI_T_TMF_QUERY_TASK           6
#define VIRTIO_SCSI_T_TMF_QUERY_TASK_SET       7

/* Valid Google control queue message subtypes. */
#define VIRTIO_SCSI_T_GOOGLE_REPORT_DRIVER_VERSION 0
#define VIRTIO_SCSI_T_GOOGLE_REPORT_SNAPSHOT_READY 1

/* Events.  */
#define VIRTIO_SCSI_T_EVENTS_MISSED            0x80000000
#define VIRTIO_SCSI_T_NO_EVENT                 0
#define VIRTIO_SCSI_T_TRANSPORT_RESET          1
#define VIRTIO_SCSI_T_ASYNC_NOTIFY             2
#define VIRTIO_SCSI_T_PARAM_CHANGE             3
// Google VSS SnapshotRequest events:
#define VIRTIO_SCSI_T_SNAPSHOT_START           100
#define VIRTIO_SCSI_T_SNAPSHOT_COMPLETE        101

/* Reasons of transport reset event */
#define VIRTIO_SCSI_EVT_RESET_HARD             0
#define VIRTIO_SCSI_EVT_RESET_RESCAN           1
#define VIRTIO_SCSI_EVT_RESET_REMOVED          2

#define VIRTIO_SCSI_S_SIMPLE                   0
#define VIRTIO_SCSI_S_ORDERED                  1
#define VIRTIO_SCSI_S_HEAD                     2
#define VIRTIO_SCSI_S_ACA                      3

#define VIRTIO_RING_F_INDIRECT_DESC            28

#define VIRTIO_SCSI_CONTROL_QUEUE              0
#define VIRTIO_SCSI_EVENTS_QUEUE               1
#define VIRTIO_SCSI_REQUEST_QUEUE_0            2
#define VIRTIO_SCSI_QUEUE_LAST                 VIRTIO_SCSI_REQUEST_QUEUE_0 + MAX_CPU

/* SCSI command request, followed by data-out */
#pragma pack(1)
typedef struct {
    u8 lun[8];        /* Logical Unit Number */
    u64 tag;          /* Command identifier */
    u8 task_attr;     /* Task attribute */
    u8 prio;
    u8 crn;
    u8 cdb[VIRTIO_SCSI_CDB_SIZE];
} VirtIOSCSICmdReq, * PVirtIOSCSICmdReq;
#pragma pack()


/* Response, followed by sense data and data-in */
#pragma pack(1)
typedef struct {
    u32 sense_len;        /* Sense data length */
    u32 resid;            /* Residual bytes in data buffer */
    u16 status_qualifier; /* Status qualifier */
    u8 status;            /* Command completion status */
    u8 response;          /* Response values */
    u8 sense[VIRTIO_SCSI_SENSE_SIZE];
} VirtIOSCSICmdResp, * PVirtIOSCSICmdResp;
#pragma pack()

/* Task Management Request */
#pragma pack(1)
typedef struct {
    u32 type;
    u32 subtype;
    u8 lun[8];
    u64 tag;
} VirtIOSCSICtrlTMFReq, * PVirtIOSCSICtrlTMFReq;
#pragma pack()

#pragma pack(1)
typedef struct {
    u8 response;
} VirtIOSCSICtrlTMFResp, * PVirtIOSCSICtrlTMFResp;
#pragma pack()

/* Asynchronous notification query/subscription */
#pragma pack(1)
typedef struct {
    u32 type;
    u8 lun[8];
    u32 event_requested;
} VirtIOSCSICtrlANReq, *PVirtIOSCSICtrlANReq;
#pragma pack()

#pragma pack(1)
typedef struct {
    u32 event_actual;
    u8 response;
} VirtIOSCSICtrlANResp, * PVirtIOSCSICtrlANResp;
#pragma pack()

/* Google control message */
#pragma pack(1)
typedef struct {
    u32 type;
    u32 subtype;
    u8 lun[8];
    u64 data;
} VirtIOSCSICtrlGoogleReq, *PVirtIOSCSICtrlGoogleReq;
#pragma pack()

#pragma pack(1)
typedef struct {
    u8 response;
} VirtIOSCSICtrlGoogleResp, *PVirtIOSCSICtrlGoogleResp;
#pragma pack()

#pragma pack(1)
typedef struct {
    u32 event;
    u8 lun[8];
    u32 reason;
} VirtIOSCSIEvent, * PVirtIOSCSIEvent;
#pragma pack()

#pragma pack(1)
typedef struct {
    u32 num_queues;
    u32 seg_max;
    u32 max_sectors;
    u32 cmd_per_lun;
    u32 event_info_size;
    u32 sense_size;
    u32 cdb_size;
    u16 max_channel;
    u16 max_target;
    u32 max_lun;
} VirtIOSCSIConfig, * PVirtIOSCSIConfig;
#pragma pack()

#pragma pack(1)
typedef struct {
    PVOID sc;
    PVOID comp;
    union {
        VirtIOSCSICmdReq      cmd;
        VirtIOSCSICtrlTMFReq  tmf;
        VirtIOSCSICtrlANReq   an;
        VirtIOSCSICtrlGoogleReq google;
    } req;
    union {
        VirtIOSCSICmdResp     cmd;
        VirtIOSCSICtrlTMFResp tmf;
        VirtIOSCSICtrlANResp  an;
        VirtIOSCSICtrlGoogleResp google;
        VirtIOSCSIEvent       event;
    } resp;
} VirtIOSCSICmd, * PVirtIOSCSICmd;
#pragma pack()

#pragma pack(1)
typedef struct {
    PVOID           adapter;
    VirtIOSCSIEvent event;
    VIO_SG          sg;
} VirtIOSCSIEventNode, * PVirtIOSCSIEventNode;
#pragma pack()

typedef struct
{
    union
    {
        ULONGLONG data[2];
        UCHAR chars[SIZE_OF_SINGLE_INDIRECT_DESC];
    }u;
} vring_desc_alias;

#pragma pack(1)
typedef struct _SRB_EXTENSION {
    PSCSI_REQUEST_BLOCK   Srb;
    ULONG                 out;
    ULONG                 in;
    ULONG                 Xfer;
    VirtIOSCSICmd         cmd;
    VIO_SG                sg[128];
#if (INDIRECT_SUPPORTED == 1)
    vring_desc_alias     desc[VIRTIO_MAX_SG];
#endif
    PROCESSOR_NUMBER      procNum;
#ifdef ENABLE_WMI
    ULONGLONG             startTsc;
#endif
 } SRB_EXTENSION, * PSRB_EXTENSION;
#pragma pack()

#pragma pack(1)
typedef struct {
    SCSI_REQUEST_BLOCK    Srb;
    PSRB_EXTENSION        SrbExtension;
} TMF_COMMAND, * PTMF_COMMAND;
#pragma pack()

#ifdef ENABLE_WMI
// Note: the members in these stat structs must be in the same
// order as in the MOF file.
typedef struct {
    ULONGLONG TotalRequests;
    ULONGLONG CompletedRequests;
    ULONGLONG TotalKicks;
    ULONGLONG SkippedKicks;
    ULONGLONG TotalInterrupts;
    ULONGLONG QueueFullEvents;
    ULONGLONG MaxLatency;
    ULONGLONG BusyRequests;
    ULONGLONG LastStartIo;
    ULONGLONG MaxStartIoDelay;
} VIRTQUEUE_STATISTICS, *PVIRTQUEUE_STATISTICS;

typedef struct {
    ULONGLONG TotalRequests;
    ULONGLONG CompletedRequests;
    ULONGLONG ResetRequests;
    ULONGLONG MaxLatency;
    ULONGLONG BusyRequests;
} TARGET_STATISTICS, *PTARGET_STATISTICS;
#define MAX_TARGET 256
#endif

typedef struct _ADAPTER_EXTENSION {
    VirtIODevice          vdev;
    VirtIODevice*         pvdev;

    PVOID                 uncachedExtensionVa;
    ULONG                 allocationSize;

    struct virtqueue *    vq[VIRTIO_SCSI_QUEUE_LAST];
    ULONG                 offset;
    ULONG_PTR             device_base;
    VirtIOSCSIConfig      scsi_config;

    ULONG                 queue_depth;
    BOOLEAN               dump_mode;

    ULONG                 features;

    ULONG                 msix_vectors;
    BOOLEAN               msix_enabled;
    BOOLEAN               indirect;

    TMF_COMMAND           tmf_cmd;
    BOOLEAN               tmf_infly;

    USHORT                original_queue_num[4];  // last element used as pad.

    PVirtIOSCSIEventNode  events;

    ULONG                 num_queues;
    UCHAR                 cpu_to_vq_map[MAX_CPU];
    ULONG                 perfFlags;
    PGROUP_AFFINITY       pmsg_affinity;
    BOOLEAN               dpc_ok;
    STOR_DPC              dpc[MAX_CPU];

    // SRB sent by agent. It's IOCTL_SCSI_MINIPORT with Control code
    // SNAPSHOT_REQUESTED. Driver complete this srb when it got a snapshot
    // request from backend.
    PSRB_TYPE             srb_snapshot_requested;
    // SRB sent by provider during commit. It's IOCTL_SCSI_MINIPORT with Control
    // code SNAPSHOT_CAN_PROCEED. Driver complete this srb when it got an event
    // notification from backend to indicate that the snapshot is finished in
    // the backend, and IO can be resumed.
    PSRB_TYPE             srb_snapshot_can_proceeed;
    // Global SRB and extension for fast-failing snapshot requests. Will only be
    // used in the interrupt routine so there is no risk of a data race.
    SCSI_REQUEST_BLOCK snapshot_fail_srb;
    SRB_EXTENSION snapshot_fail_extension;
#ifdef ENABLE_WMI
    SCSI_WMILIB_CONTEXT   WmiLibContext;
    SCSI_WMILIB_CONTEXT   PdoWmiLibContext;
    VIRTQUEUE_STATISTICS  QueueStats[MAX_CPU];
    TARGET_STATISTICS     TargetStats[MAX_TARGET];
    ULONG                 MaxTarget;
#if (NTDDI_VERSION < NTDDI_WIN8)
    USHORT                PortNumber;
#endif
#endif

} ADAPTER_EXTENSION, * PADAPTER_EXTENSION;

#if (MSI_SUPPORTED == 1)
#ifndef PCIX_TABLE_POINTER
typedef struct {
  union {
    struct {
      ULONG BaseIndexRegister :3;
      ULONG Reserved          :29;
    };
    ULONG TableOffset;
  };
} PCIX_TABLE_POINTER, *PPCIX_TABLE_POINTER;
#endif

#ifndef PCI_MSIX_CAPABILITY
typedef struct {
  PCI_CAPABILITIES_HEADER Header;
  struct {
    USHORT TableSize      :11;
    USHORT Reserved       :3;
    USHORT FunctionMask   :1;
    USHORT MSIXEnable     :1;
  } MessageControl;
  PCIX_TABLE_POINTER      MessageTable;
  PCIX_TABLE_POINTER      PBATable;
} PCI_MSIX_CAPABILITY, *PPCI_MSIX_CAPABILITY;
#endif
#endif

#define SPC3_SCSI_SENSEQ_PARAMETERS_CHANGED                 0x0
#define SPC3_SCSI_SENSEQ_MODE_PARAMETERS_CHANGED            0x01
#define SPC3_SCSI_SENSEQ_CAPACITY_DATA_HAS_CHANGED          0x09

#endif // ___VIOSCSI__H__
