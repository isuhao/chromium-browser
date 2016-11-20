# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      # GN version: //ui/base:ui_data_pack
      # As part of building Chrome on iOS, it is necessary to run a tool on
      # the host to load datapack and generate output in a format defined
      # by the platform (this is to support notifications).
      #
      # Introduce a standalone target that build on both 'host' and 'target'
      # toolset that just build the support to load datapack. The dependency
      # should be kept minimal to have to build too many targets with multiple
      # toolsets.
      'target_name': 'ui_data_pack',
      'toolsets': ['host', 'target'],
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
      ],
      'sources': [
        'resource/data_pack.cc',
        'resource/data_pack.h',
        'resource/data_pack_export.h',
        'resource/resource_handle.h',
        'resource/scale_factor.cc',
        'resource/scale_factor.h',
      ],
      'defines': [
        'UI_DATA_PACK_IMPLEMENTATION',
      ],
      'conditions': [
        ['OS=="win"', {
          # TODO(jschuh): C4267: http://crbug.com/167187 size_t -> int
          'msvs_disabled_warnings': [ 4267 ],
        }],
    ],
    },
    { # GN version: //ui/base:ui_features
      'target_name': 'ui_features',
      'includes': [ '../../build/buildflag_header.gypi' ],
      'variables': {
        'buildflag_header_path': 'ui/base/ui_features.h',
        'buildflag_flags': [
          'ENABLE_HIDPI=<(enable_hidpi)',
        ],
      },
    },
    {
      # GN version: //ui/base
      'target_name': 'ui_base',
      'type': '<(component)',
      'dependencies': [
        'ui_data_pack',
        'ui_features',
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/base.gyp:base_static',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../net/net.gyp:net',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../../third_party/zlib/zlib.gyp:zlib',
        '../../url/url.gyp:url_lib',
        '../display/display.gyp:display',
        '../events/events.gyp:events',
        '../events/events.gyp:events_base',
        '../events/platform/events_platform.gyp:events_platform',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../gfx/gfx.gyp:gfx_range',
        '../resources/ui_resources.gyp:ui_resources',
        '../strings/ui_strings.gyp:ui_strings',
      ],
      'defines': [
        'UI_BASE_IMPLEMENTATION',
      ],
      'export_dependent_settings': [
        '../../net/net.gyp:net',
        '../../skia/skia.gyp:skia',
        '../gfx/gfx.gyp:gfx',
      ],
      'sources' : [
        # Note: file list duplicated in GN build.
        'accelerators/accelerator.cc',
        'accelerators/accelerator.h',
        'accelerators/accelerator_history.cc',
        'accelerators/accelerator_history.h',
        'accelerators/accelerator_manager.cc',
        'accelerators/accelerator_manager.h',
        'accelerators/menu_label_accelerator_util_linux.cc',
        'accelerators/menu_label_accelerator_util_linux.h',
        'accelerators/platform_accelerator.h',
        'accelerators/platform_accelerator_cocoa.h',
        'accelerators/platform_accelerator_cocoa.mm',
        'android/ui_base_jni_registrar.cc',
        'android/ui_base_jni_registrar.h',
        'base_window.cc',
        'base_window.h',
        'clipboard/clipboard.cc',
        'clipboard/clipboard.h',
        'clipboard/clipboard_android.cc',
        'clipboard/clipboard_android.h',
        'clipboard/clipboard_aura.cc',
        'clipboard/clipboard_aura.h',
        'clipboard/clipboard_aurax11.cc',
        'clipboard/clipboard_aurax11.h',
        'clipboard/clipboard_constants.cc',
        'clipboard/clipboard_mac.h',
        'clipboard/clipboard_mac.mm',
        'clipboard/clipboard_monitor.cc',
        'clipboard/clipboard_monitor.h',        
        'clipboard/clipboard_observer.h',
        'clipboard/clipboard_types.h',
        'clipboard/clipboard_util_mac.h',
        'clipboard/clipboard_util_mac.mm',
        'clipboard/clipboard_util_win.cc',
        'clipboard/clipboard_util_win.h',
        'clipboard/clipboard_win.cc',
        'clipboard/clipboard_win.h',
        'clipboard/custom_data_helper.cc',
        'clipboard/custom_data_helper.h',
        'clipboard/custom_data_helper_mac.mm',
        'clipboard/scoped_clipboard_writer.cc',
        'clipboard/scoped_clipboard_writer.h',
        'cocoa/animation_utils.h',
        'cocoa/appkit_utils.h',
        'cocoa/appkit_utils.mm',
        'cocoa/base_view.h',
        'cocoa/base_view.mm',
        'cocoa/cocoa_base_utils.h',
        'cocoa/cocoa_base_utils.mm',
        'cocoa/command_dispatcher.h',
        'cocoa/command_dispatcher.mm',
        'cocoa/constrained_window/constrained_window_animation.h',
        'cocoa/constrained_window/constrained_window_animation.mm',
        'cocoa/controls/blue_label_button.h',
        'cocoa/controls/blue_label_button.mm',
        'cocoa/controls/hover_image_menu_button.h',
        'cocoa/controls/hover_image_menu_button.mm',
        'cocoa/controls/hover_image_menu_button_cell.h',
        'cocoa/controls/hover_image_menu_button_cell.mm',
        'cocoa/controls/hyperlink_button_cell.h',
        'cocoa/controls/hyperlink_button_cell.mm',
        'cocoa/controls/hyperlink_text_view.h',
        'cocoa/controls/hyperlink_text_view.mm',
        'cocoa/find_pasteboard.h',
        'cocoa/find_pasteboard.mm',
        'cocoa/flipped_view.h',
        'cocoa/flipped_view.mm',
        'cocoa/focus_tracker.h',
        'cocoa/focus_tracker.mm',
        'cocoa/focus_window_set.h',
        'cocoa/focus_window_set.mm',
        'cocoa/fullscreen_window_manager.h',
        'cocoa/fullscreen_window_manager.mm',
        'cocoa/hover_button.h',
        'cocoa/hover_button.mm',
        'cocoa/hover_image_button.h',
        'cocoa/hover_image_button.mm',
        'cocoa/menu_controller.h',
        'cocoa/menu_controller.mm',
        'cocoa/nib_loading.h',
        'cocoa/nib_loading.mm',
        'cocoa/nscolor_additions.h',
        'cocoa/nscolor_additions.mm',
        'cocoa/nsgraphics_context_additions.h',
        'cocoa/nsgraphics_context_additions.mm',
        'cocoa/nsview_additions.h',
        'cocoa/nsview_additions.mm',
        'cocoa/remote_layer_api.h',
        'cocoa/remote_layer_api.mm',
        'cocoa/scoped_cg_context_smooth_fonts.h',
        'cocoa/scoped_cg_context_smooth_fonts.mm',
        'cocoa/three_part_image.h',
        'cocoa/three_part_image.mm',
        'cocoa/tool_tip_base_view.h',
        'cocoa/tool_tip_base_view.mm',
        'cocoa/tracking_area.h',
        'cocoa/tracking_area.mm',
        'cocoa/underlay_opengl_hosting_window.h',
        'cocoa/underlay_opengl_hosting_window.mm',
        'cocoa/user_interface_item_command_handler.h',
        'cocoa/view_description.h',
        'cocoa/view_description.mm',
        'cocoa/window_size_constants.h',
        'cocoa/window_size_constants.mm',
        'cursor/cursor.cc',
        'cursor/cursor.h',
        'cursor/cursor_android.cc',
        'cursor/cursor_loader.h',
        'cursor/cursor_loader_android.cc',
        'cursor/cursor_loader_ozone.cc',
        'cursor/cursor_loader_ozone.h',
        'cursor/cursor_loader_win.cc',
        'cursor/cursor_loader_win.h',
        'cursor/cursor_loader_x11.cc',
        'cursor/cursor_loader_x11.h',
        'cursor/cursor_ozone.cc',
        'cursor/cursor_util.cc',
        'cursor/cursor_util.h',
        'cursor/cursor_win.cc',
        'cursor/cursor_x11.cc',
        'cursor/cursors_aura.cc',
        'cursor/cursors_aura.h',
        'cursor/image_cursors.cc',
        'cursor/image_cursors.h',
        'cursor/ozone/bitmap_cursor_factory_ozone.cc',
        'cursor/ozone/bitmap_cursor_factory_ozone.h',
        'default_style.h',
        'default_theme_provider.cc',
        'default_theme_provider.h',
        'default_theme_provider_mac.mm',
        'device_form_factor.h',
        'device_form_factor_android.cc',
        'device_form_factor_android.h',
        'device_form_factor_desktop.cc',
        'device_form_factor_ios.mm',
        'dragdrop/cocoa_dnd_util.h',
        'dragdrop/cocoa_dnd_util.mm',
        'dragdrop/drag_drop_types.h',
        'dragdrop/drag_drop_types_mac.mm',
        'dragdrop/drag_drop_types_win.cc',
        'dragdrop/drag_source_win.cc',
        'dragdrop/drag_source_win.h',
        'dragdrop/drag_utils.cc',
        'dragdrop/drag_utils.h',
        'dragdrop/drag_utils_win.cc',
        'dragdrop/drop_target_event.cc',
        'dragdrop/drop_target_event.h',
        'dragdrop/drop_target_win.cc',
        'dragdrop/drop_target_win.h',
        'dragdrop/file_info.cc',
        'dragdrop/file_info.h',
        'dragdrop/os_exchange_data.cc',
        'dragdrop/os_exchange_data.h',
        'dragdrop/os_exchange_data_provider_aura.cc',
        'dragdrop/os_exchange_data_provider_aura.h',
        'dragdrop/os_exchange_data_provider_aurax11.cc',
        'dragdrop/os_exchange_data_provider_aurax11.h',
        'dragdrop/os_exchange_data_provider_mac.h',
        'dragdrop/os_exchange_data_provider_mac.mm',
        'dragdrop/os_exchange_data_provider_win.cc',
        'dragdrop/os_exchange_data_provider_win.h',
        'hit_test.h',
        'idle/idle.cc',
        'idle/idle.h',
        'idle/idle_android.cc',
        'idle/idle_chromeos.cc',
        'idle/idle_linux.cc',
        'idle/idle_mac.mm',
        'idle/idle_query_x11.cc',
        'idle/idle_query_x11.h',
        'idle/idle_win.cc',
        'idle/screensaver_window_finder_x11.cc',
        'idle/screensaver_window_finder_x11.h',
        'l10n/formatter.cc',
        'l10n/formatter.h',
        'l10n/l10n_font_util.cc',
        'l10n/l10n_font_util.h',
        'l10n/l10n_util.cc',
        'l10n/l10n_util.h',
        'l10n/l10n_util_android.cc',
        'l10n/l10n_util_android.h',
        'l10n/l10n_util_collator.h',
        'l10n/l10n_util_mac.h',
        'l10n/l10n_util_mac.mm',
        'l10n/l10n_util_posix.cc',
        'l10n/l10n_util_win.cc',
        'l10n/l10n_util_win.h',
        'l10n/time_format.cc',
        'l10n/time_format.h',
        'layout.cc',
        'layout.h',
        'layout_mac.mm',
        'material_design/material_design_controller.cc',
        'material_design/material_design_controller.h',
        'models/button_menu_item_model.cc',
        'models/button_menu_item_model.h',
        'models/combobox_model.cc',
        'models/combobox_model.h',
        'models/combobox_model_observer.h',
        'models/dialog_model.cc',
        'models/dialog_model.h',
        'models/list_model.h',
        'models/list_model_observer.h',
        'models/list_selection_model.cc',
        'models/list_selection_model.h',
        'models/menu_model.cc',
        'models/menu_model.h',
        'models/menu_model_delegate.h',
        'models/menu_separator_types.h',
        'models/simple_combobox_model.cc',
        'models/simple_combobox_model.h',
        'models/simple_menu_model.cc',
        'models/simple_menu_model.h',
        'models/table_model.cc',
        'models/table_model.h',
        'models/table_model_observer.h',
        'models/tree_model.cc',
        'models/tree_model.h',
        'models/tree_node_iterator.h',
        'models/tree_node_model.h',
        'nine_image_painter_factory.cc',
        'nine_image_painter_factory.h',
        'page_transition_types.cc',
        'page_transition_types.h',
        'resource/resource_bundle.cc',
        'resource/resource_bundle.h',
        'resource/resource_bundle_android.cc',
        'resource/resource_bundle_auralinux.cc',
        'resource/resource_bundle_ios.mm',
        'resource/resource_bundle_mac.mm',
        'resource/resource_bundle_win.cc',
        'resource/resource_bundle_win.h',
        'resource/resource_data_dll_win.cc',
        'resource/resource_data_dll_win.h',
        'template_expressions.cc',
        'template_expressions.h',
        'text/bytes_formatting.cc',
        'text/bytes_formatting.h',
        'theme_provider.cc',
        'theme_provider.h',
        'touch/touch_device.cc',
        'touch/touch_device.h',
        'touch/touch_device_android.cc',
        'touch/touch_device_ios.cc',
        'touch/touch_device_linux.cc',
        'touch/touch_device_win.cc',
        'touch/touch_editing_controller.cc',
        'touch/touch_editing_controller.h',
        'touch/touch_enabled.cc',
        'touch/touch_enabled.h',
        'ui_base_export.h',
        'ui_base_exports.cc',
        'ui_base_paths.cc',
        'ui_base_paths.h',
        'ui_base_switches.cc',
        'ui_base_switches.h',
        'ui_base_switches_util.cc',
        'ui_base_switches_util.h',
        'ui_base_types.cc',
        'ui_base_types.h',
        'user_activity/user_activity_detector.cc',
        'user_activity/user_activity_detector.h',
        'user_activity/user_activity_observer.h',
        'view_prop.cc',
        'view_prop.h',
        'webui/jstemplate_builder.cc',
        'webui/jstemplate_builder.h',
        'webui/web_ui_util.cc',
        'webui/web_ui_util.h',
        'win/accessibility_ids_win.h',
        'win/accessibility_misc_utils.cc',
        'win/accessibility_misc_utils.h',
        'win/atl_module.h',
        'win/foreground_helper.cc',
        'win/foreground_helper.h',
        'win/hidden_window.cc',
        'win/hidden_window.h',
        'win/hwnd_subclass.cc',
        'win/hwnd_subclass.h',
        'win/internal_constants.cc',
        'win/internal_constants.h',
        'win/lock_state.cc',
        'win/lock_state.h',
        'win/message_box_win.cc',
        'win/message_box_win.h',
        'win/mouse_wheel_util.cc',
        'win/mouse_wheel_util.h',
        'win/open_file_name_win.cc',
        'win/open_file_name_win.h',
        'win/osk_display_manager.cc',
        'win/osk_display_manager.h',
        'win/osk_display_observer.h',
        'win/scoped_ole_initializer.cc',
        'win/scoped_ole_initializer.h',
        'win/shell.cc',
        'win/shell.h',
        'win/touch_input.cc',
        'win/touch_input.h',
        'win/window_event_target.cc',
        'win/window_event_target.h',
        'window_open_disposition.cc',
        'window_open_disposition.h',
        'work_area_watcher_observer.h',
        'x/selection_owner.cc',
        'x/selection_owner.h',
        'x/selection_requestor.cc',
        'x/selection_requestor.h',
        'x/selection_utils.cc',
        'x/selection_utils.h',
      ],
      'target_conditions': [
        ['OS == "ios"', {
          'sources/': [
            ['include', '^l10n/l10n_util_mac\\.mm$'],
          ],
        }],
      ],
      'conditions': [
        ['OS=="ios"', {
          # iOS only uses a subset of UI.
          'sources/': [
            ['exclude', '\\.(cc|mm)$'],
            ['include', '_ios\\.(cc|mm)$'],
            ['include', '(^|/)ios/'],
            ['include', '^l10n/'],
            ['include', '^layout'],
            ['include', '^material_design/'],
            ['include', '^page_transition_type'],
            ['include', '^resource/'],
            ['include', 'template_expressions.cc'],
            ['include', '^ui_base_'],
            ['include', '^webui/'],
            ['include', '^window_open_disposition\\.cc'],
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
            ],
          },
        }],
        ['use_aura==1', {
          'sources/': [
            ['exclude', 'clipboard/clipboard_mac.mm'],
            ['exclude', 'layout_mac.mm'],
            ['exclude', 'work_area_watcher_observer.h'],
            ['include', 'window_tracker_template.h'],
          ],
        }, {  # use_aura!=1
          'sources!': [
            'cursor/cursor.cc',
            'cursor/cursor.h',
            'cursor/cursor_loader_android.cc',
            'cursor/cursor_loader_x11.cc',
            'cursor/cursor_loader_x11.h',
            'cursor/cursor_win.cc',
            'cursor/cursor_x11.cc',
            'x/selection_owner.cc',
            'x/selection_owner.h',
            'x/selection_requestor.cc',
            'x/selection_requestor.h',
            'x/selection_utils.cc',
            'x/selection_utils.h',
          ],
        }],
        ['use_aura==0 or OS!="linux"', {
          'sources!': [
            'resource/resource_bundle_auralinux.cc',
          ],
        }],
        ['use_ozone==1', {
          'dependencies': [
            '../events/devices/events_devices.gyp:events_devices',
            '../events/ozone/events_ozone.gyp:events_ozone_evdev',
            '../events/ozone/events_ozone.gyp:events_ozone_layout',
            '../ozone/ozone.gyp:ozone_base',
          ],
        }],
        ['use_glib == 1', {
          'dependencies': [
            '../../build/linux/system.gyp:fontconfig',
            '../../build/linux/system.gyp:glib',
          ],
        }],
        ['OS=="linux"', {
          'conditions': [
            ['toolkit_views==0 and use_aura==0', {
              # Note: because of gyp predence rules this has to be defined as
              # 'sources/' rather than 'sources!'.
              'sources/': [
                ['exclude', '^dragdrop/drag_utils.cc'],
                ['exclude', '^dragdrop/drag_utils.h'],
              ],
            }, {
              'sources/': [
                ['include', '^dragdrop/os_exchange_data.cc'],
                ['include', '^dragdrop/os_exchange_data.h'],
                ['include', '^nine_image_painter_factory.cc'],
                ['include', '^nine_image_painter_factory.h'],
              ],
            }],
          ],
        }],
        ['use_pango==1', {
          'dependencies': [
            '../../build/linux/system.gyp:pangocairo',
          ],
        }],
        ['OS=="win" or use_clipboard_aurax11==1', {
          'sources!': [
            'clipboard/clipboard_aura.cc',
          ],
        }, {
          'sources!': [
            'clipboard/clipboard_aurax11.cc',
          ],
        }],
        ['chromeos==1 or (use_aura==1 and OS=="linux" and use_x11==0)', {
          'sources!': [
            'dragdrop/os_exchange_data_provider_aurax11.cc',
          ],
        }, {
          'sources!': [
            'dragdrop/os_exchange_data_provider_aura.cc',
            'dragdrop/os_exchange_data_provider_aura.h',
          ],
        }],
        ['OS=="linux"', {
          'sources!': [
            'touch/touch_device.cc',
          ],
        }, {
          'sources!': [
            'touch/touch_device_linux.cc',
          ],
        }],
        ['OS=="win"', {
          'sources!': [
            'touch/touch_device.cc',
          ],
          'include_dirs': [
            '../..',
            '../../third_party/wtl/include',
          ],
          # TODO(jschuh): C4267: http://crbug.com/167187 size_t -> int
          # C4324 is structure was padded due to __declspec(align()), which is
          # uninteresting.
          'msvs_disabled_warnings': [ 4267, 4324 ],
          'msvs_settings': {
            'VCLinkerTool': {
              'DelayLoadDLLs': [
                'd2d1.dll',
                'd3d10_1.dll',
                'dwmapi.dll',
              ],
              'AdditionalDependencies': [
                'd2d1.lib',
                'd3d10_1.lib',
                'dwmapi.lib',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-ld2d1.lib',
              '-ldwmapi.lib',
              '-loleacc.lib',
            ],
          },
        }, {  # OS!="win"
          'conditions': [
            ['use_aura==0', {
              'sources!': [
                'view_prop.cc',
                'view_prop.h',
              ],
            }],
          ],
        }],
        ['OS=="mac"', {
          'dependencies': [
            '../../third_party/mozilla/mozilla.gyp:mozilla',
          ],
          'sources!': [
            'cursor/image_cursors.cc',
            'cursor/image_cursors.h',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/Accelerate.framework',
              '$(SDKROOT)/System/Library/Frameworks/AudioUnit.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
            ],
          },
        }],
        ['use_x11==1', {
          'all_dependent_settings': {
            'ldflags': [
              '-L<(PRODUCT_DIR)',
            ],
          },
          'dependencies': [
            '../../build/linux/system.gyp:x11',
            '../../build/linux/system.gyp:xcursor',
            '../../build/linux/system.gyp:xext',
            '../../build/linux/system.gyp:xfixes',
            '../../build/linux/system.gyp:xrender',  # For XRender* function calls in x11_util.cc.
            '../events/devices/events_devices.gyp:events_devices',
            '../events/devices/x11/events_devices_x11.gyp:events_devices_x11',
            '../events/platform/x11/x11_events_platform.gyp:x11_events_platform',
            '../gfx/x/gfx_x11.gyp:gfx_x11',
            'x/ui_base_x.gyp:ui_base_x',
          ],
        }],
        ['use_x11==1 and chromeos==0', {
          'dependencies': [
            '../../build/linux/system.gyp:xscrnsaver',
          ],
        }],
        ['toolkit_views==0', {
          'sources!': [
            'dragdrop/drag_drop_types.h',
            'dragdrop/drop_target_event.cc',
            'dragdrop/drop_target_event.h',
            'dragdrop/os_exchange_data.cc',
            'dragdrop/os_exchange_data.h',
            'nine_image_painter_factory.cc',
            'nine_image_painter_factory.h',
          ],
        }],
        ['OS=="android"', {
          'sources!': [
            'cursor/image_cursors.cc',
            'cursor/image_cursors.h',
            'default_theme_provider.cc',
            'dragdrop/drag_utils.cc',
            'dragdrop/drag_utils.h',
            'l10n/l10n_font_util.cc',
            'models/button_menu_item_model.cc',
            'models/dialog_model.cc',
            'theme_provider.cc',
            'touch/touch_device.cc',
            'touch/touch_editing_controller.cc',
            'ui_base_types.cc',
          ],
          'dependencies': [
            '../android/ui_android.gyp:ui_java',
            'ui_base_jni_headers',
          ],
          'link_settings': {
            'libraries': [
              '-ljnigraphics',
            ],
          },
        }],
        ['OS=="android" and use_aura==0', {
          'sources!': [
            'cursor/cursor_android.cc',
            'idle/idle.cc',
            'idle/idle.h',
            'idle/idle_android.cc',
          ],
        }],
        ['OS=="android" and use_aura==1', {
          'sources!': [
            'clipboard/clipboard_aura.cc'
          ],
        }],
        ['OS=="android" or OS=="ios"', {
          'sources!': [
            'device_form_factor_desktop.cc'
          ],
        }],
        ['OS=="linux"', {
          'libraries': [
            '-ldl',
          ],
        }],
        ['use_system_icu==1', {
          # When using the system icu, the icu targets generate shim headers
          # which are included by public headers in the ui target, so we need
          # ui to be a hard dependency for all its users.
          'hard_dependency': 1,
        }],
        ['chromeos==1', {
          'dependencies': [
            '../../chromeos/chromeos.gyp:chromeos',
          ],
          'sources!': [
            'idle/idle_linux.cc',
            'idle/idle_query_x11.cc',
            'idle/idle_query_x11.h',
            'idle/screensaver_window_finder_x11.cc',
            'idle/screensaver_window_finder_x11.h',
          ],
        }],
      ],
    },
    {
      # GN version: //ui/base:test_support
      'target_name': 'ui_base_test_support',
      'type': 'static_library',
      'dependencies': [
        'ui_data_pack',
        '../../base/base.gyp:base',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../gfx/gfx.gyp:gfx',
        '../gfx/gfx.gyp:gfx_geometry',
        '../gfx/gfx.gyp:gfx_range',
      ],
      'sources': [
        # Note: file list duplicated in GN build.
        'test/ios/keyboard_appearance_listener.h',
        'test/ios/keyboard_appearance_listener.mm',
        'test/ios/ui_view_test_utils.h',
        'test/ios/ui_view_test_utils.mm',
        'test/material_design_controller_test_api.cc',
        'test/material_design_controller_test_api.h',
        'test/test_clipboard.cc',
        'test/test_clipboard.h',
        'test/ui_controls.h',
        'test/ui_controls_aura.cc',
        'test/ui_controls_internal_win.cc',
        'test/ui_controls_internal_win.h',
        'test/ui_controls_mac.mm',
        'test/ui_controls_win.cc',
      ],
      'include_dirs': [
        '../..',
      ],
      'conditions': [
        ['OS!="ios"', {
          'dependencies': [
            '../events/events.gyp:events',
            'ime/ui_base_ime.gyp:ui_base_ime',
          ],
          'sources': [
            'ime/dummy_input_method.cc',
            'ime/dummy_input_method.h',
            'ime/dummy_text_input_client.cc',
            'ime/dummy_text_input_client.h',
            'test/nswindow_fullscreen_notification_waiter.h',
            'test/nswindow_fullscreen_notification_waiter.mm',
            'test/scoped_fake_full_keyboard_access.h',
            'test/scoped_fake_full_keyboard_access.mm',
            'test/scoped_fake_nswindow_focus.h',
            'test/scoped_fake_nswindow_focus.mm',
            'test/scoped_fake_nswindow_fullscreen.h',
            'test/scoped_fake_nswindow_fullscreen.mm',
            'test/scoped_preferred_scroller_style_mac.h',
            'test/scoped_preferred_scroller_style_mac.mm',
            'test/windowed_nsnotification_observer.h',
            'test/windowed_nsnotification_observer.mm',
          ],
        }],
        ['use_aura==1', {
          'sources!': [
            'test/ui_controls_mac.mm',
            'test/ui_controls_win.cc',
          ],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="android"' , {
       'targets': [
         {
           # GN version: //ui/base:ui_base_jni_headers
           'target_name': 'ui_base_jni_headers',
           'type': 'none',
           'sources': [
             # Note: file list duplicated in GN build.
             '../android/java/src/org/chromium/ui/base/Clipboard.java',
             '../android/java/src/org/chromium/ui/base/DeviceFormFactor.java',
             '../android/java/src/org/chromium/ui/base/LocalizationUtils.java',
             '../android/java/src/org/chromium/ui/base/ResourceBundle.java',
             '../android/java/src/org/chromium/ui/base/SelectFileDialog.java',
             '../android/java/src/org/chromium/ui/base/TouchDevice.java',
           ],
           'variables': {
             'jni_gen_package': 'ui',
           },
           'includes': [ '../../build/jni_generator.gypi' ],
         },
       ],
    }],
  ],
}
