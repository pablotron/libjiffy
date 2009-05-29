
#include <ruby.h>
#include <jiffy/jiffy.h>

#define VERSION "0.2.0"
#define UNUSED(x) ((void) (x))

static VALUE mJiffy,
             eError,
             cParser;

/**********************/
/* CONNECTION METHODS */
/**********************/
static void 
jfr_parser_free(void *conn)
{
  xfree(conn);
}

static VALUE 
jfr_parser_s_alloc(VALUE klass)
{
  jf_t *parser = ALLOC(jf_t);
  memset(parser, 0, sizeof(jf_t));
  return Data_Wrap_Struct(klass, 0, jfr_parser_free, parser);
}

#ifndef HAVE_RB_DEFINE_ALLOC_FUNC
static VALUE jfr_parser_s_new(int argc, VALUE *argv, VALUE klass)
{
  VALUE self = jfr_parser_s_alloc(klass);

  rb_obj_call_init(self, argc, argv);
  return self;
}
#endif

static jf_err_t 
jfr_parse_cb(jf_t *p, jf_type_t type, uint8_t *jf_buf, size_t jf_buf_len) {
  VALUE ary[2];

  if (p->user_data) {
    /* populate arguments */
    ary[0] = INT2FIX(type);
    ary[1] = (jf_buf_len > 0) ? rb_str_new(jf_buf, jf_buf_len) : Qnil;

    /* call parse proc */
    rb_proc_call((VALUE) p->user_data, rb_ary_new4(2, ary));
  }

  /* FIXME: handle exceptions */
  return JF_OK;
}

static VALUE 
jfr_parser_init(int argc, VALUE *argv, VALUE self)
{
  jf_t *parser;

  Data_Get_Struct(self, jf_t, parser);
  jf_init(parser, (jf_cb_t) jfr_parse_cb);

  if (rb_block_given_p())
    parser->user_data = (void*) rb_block_proc();

  switch (argc) {
  case 0:
    /* do nothing */
    break;
  case 1:
    /* FIXME: add support for flags */
    break;
  default:
    rb_raise(rb_eArgError, "invalid argument count (not 0 or 1)");
  }
  
  return self;
}

static VALUE
jfr_parser_parse(VALUE self, VALUE str) {
  jf_err_t err;
  jf_t *parser;
  char err_buf[1024];

  Data_Get_Struct(self, jf_t, parser);

  if (rb_block_given_p())
    parser->user_data = (void*) rb_block_proc();

  if (str != Qnil)
    err = jf_parse(parser, RSTRING(str)->ptr, RSTRING(str)->len, 1);
  else
    err = jf_parse(parser, 0, 0, 0);

  if (err != JF_OK) {
    jf_strerror_r(err, err_buf, sizeof(err_buf));
    rb_raise(eError, err_buf);
  }

  return INT2FIX(parser->num_bytes);
}

static VALUE
jfr_parser_num_bytes(VALUE self) {
  jf_t *parser;
  Data_Get_Struct(self, jf_t, parser);
  return INT2FIX(parser->num_bytes);
}

void Init_jiffy(void) {
  mJiffy = rb_define_module("Jiffy");

  rb_define_const(mJiffy, "VERSION", rb_str_new2(jf_version()));

  /* object tokens */
  rb_define_const(mJiffy, "TYPE_BGN_OBJECT", INT2FIX(JF_TYPE_BGN_OBJECT));
  rb_define_const(mJiffy, "TYPE_END_OBJECT", INT2FIX(JF_TYPE_END_OBJECT));

  /* array tokens */
  rb_define_const(mJiffy, "TYPE_BGN_ARRAY", INT2FIX(JF_TYPE_BGN_ARRAY));
  rb_define_const(mJiffy, "TYPE_END_ARRAY", INT2FIX(JF_TYPE_END_ARRAY));

  /* string tokens */
  rb_define_const(mJiffy, "TYPE_BGN_STRING", INT2FIX(JF_TYPE_BGN_STRING));
  rb_define_const(mJiffy, "TYPE_STRING_FRAGMENT", INT2FIX(JF_TYPE_STRING_FRAGMENT));
  rb_define_const(mJiffy, "TYPE_END_STRING", INT2FIX(JF_TYPE_END_STRING));

  /* number tokens */
  rb_define_const(mJiffy, "TYPE_INTEGER", INT2FIX(JF_TYPE_INTEGER));
  rb_define_const(mJiffy, "TYPE_FLOAT", INT2FIX(JF_TYPE_FLOAT));

  /* literal tokens */
  rb_define_const(mJiffy, "TYPE_TRUE", INT2FIX(JF_TYPE_TRUE));
  rb_define_const(mJiffy, "TYPE_FALSE", INT2FIX(JF_TYPE_FALSE));
  rb_define_const(mJiffy, "TYPE_NULL", INT2FIX(JF_TYPE_NULL));


  eError = rb_define_class_under(mJiffy, "Error", rb_eStandardError);
  cParser = rb_define_class_under(mJiffy, "Parser", rb_cData);

#ifdef HAVE_RB_DEFINE_ALLOC_FUNC
  rb_define_alloc_func(cParser, jfr_parser_s_alloc);
#else
  rb_define_singleton_method(cConn, "new", jfr_parser_s_new, 0);
#endif

  rb_define_method(cParser, "initialize", jfr_parser_init, -1);
  rb_define_method(cParser, "parse", jfr_parser_parse, 1);
  rb_define_method(cParser, "num_bytes", jfr_parser_num_bytes, 0);
}
