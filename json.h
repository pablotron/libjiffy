#ifndef JSON_H
#define JSON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h> /* for uint8_t, uint32_t, size_t */

/* 
 * Maximum stack size: this is approxmately the deepest level of 
 * data structure nesting allowed.  
 *
 * Note: Reducing this value will reduce memory consumption, but Jiffy
 * will be unable to handle deeply data structures.
 *
 * Note: You can lower this value to decrease memory use on embedded
 * systems, but you'll need to recompile Jiffy.
 *
 */
#define JSON_MAX_STACK_DEPTH  1024

/* 
 * Maximum buffer length: the longest literal number string allowed.
 *
 * Note: Reducing this value will reduce memory consumption, but strings
 * will fragment more frequently and it will reduce the length of the
 * longest literal number string.
 *
 */
#define JSON_MAX_BUF_LEN      128

typedef struct json_parser_t_ json_parser_t;

/* 
 * json_err_t - Full list of error codes returned by Jiffy.
 */
typedef enum {
  /* success */
  JSON_OK, /* success (no error) */

  /* error code errors */
  JSON_ERR_INVALID_ERROR_CODE, /* invalid error code */
  JSON_ERR_INVALID_ERROR_BUFFER, /* buffer too small for error string */

  /* stack errors */
  JSON_ERR_STACK_UNDERFLOW, /* stack underflow */
  JSON_ERR_STACK_OVERFLOW, /* stack overflow */

  /* invalid state errors */
  JSON_ERR_INVALID_STATE, /* invalid state (memory corruption?) */
  JSON_ERR_INVALID_FINAL_STATE_WRONG_VALUE, /* wrong value (truncated string?) */
  JSON_ERR_INVALID_FINAL_STATE_STACK_TOO_BIG, /* stack too big (truncated string?) */
  JSON_ERR_INVALID_FINAL_STATE_STACK_TOO_SMALL, /* stack too small (truncated string?) */

  /* invalid token errors */
  JSON_ERR_INVALID_TOKEN, /* invalid token */
  JSON_ERR_INVALID_TOKEN_EXPECTED_PAREN_SPACE, /* expected '(' or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CL_PAREN_SPACE, /* expected ')' or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_PAREN_EXPR, /* expected '(' or value */
  JSON_ERR_INVALID_TOKEN_EXPECTED_SPACE, /* expected ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_DOT, /* expected digit, 'e', or '.' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_END_NUM, /* expected digit, 'e', or end of number */
  JSON_ERR_INVALID_TOKEN_EXPECTED_DIGIT_PLUS_MINUS, /* expected digit, '+', or '-' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_DIGIT_END_NUM, /* expected digit or end of number */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_U, /* expected 'u' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_L, /* expected 'l' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_R, /* expected 'r' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_E, /* expected 'e' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_A, /* expected 'a' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CHAR_S, /* expected 's' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_EXPR, /* expected ']' or value */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_COMMA_SPACE, /* expected ']', ',', or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_QUOTE_SPACE, /* expected '}', double quote, or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_COLON_SPACE, /* expected ':' or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_EXPR, /* expected value */
  JSON_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_COMMA_SPACE, /* expected '}', ',', or ' ' */
  JSON_ERR_INVALID_TOKEN_EXPECTED_HEX, /* expected hexadecimal value (0-9 or a-f) */
  JSON_ERR_INVALID_TOKEN_EMBEDDED_CTRL_CHAR, /* embedded control character (e.g. unescaped newline, tab, etc) */
  JSON_ERR_INVALID_TOKEN_BAD_UTF8_BYTE, /* invalid UTF-8 byte */
  JSON_ERR_INVALID_TOKEN_BAD_ESCAPE_CHAR,  /* invalid backslash escape  */

  /* misc errors */
  JSON_ERR_NUMBER_TOO_BIG, /* number string too long for buffer */
  JSON_STOP, /* callback returned error */

  /* last error */
  JSON_ERR_LAST
} json_err_t;

/* 
 * json_type_t - Full list token types emitted by Jiffy.
 */
typedef enum {
  /* object tokens */
  JSON_TYPE_BGN_OBJECT,
  JSON_TYPE_END_OBJECT,

  /* array tokens */
  JSON_TYPE_BGN_ARRAY,
  JSON_TYPE_END_ARRAY,

  /* string tokens */
  JSON_TYPE_BGN_STRING,
  JSON_TYPE_STRING_FRAGMENT,
  JSON_TYPE_END_STRING,

  /* number tokens */
  JSON_TYPE_INTEGER,
  JSON_TYPE_FLOAT,

  /* literal tokens */
  JSON_TYPE_TRUE,
  JSON_TYPE_FALSE,
  JSON_TYPE_NULL,

  JSON_TYPE_LAST
} json_type_t;

/* 
 * Ignore RFC3629 constraints on valid UTF-8 values.  Enable this flag
 * if you've got a JSON stream with invalid UTF-8 values.
 */
#define JSON_FLAG_IGNORE_RFC3629 (1 << 0)

/* 
 * json_parser_cb_t - Parser callback prototype.
 */
typedef json_err_t (*json_parser_cb_t)(json_parser_t *, json_type_t, const uint8_t  *, const size_t);

/* 
 * json_parser_t - Main parser context.
 */
struct json_parser_t_ {
  /* user data (public, editable at any point) */
  void *user_data;

  /* parser callback (public, editable before first call to json_parse() */
  json_parser_cb_t cb;

  /* parser flags (public, editable before first call to json_parse()) */
  uint32_t flags;

  /* number of bytes parsed (public, read-only) */
  size_t num_bytes;

  /************************/
  /* private parser state */
  /************************/

  /* state stack (private) */
  char stack[JSON_MAX_STACK_DEPTH];
  size_t sp;

  /* string/number buffer (private) */
  uint8_t buf[JSON_MAX_BUF_LEN];
  size_t buf_len;
};

/* 
 * json_version() - Get the version of Jiffy.
 *
 * Note: Returns an internal string that should not be modified or
 * freed.
 *
 */
const char *json_version(void);

/* 
 * json_strerror_r() - Populate buffer with description of error code.
 *
 * Returns JSON_OK if the error code was valid and the buffer was large 
 * enough to hold the error message.
 *
 */
json_err_t json_strerror_r(json_err_t err, char *buf, size_t buf_len);

/* 
 * json_parser_init() - Initialize parser context.
 *
 */
void json_parser_init(json_parser_t *, json_parser_cb_t);

/* 
 * json_parser_reset() - Reset given parser context.
 *
 */
void json_parser_reset(json_parser_t *);

/*
 * json_parse() - Parse given JSON data with parser.
 */
json_err_t json_parse(json_parser_t *, const uint8_t *, const size_t, const int);


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* JSON_H */
