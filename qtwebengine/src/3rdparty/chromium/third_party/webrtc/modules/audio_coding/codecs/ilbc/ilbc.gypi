# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'targets': [
    {
      'target_name': 'iLBC',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/common_audio/common_audio.gyp:common_audio',
      ],
      'include_dirs': [
        'interface',
        '<(webrtc_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'interface',
          '<(webrtc_root)',
        ],
      },
      'sources': [
        'interface/ilbc.h',
        'abs_quant.c',
        'abs_quant_loop.c',
        'augmented_cb_corr.c',
        'bw_expand.c',
        'cb_construct.c',
        'cb_mem_energy.c',
        'cb_mem_energy_augmentation.c',
        'cb_mem_energy_calc.c',
        'cb_search.c',
        'cb_search_core.c',
        'cb_update_best_index.c',
        'chebyshev.c',
        'comp_corr.c',
        'constants.c',
        'create_augmented_vec.c',
        'decode.c',
        'decode_residual.c',
        'decoder_interpolate_lsf.c',
        'do_plc.c',
        'encode.c',
        'energy_inverse.c',
        'enh_upsample.c',
        'enhancer.c',
        'enhancer_interface.c',
        'filtered_cb_vecs.c',
        'frame_classify.c',
        'gain_dequant.c',
        'gain_quant.c',
        'get_cd_vec.c',
        'get_lsp_poly.c',
        'get_sync_seq.c',
        'hp_input.c',
        'hp_output.c',
        'ilbc.c',
        'index_conv_dec.c',
        'index_conv_enc.c',
        'init_decode.c',
        'init_encode.c',
        'interpolate.c',
        'interpolate_samples.c',
        'lpc_encode.c',
        'lsf_check.c',
        'lsf_interpolate_to_poly_dec.c',
        'lsf_interpolate_to_poly_enc.c',
        'lsf_to_lsp.c',
        'lsf_to_poly.c',
        'lsp_to_lsf.c',
        'my_corr.c',
        'nearest_neighbor.c',
        'pack_bits.c',
        'poly_to_lsf.c',
        'poly_to_lsp.c',
        'refiner.c',
        'simple_interpolate_lsf.c',
        'simple_lpc_analysis.c',
        'simple_lsf_dequant.c',
        'simple_lsf_quant.c',
        'smooth.c',
        'smooth_out_data.c',
        'sort_sq.c',
        'split_vq.c',
        'state_construct.c',
        'state_search.c',
        'swap_bytes.c',
        'unpack_bits.c',
        'vq3.c',
        'vq4.c',
        'window32_w32.c',
        'xcorr_coef.c',
        'abs_quant.h',
        'abs_quant_loop.h',
        'augmented_cb_corr.h',
        'bw_expand.h',
        'cb_construct.h',
        'cb_mem_energy.h',
        'cb_mem_energy_augmentation.h',
        'cb_mem_energy_calc.h',
        'cb_search.h',
        'cb_search_core.h',
        'cb_update_best_index.h',
        'chebyshev.h',
        'comp_corr.h',
        'constants.h',
        'create_augmented_vec.h',
        'decode.h',
        'decode_residual.h',
        'decoder_interpolate_lsf.h',
        'do_plc.h',
        'encode.h',
        'energy_inverse.h',
        'enh_upsample.h',
        'enhancer.h',
        'enhancer_interface.h',
        'filtered_cb_vecs.h',
        'frame_classify.h',
        'gain_dequant.h',
        'gain_quant.h',
        'get_cd_vec.h',
        'get_lsp_poly.h',
        'get_sync_seq.h',
        'hp_input.h',
        'hp_output.h',
        'defines.h',
        'index_conv_dec.h',
        'index_conv_enc.h',
        'init_decode.h',
        'init_encode.h',
        'interpolate.h',
        'interpolate_samples.h',
        'lpc_encode.h',
        'lsf_check.h',
        'lsf_interpolate_to_poly_dec.h',
        'lsf_interpolate_to_poly_enc.h',
        'lsf_to_lsp.h',
        'lsf_to_poly.h',
        'lsp_to_lsf.h',
        'my_corr.h',
        'nearest_neighbor.h',
        'pack_bits.h',
        'poly_to_lsf.h',
        'poly_to_lsp.h',
        'refiner.h',
        'simple_interpolate_lsf.h',
        'simple_lpc_analysis.h',
        'simple_lsf_dequant.h',
        'simple_lsf_quant.h',
        'smooth.h',
        'smooth_out_data.h',
        'sort_sq.h',
        'split_vq.h',
        'state_construct.h',
        'state_search.h',
        'swap_bytes.h',
        'unpack_bits.h',
        'vq3.h',
        'vq4.h',
        'window32_w32.h',
        'xcorr_coef.h',
     ], # sources
    }, # iLBC
  ], # targets
  'conditions': [
    ['include_tests==1', {
      'targets': [
        {
          'target_name': 'iLBCtest',
          'type': 'executable',
          'dependencies': [
            'iLBC',
          ],
          'sources': [
            'test/iLBC_test.c',
          ],
        }, # iLBCtest
      ], # targets
    }], # include_tests
  ], # conditions
}
