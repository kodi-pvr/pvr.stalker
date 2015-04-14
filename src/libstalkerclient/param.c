#include "param.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

sc_param_t* sc_param_create(const char *name, sc_param_type_t type, bool required) {
  sc_param_t *param;

  param = (sc_param_t *) malloc(sizeof (sc_param_t));
  param->name = name;
  param->type = type;
  param->required = required;
  
  param->first = NULL;
  param->prev = NULL;
  param->next = NULL;

  return param;
}

sc_param_t* sc_param_create_string(const char *name, char *value, bool required) {
  sc_param_t *param;

  param = sc_param_create(name, SC_STRING, required);
  param->value.string = sc_util_strcpy(value);

  return param;
}

sc_param_t* sc_param_create_integer(const char *name, int value, bool required) {
  sc_param_t *param;

  param = sc_param_create(name, SC_INTEGER, required);
  param->value.integer = value;

  return param;
}

sc_param_t* sc_param_create_boolean(const char *name, bool value, bool required) {
  sc_param_t *param;

  param = sc_param_create(name, SC_BOOLEAN, required);
  param->value.boolean = value;

  return param;
}

sc_param_t* sc_param_link(sc_param_t *a, sc_param_t *b) {
  b->first = a->first;
  b->prev = a;
  a->next = b;

  return b;
}

sc_param_t* sc_param_get(sc_param_request_t *params, const char *name) {
  sc_param_t *param;

  param = params->param;
  while (param) {
    if (strcmp(param->name, name) == 0) {
      return param;
    }

    param = param->next;
  }

  return NULL;
}

void sc_param_destroy(sc_param_request_t *params, sc_param_t *param) {
  sc_param_t *first;
  sc_param_t *prev;
  sc_param_t *next;

  first = param->first;
  prev = param->prev;
  next = param->next;

  if (first == param) {
    sc_param_t *oparam;
    
    oparam = next;
    while (oparam) {
      oparam->first = next;

      oparam = oparam->next;
    }
    
    params->param = next;
  }
  
  if (prev) {
    prev->next = next;
  }

  if (next) {
    next->prev = prev;
  }
  
  sc_param_free_param(param);
}

sc_param_t* sc_param_copy(sc_param_t *param) {
  sc_param_t *copy;

  copy = (sc_param_t *) malloc(sizeof (sc_param_t));
  copy->name = param->name;

  copy->type = param->type;
  switch (param->type) {
    case SC_STRING:
    {
      copy->value.string = sc_util_strcpy(param->value.string);

      break;
    }
    case SC_INTEGER:
      copy->value.integer = param->value.integer;
      break;
    case SC_BOOLEAN:
      copy->value.boolean = param->value.boolean;
      break;
  }

  copy->required = param->required;

  copy->first = NULL;
  copy->prev = NULL;
  copy->next = NULL;

  return copy;
}

void sc_param_append(sc_param_request_t *params, sc_param_t *param) {
  sc_param_t *oparam;

  oparam = params->param;
  while (oparam->next) {
    oparam = oparam->next;
  }

  oparam->next = param;

  param->prev = oparam;
  param->next = NULL;
}

void sc_param_free_param(sc_param_t *param) {
  if (param->type == SC_STRING) {
    free(param->value.string);
  }
  free(param);
  param = NULL;
}

void sc_param_free_params(sc_param_t *param) {
  while (param) {
    sc_param_t *next;
    next = param->next;

    sc_param_free_param(param);

    param = next;
  }
}
