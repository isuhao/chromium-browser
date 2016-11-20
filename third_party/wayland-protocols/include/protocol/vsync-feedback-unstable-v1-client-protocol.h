/* Generated by wayland-scanner 1.11.0 */

#ifndef VSYNC_FEEDBACK_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define VSYNC_FEEDBACK_UNSTABLE_V1_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_vsync_feedback_unstable_v1 The vsync_feedback_unstable_v1 protocol
 * Protocol for providing vertical synchronization timing
 *
 * @section page_desc_vsync_feedback_unstable_v1 Description
 *
 * This protocol specifies a set of interfaces used to inform the client about
 * vertical synchronization timing in a precise way and without unnecessary
 * overhead.
 *
 * Warning! The protocol described in this file is experimental and backward
 * incompatible changes may be made. Backward compatible changes may be added
 * together with the corresponding interface version bump. Backward
 * incompatible changes are done by bumping the version number in the protocol
 * and interface names and resetting the interface version. Once the protocol
 * is to be declared stable, the version number in the protocol is removed and
 * the interface version number is reset.
 *
 * @section page_ifaces_vsync_feedback_unstable_v1 Interfaces
 * - @subpage page_iface_zwp_vsync_feedback_v1 - Protocol for providing vertical synchronization timing
 * - @subpage page_iface_zwp_vsync_timing_v1 - 
 * @section page_copyright_vsync_feedback_unstable_v1 Copyright
 * <pre>
 *
 * Copyright 2016 The Chromium Authors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * </pre>
 */
struct wl_output;
struct zwp_vsync_feedback_v1;
struct zwp_vsync_timing_v1;

/**
 * @page page_iface_zwp_vsync_feedback_v1 zwp_vsync_feedback_v1
 * @section page_iface_zwp_vsync_feedback_v1_desc Description
 *
 * The global interface that allows clients to subscribe for vertical
 * synchronization timing data for given wl_output.
 * @section page_iface_zwp_vsync_feedback_v1_api API
 * See @ref iface_zwp_vsync_feedback_v1.
 */
/**
 * @defgroup iface_zwp_vsync_feedback_v1 The zwp_vsync_feedback_v1 interface
 *
 * The global interface that allows clients to subscribe for vertical
 * synchronization timing data for given wl_output.
 */
extern const struct wl_interface zwp_vsync_feedback_v1_interface;
/**
 * @page page_iface_zwp_vsync_timing_v1 zwp_vsync_timing_v1
 * @section page_iface_zwp_vsync_timing_v1_api API
 * See @ref iface_zwp_vsync_timing_v1.
 */
/**
 * @defgroup iface_zwp_vsync_timing_v1 The zwp_vsync_timing_v1 interface
 */
extern const struct wl_interface zwp_vsync_timing_v1_interface;

#define ZWP_VSYNC_FEEDBACK_V1_DESTROY	0
#define ZWP_VSYNC_FEEDBACK_V1_GET_VSYNC_TIMING	1

/**
 * @ingroup iface_zwp_vsync_feedback_v1
 */
#define ZWP_VSYNC_FEEDBACK_V1_DESTROY_SINCE_VERSION	1
/**
 * @ingroup iface_zwp_vsync_feedback_v1
 */
#define ZWP_VSYNC_FEEDBACK_V1_GET_VSYNC_TIMING_SINCE_VERSION	1

/** @ingroup iface_zwp_vsync_feedback_v1 */
static inline void
zwp_vsync_feedback_v1_set_user_data(struct zwp_vsync_feedback_v1 *zwp_vsync_feedback_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwp_vsync_feedback_v1, user_data);
}

/** @ingroup iface_zwp_vsync_feedback_v1 */
static inline void *
zwp_vsync_feedback_v1_get_user_data(struct zwp_vsync_feedback_v1 *zwp_vsync_feedback_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwp_vsync_feedback_v1);
}

static inline uint32_t
zwp_vsync_feedback_v1_get_version(struct zwp_vsync_feedback_v1 *zwp_vsync_feedback_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwp_vsync_feedback_v1);
}

/**
 * @ingroup iface_zwp_vsync_feedback_v1
 *
 * Destroy this vsync feedback object. Existing vsync timing objects shall
 * not be affected by this request.
 */
static inline void
zwp_vsync_feedback_v1_destroy(struct zwp_vsync_feedback_v1 *zwp_vsync_feedback_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_vsync_feedback_v1,
			 ZWP_VSYNC_FEEDBACK_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zwp_vsync_feedback_v1);
}

/**
 * @ingroup iface_zwp_vsync_feedback_v1
 *
 * Create a new vsync timing object that represents a subscription to
 * vertical synchronization timing updates of given wl_output object.
 *
 * The newly created object will immediately signal an update to notify
 * the subscriber of initial timing parameters.
 */
static inline struct zwp_vsync_timing_v1 *
zwp_vsync_feedback_v1_get_vsync_timing(struct zwp_vsync_feedback_v1 *zwp_vsync_feedback_v1, struct wl_output *output)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zwp_vsync_feedback_v1,
			 ZWP_VSYNC_FEEDBACK_V1_GET_VSYNC_TIMING, &zwp_vsync_timing_v1_interface, NULL, output);

	return (struct zwp_vsync_timing_v1 *) id;
}

/**
 * @ingroup iface_zwp_vsync_timing_v1
 * @struct zwp_vsync_timing_v1_listener
 */
struct zwp_vsync_timing_v1_listener {
	/**
	 * vsync timing updated
	 *
	 * Notifies client that vertical synchronization timing of given
	 * wl_output has changed.
	 *
	 * Timing information consists of two data, timebase and interval.
	 * Timebase is an absolute timestamp of the vsync event that caused
	 * the timing to change. Interval is a period of time between
	 * subsequent vsync events.
	 *
	 * The unit of all above mentioned time values shall be
	 * microseconds and absolute timestamps should match the realm of
	 * the primary system monotonic counter, i.e. the POSIX
	 * clock_gettime(CLOCK_MONOTONIC). Data type of both values is
	 * defined to be a 64-bit unsigned integer, but since the biggest
	 * unsigned integer datatype defined by the Wayland protocol is the
	 * 32-bit uint, both timebase and interval are split into most
	 * significant and least significant part, suffixed by "_h" and
	 * "_l" respectively.
	 * @param timebase_l new vsync timebase (lower 32 bits)
	 * @param timebase_h new vsync timebase (upper 32 bits)
	 * @param interval_l new vsync interval (lower 32 bits)
	 * @param interval_h new vsync interval (upper 32 bits)
	 */
	void (*update)(void *data,
		       struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1,
		       uint32_t timebase_l,
		       uint32_t timebase_h,
		       uint32_t interval_l,
		       uint32_t interval_h);
};

/**
 * @ingroup zwp_vsync_timing_v1_iface
 */
static inline int
zwp_vsync_timing_v1_add_listener(struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1,
				 const struct zwp_vsync_timing_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwp_vsync_timing_v1,
				     (void (**)(void)) listener, data);
}

#define ZWP_VSYNC_TIMING_V1_DESTROY	0

/**
 * @ingroup iface_zwp_vsync_timing_v1
 */
#define ZWP_VSYNC_TIMING_V1_DESTROY_SINCE_VERSION	1

/** @ingroup iface_zwp_vsync_timing_v1 */
static inline void
zwp_vsync_timing_v1_set_user_data(struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwp_vsync_timing_v1, user_data);
}

/** @ingroup iface_zwp_vsync_timing_v1 */
static inline void *
zwp_vsync_timing_v1_get_user_data(struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwp_vsync_timing_v1);
}

static inline uint32_t
zwp_vsync_timing_v1_get_version(struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1)
{
	return wl_proxy_get_version((struct wl_proxy *) zwp_vsync_timing_v1);
}

/**
 * @ingroup iface_zwp_vsync_timing_v1
 *
 * Destroy this vsync timing object.
 */
static inline void
zwp_vsync_timing_v1_destroy(struct zwp_vsync_timing_v1 *zwp_vsync_timing_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_vsync_timing_v1,
			 ZWP_VSYNC_TIMING_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zwp_vsync_timing_v1);
}

#ifdef  __cplusplus
}
#endif

#endif
