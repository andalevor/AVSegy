read_text_hdr = executable('read_text_hdr', ['read_text_hdr.c', '../src/AVSegyReader.c', '../src/AVSegyCommon.c', '../src/AVSegyUtils.c'],
  include_directories : inc)
test('Test SEGY text header reading', read_text_hdr,
  args : '../samples/ieee_single.sgy')
read_bin_hdr = executable('read_bin_hdr', ['read_bin_hdr.c', '../src/AVSegyReader.c', '../src/AVSegyCommon.c', '../src/AVSegyUtils.c'],
  include_directories : inc)
test('Test SEGY binary header reading', read_bin_hdr,
  args : '../samples/ieee_single.sgy')
read_headers_group = executable('read_headers_group', ['read_headers_group.c', '../src/AVSegyReader.c', '../src/AVSegyCommon.c', '../src/AVSegyUtils.c'],
  include_directories : inc)
test('Test SEGY trace headers reading by group', read_headers_group,
  args : '../samples/ieee_single.sgy')
read_headers_single = executable('read_headers_single', ['read_headers_single.c', '../src/AVSegyReader.c', '../src/AVSegyCommon.c', '../src/AVSegyUtils.c'],
  include_directories : inc)
test('Test SEGY trace headers reading by one', read_headers_single,
  args : '../samples/ieee_single.sgy')
read_samples = executable('read_samples', ['read_samples.c', '../src/AVSegyReader.c', '../src/AVSegyCommon.c', '../src/AVSegyUtils.c'],
  include_directories : inc)
test('Test SEGY trace samples reading', read_samples,
  args : '../samples/ieee_single.sgy')
