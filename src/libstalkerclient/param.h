/* 
 * File:   param.h
 * Author: kenji123
 *
 * Created on April 9, 2015, 10:12 PM
 */

#ifndef PARAM_H
#define	PARAM_H

#include <stdbool.h>

#include "action.h"

#ifdef	__cplusplus
extern "C" {
#endif

  typedef enum {
    SC_STRING,
    SC_INTEGER,
    SC_BOOLEAN
  } sc_param_type_t;

  struct sc_param;

  typedef struct sc_param {
    const char *name;
    sc_param_type_t type;

    union {
      char *string;
      int integer;
      bool boolean;
    } value;
    bool required;
    struct sc_param *first;
    struct sc_param *prev;
    struct sc_param *next;
  } sc_param_t;

  typedef struct {
    sc_action_t action;
    sc_param_t *param;
  } sc_param_request_t;

  sc_param_t* sc_param_create(const char *name, sc_param_type_t type, bool required);
  sc_param_t* sc_param_create_string(const char *name, char *value, bool required);
  sc_param_t* sc_param_create_integer(const char *name, int value, bool required);
  sc_param_t* sc_param_create_boolean(const char *name, bool value, bool required);
  sc_param_t* sc_param_link(sc_param_t *a, sc_param_t *b);
  sc_param_t* sc_param_get(sc_param_request_t *params, const char *name);
  void sc_param_destroy(sc_param_request_t *params, sc_param_t *param);
  sc_param_t* sc_param_copy(sc_param_t *param);
  void sc_param_append(sc_param_request_t *params, sc_param_t *param);
  void sc_param_append(sc_param_request_t *params, sc_param_t *param);
  void sc_param_free_param(sc_param_t *param);
  void sc_param_free_params(sc_param_t *param);


#ifdef	__cplusplus
}
#endif

#endif	/* PARAM_H */

