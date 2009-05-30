#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jiffy/jiffy.h>

#define INPUT_ERR_MSG "ERROR: Couldn't open input file '%s': %s\n"
#define UNUSED(a) ((void) (a))

static void 
dump_stack(jf_t *p) {
  fprintf(
    stderr, 
    "DEBUG: num_bytes = %lu, sp = %lu, stack = %s\n", 
    p->num_bytes, p->sp, p->stack
  );
}

static void
print_error_and_die(jf_t *p, jf_err_t err) {
  char buf[1024];

  jf_strerror_r(err, buf, sizeof(buf));
  fprintf(stderr, "ERROR: parser error: %s\n", buf);
  dump_stack(p);

  exit(EXIT_FAILURE);
}

static jf_err_t
parse_cb(jf_t *p, jf_type_t type, const char *jf_buf, const size_t jf_buf_len) {
  char val[JF_MAX_BUF_LEN];

  printf("[%4lu] ", p->num_bytes);

  switch (type) {
  case JF_TYPE_BGN_ARRAY:
    printf("begin array\n");
    break;
  case JF_TYPE_END_ARRAY:
    printf("end array\n");
    break;
  case JF_TYPE_BGN_OBJECT:
    printf("begin object\n");
    break;
  case JF_TYPE_END_OBJECT:
    printf("end object\n");
    break;
  case JF_TYPE_BGN_STRING:
    printf("begin string\n");
    break;
  case JF_TYPE_STRING_FRAGMENT:
    printf("string fragment (%lu)\n", jf_buf_len);
    break;
  case JF_TYPE_END_STRING:
    printf("end string\n");
    break;
  case JF_TYPE_INTEGER:
    printf("integer\n");
    break;
  case JF_TYPE_FLOAT:
    printf("float\n");
    break;
  case JF_TYPE_TRUE:
    printf("true\n");
    break;
  case JF_TYPE_FALSE:
    printf("false\n");
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

int main(int argc, char *argv[]) {
  uint8_t buf[BUFSIZ];
  size_t len;
  FILE *fh;
  jf_err_t err;
  jf_t p;

  /* init parser */
  jf_init(&p, (jf_cb_t) parse_cb);

  /* handle command-line arguments */
  if (argc < 2 || !strncmp("-", argv[1], 2)) {
    /* 
     * if there were no arguments or the first argument is a dash, 
     * then read input from stdin 
     */
    fh = stdin;
  } else if ((fh = fopen(argv[1], "rb")) == NULL) {
    /* 
     * if we couldn't open the input file, then print an error
     * explaining why and exit
     */
    fprintf(stderr, INPUT_ERR_MSG, argv[1], strerror(errno));
    return EXIT_FAILURE;;
  }
  
  /* read input file */
  while (!feof(fh) && (len = fread(buf, 1, sizeof(buf), fh)) > 0)
    if ((err = jf_parse(&p, buf, len)) != JF_OK)
      print_error_and_die(&p, err);

  /* finish parsing */
  if ((err = jf_done(&p)) != JF_OK)
    print_error_and_die(&p, err);
  
  /* close input file */
  if (fh != stdin)
    fclose(fh);

  /* return success */
  return EXIT_SUCCESS;
}
