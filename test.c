#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "json.h"

#define INPUT_ERR_MSG "ERROR: Couldn't open input file '%s': %s\n"
#define UNUSED(a) ((void) (a))

static json_err_t
parser_cb(json_parser_t *p, json_type_t type, char *buf, size_t len) {
  UNUSED(p);
  UNUSED(buf);

  switch (type) {
  case JSON_TYPE_BGN_ARRAY:
    printf("got: bgn array\n");
    break;
  case JSON_TYPE_END_ARRAY:
    printf("got: end array\n");
    break;
  case JSON_TYPE_BGN_OBJECT:
    printf("got: bgn object\n");
    break;
  case JSON_TYPE_END_OBJECT:
    printf("got: end object\n");
    break;
  case JSON_TYPE_BGN_STRING:
    printf("got: bgn string\n");
    break;
  case JSON_TYPE_STRING_FRAGMENT:
    printf("got: string fragment (%lu).\n", len);
    break;
  case JSON_TYPE_END_STRING:
    printf("got: end string\n");
    break;
  case JSON_TYPE_INTEGER:
    printf("got: integer\n");
    break;
  case JSON_TYPE_FLOAT:
    printf("got: float\n");
    break;
  case JSON_TYPE_TRUE:
    printf("got: true\n");
    break;
  case JSON_TYPE_FALSE:
    printf("got: false\n");
    break;
  case JSON_TYPE_NULL:
    printf("got: null\n");
    break;
  default:
    fprintf(stderr, "ERROR: unknown JSON type: %d\n", type);
    return JSON_STOP;
  }

  return JSON_OK;
}

static void dump_stack(json_parser_t *p) {
  fprintf(
    stderr, 
    "DEBUG: offset = %lu, sp = %lu, stack = %s\n", 
    p->offset, p->sp, p->stack
  );
}

int main(int argc, char *argv[]) {
  uint8_t buf[BUFSIZ];
  char err_buf[1024];
  FILE *fh;
  size_t len;
  json_parser_t p;
  json_err_t err;

  /* init parser */
  json_parser_init(&p, (json_parser_cb_t) parser_cb);

  /* handle command-line arguments */
  if (argc < 2 || !strncmp("-", argv[1], 2)) {
    fh = stdin;
  } else if ((fh = fopen(argv[1], "rb")) == NULL) {
    fprintf(stderr, INPUT_ERR_MSG, argv[1], strerror(errno));
    return EXIT_FAILURE;;
  }
  
  /* read input file */
  while (!feof(fh) && (len = fread(buf, 1, sizeof(buf), fh)) > 0) {
    if ((err = json_parse(&p, buf, len, 1)) != JSON_OK) {
      json_strerror_r(err, err_buf, sizeof(err_buf));
      fprintf(stderr, "ERROR: parser error: %s\n", err_buf);
      dump_stack(&p);
      return EXIT_FAILURE;
    }
  }

  /* finish parsing */
  if ((err = json_parse(&p, 0, 0, 0)) != JSON_OK) {
    json_strerror_r(err, err_buf, sizeof(err_buf));
    fprintf(stderr, "ERROR: parser error: %s\n", err_buf);
    dump_stack(&p);
    return EXIT_FAILURE;
  }
  
  /* return success */
  return EXIT_SUCCESS;
}
