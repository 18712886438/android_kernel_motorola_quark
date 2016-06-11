/*
 * Copyright (c) 2011, 2015 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#ifndef _HTT_INTERNAL__H_
#define _HTT_INTERNAL__H_

#include <athdefs.h>     /* A_STATUS */
#include <adf_nbuf.h>    /* adf_nbuf_t */
#include <adf_os_util.h> /* adf_os_assert */
#include <htc_api.h>     /* HTC_PACKET */

#include <htt_types.h>

#ifndef offsetof
#define offsetof(type, field)   ((size_t)(&((type *)0)->field))
#endif

#undef MS
#define MS(_v, _f) (((_v) & _f##_MASK) >> _f##_LSB)
#undef SM
#define SM(_v, _f) (((_v) << _f##_LSB) & _f##_MASK)
#undef WO
#define WO(_f)      ((_f##_OFFSET) >> 2)

#define GET_FIELD(_addr, _f) MS(*((A_UINT32 *)(_addr) + WO(_f)), _f)

#include <rx_desc.h>
#include <wal_rx_desc.h> /* struct rx_attention, etc */

struct htt_host_fw_desc_base {
    union {
        struct fw_rx_desc_base val;
        A_UINT32 dummy_pad; /* make sure it is DOWRD aligned */
    } u;
};

/*
 * This struct defines the basic descriptor information used by host,
 * which is written either by the 11ac HW MAC into the host Rx data
 * buffer ring directly or generated by FW and copied from Rx indication
 */
#define RX_HTT_HDR_STATUS_LEN 64
struct htt_host_rx_desc_base {
    struct htt_host_fw_desc_base fw_desc;
    struct rx_attention  attention;
    struct rx_frag_info  frag_info;
    struct rx_mpdu_start mpdu_start;
    struct rx_msdu_start msdu_start;
    struct rx_msdu_end   msdu_end;
    struct rx_mpdu_end   mpdu_end;
    struct rx_ppdu_start ppdu_start;
    struct rx_ppdu_end   ppdu_end;
    char rx_hdr_status[RX_HTT_HDR_STATUS_LEN];
};

#define RX_STD_DESC_ATTN_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, attention))
#define RX_STD_DESC_FRAG_INFO_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, frag_info))
#define RX_STD_DESC_MPDU_START_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, mpdu_start))
#define RX_STD_DESC_MSDU_START_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, msdu_start))
#define RX_STD_DESC_MSDU_END_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, msdu_end))
#define RX_STD_DESC_MPDU_END_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, mpdu_end))
#define RX_STD_DESC_PPDU_START_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, ppdu_start))
#define RX_STD_DESC_PPDU_END_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, ppdu_end))
#define RX_STD_DESC_HDR_STATUS_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, rx_hdr_status))

#define RX_STD_DESC_FW_MSDU_OFFSET \
    (offsetof(struct htt_host_rx_desc_base, fw_desc))

#define RX_STD_DESC_SIZE (sizeof(struct htt_host_rx_desc_base))


#define RX_STD_DESC_ATTN_OFFSET_DWORD       (RX_STD_DESC_ATTN_OFFSET >> 2)
#define RX_STD_DESC_FRAG_INFO_OFFSET_DWORD  (RX_STD_DESC_FRAG_INFO_OFFSET >> 2)
#define RX_STD_DESC_MPDU_START_OFFSET_DWORD (RX_STD_DESC_MPDU_START_OFFSET >> 2)
#define RX_STD_DESC_MSDU_START_OFFSET_DWORD (RX_STD_DESC_MSDU_START_OFFSET >> 2)
#define RX_STD_DESC_MSDU_END_OFFSET_DWORD   (RX_STD_DESC_MSDU_END_OFFSET >> 2)
#define RX_STD_DESC_MPDU_END_OFFSET_DWORD   (RX_STD_DESC_MPDU_END_OFFSET >> 2)
#define RX_STD_DESC_PPDU_START_OFFSET_DWORD (RX_STD_DESC_PPDU_START_OFFSET >> 2)
#define RX_STD_DESC_PPDU_END_OFFSET_DWORD   (RX_STD_DESC_PPDU_END_OFFSET >> 2)
#define RX_STD_DESC_HDR_STATUS_OFFSET_DWORD (RX_STD_DESC_HDR_STATUS_OFFSET >> 2)

#define RX_STD_DESC_SIZE_DWORD              (RX_STD_DESC_SIZE >> 2)

/*
 * Make sure there is a minimum headroom provided in the rx netbufs
 * for use by the OS shim and OS and rx data consumers.
 */
#define HTT_RX_BUF_OS_MIN_HEADROOM 32
#define HTT_RX_STD_DESC_RESERVATION  \
    ((HTT_RX_BUF_OS_MIN_HEADROOM > RX_STD_DESC_SIZE) ? \
    HTT_RX_BUF_OS_MIN_HEADROOM : RX_STD_DESC_SIZE)
#define HTT_RX_STD_DESC_RESERVATION_DWORD \
    (HTT_RX_STD_DESC_RESERVATION >> 2)

#define HTT_RX_DESC_ALIGN_MASK 7 /* 8-byte alignment */
static inline
struct htt_host_rx_desc_base *
htt_rx_desc(adf_nbuf_t msdu)
{
    return
        (struct htt_host_rx_desc_base *)
            (((size_t) (adf_nbuf_head(msdu) + HTT_RX_DESC_ALIGN_MASK)) &
            ~HTT_RX_DESC_ALIGN_MASK);
}

static inline
void
htt_print_rx_desc(struct htt_host_rx_desc_base *rx_desc)
{
    adf_os_print("----------------------RX DESC----------------------------\n");
    adf_os_print("attention: %#010x\n",
                 (unsigned int)(*(u_int32_t *) &rx_desc->attention));
    adf_os_print("frag_info: %#010x\n",
                 (unsigned int)(*(u_int32_t *) &rx_desc->frag_info));
    adf_os_print("mpdu_start: %#010x %#010x %#010x\n",
                  (unsigned int)(((u_int32_t *) &rx_desc->mpdu_start)[0]),
                  (unsigned int)(((u_int32_t *) &rx_desc->mpdu_start)[1]),
                  (unsigned int)(((u_int32_t *) &rx_desc->mpdu_start)[2]));
    adf_os_print("msdu_start: %#010x %#010x %#010x\n",
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_start)[0]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_start)[1]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_start)[2]));
    adf_os_print("msdu_end: %#010x %#010x %#010x %#010x %#010x\n",
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_end)[0]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_end)[1]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_end)[2]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_end)[3]),
                  (unsigned int)(((u_int32_t *) &rx_desc->msdu_end)[4]));
    adf_os_print("mpdu_end: %#010x\n",
                 (unsigned int)(*(u_int32_t *) &rx_desc->mpdu_end));
    adf_os_print("ppdu_start: "
                 "%#010x %#010x %#010x %#010x %#010x\n"
                 "%#010x %#010x %#010x %#010x %#010x\n",
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[0]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[1]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[2]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[3]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[4]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[5]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[6]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[7]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[8]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_start)[9]));
    adf_os_print("ppdu_end:"
                 "%#010x %#010x %#010x %#010x %#010x\n"
                 "%#010x %#010x %#010x %#010x %#010x\n"
                 "%#010x,%#010x %#010x %#010x %#010x\n"
                 "%#010x %#010x %#010x %#010x %#010x\n"
                 "%#010x %#010x\n",
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[0]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[1]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[2]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[3]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[4]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[5]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[6]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[7]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[8]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[9]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[10]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[11]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[12]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[13]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[14]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[15]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[16]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[17]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[18]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[19]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[20]),
                  (unsigned int)(((u_int32_t *) &rx_desc->ppdu_end)[21]));
    adf_os_print("---------------------------------------------------------\n");
}


#ifndef HTT_ASSERT_LEVEL
#define HTT_ASSERT_LEVEL 3
#endif

#define HTT_ASSERT_ALWAYS(condition) adf_os_assert_always((condition))

#define HTT_ASSERT0(condition) adf_os_assert((condition))
#if HTT_ASSERT_LEVEL > 0
#define HTT_ASSERT1(condition) adf_os_assert((condition))
#else
#define HTT_ASSERT1(condition)
#endif

#if HTT_ASSERT_LEVEL > 1
#define HTT_ASSERT2(condition) adf_os_assert((condition))
#else
#define HTT_ASSERT2(condition)
#endif

#if HTT_ASSERT_LEVEL > 2
#define HTT_ASSERT3(condition) adf_os_assert((condition))
#else
#define HTT_ASSERT3(condition)
#endif


#define HTT_MAC_ADDR_LEN 6


/*
 * HTT_MAX_SEND_QUEUE_DEPTH -
 * How many packets HTC should allow to accumulate in a send queue
 * before calling the EpSendFull callback to see whether to retain
 * or drop packets.
 * This is not relevant for LL, where tx descriptors should be immediately
 * downloaded to the target.
 * This is not very relevant for HL either, since it is anticipated that
 * the HL tx download scheduler will not work this far in advance - rather,
 * it will make its decisions just-in-time, so it can be responsive to
 * changing conditions.
 * Hence, this queue depth threshold spec is mostly just a formality.
 */
#define HTT_MAX_SEND_QUEUE_DEPTH 64


#define IS_PWR2(value) (((value) ^ ((value)-1)) == ((value) << 1) - 1)


/* FIX THIS
 * Should be: sizeof(struct htt_host_rx_desc) + max rx MSDU size,
 * rounded up to a cache line size.
 */
#define HTT_RX_BUF_SIZE 1920
/*
 * DMA_MAP expects the buffer to be an integral number of cache lines.
 * Rather than checking the actual cache line size, this code makes a
 * conservative estimate of what the cache line size could be.
 */
#define HTT_LOG2_MAX_CACHE_LINE_SIZE 7 /* 2^7 = 128 */
#define HTT_MAX_CACHE_LINE_SIZE_MASK ((1 << HTT_LOG2_MAX_CACHE_LINE_SIZE) - 1)

#ifdef BIG_ENDIAN_HOST
/*
 * big-endian: bytes within a 4-byte "word" are swapped:
 * pre-swap  post-swap
 *  index     index
 *    0         3
 *    1         2
 *    2         1
 *    3         0
 *    4         7
 *    5         6
 * etc.
 * To compute the post-swap index from the pre-swap index, compute
 * the byte offset for the start of the word (index & ~0x3) and add
 * the swapped byte offset within the word (3 - (index & 0x3)).
 */
#define HTT_ENDIAN_BYTE_IDX_SWAP(idx) (((idx) & ~0x3) + (3 - ((idx) & 0x3)))
#else
/* little-endian: no adjustment needed */
#define HTT_ENDIAN_BYTE_IDX_SWAP(idx) idx
#endif


#define HTT_TX_MUTEX_INIT(_mutex)                       \
    adf_os_spinlock_init(_mutex)

#define HTT_TX_MUTEX_ACQUIRE(_mutex)                    \
    adf_os_spin_lock_bh(_mutex)

#define HTT_TX_MUTEX_RELEASE(_mutex)                    \
    adf_os_spin_unlock_bh(_mutex)

#define HTT_TX_MUTEX_DESTROY(_mutex)                    \
    adf_os_spinlock_destroy(_mutex)

#define HTT_TX_DESC_PADDR(_pdev, _tx_desc_vaddr)       \
    ((_pdev)->tx_descs.pool_paddr +  (u_int32_t)       \
     ((char *)(_tx_desc_vaddr) -                       \
      (char *)((_pdev)->tx_descs.pool_vaddr)))

#ifdef ATH_11AC_TXCOMPACT

#define HTT_TX_NBUF_QUEUE_MUTEX_INIT(_pdev)             \
        adf_os_spinlock_init(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_MUTEX_DESTROY(_pdev)         \
        HTT_TX_MUTEX_DESTROY(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_REMOVE(_pdev, _msdu)          \
        HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);    \
        _msdu =  adf_nbuf_queue_remove(&_pdev->txnbufq);\
        HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_ADD(_pdev, _msdu)          \
        HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);    \
        adf_nbuf_queue_add(&_pdev->txnbufq, _msdu);\
        HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex)

#define HTT_TX_NBUF_QUEUE_INSERT_HEAD(_pdev, _msdu)        \
        HTT_TX_MUTEX_ACQUIRE(&_pdev->txnbufq_mutex);       \
        adf_nbuf_queue_insert_head(&_pdev->txnbufq, _msdu);\
        HTT_TX_MUTEX_RELEASE(&_pdev->txnbufq_mutex)
#else

#define HTT_TX_NBUF_QUEUE_MUTEX_INIT(_pdev)
#define HTT_TX_NBUF_QUEUE_REMOVE(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_ADD(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_INSERT_HEAD(_pdev, _msdu)
#define HTT_TX_NBUF_QUEUE_MUTEX_DESTROY(_pdev)

#endif

#ifdef ATH_11AC_TXCOMPACT
#define HTT_TX_SCHED htt_tx_sched
#else
#define HTT_TX_SCHED(pdev) /* no-op */
#endif

int
htt_tx_attach(struct htt_pdev_t *pdev, int desc_pool_elems);

void
htt_tx_detach(struct htt_pdev_t *pdev);

int
htt_rx_attach(struct htt_pdev_t *pdev);

void
htt_rx_detach(struct htt_pdev_t *pdev);

int
htt_htc_attach(struct htt_pdev_t *pdev);

void
htt_t2h_msg_handler(void *context, HTC_PACKET *pkt);

void
htt_tx_resume_handler(void *);

void
htt_h2t_send_complete(void *context, HTC_PACKET *pkt);

A_STATUS
htt_h2t_ver_req_msg(struct htt_pdev_t *pdev);

extern A_STATUS (*htt_h2t_rx_ring_cfg_msg)(
        struct htt_pdev_t *pdev);

HTC_SEND_FULL_ACTION
htt_h2t_full(void *context, HTC_PACKET *pkt);

struct htt_htc_pkt *
htt_htc_pkt_alloc(struct htt_pdev_t *pdev);

void
htt_htc_pkt_free(struct htt_pdev_t *pdev, struct htt_htc_pkt *pkt);

void
htt_htc_pkt_pool_free(struct htt_pdev_t *pdev);

#ifdef ATH_11AC_TXCOMPACT
void
htt_htc_misc_pkt_list_add(struct htt_pdev_t *pdev, struct htt_htc_pkt *pkt);

void
htt_htc_misc_pkt_pool_free(struct htt_pdev_t *pdev);
#endif

void htt_htc_disable_aspm(void);

int
htt_rx_hash_list_insert(struct htt_pdev_t *pdev, u_int32_t paddr,
     adf_nbuf_t netbuf);

adf_nbuf_t
htt_rx_hash_list_lookup(struct htt_pdev_t *pdev, u_int32_t paddr);

#ifdef IPA_UC_OFFLOAD
int
htt_tx_ipa_uc_attach(struct htt_pdev_t *pdev,
    unsigned int uc_tx_buf_sz,
    unsigned int uc_tx_buf_cnt,
    unsigned int uc_tx_partition_base);

int
htt_rx_ipa_uc_attach(struct htt_pdev_t *pdev,
       unsigned int rx_ind_ring_size);

int
htt_tx_ipa_uc_detach(struct htt_pdev_t *pdev);

int
htt_rx_ipa_uc_detach(struct htt_pdev_t *pdev);
#endif /* IPA_UC_OFFLOAD */

#endif /* _HTT_INTERNAL__H_ */
