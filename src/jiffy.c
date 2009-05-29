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

#include <string.h> /* for memset() */
#include <jiffy/jiffy.h>

/*
 * static list of error strings 
 * (automagically generated from json.h; see the jf_err_t enum)
 */
static const char *
errors[] = {
  /* success */
  "success (no error)",

  /* error code errors */
  "invalid error code",
  "buffer too small for error string",

  /* stack errors */
  "stack underflow",
  "stack overflow",

  /* invalid state errors */
  "invalid state (memory corruption?)",
  "wrong value (truncated string?)",
  "stack too big (truncated string?)",
  "stack too small (truncated string?)",

  /* invalid token errors */
  "invalid token",
  "expected '(', ' ', or value",
  "expected ')' or ' '",
  "expected '(' or value",
  "expected ' '",
  "expected digit, 'e', or '.'",
  "expected digit, 'e', or end of number",
  "expected digit, '+', or '-'",
  "expected digit or end of number",
  "expected 'u'",
  "expected 'l'",
  "expected 'r'",
  "expected 'e'",
  "expected 'a'",
  "expected 's'",
  "expected ']' or value",
  "expected ']', ',', or ' '",
  "expected '}', double quote, or ' '",
  "expected ':' or ' '",
  "expected value",
  "expected '}', ',', or ' '",
  "expected hexadecimal value (0-9 or a-f)",
  "embedded control character (e.g. unescaped newline, tab, etc)",
  "invalid UTF-8 byte",
  "invalid backslash escape ",

  /* misc errors */
  "number string too long for buffer",
  "callback returned error",

  /* last error (sentinel) */
  NULL
};

static const char *
version = "0.1.0";

const char *
jf_version(void) {
  return version;
}

jf_err_t
jf_strerror_r(jf_err_t err, char *buf, size_t buf_len) {
  size_t len;

  /* check error code */
  if (err >= JF_ERR_LAST)
    return JF_ERR_INVALID_ERROR_CODE;

  /* get length of error string */
  len = strlen(errors[err]) + 1;
  
  /* check buffer length */
  if (buf_len < len)
    return JF_ERR_INVALID_ERROR_BUFFER;

  /* copy error string to output buffer */
  memcpy(buf, errors[err], len);

  /* return success */
  return JF_OK;
}

void
jf_init(jf_t *p, jf_cb_t cb) {
  memset(p, 0, sizeof(jf_t));
  p->cb = cb;
}

#if 0
void
jf_reset(jf_t *p) {
  jf_init(p, p->cb);
}
#endif /* 0 */

#define MASK(bits, shift) (((1 << (bits)) - 1) << (shift))
#define FROM_HEX(c) MASK(4, 0) & (((c) >= '0' && (c) <= '9') ? ((c) - '0') : (((c) - 'a') + 10))

/* FIXME: i don't think this is working right */
static jf_err_t
decode_utf8(jf_t *p) {
  uint8_t *u = p->buf + p->buf_len - 4;
  uint32_t v;

  /* decode value */
  v = (FROM_HEX(u[0]) << 12) | 
      (FROM_HEX(u[1]) <<  8) |
      (FROM_HEX(u[2]) <<  4) |
      (FROM_HEX(u[3])      );

  /* pop 4 characters from buffer */
  p->buf_len -= 4;

  if (v < 0x80) {
    /* one-byte sequence */

    p->buf[p->buf_len] = (uint8_t) v;
    p->buf_len += 1;
  } else if (v < 0x800) {
    /* two-byte sequence */

    p->buf[p->buf_len]     = MASK(2, 6) |
                             ((v & MASK(3, 8)) >> 8) << 2 |
                             ((v & MASK(2, 6)) >> 6);
    p->buf[p->buf_len + 1] = (1 << 7) |
                             (v & MASK(6, 0));

    p->buf_len += 2;
  } else if (v < 0x10000) {
    /* three-byte sequence */

    p->buf[p->buf_len]     = MASK(3, 5) | 
                             (v & MASK(4, 12)) >> 12;
    p->buf[p->buf_len + 1] = (1 << 7) | 
                             (v & MASK(4, 8)) >> 6 |
                             (v & MASK(2, 6)) >> 6;
    p->buf[p->buf_len + 2] = (1 << 7) | 
                             (v & MASK(6, 0));


    p->buf_len += 3;
  } else if (v < 0x10ffff) {
    /* four-byte sequence */
    /* FIXME: these are impossible, why do i support them? */

    p->buf[p->buf_len]     = MASK(4, 4) | 
                             (v & MASK(3, 18)) >> 18;
    p->buf[p->buf_len + 1] = (1 << 7) | 
                             (v & MASK(2, 16)) >> 12 |
                             (v & MASK(4, 12)) >> 12;
    p->buf[p->buf_len + 2] = (1 << 7) | 
                             (v & MASK(4, 8)) >> 6 | 
                             (v & MASK(2, 6)) >> 6;
    p->buf[p->buf_len + 3] = (1 << 7) | 
                             (v & MASK(6, 0));

    p->buf_len += 4;
  } else {
    /* invalid unicode sequence */
  }


  return JF_OK;
}

#define PUSH_STATE(ps, state) do {                  \
  /* check for stack overflow */                    \
  if ((ps)->sp + 1 >= JF_MAX_STACK_DEPTH)           \
    return JF_ERR_STACK_OVERFLOW;                   \
                                                    \
  /* push state */                                  \
  (ps)->stack[(ps)->sp] = (state);                  \
                                                    \
  /* increment stack pointer */                     \
  (ps)->sp++;                                       \
} while (0)

#define POP_STATE(ps) do {                          \
  /* check for stack underflow */                   \
  if (!(ps)->sp)                                    \
    return JF_ERR_STACK_UNDERFLOW;                  \
                                                    \
  /* decriment stack pointer */                     \
  (ps)->sp--;                                       \
} while (0)

#define SEND_FULL(ps, type, str, str_len) do {      \
  if ((ps)->cb) {                                   \
    err = (ps)->cb((ps), (type), (str), (str_len)); \
                                                    \
    if (err != JF_OK)                               \
      return err;                                   \
  }                                                 \
} while (0)

#define SEND(ps, type) SEND_FULL(ps, type, 0, 0)

#define SEND_STRING_FRAGMENT(ps) do {               \
  if ((ps)->buf_len > 0) {                          \
    SEND_FULL(                                      \
      (ps), JF_TYPE_STRING_FRAGMENT,                \
      (ps)->buf, (ps)->buf_len                      \
    );                                              \
                                                    \
    (ps)->buf_len = 0;                              \
  }                                                 \
} while (0)

#define PUSH_CHAR(ps, c) do {                       \
  if ((ps)->buf_len + 1 >= JF_MAX_BUF_LEN)          \
    SEND_STRING_FRAGMENT(ps);                       \
  (ps)->buf[(ps)->buf_len++] = (c);                 \
} while (0)

#define PUSH_NUM(ps, c) do {                        \
  if ((ps)->buf_len + 1 >= JF_MAX_BUF_LEN)          \
    return JF_ERR_NUMBER_TOO_BIG;                   \
  (ps)->buf[(ps)->buf_len++] = (c);                 \
} while (0)

#define CASE_WHITESPACE                             \
  case ' ':                                         \
  case '\b':                                        \
  case '\f':                                        \
  case '\t':                                        \
  case '\n':                                        \
  case '\r':                                        \
  case '\v':                

#define CASE_DIGIT                                  \
  case '0':                                         \
  case '1':                                         \
  case '2':                                         \
  case '3':                                         \
  case '4':                                         \
  case '5':                                         \
  case '6':                                         \
  case '7':                                         \
  case '8':                                         \
  case '9':                 

#define CASE_HEX                                    \
  CASE_DIGIT                                        \
  case 'a':                                         \
  case 'b':                                         \
  case 'c':                                         \
  case 'd':                                         \
  case 'e':                                         \
  case 'f':

#define CASE_END_NUM                                \
  CASE_WHITESPACE                                   \
  case ',':                                         \
  case ']':                                         \
  case '}':                                         \
  case ')':

#define ACCEPT_EXPR(ps, buffer, d)                  \
  CASE_WHITESPACE                                   \
    /* ignore whitespace */                         \
    break;                                          \
  case '{':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'o');                          \
    SEND((ps), JF_TYPE_BGN_OBJECT);                 \
                                                    \
    break;                                          \
  case '[':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'a');                          \
    SEND((ps), JF_TYPE_BGN_ARRAY);                  \
                                                    \
    break;                                          \
  case '"':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 's');                          \
    SEND((ps), JF_TYPE_BGN_STRING);                 \
                                                    \
    break;                                          \
  case 't':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'T');                          \
                                                    \
    break;                                          \
  case 'f':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'F');                          \
                                                    \
    break;                                          \
  case 'n':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'N');                          \
                                                    \
    break;                                          \
  CASE_DIGIT                                        \
  case '-':                                         \
    if (d)                                          \
      PUSH_STATE((ps), (d));                        \
                                                    \
    PUSH_STATE((ps), 'n');                          \
    (ps)->buf_len = 1;                              \
    (ps)->buf[0] = (buffer)[i];                     \
                                                    \
    break;

#define CHECK_UTF8_BYTE(ps, ch) do {                \
  if (((ch) > 0x7f) && !((ps)->flags & JF_FLAG_IGNORE_RFC3629)) {  \
    if (((ch) >= 0xc0 && (ch) <= 0xc1) ||           \
        ((ch) >= 0xf5 && (ch) <= 0xf7) ||           \
        ((ch) >= 0xf8 && (ch) <= 0xfb) ||           \
        ((ch) >= 0xfc && (ch) <= 0xfd) ||           \
        ((ch) >= 0xfe /* && (ch) <= 0xff */))       \
      return JF_ERR_INVALID_TOKEN_BAD_UTF8_BYTE;    \
  }                                                 \
} while (0)
  

jf_err_t
jf_parse(jf_t *p, const uint8_t *buf, const size_t buf_len) {
  size_t i, base;
  jf_err_t err;

  /* save initial byte count */
  base = p->num_bytes;

  /* iterate over each character in buffer */
  for (i = 0; i < buf_len; i++) {
    /* add to byte count */
    p->num_bytes = base + i;

retry:
    if (!p->sp) {
      /* no state; look for opening parenthesis */

      switch (buf[i]) {
      ACCEPT_EXPR(p, buf, ' ')
      case '(':
        /* push init state */
        PUSH_STATE(p, 'i');

        break;
      default:
        return JF_ERR_INVALID_TOKEN_EXPECTED_PAREN_SPACE_EXPR;
      }
    } else {
      switch (p->stack[p->sp - 1]) {

      /**************/
      /* init state */
      /**************/

      case 'i':
        switch (buf[i]) {
        ACCEPT_EXPR(p, buf, 'f')
        case ')':
          POP_STATE(p);
          PUSH_STATE(p, ' ');
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_PAREN_EXPR;
        }

        break;

      /**************/
      /* fini state */
      /**************/

      case 'f':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        case ')':
          POP_STATE(p);
          POP_STATE(p);
          PUSH_STATE(p, ' ');
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_CL_PAREN_SPACE;
        }

        break;

      case ' ':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_SPACE;
        }

        break;

      /*****************/
      /* number states */
      /*****************/

      case 'n':
        switch (buf[i]) {
        CASE_DIGIT
          PUSH_NUM(p, buf[i]);
          break;
        case '.':
          /* handle decimal */
          POP_STATE(p);
          PUSH_STATE(p, 'd');
          PUSH_NUM(p, buf[i]);
          break;
        case 'e':
        case 'E':
          /* handle exponent */
          POP_STATE(p);
          PUSH_STATE(p, 'e');
          PUSH_NUM(p, 'e');
          break;
        CASE_END_NUM
          /* send number */
          SEND_FULL(p, JF_TYPE_INTEGER, p->buf, p->buf_len);
          p->buf_len = 0;

          /* pop state and retry token */
          POP_STATE(p);
          goto retry;

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_DOT;
        }

        break;
      case 'd':
        switch (buf[i]) {
        CASE_DIGIT
          PUSH_NUM(p, buf[i]);
          break;
        case 'e':
        case 'E':
          /* handle exponent */
          POP_STATE(p);
          PUSH_STATE(p, 'e');
          PUSH_NUM(p, 'e');
          break;
        CASE_END_NUM
          /* send number */
          SEND_FULL(p, JF_TYPE_FLOAT, p->buf, p->buf_len);
          p->buf_len = 0;

          /* pop state and retry token */
          POP_STATE(p);
          goto retry;

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_E_END_NUM;
        }

        break;
      case 'e':
        switch (buf[i]) {
        CASE_DIGIT
        case '+':
        case '-':
          POP_STATE(p);
          PUSH_STATE(p, 'g');
          PUSH_NUM(p, buf[i]);
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_PLUS_MINUS;
        }

        break;
      case 'g':
        switch (buf[i]) {
        CASE_DIGIT
          PUSH_NUM(p, buf[i]);
          break;
        CASE_END_NUM
          /* send number */
          SEND_FULL(p, JF_TYPE_FLOAT, p->buf, p->buf_len);
          p->buf_len = 0;

          /* pop state and retry token */
          POP_STATE(p);
          goto retry;

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_DIGIT_END_NUM;
        }

        break;
      /*****************/
      /* "null" states */
      /*****************/

      case 'N':
        if (buf[i] == 'u') {
          PUSH_STATE(p, 'U');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_U;
        }

        break;
      case 'U':
        if (buf[i] == 'l') {
          PUSH_STATE(p, 'L');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_L;
        }

        break;
      case 'L':
        if (buf[i] == 'l') {
          SEND(p, JF_TYPE_NULL);

          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_L;
        }

        break;

      /*****************/
      /* "true" states */
      /*****************/

      case 'T':
        if (buf[i] == 'r') {
          PUSH_STATE(p, 'R');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_R;
        }

        break;
      case 'R':
        if (buf[i] == 'u') {
          PUSH_STATE(p, 'W');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_U;
        }

        break;
      case 'W':
        if (buf[i] == 'e') {
          SEND(p, JF_TYPE_TRUE);

          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_E;
        }

        break;

      /******************/
      /* "false" states */
      /******************/

      case 'F':
        if (buf[i] == 'a') {
          PUSH_STATE(p, 'A');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_A;
        }

        break;
      case 'A':
        if (buf[i] == 'l') {
          PUSH_STATE(p, 'M');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_L;
        }

        break;
      case 'M':
        if (buf[i] == 's') {
          PUSH_STATE(p, 'S');
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_S;
        }

        break;
      case 'S':
        if (buf[i] == 'e') {
          SEND(p, JF_TYPE_FALSE);

          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);
        } else {
          return JF_ERR_INVALID_TOKEN_EXPECTED_CHAR_E;
        }

        break;

      /***************/
      /* array state */
      /***************/

      case 'a':
        switch (buf[i]) {
        ACCEPT_EXPR(p, buf, ',')
        case ']':
          SEND(p, JF_TYPE_END_ARRAY);
          
          POP_STATE(p);

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_EXPR;
        }

        break;
      case ',':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        case ',':
          POP_STATE(p);
          break;
        case ']':
          POP_STATE(p);
          goto retry;
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_CL_BRACKET_COMMA_SPACE;
        }

        break;

      /****************/
      /* object state */
      /****************/

      case 'o':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        case '"':
          PUSH_STATE(p, ':');
          PUSH_STATE(p, 's');

          SEND(p, JF_TYPE_BGN_STRING);

          break;
        case '}':
          POP_STATE(p);

          SEND(p, JF_TYPE_END_OBJECT);

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_QUOTE_SPACE;
        }

        break;
      case ':':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        case ':':
          PUSH_STATE(p, 'v');
          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_COLON_SPACE;
        }

        break;
      case 'v':
        switch (buf[i]) {
        ACCEPT_EXPR(p, buf, 'c')
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_EXPR;
        }

        break;
      case 'c':
        switch (buf[i]) {
        CASE_WHITESPACE
          /* ignore whitespace */
          break;
        case ',':
          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);

          break;
        case '}':
          POP_STATE(p);
          POP_STATE(p);
          POP_STATE(p);
          goto retry;

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_CL_SQ_BRACKET_COMMA_SPACE;
        }

        break;

      /****************/
      /* string state */
      /****************/

      case 's':
        /* check for control characters */
        if (buf[i] >= ' ') {
          CHECK_UTF8_BYTE(p, buf[i]);

          switch (buf[i]) {
          case '"':
            SEND_STRING_FRAGMENT(p);
            SEND(p, JF_TYPE_END_STRING);
            POP_STATE(p);
            break;
          case '\\':
            PUSH_STATE(p, '\\');
            break;
          default:
            PUSH_CHAR(p, buf[i]);
          }
        } else {
          return JF_ERR_INVALID_TOKEN_EMBEDDED_CTRL_CHAR;
        }

        break;
      case '\\':
        switch (buf[i]) {
        case '"':
        case '/':
        case '\\':
          PUSH_CHAR(p, buf[i]);
          POP_STATE(p);
          break;
        case 'b':
          PUSH_CHAR(p, '\b');
          POP_STATE(p);
          break;
        case 'f':
          PUSH_CHAR(p, '\f');
          POP_STATE(p);
          break;
        case 'n':
          PUSH_CHAR(p, '\n');
          POP_STATE(p);
          break;
        case 'r':
          PUSH_CHAR(p, '\r');
          POP_STATE(p);
          break;
        case 't':
          PUSH_CHAR(p, '\t');
          POP_STATE(p);
          break;
        case 'u':
          /* handle unicode escape */
          POP_STATE(p);
          PUSH_STATE(p, 'u');
          break;
        default:
          return JF_ERR_INVALID_TOKEN_BAD_ESCAPE_CHAR;
        }

        break;
      case 'u':
        switch (buf[i]) {
        CASE_HEX
          /* flush pending string fragment so we don't end up on a 
           * buffer boundary when mucking around with utf8 chars */
          SEND_STRING_FRAGMENT(p);

          PUSH_CHAR(p, buf[i]);
          POP_STATE(p);
          PUSH_STATE(p, '1');

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_HEX;
        };

        break;
      case '1':
        switch (buf[i]) {
        CASE_HEX
          PUSH_CHAR(p, buf[i]);
          POP_STATE(p);
          PUSH_STATE(p, '2');

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_HEX;
        };

        break;
      case '2':
        switch (buf[i]) {
        CASE_HEX
          PUSH_CHAR(p, buf[i]);
          POP_STATE(p);
          PUSH_STATE(p, '3');

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_HEX;
        };

        break;
      case '3':
        switch (buf[i]) {
        CASE_HEX
          PUSH_CHAR(p, buf[i]);
          
          /* decode unicode hex sequence */
          if ((err = decode_utf8(p)) != JF_OK)
            return err;

          POP_STATE(p);

          break;
        default:
          return JF_ERR_INVALID_TOKEN_EXPECTED_HEX;
        };

        break;
      default:
        /* unknown state? probably memory corruption */
        return JF_ERR_INVALID_STATE;
      }
    }
  }

  /* save final byte count */
  p->num_bytes = base + buf_len;

  /* if this is the final block, then make sure the stack is sane */
  if (!buf && !buf_len) {
    if (p->sp == 1) {
      /* check for final state */
      if (p->stack[0] != ' ')
        return JF_ERR_INVALID_FINAL_STATE_WRONG_VALUE;
    } else if (p->sp > 1) {
      return JF_ERR_INVALID_FINAL_STATE_STACK_TOO_BIG;
    } else {
      return JF_ERR_INVALID_FINAL_STATE_STACK_TOO_SMALL;
    }
  }
  
  /* return success */
  return JF_OK;
}
