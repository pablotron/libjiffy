#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jiffy/jiffy.h>

#define INPUT_ERR_MSG "ERROR: Couldn't open input file '%s': %s\n"
#define UNUSED(a) ((void) (a))

static jf_err_t
parse_cb(jf_t *p, jf_type_t type, char *buf, size_t len) {
  UNUSED(p);
  UNUSED(buf);

  switch (type) {
  case JF_TYPE_BGN_ARRAY:
    printf("got: bgn array\n");
    break;
  case JF_TYPE_END_ARRAY:
    printf("got: end array\n");
    break;
  case JF_TYPE_BGN_OBJECT:
    printf("got: bgn object\n");
    break;
  case JF_TYPE_END_OBJECT:
    printf("got: end object\n");
    break;
  case JF_TYPE_BGN_STRING:
    printf("got: bgn string\n");
    break;
  case JF_TYPE_STRING_FRAGMENT:
    printf("got: string fragment (%lu).\n", len);
    break;
  case JF_TYPE_END_STRING:
    printf("got: end string\n");
    break;
  case JF_TYPE_INTEGER:
    printf("got: integer\n");
    break;
  case JF_TYPE_FLOAT:
    printf("got: float\n");
    break;
  case JF_TYPE_TRUE:
    printf("got: true\n");
    break;
  case JF_TYPE_FALSE:
    printf("got: false\n");
    break;
  case JF_TYPE_NULL:
    printf("got: null\n");
    break;
  default:
    fprintf(stderr, "ERROR: unknown JSON type: %d\n", type);
    return JF_STOP;
  }

  return JF_OK;
}

static void dump_stack(jf_t *p) {
  fprintf(
    stderr, 
    "DEBUG: num_bytes = %lu, sp = %lu, stack = %s\n", 
    p->num_bytes, p->sp, p->stack
  );
}

int main(int argc, char *argv[]) {
  uint8_t buf[BUFSIZ];
  char err_buf[1024];
  FILE *fh;
  size_t len;
  jf_t p;
  jf_err_t err;

  /* init parser */
  jf_init(&p, (jf_cb_t) parse_cb);

  /* handle command-line arguments */
  if (argc < 2 || !strncmp("-", argv[1], 2)) {
    fh = stdin;
  } else if ((fh = fopen(argv[1], "rb")) == NULL) {
    fprintf(stderr, INPUT_ERR_MSG, argv[1], strerror(errno));
    return EXIT_FAILURE;;
  }
  
  /* read input file */
  while (!feof(fh) && (len = fread(buf, 1, sizeof(buf), fh)) > 0) {
    if ((err = jf_parse(&p, buf, len, 1)) != JF_OK) {
      jf_strerror_r(err, err_buf, sizeof(err_buf));
      fprintf(stderr, "ERROR: parser error: %s\n", err_buf);
      dump_stack(&p);
      return EXIT_FAILURE;
    }
  }

  /* finish parsing */
  if ((err = jf_parse(&p, 0, 0, 0)) != JF_OK) {
    jf_strerror_r(err, err_buf, sizeof(err_buf));
    fprintf(stderr, "ERROR: parser error: %s\n", err_buf);
    dump_stack(&p);
    return EXIT_FAILURE;
  }
  
  /* return success */
  return EXIT_SUCCESS;
}
