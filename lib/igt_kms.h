/*
 * Copyright © 2013 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 * 	Daniel Vetter <daniel.vetter@ffwll.ch>
 * 	Damien Lespiau <damien.lespiau@intel.com>
 */

/**
 * SECTION:igt_kms
 * @short_description: Kernel modesetting support library
 * @title: KMS
 * @include: igt.h
 *
 * This library provides support to enumerate and set modeset configurations.
 *
 * There are two parts in this library: First the low level helper function
 * which directly build on top of raw ioctls or the interfaces provided by
 * libdrm. Those functions all have a kmstest_ prefix.
 *
 * The second part is a high-level library to manage modeset configurations
 * which abstracts away some of the low-level details like the difference
 * between legacy and universal plane support for setting cursors or in the
 * future the difference between legacy and atomic commit. These high-level
 * functions have all igt_ prefixes. This part is still very much work in
 * progress and so also lacks a bit documentation for the individual functions.
 *
 * Note that this library's header pulls in the [i-g-t
 * framebuffer](intel-gpu-tools-Framebuffer.html) library as a
 * dependency.
 */

#ifndef __IGT_KMS_H__
#define __IGT_KMS_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <xf86drmMode.h>

#include "igt_fb.h"
#include "ioctl_wrappers.h"


/**
 * pipe:
 *
 * The front end of the display contains the pipes. There are three instances
 * which are referred to as Pipe A, Pipe B, and Pipe C.
 */
enum pipe {
        PIPE_ANY = -1,
        PIPE_A = 0,
        PIPE_B,
        PIPE_C,
        I915_MAX_PIPES
};


/**
 * igt_plane:
 *
 * Prior to GEN9, each display pipe had a primary plane, a overlay/sprite
 * plane, and a cursor. With the introduction of SKL/BXT the term of universal
 * plane has been introduced, where each plane can serve different purposes.
 *
 * See #igt_display and #igt_display_init that retrieves this information.
 *
 * We namespace this enum to not conflict with the Android i915_drm.h.
 */
enum igt_plane {
        IGT_PLANE_1 = 0,
        IGT_PLANE_PRIMARY = IGT_PLANE_1,
        IGT_PLANE_2,
        IGT_PLANE_3,
        IGT_PLANE_CURSOR,
};

enum port {
        PORT_A = 0,
        PORT_B,
        PORT_C,
        PORT_D,
        PORT_E,
        I915_MAX_PORTS
};

/**
 * kmstest_connector_config:
 * @crtc: DRM pipe, pointer to #drmModeCrtc
 * @connector: end-point connector; pointer to #drmModeConnector
 * @encoder: pointer to #drmModeEncoder
 * @default_mode: default mode found
 * @crtc_idx: pipe found when searching for available connectors
 * @pipe: this translates directly to #pipe
 *
 * Structure used to store encoders, connectors and pipes.
 * See #kmstest_get_connector_config()
 *
 * The relationship between frame buffer, pipes and outputs can be illustrated
 * as follows:
 *
 * |[!<-- language="C" -->
 * +--------------+	+-----+    +-------+    +-----------+
 * | Frame buffer | ->	|Pipes| -> |Encoder| -> | Connector |
 * +--------------+	+-----+    +-------+    +-----------+
 * ]|
 *
 * The frame buffer ties a reference to a memory object and provides
 * a pointer to the actual data (GEM object).
 *
 * The pipe (drmModeCrtc) is used to set the display mode, timings and
 * gamma tables. On some hardware models this is tied with the transcoder.
 *
 * Each pipe can have mutiple planes. On older hardware these planes where
 * known as primary, overlay/sprite, and cursor plane. From GEN9 (SKL/BXT) each
 * pipe has three planes and a cursor plane. Each plane can be used as a
 * primary, as a sprite or as an overlay plane, having the term of universal
 * plane.
 *
 * The encoder never sets the mode but is used to convert, depending
 * on the output, pixels to signals understood by the output.
 *
 * The connector will connect to the output display. This contains information
 * about the attached display such as EDID, DPMS and information about modes
 * supported by the display.
 *
 */
struct kmstest_connector_config {
	drmModeCrtc *crtc;
	drmModeConnector *connector;
	drmModeEncoder *encoder;
	drmModeModeInfo default_mode;
	int crtc_idx;
	int pipe;
};

/**
 * kmstest_force_connector_state:
 * @FORCE_CONNECTOR_UNSPECIFIED: Unspecified
 * @FORCE_CONNECTOR_ON: On
 * @FORCE_CONNECTOR_DIGITAL: Digital
 * @FORCE_CONNECTOR_OFF: Off
 */
enum kmstest_force_connector_state {
	FORCE_CONNECTOR_UNSPECIFIED,
	FORCE_CONNECTOR_ON,
	FORCE_CONNECTOR_DIGITAL,
	FORCE_CONNECTOR_OFF
};

/*
 * A small modeset API
 */

enum igt_commit_style {
	COMMIT_LEGACY = 0,
	COMMIT_UNIVERSAL,
	/* We'll add atomic here eventually. */
};

/**
 * igt_display_t:
 *
 * see #igt_display
 */
typedef struct igt_display igt_display_t;

/**
 * igt_fixed_t:
 *
 * 16.16 fixed point
 */
typedef uint32_t igt_fixed_t;

/**
 * igt_pipe_t:
 *
 * see #igt_pipe
 */
typedef struct igt_pipe igt_pipe_t;

/**
 * igt_rotation_t:
 *
 * maps to the kernel API
 */
typedef enum {
	IGT_ROTATION_0   = 1 << 0,
	IGT_ROTATION_90  = 1 << 1,
	IGT_ROTATION_180 = 1 << 2,
	IGT_ROTATION_270 = 1 << 3,
} igt_rotation_t;

typedef struct {
	/*< private >*/
	igt_pipe_t *pipe;
	int index;
	/* capabilities */
	unsigned int is_primary       : 1;
	unsigned int is_cursor        : 1;
	/* state tracking */
	unsigned int fb_changed       : 1;
	unsigned int position_changed : 1;
	unsigned int panning_changed  : 1;
	unsigned int rotation_changed : 1;
	unsigned int size_changed     : 1;
	/*
	 * drm_plane can be NULL for primary and cursor planes (when not
	 * using the atomic modeset API)
	 */
	drmModePlane *drm_plane;
	struct igt_fb *fb;

	uint32_t rotation_property;

	/* position within pipe_src_w x pipe_src_h */
	int crtc_x, crtc_y;
	/* size within pipe_src_w x pipe_src_h */
	int crtc_w, crtc_h;
	/* panning offset within the fb */
	unsigned int pan_x, pan_y;
	igt_rotation_t rotation;
} igt_plane_t;

/**
 * igt_pipe:
 * @display: a pointer to #igt_display_t
 * @pipe: which pipe see #pipe
 * @enabled: if this pipe is enabled
 * @n_planes: number of planes for this pipe
 * @planes: array of #igt_planes_t
 * @background: Background color MSB BGR 16bpc LSB
 * @background_changed: if the background has changed
 * @background_property: background property
 *
 * Representation of a pipe, that connects to #igt_panel_t and
 * #igt_display_t.
 *
 */
struct igt_pipe {
	igt_display_t *display;
	enum pipe pipe;
	bool enabled;
#define IGT_MAX_PLANES	4
	int n_planes;
	igt_plane_t planes[IGT_MAX_PLANES];
	uint64_t background;
	uint32_t background_changed : 1;
	uint32_t background_property;
};

typedef struct {
	/*< private >*/
	igt_display_t *display;
	uint32_t id;					/* KMS id */
	struct kmstest_connector_config config;
	char *name;
	bool valid;
	unsigned long pending_crtc_idx_mask;
	bool use_override_mode;
	drmModeModeInfo override_mode;
} igt_output_t;

/**
 * igt_display:
 * @drm_fd: DRM fd
 * @log_shift: useful for logging; see #LOG_INDENT
 * @n_pipes: number of pipes
 * @n_outputs: number of outputs
 * @pipes_in_use: how many pipes are in use
 * @outputs: a pointer to #igt_output_t; see #igt_display_init
 * @pipes: array of #igt_pipe_t
 * @has_universal_planes: if the hardware supports universal planes
 */
struct igt_display {
	int drm_fd;
	int log_shift;
	int n_pipes;
	int n_outputs;
	unsigned long pipes_in_use;
	igt_output_t *outputs;
	igt_pipe_t pipes[I915_MAX_PIPES];
	bool has_universal_planes;
};

/* Low-level helpers with kmstest_ prefix */

/**
 * kmstest_port_name:
 * @port: display plane
 *
 * Returns: String representing @port, e.g. "A".
 */
#define kmstest_port_name(port) ((port) + 'A')

/**
 * kmstest_pipe_name:
 * @pipe: display pipe
 *
 * Returns: String represnting @pipe, e.g. "A".
 */
const char *kmstest_pipe_name(enum pipe pipe);

/**
 * kmstest_plane_name:
 * @plane: display plane
 *
 * Returns: String represnting @pipe, e.g. "plane1".
 */
const char *kmstest_plane_name(enum igt_plane plane);


/**
 * kmstest_encoder_type_str:
 * @type: DRM_MODE_ENCODER_* enumeration value
 *
 * Returns: A string representing the drm encoder @type.
 */
const char *kmstest_encoder_type_str(int type);

/**
 * kmstest_connector_status_str:
 * @status: DRM_MODE_* connector status value
 *
 * Returns: A string representing the drm connector status @status.
 */
const char *kmstest_connector_status_str(int status);

/**
 * kmstest_connector_type_str:
 * @type: DRM_MODE_CONNECTOR_* enumeration value
 *
 * Returns: A string representing the drm connector @type.
 */
const char *kmstest_connector_type_str(int type);

/**
 * kmstest_dump_mode:
 * @mode: libdrm mode structure
 *
 * Prints @mode to stdout in a human-readable form.
 */
void kmstest_dump_mode(drmModeModeInfo *mode);

/**
 * kmstest_get_pipe_from_crtc_id:
 * @fd: DRM fd
 * @crtc_id: DRM CRTC id
 *
 * Returns: The pipe number for the given DRM CRTC @crtc_id. This maps directly
 * to an enum pipe value used in other helper functions.
 */
int kmstest_get_pipe_from_crtc_id(int fd, int crtc_id);

/**
 * kmstest_set_vt_graphics_mode:
 *
 * Sets the controlling VT (if available) into graphics/raw mode and installs
 * an igt exit handler to set the VT back to text mode on exit. Use
 * #kmstest_restore_vt_mode to restore the previous VT mode manually.
 *
 * All kms tests must call this function to make sure that the fbcon doesn't
 * interfere by e.g. blanking the screen.
 */
void kmstest_set_vt_graphics_mode(void);

/**
 * kmstest_restore_vt_mode:
 *
 * Restore the VT mode in use before #kmstest_set_vt_graphics_mode was called.
 */
void kmstest_restore_vt_mode(void);

/**
 * kmstest_force_connector:
 * @fd: drm file descriptor
 * @connector: connector
 * @state: state to force on @connector
 *
 * Force the specified state on the specified connector.
 *
 * Returns: true on success
 */
bool kmstest_force_connector(int fd, drmModeConnector *connector,
			     enum kmstest_force_connector_state state);

/**
 * kmstest_edid_add_3d:
 * @edid: an existing valid edid block
 * @length: length of @edid
 * @new_edid_ptr: pointer to where the new edid will be placed
 * @new_length: pointer to the size of the new edid
 *
 * Makes a copy of an existing edid block and adds an extension indicating
 * stereo 3D capabilities.
 */
void kmstest_edid_add_3d(const unsigned char *edid, size_t length,
			 unsigned char *new_edid_ptr[], size_t *new_length);

/**
 * kmstest_force_edid:
 * @drm_fd: drm file descriptor
 * @connector: connector to set @edid on
 * @edid: An EDID data block
 * @length: length of the EDID data. #EDID_LENGTH defines the standard EDID
 * length
 *
 * Set the EDID data on @connector to @edid. See also #igt_kms_get_base_edid.
 *
 * If @length is zero, the forced EDID will be removed.
 */
void kmstest_force_edid(int drm_fd, drmModeConnector *connector,
			const unsigned char *edid, size_t length);

/**
 * kmstest_get_connector_default_mode:
 * @drm_fd: DRM fd
 * @connector: libdrm connector
 * @mode: libdrm mode
 *
 * Retrieves the default mode for @connector and stores it in @mode.
 *
 * Returns: true on success, false on failure
 */
bool kmstest_get_connector_default_mode(int drm_fd, drmModeConnector *connector,
					drmModeModeInfo *mode);

/**
 * kmstest_get_connector_config:
 * @drm_fd: DRM fd
 * @connector_id: DRM connector id
 * @crtc_idx_mask: mask of allowed DRM CRTC indices
 * @config: structure filled with the possible configuration
 *
 * This tries to find a suitable configuration for the given connector and CRTC
 * constraint and fills it into @config.
 */
bool kmstest_get_connector_config(int drm_fd, uint32_t connector_id,
				  unsigned long crtc_idx_mask,
				  struct kmstest_connector_config *config);

/**
 * kmstest_free_connector_config:
 * @config: connector configuration structure
 *
 * Free any resources in @config allocated in kmstest_get_connector_config().
 */
void kmstest_free_connector_config(struct kmstest_connector_config *config);

/**
 * kmstest_set_connector_dpms:
 * @fd: DRM fd
 * @connector: libdrm connector
 * @mode: DRM DPMS value
 *
 * This function sets the DPMS setting of @connector to @mode.
 */
void kmstest_set_connector_dpms(int fd, drmModeConnector *connector, int mode);

/**
 * kmstest_get_property:
 * @drm_fd: drm file descriptor
 * @object_id: object whose properties we're going to get
 * @object_type: type of obj_id (DRM_MODE_OBJECT_*)
 * @name: name of the property we're going to get
 * @prop_id: if not NULL, returns the property id
 * @value: if not NULL, returns the property value
 * @prop: if not NULL, returns the property, and the caller will have to free
 *        it manually.
 *
 * Finds a property with the given name on the given object.
 *
 * Returns: true in case we found something.
 */
bool kmstest_get_property(int drm_fd, uint32_t object_id, uint32_t object_type,
			  const char *name, uint32_t *prop_id, uint64_t *value,
			  drmModePropertyPtr *prop);

/**
 * kmstest_unset_all_crtcs:
 * @drm_fd: the DRM fd
 * @resources: libdrm resources pointer
 *
 * Disables all the screens.
 */
void kmstest_unset_all_crtcs(int drm_fd, drmModeResPtr resources);


/* High-level kms api with igt_ prefix */

/**
 * igt_display_init:
 * @display: a pointer to an #igt_display_t structure
 * @drm_fd: a drm file descriptor
 *
 * Initialize @display and allocate the various resources required. Use
 * #igt_display_fini to release the resources when they are no longer required.
 *
 */
void igt_display_init(igt_display_t *display, int drm_fd);

/**
 * igt_display_fini:
 * @display: a pointer to #igt_display_t structure
 *
 * Release any resources associated with @display. This does not free @display
 * itself.
 */
void igt_display_fini(igt_display_t *display);

/**
 * igt_display_commit2:
 * @display: DRM device handle
 * @s: Commit style
 *
 * Commits framebuffer and positioning changes to all planes of each display
 * pipe, using a specific API to perform the programming.  This function should
 * be used to exercise a specific driver programming API; #igt_display_commit
 * should be used instead if the API used is unimportant to the test being run.
 *
 * This function should only be used to commit changes that are expected to
 * succeed, since any failure during the commit process will cause the IGT
 * subtest to fail.  To commit changes that are expected to fail, use
 * #igt_try_display_commit2 instead.
 *
 * Returns: 0 upon success.  This function will never return upon failure
 * since #igt_fail() at lower levels will longjmp out of it.
 */
int igt_display_commit2(igt_display_t *display, enum igt_commit_style s);

/**
 * igt_display_commit:
 * @display: DRM device handle
 *
 * Commits framebuffer and positioning changes to all planes of each display
 * pipe.
 *
 * Returns: 0 upon success.  This function will never return upon failure
 * since igt_fail() at lower levels will longjmp out of it.
 */
int igt_display_commit(igt_display_t *display);

/**
 * igt_display_try_commit2:
 * @display: DRM device handle
 * @s: Commit style
 *
 * Attempts to commit framebuffer and positioning changes to all planes of each
 * display pipe.  This function should be used to commit changes that are
 * expected to fail, so that the error code can be checked for correctness.
 * For changes that are expected to succeed, use @igt_display_commit instead.
 *
 * Note that in non-atomic commit styles, no display programming will be
 * performed after the first failure is encountered, so only some of the
 * operations requested by a test may have been completed.  Tests that catch
 * errors returned by this function should take care to restore the display to
 * a sane state after a failure is detected.
 *
 * Returns: 0 upon success, otherwise the error code of the first error
 * encountered.
 */
int igt_display_try_commit2(igt_display_t *display, enum igt_commit_style s);

/**
 * igt_display_get_n_pipes:
 * @display: the display of which retrieve the number of pipes
 *
 * Returns: An integer with total number of pipes for that @display.
 */
int igt_display_get_n_pipes(igt_display_t *display);

/**
 * igt_output_name:
 * @output: Output of which we retrieve the name
 *
 * Returns: A string representing the name of @output.
 */
const char *igt_output_name(igt_output_t *output);

/**
 * igt_output_get_mode:
 * @output: Output as pointer to #igt_output_t
 *
 * Retrieves the default mode of @output
 *
 * Returns: a pointer to drmModeModeInfo
 *
 */
drmModeModeInfo *igt_output_get_mode(igt_output_t *output);

/**
 * igt_output_override_mode:
 * @output: Output of which the mode will be overridden
 * @mode: New mode
 *
 * Overrides the output's mode with @mode, so that it is used instead of the
 * mode obtained with get connectors. Note that the mode is used without
 * checking if the output supports it, so this might lead to unexpected results.
 */
void igt_output_override_mode(igt_output_t *output, drmModeModeInfo *mode);


/**
 * igt_output_set_plane:
 * @output: Output of which the pipe will be overriden
 * @pipe: one of #pipe
 *
 */
void igt_output_set_pipe(igt_output_t *output, enum pipe pipe);

/**
 * igt_output_get_plane:
 * @output: a pointer to #igt_output_t
 * @plane: a #igt_plane plane
 *
 * Returns: a pointer to #igt_plane_t matching @plane
 *
 */
igt_plane_t *igt_output_get_plane(igt_output_t *output, enum igt_plane plane);

static inline bool igt_plane_supports_rotation(igt_plane_t *plane)
{
	return plane->rotation_property != 0;
}

/**
 * igt_plane_set_fb:
 * @plane: plane pointer
 * @fb: pointer to a #igt_fb
 *
 * Sets the default plane size and position as framebuffers size.
 *
 */
void igt_plane_set_fb(igt_plane_t *plane, struct igt_fb *fb);

/**
 * igt_plane_set_position:
 * @plane: plane pointer for which the position should be set
 * @x: the x-axis
 * @y: the y-axis
 *
 */
void igt_plane_set_position(igt_plane_t *plane, int x, int y);

/**
 * igt_plane_set_size:
 * @plane: plane pointer for which size to be set
 * @w: width
 * @h: height
 *
 * This function sets width and height for requested plane.
 * New size will be committed at plane commit time via
 * drmModeSetPlane().
 */
void igt_plane_set_size(igt_plane_t *plane, int w, int h);

/**
 * igt_plane_set_panning:
 * @plane: plane pointer for which pannig should be set
 * @x: specifies the x-axis
 * @y: specifies the y-axis
 *
 */
void igt_plane_set_panning(igt_plane_t *plane, int x, int y);

/**
 * igt_plane_set_rotation:
 * @plane: plane pointer for which rotation should be set
 * @rotation: one of #igt_rotation_t
 *
 */
void igt_plane_set_rotation(igt_plane_t *plane, igt_rotation_t rotation);

/**
 * igt_crtc_set_background:
 * @pipe: pipe pointer to which background color to be set
 * @background: background color value in BGR 16bpc
 *
 * Sets background color for requested pipe. Color value provided here
 * will be actually submitted at output commit time via "background_color"
 * property.
 * For example to get red as background, set background = 0x00000000FFFF.
 */
void igt_crtc_set_background(igt_pipe_t *pipe, uint64_t background);

/**
 * igt_fb_set_position:
 * @fb: framebuffer pointer
 * @plane: plane
 * @x: X position
 * @y: Y position
 *
 * This function sets position for requested framebuffer as src to plane.
 * New position will be committed at plane commit time via drmModeSetPlane().
 */
void igt_fb_set_position(struct igt_fb *fb, igt_plane_t *plane, uint32_t x, uint32_t y);

/**
 * igt_fb_set_size:
 * @fb: framebuffer pointer
 * @plane: plane
 * @w: width
 * @h: height
 *
 * This function sets fetch rect size from requested framebuffer as src
 * to plane. New size will be committed at plane commit time via
 * drmModeSetPlane().
 */
void igt_fb_set_size(struct igt_fb *fb, igt_plane_t *plane, uint32_t w, uint32_t h);

/**
 * igt_wait_for_vblank:
 * @drm_fd: DRM fd
 * @pipe: the pipe on which to wait for a vertical blank
 *
 * This functions can used to wait until the end of the frame and start of the
 * next frame on @pipe, called a vblank.
 *
 *
 */
void igt_wait_for_vblank(int drm_fd, enum pipe pipe);

#define for_each_connected_output(display, output)		\
	for (int i__ = 0;  i__ < (display)->n_outputs; i__++)	\
		if ((output = &(display)->outputs[i__]), output->valid)

#define for_each_pipe(display, pipe)					\
	for (pipe = 0; pipe < igt_display_get_n_pipes(display); pipe++)	\

#define for_each_plane_on_pipe(display, pipe, plane)			\
	for (int i__ = 0; (plane) = &(display)->pipes[(pipe)].planes[i__], \
		     i__ < (display)->pipes[(pipe)].n_planes; i__++)

#define IGT_FIXED(i,f)	((i) << 16 | (f))

/**
 * igt_enable_connectors:
 *
 * Force connectors to be enabled where this is known to work well. Use
 * #igt_reset_connectors to revert the changes.
 *
 * An exit handler is installed to ensure connectors are reset when the test
 * exits.
 */
void igt_enable_connectors(void);

/**
 * igt_reset_connectors:
 *
 * Remove any forced state from the connectors.
 */
void igt_reset_connectors(void);

#define EDID_LENGTH 128

/**
 * igt_kms_get_base_edid:
 *
 * Get the base edid block, which includes the following modes:
 *
 *  - 1920x1080 60Hz
 *  - 1280x720 60Hz
 *  - 1024x768 60Hz
 *  - 800x600 60Hz
 *  - 640x480 60Hz
 *
 * This can be extended with further features using functions such as
 * #kmstest_edid_add_3d.
 *
 * Returns: a basic edid block
 */
const unsigned char *igt_kms_get_base_edid(void);

/**
 * igt_kms_get_alt_edid:
 *
 * Get an alternate edid block, which includes the following modes:
 *
 *  - 1400x1050 60Hz
 *  - 1920x1080 60Hz
 *  - 1280x720 60Hz
 *  - 1024x768 60Hz
 *  - 800x600 60Hz
 *  - 640x480 60Hz
 *
 * This can be extended with further features using functions such as
 * #kmstest_edid_add_3d.
 *
 * Returns: an alternate edid block
 */
const unsigned char *igt_kms_get_alt_edid(void);


#endif /* __IGT_KMS_H__ */
