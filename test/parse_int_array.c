#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jiffy/jiffy.h>

#define UNUSED(a) ((void) (a))

/* global that stores the capture state */
static int capture = 0;

static void
run(jf_t *p, uint8_t *str, size_t str_len) {
  jf_err_t err;
  char buf[1024];

  if ((err = jf_parse(p, str, str_len)) != JF_OK) {
    /* 
     * if we got here, then there was a parsing error, 
     * so print the error message and exit
     */

    /* get human-readable error string */
    jf_strerror_r(err, buf, sizeof(buf));

    /* print error */
    fprintf(stderr, "ERROR: got \"%s\" at byte %lu\n", buf, p->num_bytes);

    /* exit program */
    exit(EXIT_FAILURE);
  }
}

static jf_err_t
parse_cb(jf_t *p, jf_type_t type, const char *val, const size_t val_len) {
  char buf[JF_MAX_BUF_LEN];
  UNUSED(p);

  switch (type) {
  case JF_TYPE_BGN_ARRAY:
    if (capture) {
      /* don't allow nested arrays */
      fprintf(stderr, "ERROR: nested array\n");

      /* stop parsing */
      return JF_STOP;
    }

    /* start capturing integers */
    capture = 1;

    break;
  case JF_TYPE_END_ARRAY:
    /* stop capturing integers */
    capture = 0;

    break;
  case JF_TYPE_INTEGER:
    if (capture > 0) {
      /* populate and null-terminate buffer */
      memcpy(buf, val, val_len);
      buf[val_len] = '\0';

      /* print integer */
      printf("%s\n", buf);
    } else {
      /* user entered a bare integer instead of an array */
      fprintf(stderr, "ERROR: bare integer (must be array of integers)\n");

      /* stop parsing */
      return JF_STOP;
    }

    break;
  default:
    fprintf(stderr, "ERROR: unknown item type: %d\n", type);

    /* stop parsing */
    return JF_STOP;
  }

  /* continue parsing */
  return JF_OK;
}

int main(int argc, char *argv[]) {
  uint8_t buf[BUFSIZ];
  size_t len;
  jf_t p;

  UNUSED(argc);
  UNUSED(argv);

  /* init parser */
  jf_init(&p, (jf_cb_t) parse_cb);
  
  /* read and parse standard input */
  while (!feof(stdin) && (len = fread(buf, 1, sizeof(buf), stdin)) > 0)
    run(&p, buf, len);

  /* finish parsing */
  run(&p, 0, 0);
  
  /* return success */
  return EXIT_SUCCESS;
}
