#include "AVSegyCommon.h"
#include "AVSegyReader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s file_name.sgy\n", argv[0]);
    return 1;
  }
  AVSegyReader *sgy = av_segy_reader_new();
  if (!sgy) {
    printf("Can not allocate memory for SEGY reader struct\n");
    return 1;
  }
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  char *buf = NULL;
  if (av_segy_reader_open(sgy, argv[1]))
    goto error;
  buf = (char *)malloc(AV_SEGY_TEXT_HEADER_SIZE);
  if (!buf)
    goto error;
  if (av_segy_reader_read_text_header(sgy, buf))
    goto error;
  for (int i = 0; i < 40; ++i)
    printf("%.*s\n", 80, buf + i * 80);
  if (strncmp(buf, av_segy_default_text_header, AV_SEGY_TEXT_HEADER_SIZE))
    goto error;
  free(buf);
  av_segy_reader_delete(&sgy);
  return 0;
error:
  printf("%s\n", err->message);
  av_segy_reader_delete(&sgy);
  if (buf)
    free(buf);
  return 1;
}
