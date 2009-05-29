#ifndef JIFFY_H
#define JIFFY_H

/*
 * Jiffy - Fast, lighweight, and reentrant JSON stream parser.
 *  
 * Copyright (C) 2009 Paul Duncan <pabs@pablotron.org>
 *  
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *   
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the of the
 * Software.
 *    
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  
 *  
 */

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
#define JF_MAX_STACK_DEPTH  1024

/* 
 * Maximum buffer length: the longest literal number string allowed.
 *
 * Note: Reducing this value will reduce memory consumption, but strings
 * will fragment more frequently and it will reduce the length of the
 * longest literal number string.
 *
 */
#define JF_MAX_BUF_LEN      128

typedef struct jf_t_ jf_t;

/* 
 * jf_err_t - Full list of error codes returned by Jiffy.
 */
typedef enum {
  /* success */
  JF_OK, /* success (no error) */

  /* error code errors */
  JF_ERR_INVALID_ERROR_CODE, /* invalid error code */
  JF_ERR_INVALID_ERROR_BUFFER, /* buffer too small for error string */

  /* stack errors */
  JF_ERR_STACK_UNDERFLOW, /* stack underflow */
  JF_ERR_STACK_OVERFLOW, /* stack overflow */

  /* invalid state errors */
  JF_ERR_INVALID_STATE, /* invalid state (memory corruption?) */
  JF_ERR_INVALID_FINAL_STATE_WRONG_VALUE, /* wrong value (truncated string?) */
  JF_ERR_INVALID_FINAL_STATE_STACK_TOO_BIG, /* stack too big (truncated string?) */
  JF_ERR_INVALID_FINAL_STATE_STACK_TOO_SMALL, /* stack too small (truncated string?) */

  /* invalid token errors */
  JF_ERR_INVALID_TOKEN, /* invalid token */
  JF_ERR_INVALID_TOKEN_EXPECTED_PAREN_SPACE_EXPR, /* expected '(', ' ', or value */
  JF_ERR_INVALID_TOKEN_EXPECTED_CL_PAREN_SPACE, /* expected ')' or ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_PAREN_EXPR, /* expected '(' or value */
  JF_ERR_INVALID_TOKEN_EXPECTED_SPACE, /* expected ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_DOT, /* expected digit, 'e', or '.' */
  JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_END_NUM, /* expected digit, 'e', or end of number */
  JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_PLUS_MINUS, /* expected digit, '+', or '-' */
  JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_END_NUM, /* expected digit or end of number */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_U, /* expected 'u' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_L, /* expected 'l' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_R, /* expected 'r' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_E, /* expected 'e' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_A, /* expected 'a' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_S, /* expected 's' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_EXPR, /* expected ']' or value */
  JF_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_COMMA_SPACE, /* expected ']', ',', or ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_QUOTE_SPACE, /* expected '}', double quote, or ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_COLON_SPACE, /* expected ':' or ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_EXPR, /* expected value */
  JF_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_COMMA_SPACE, /* expected '}', ',', or ' ' */
  JF_ERR_INVALID_TOKEN_EXPECTED_HEX, /* expected hexadecimal value (0-9 or a-f) */
  JF_ERR_INVALID_TOKEN_EMBEDDED_CTRL_CHAR, /* embedded control character (e.g. unescaped newline, tab, etc) */
  JF_ERR_INVALID_TOKEN_BAD_UTF8_BYTE, /* invalid UTF-8 byte */
  JF_ERR_INVALID_TOKEN_BAD_ESCAPE_CHAR,  /* invalid backslash escape  */

  /* misc errors */
  JF_ERR_NUMBER_TOO_BIG, /* number string too long for buffer */
  JF_STOP, /* callback returned error */

  /* last error */
  JF_ERR_LAST
} jf_err_t;

/* 
 * jf_type_t - Full list token types emitted by Jiffy.
 */
typedef enum {
  /* object tokens */
  JF_TYPE_BGN_OBJECT,
  JF_TYPE_END_OBJECT,

  /* array tokens */
  JF_TYPE_BGN_ARRAY,
  JF_TYPE_END_ARRAY,

  /* string tokens */
  JF_TYPE_BGN_STRING,
  JF_TYPE_STRING_FRAGMENT,
  JF_TYPE_END_STRING,

  /* number tokens */
  JF_TYPE_INTEGER,
  JF_TYPE_FLOAT,

  /* literal tokens */
  JF_TYPE_TRUE,
  JF_TYPE_FALSE,
  JF_TYPE_NULL,

  JF_TYPE_LAST
} jf_type_t;

/* 
 * Ignore RFC3629 constraints on valid UTF-8 values.  Enable this flag
 * if you've got a JSON stream with invalid UTF-8 values.
 */
#define JF_FLAG_IGNORE_RFC3629 (1 << 0)

/* 
 * jf_cb_t - Parser callback prototype.
 */
typedef jf_err_t (*jf_cb_t)(jf_t *, jf_type_t, const uint8_t  *, const size_t);

/* 
 * jf_t - Main parser context.
 */
struct jf_t_ {
  /* user data (public, editable at any point) */
  void *user_data;

  /* parser callback (public, editable before first call to jf_parse() */
  jf_cb_t cb;

  /* parser flags (public, editable before first call to jf_parse()) */
  uint32_t flags;

  /* number of bytes parsed (public, read-only) */
  size_t num_bytes;

  /************************/
  /* private parser state */
  /************************/

  /* state stack (private) */
  char stack[JF_MAX_STACK_DEPTH];
  size_t sp;

  /* string/number buffer (private) */
  uint8_t buf[JF_MAX_BUF_LEN];
  size_t buf_len;
};

/* 
 * jf_version() - Get the version of Jiffy.
 *
 * Note: Returns an internal string that should not be modified or
 * freed.
 *
 */
const char *jf_version(void);

/* 
 * jf_strerror_r() - Populate buffer with description of error code.
 *
 * Returns JF_OK if the error code was valid and the buffer was large 
 * enough to hold the error message.
 *
 */
jf_err_t jf_strerror_r(jf_err_t err, char *buf, size_t buf_len);

/* 
 * jf_init() - Initialize parser context.
 *
 */
void jf_init(jf_t *, jf_cb_t);

/* 
 * jf_reset() - Reset given parser context.
 *
 */
void jf_reset(jf_t *);

/*
 * jf_parse() - Parse given JSON data with parser.
 *
 * Note: pasa a NULL buffer and a length of zero to indicate the final
 * block (or use `jf_done()`).
 */
jf_err_t jf_parse(jf_t *, const uint8_t *, const size_t);

/*
 * jf_done() - Mark parser as done.
 * 
 * Note: this is equivalent to calling jf_parse(parser, NULL, 0);
 */
jf_err_t jf_done(jf_t *);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* JIFFY_H */
